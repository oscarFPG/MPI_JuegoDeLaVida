#include "master.h"
#include "worker.h"
#include "mpi.h"
#include <string.h>

/**
 * Shows an error message if the input parameters are not correct.
 * 
 * @param programName Name of the executable program. 
 * @param msg Error message.
 */
void wrongUsage (int rank, char* programName, char* msg){
	
	// Only the master process show this message
	if (rank == MASTER){
		printf ("%s\n", msg);
		printf ("Usage: >%s worldWidth worldHeight iterations [step|auto] outputImage [static|dynamic grainSize] \n", programName);
	}
	
	MPI_Finalize();
}

/**
 * Shows an error message if the initialization stage is not performed successfully.
 * 
 * @param msg Error message.
 */
void showError (char* msg){	
	printf ("ERROR: %s\n", msg);	
	MPI_Abort(MPI_COMM_WORLD, -1);
}


void initializeGame(unsigned short* worldA, unsigned short* worldB, int worldWidth, int worldHeight){

	// Create empty worlds
	worldA = (unsigned short*) malloc (worldWidth * worldHeight * sizeof (unsigned short));
	worldB = (unsigned short*) malloc (worldWidth * worldHeight * sizeof (unsigned short));
	clearWorld(worldA, worldWidth, worldHeight);
	clearWorld(worldB, worldWidth, worldHeight);		
			
	// Create a random world		
	initRandomWorld(worldA, worldWidth, worldHeight);
}

void repartirTablero(unsigned short* worldA, int worldWidth, int worldHeight, int workers){

	// Repartir tablero entre los workers
	unsigned short* buffer = NULL;
	int areaTotal = worldWidth * worldHeight;
	int porcion = areaTotal / workers;
	int ini = 0;	
	for(int i = 0; i < workers - 1; i++){

		// Crear buffer
		buffer = malloc(porcion * sizeof(unsigned short));	// No hacer copia
		//strncpy(buffer, worldA + ini, porcion);	// Desde la posicion ini hasta ini + porcion

		// Enviamos la porcion de tablero correspondiente
		MPI_Send(worldA + ini, porcion, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
		ini += porcion;
		
		// Liberar memoria
		free(buffer);
	}
	
	// Enviar la porcion restante a el ultimo worker
	buffer = malloc( (areaTotal - ini) * sizeof(unsigned short) );
	strncpy(buffer, worldA + (areaTotal - ini), areaTotal);

	MPI_Send(buffer, areaTotal - ini, MPI_UNSIGNED_SHORT, workers, 0, MPI_COMM_WORLD);
	free(buffer);
}


// -------------------- Masters and workers functionality --------------------
void masterExecution(const unsigned short worldWidth, const unsigned short worldHeight, const int num_workers, const int total_iterations){

	// Initicializar mundos
	unsigned short* worldA, worldB;
	initializeGame(worldA, worldB, worldWidth, worldHeight);

	for(int iterations = 0; iterations < total_iterations; ++iterations){
		repartirTablero(worldA, worldWidth, worldHeight, num_workers);
	}

}

void workerExecution(){


	MPI_Recv();
}
// -------------------------------------------------

int main(int argc, char* argv[]){
	
	// The window to display the cells
	SDL_Window* window = NULL;

	// The window renderer
	SDL_Renderer* renderer = NULL;
	
	// World size and execution modes
	int worldWidth, worldHeight, autoMode, distModeStatic;
	
	// Iterations, grain size, and processes' info
	int totalIterations, grainSize, rank, size;
		
	// Output file
	char* outputFile = NULL;
	
	// Time
	double startTime, endTime;
	
	
	// Init MPI
	MPI_Init(&argc, &argv);
	
	// Get rank and size
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank( MPI_COMM_WORLD, &rank);
	
	// Check the number of processes
	if (size < 3){
		printf ("The number of processes must be > 2\n");
		MPI_Finalize();		
		exit(0);			
	}				
	
	// Check the number of parameters
	if ((argc < 7) || (argc > 8)){
		wrongUsage (rank, "Wrong number of parameters!\n", argv[0]);
		exit(0);
	}
	
	// Read world's width (common for master and workers)
	worldWidth = atoi (argv[1]);	
	worldHeight = atoi (argv[2]);
	totalIterations = atoi (argv[3]);
	
	// Execution mode
	if (strcmp(argv[4], "step") == 0)
		autoMode = 0;
	else if (strcmp(argv[4], "auto") == 0)
		autoMode = 1;
	else
		wrongUsage (rank, "Wrong execution mode, please select [step|auto]\n", argv[0]);
			
	// Read input parameters					
	outputFile = argv[5];
	
	// Distribution mode
	if (strcmp(argv[6], "static") == 0  && argc == 7)
		distModeStatic = 1;
	else if (strcmp(argv[6], "dynamic") == 0 && argc == 8){				
		distModeStatic = 0;
		grainSize = atoi (argv[7]);				
	}
	else
		wrongUsage (rank, "Wrong distribution mode, please select [static|dynamic grainSize]\n", argv[0]);	
	
	// Randomize the generator
	srand (SEED);
	
	// Master process
	if (rank == MASTER){		
		
		// Init video mode	
		if(SDL_Init(SDL_INIT_VIDEO) < 0){
			showError ("Error initializing SDL\n");
			exit (0);
		}			
		// Create window
		window = SDL_CreateWindow( "Práctica 3 de PSD",
									0, 0,
									worldWidth * CELL_SIZE, worldHeight * CELL_SIZE,
									SDL_WINDOW_SHOWN);			

		// Check if the window has been successfully created
		if(window == NULL){
			showError("Window could not be created!\n");
			exit(0);
		}
		
		// Create a renderer
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
							
		// Show parameters...
		printf ("Executing with:\n");			
		printf ("\tWorld size:%d x %d\n", worldWidth, worldHeight);
		printf ("\tScreen size:%d x %d\n", worldWidth*CELL_SIZE, worldHeight*CELL_SIZE);
		printf ("\tNumber of iterations:%d\n", totalIterations);
		printf ("\tExecution mode:%s\n", argv[4]);
		printf ("\tOutputFile:[%s]\n", outputFile);
		printf ("\tDistribution mode: %s", argv[6]);
		
		if (distModeStatic)
			printf ("\n");
		else
			printf (" - [%d rows]\n", grainSize);
		
		// Set timer
		startTime = MPI_Wtime();
		
		// Masters action
		masterExecution(worldWidth, worldHeight, size - 1, totalIterations);
		
		// Set timer
		endTime = MPI_Wtime();
		printf ("Total execution time:%f seconds\n", endTime-startTime);
	}
	
	// Workers
	else{
		workerExecution();
	}

    return 0;
}