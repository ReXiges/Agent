#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <time.h>
#include <stdlib.h>
int* map;
time_t t;//variable necesaria para generador aleatorio
//variables estaticas para las dimensiones del mapa
static int lenght=10;
static int width=10;

//Prototypes
struct agent;
int isSpaceEmpty(int x, int y, int *map);
int readPosition(int x, int y, int *map);
void overwritePosition(int, int, int*, int);
int backtrack(struct agent*);

/* #region Sensor */
struct sensor{
	int x;	
	int y;
	char facing[];
};

/* #endregion */
struct sensor* new_sensor(char facing[], int x, int y){
	struct sensor* newStruct = malloc(sizeof *newStruct);
	strcpy(newStruct->facing, facing);
	newStruct->x=x;
	newStruct->y=y;
	return newStruct;
}

struct agent{
	int x;	
	int y;
	struct sensor* listedSensors[8];	//sensores del agente
	struct pile* myMap;// se le agrego memoria al agente
	struct pile* visited;
};

struct node {// nodo
	int x;	
	int y;
	//el peso es equivalente a la cantidad de celdas adyancentes que
	//no ha visitado y no esten bloqueadas
	int weight;
	struct node* next;
	struct node* previous;
};

struct pile {
	struct node* first;
	struct node* last;
};

struct node* popPile(struct pile* P){
	if(P->first==NULL){
		return NULL;	
	}

	if(P->first==P->last){
		struct node* result = P->last;
		P->first = NULL;
		P->last = NULL;		
		return result;
	}

	struct node* last= P->last;
	struct node* newLast= last->previous;
	
	newLast->next=NULL;
	P->last=newLast;
	
	return last;
};

void pushPile (struct pile* P, struct node* newNode){
	if(P->first==NULL){
		P->first=newNode;
		P->last=newNode;
		return;
	}

	struct node* last= P->last;
	last->next=newNode;
	newNode->previous=last;
	P->last=newNode;
}

/*
Decrementa el peso de los nodos, las coordenadas que se piden 'x' y 'y'
son de la celda recien visitada, se bajara el peso a todas las visitadas adyancentes.
*/
void decreaseWeight(struct pile* p, int x, int y){
	struct node* current=p->last;
	int changes=0;
	int newX;
	int newY;

	while (current !=NULL && changes<8){
		newX=current->x;
		newY=current->y;

		if(newX >= x-1 && newX <= x+1 && newY>=y-1 && newY<=y+1){
			current->weight--;
			changes++;
		}

		current=current->previous;
	}
}


struct node* findNode(struct pile* p, int x, int y){//encuentra un nodo visitado dadas sus coordenadas x y y si devuelve null es que ese nodo no ha sido visitado
	struct node* current=p->last;

	while (current != NULL){
		if(current->x == x && current->y == y)	break;//return current;
		
		current = current->previous;
	}

	return current;//NULL;	
}


//New ones need this
int calculateWeight(struct agent *a, int x, int y){
	int weight = 0;
	//examina los vecinos
	for (int row = -1; row < 2; row++){
		for (int column = -1; column < 2; column++){
			
			if ( findNode(a->visited, x+column, y+row) == NULL ) //if not finded before
				weight += isSpaceEmpty(x+column, y+row, map);

		}
	}
	return weight;
}


struct node* new_node(int xAux, int yAux, int weightAux){
	struct node* n = malloc (sizeof(*n));
	n->x = xAux;
	n->y = yAux;
	n->weight = weightAux;
	n->next = NULL;
	n->previous = NULL;
}


struct pile* new_pile(){
	struct pile* p = malloc (sizeof(*p));
	p->first = NULL;
	p->last = NULL;
	return p;
}


struct agent* new_agent(){
	struct agent* newStruct = malloc(sizeof (*newStruct));
	
	newStruct->listedSensors[0] = new_sensor("N", 0, -1);
	newStruct->listedSensors[1] = new_sensor("NE", 1, -1);
	newStruct->listedSensors[2] = new_sensor("E", 1, 0);
	newStruct->listedSensors[3] = new_sensor("SE", 1, 1);
	newStruct->listedSensors[4] = new_sensor("S", 0, 1);
	newStruct->listedSensors[5] = new_sensor("SW", -1, +1);
	newStruct->listedSensors[6] = new_sensor("W", -1, 0);
	newStruct->listedSensors[7] = new_sensor("NW", -1, -1);

	newStruct->myMap= new_pile();
	newStruct->visited = new_pile();

	return newStruct;
}

//funcion para mantener coordenadas dentro de los limites de la matriz
int keepInBounds(int number, int boundLimit){
	if (0 <= number && number < boundLimit) return number;

	return number < 0 ? number + boundLimit : number - boundLimit;
}

//funcion de movimiento del agente
void move_agent(struct agent* pAgent, struct sensor* orientation){
	printf("Moving %s \n", orientation->facing);
	int newX = keepInBounds(pAgent->x + orientation->x, width);
	int newY = keepInBounds(pAgent->y + orientation->y, lenght);
	pAgent->x = newX;
	pAgent->y = newY;
}


// funcion de regreso a origen
void backHome(struct agent* a) {
	struct pile* wayHome = a->myMap;
	while(wayHome->first != NULL){
		backtrack(a);
	}
	return;
}

//funcion de backtracking
int backtrack(struct agent* pAgent){
	printf("Backtracking");
	struct node* back= popPile(pAgent->myMap);
	if(back==NULL){
		printf("im Home");
	}
	else{
		pAgent->x = back->x;
		pAgent->y = back->y;
	}
}



struct sensor* choose_path(struct agent* a, struct sensor** possibleMoves, int size){
	int totalWeight = 0;
	int* weights = malloc(sizeof(int)*size);
	int newX = 0;
	int newY = 0;

	for (int i = 0; i < size; i++) {
		struct sensor* orientation = possibleMoves[i];
		newX = a->x + orientation->x;
		newY = a->y + orientation->y;
		
		if (readPosition(newX, newY, map) == 64) //Priority
			return orientation; 
	}

	return possibleMoves[rand()%size];
}

//Recibe una lista de sensores posibles, el agente para moverlo, una lista de pesos
void weight_move(struct agent* a, struct sensor* orientation){
	move_agent(a, orientation);
	struct node* newNode= new_node(a->x,a->y,calculateWeight(a,a->x,a->y));
	struct node* step= new_node(a->x,a->y,calculateWeight(a,a->x,a->y));
	pushPile(a->myMap,step);
	pushPile(a->visited,newNode);
	decreaseWeight(a->visited,a->x,a->y);
}


int readPosition(int x, int y, int* map){
	return map[y*width+x];
}

void overwritePosition(int x, int y, int* map, int data){
	map[y*width+x] = data;
}

//lee la posicion de manera circular, acepta negativos y numero mucho mas grandes que las dimensiones como entrada
int isSpaceEmpty(int x, int y, int* map){
	if(x>=width || x<0 || y >=lenght || y<0)
		return 0;

	char res = readPosition(x, y, map);
	return res == 32 || res == 64 ? 1: 0;
}

//lectura de los sensores para ver si el espacio esta disponible
struct sensor** readSensors(struct agent *a){
	struct sensor** possibleMoves = malloc(sizeof (possibleMoves));
	possibleMoves[0] = NULL;
	struct sensor** obj = malloc(sizeof (obj));
	int x = a->x;
	int y = a->y;
	int added = 0;
	
	for (int i = 0; i<8; i++){
		struct sensor* currSensor = a->listedSensors[i];
		int noObstacle = isSpaceEmpty(x+currSensor->x, y+currSensor->y, map);
		
		printf("Sensor %s: %d \n",currSensor->facing, noObstacle);

		if (!noObstacle 
		|| findNode(a->visited,x+currSensor->x, y+currSensor->y) != NULL //Si ya está visitado 
		|| calculateWeight(a,x+currSensor->x, y+currSensor->y) < 1) { //Si el peso es de 0
			continue; //No hace falta verle
		}
		
		possibleMoves = realloc(possibleMoves, sizeof(possibleMoves) * (added+1));
		possibleMoves[added++] = currSensor;	
	}

	possibleMoves = realloc(possibleMoves, sizeof(possibleMoves) * (added+1));
	possibleMoves[added] = NULL;

	return possibleMoves;
}


int sizeOfMovements(struct sensor** possibleMoves){
	int size = 0;
	while (possibleMoves[size] != NULL)	size++;
	return size;
}


void printMap(int *map, struct agent* a){
    int x=0;
    int y=0;
    printf("\n");
	
	for(int a=0;a<width;a++) printf("_");

	printf("\n");
    while(y<lenght && x<width){
		if (a->x == x && a->y == y){
			printf("%c",'!');
		} else {
			printf("%c",map[y*width+x]);
		}
		
		x++;
		if(x==width){
			x=0;
			y++;
			printf("|\n");
		}
    }
	
	for(int a=0;a<width;a++) printf("_");
	
	printf("\n");
}


//coloca el agente de manera aleatoria en una posicion vacia del mapa.
void placeAgent(struct agent *a, int* localMap){
	srand((unsigned) time(&t));
	int y;
	int x;
	while(1){ //localizacion aleatoria del agente
	    x = rand()%lenght;
		y = rand()%width;
		if(localMap[y*width+x]==32){
			localMap[y*width+x]='@';
			break;
		}
	}
	while(1){ //localizacion aleatoria del agente
	    x = rand()%lenght;
		y = rand()%width;
		if(localMap[y*width+x]==32){
			a->x=x;
			a->y=y;
			return;	
		}
	}
}

//Abre el archivo del laberinto?
int openMap(struct agent *a){
    char nombreArchivo[50];
	char buffer[255];

    printf("\n ingrese la direccion del archivo: ");
    scanf("%s", nombreArchivo);
    
	FILE* archivo = fopen(nombreArchivo,"r");
    
	if(!archivo){
		return 0;
	}

    fgets(buffer, 255, archivo);
    char* clenght = strtok(buffer," ");
    char* cwidth  = strtok(NULL, " ");
    
	printf(clenght);
    printf("x");
    printf(cwidth);
    
	int x = 0;
    int y = 0;
    
	lenght = atoi(clenght);
    width  = atoi(cwidth);
    
	int* localMap=malloc(lenght*width*sizeof(int));
    char entry;
    int i =0;

	while ( (entry= fgetc(archivo)) != EOF ){
		
		if (entry == '\t' || entry == '\n')
			entry = ' ';

		localMap[y*width+x]= (int) entry;

		x++;

		if(x==width){
			if (y == lenght -1 ) break;

			if (entry != '\n'){
				while (!(entry == '\n' || entry == EOF)){
					entry= fgetc(archivo);
				}	
			}

			x=0;
			y++;
		}
	}
	fclose(archivo);
	placeAgent(a,localMap);
	map = localMap;
	return 1;
}


//crea un mapa aleatorio
void randomizeMap(struct agent *a){
   	int mapSize=lenght*width;
    int* localMap=malloc(mapSize*sizeof(int));
	
	srand((unsigned) time(&t));
	int r;
	int x=0;
	int y=0;

	while(mapSize>0){//localizacion aleatoria de obstaculos
		sleep(0.1);
		r = rand()%100;//prob. del 13% de generar obstaculos en una celda dada

		localMap[y*width+x] = r < 13 ? 35 : 32;

		x++;
		if(x==width){
			x=0;
			y++;
		}
		mapSize--;
    }
	
	placeAgent(a,localMap);
	map=localMap;
}

int wantToConinue(){
	printf("I found one unit of fruit.");
	printf("\nThere may be another one.");
	char option; //To know what they reply
	pregunta: printf("\nWanna continue: (y/n) ");
	
	scanf("%c", &option);
	if (!(option == 'n' || option == 'y'))
		goto pregunta;

	return option == 'y';// ? 1 : 0;
}


int main() {
	struct agent *miVBot = new_agent();	//asignacion de memoria del agente
	
	
	if (openMap(miVBot) == 0){
		printf("No se encontro mapa generando mapa aleatorio.\n");
		sleep(2);
		randomizeMap(miVBot);
	}

	struct node* newNode= new_node(miVBot->x,miVBot->y,calculateWeight(miVBot,miVBot->x,miVBot->y));
	pushPile(miVBot->myMap,newNode);
	pushPile(miVBot->visited,newNode);
	decreaseWeight(miVBot->visited,miVBot->x,miVBot->y);

	while(1){
		printMap(map, miVBot);
		if (readPosition(miVBot->x, miVBot->y, map) == '@'){
			overwritePosition(miVBot->x, miVBot->y, map, ' ');

			if (!wantToConinue()) backHome(miVBot);;
			else printf("");//Seguir buscando
		}

		struct sensor** read = readSensors(miVBot);
		int size = sizeOfMovements(read);

		if (size == 0){
			printf("Nowhere to go\n");
			backtrack(miVBot);
			//
		}
		else {	
			struct sensor * orientation = choose_path(miVBot, read, size);
			weight_move(miVBot, orientation);
		}

		
		
		sleep(2);
		system("clear");
	}

		/*
		struct sensor** read = readSensors(miVBot);
		int size = sizeOfMovements(read);

		if (size == 0){
			
		}
		*/
	

	/*struct node *result;
	struct pile *p= new_pile();
	struct node *n0= new_node(-1,0,3);
	struct node *n1= new_node(0,0,8);
	struct node *n2= new_node(0,1,7);
	struct node *n3= new_node(1,0,6);
	pushPile(p,n0);	
	pushPile(p,n1);
	pushPile(p,n2);
	pushPile(p,n3);
	result=findNode(p,0,0);
	printf("%d \n",result->weight);*/
	
	return 1;
}
