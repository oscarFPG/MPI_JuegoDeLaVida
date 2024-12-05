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


// -------------------------------- Master and Workers Functionality -------------------------------- //


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
		window = SDL_CreateWindow("Práctica 3 de PSD",
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
		int num_workers = size - 1;
		int maxSize = 0;
		unsigned short* worldA = (unsigned short*) malloc(sizeof(unsigned short) * worldWidth * worldHeight);
		unsigned short* worldB = (unsigned short*) malloc(sizeof(unsigned short) * worldWidth * worldHeight);
		tWorkerInfo* masterIndex = malloc(num_workers * sizeof(tWorkerInfo));
		MPI_Status status;

		// Inicializar mundos
		clearWorld(worldA, worldWidth, worldHeight);
		clearWorld(worldB, worldWidth, worldHeight);

		// Create a random world		
		initRandomWorld(worldA, worldWidth, worldHeight);

		printf("Tablero\n");
		int c = 1;
		for(int i = 0; i < worldWidth * worldHeight; i++){
			printf("| %hu |", worldA[i]);
			if(c == worldWidth){
				c = 1;
				printf("\n");
			}
			else{
				c++;
				printf(" ");
			}
		}
		
		// Show initial state
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);
		drawWorld(worldB, worldA, renderer, 0, worldHeight, worldWidth, worldHeight);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);

		// Mandar numero de filas y tamaño de fila
		send_number_of_rows_and_size(worldA, worldWidth, worldHeight, num_workers, masterIndex);

		// Enviar porciones de mapa
		send_board_partitions(worldA, num_workers, worldWidth, worldHeight, masterIndex, &maxSize);
		if(DEBUG_MASTER)
			printf("Maximum size send is %d\n", maxSize);


		unsigned short* auxiliar = malloc(sizeof(unsigned short) * maxSize);
		int workersLeft = size - 1;
		calculateLonelyCell();
		MPI_Finalize();


		// Bucle de juego
		int currentIteration = 0;
		while(currentIteration < 9873459){
			
			// Clear renderer
			SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
			SDL_RenderClear(renderer);

			// Enviar porciones de mapa
			send_board_partitions(worldA, num_workers, worldWidth, worldHeight, masterIndex, &maxSize);

			// Mostrar por pantalla
			drawWorld(worldB, worldA, renderer, 0, worldHeight, worldWidth, worldHeight);

			// Update surface
			SDL_RenderPresent(renderer);
			SDL_UpdateWindowSurface(window);
			SDL_Delay(300);

			++currentIteration;
		}

		// Set timer
		endTime = MPI_Wtime();
		printf ("Total execution time:%f seconds\n", endTime-startTime);
		MPI_Finalize();
	}
	
	// Workers
	else{
		unsigned short* partition = NULL;
		unsigned short* rowAbove = NULL;
		unsigned short* rowUnder = NULL;
		int totalSize = 0;
		int worldWidth, numberOfRows;


		receive_sizes_of_work(rank, &worldWidth, &numberOfRows);
		if(0 && DEBUG_WORKER)
			printf("Worker %d recibio el ancho del mundo(%d) y el numero de filas(%d)\n", rank, worldWidth, numberOfRows);

		// Calculate size of each board partition
		totalSize = numberOfRows * worldWidth;
		partition = malloc(sizeof(unsigned short) * totalSize);
		rowAbove = malloc(sizeof(unsigned short) * worldWidth);
		rowUnder = malloc(sizeof(unsigned short) * worldWidth);
		receive_world_partition(partition, rowAbove, rowUnder, totalSize, worldWidth);
		
		// Transform into matrix
		unsigned short* matrix = transformIntoMatrix(rowAbove, partition, rowUnder, totalSize, worldWidth);
		unsigned short* newMatrix = malloc(totalSize);
		free(partition);
		free(rowAbove);
		free(rowUnder);

		// Update each cell
		printf("MATRIZ old de worker %d\n", rank);
		int N = totalSize + worldWidth + worldWidth;
		int count = 1;
		for(int i = 0; i < N; i++){
			printf("| %hu |", matrix[i]);
			if(count == 10){
				count = 1;
				printf("\n");
			}
			else{
				count++;
				printf(" ");
			}
		}
		printf("\nFinal de MATRIZ old\n");


		// Actualizar todas las celdas de la seccion de trabajo
		tCoordinate cell;
		for(int row = 1; row < numberOfRows; row++){
			for(int col = 0; col < worldWidth; col++){
				cell.row = row;
				cell.col = col;
				updateCell(&cell, matrix, newMatrix, worldWidth, numberOfRows);
			}
		}

		printf("MATRIZ new de worker %d\n", rank);
		count = 1;
		for(int i = 0; i < N; i++){
			printf("| %hu |", newMatrix[i]);
			if(count == 10){
				count = 1;
				printf("\n");
			}
			else{
				count++;
				printf(" ");
			}
		}
		printf("\nFinal de MATRIZ new\n");
		
		// Send to master

		MPI_Finalize();
	}

    return 0;
}