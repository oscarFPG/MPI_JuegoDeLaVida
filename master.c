#include "master.h"


// ------------------------------------- MASTER ESTATIC FUNCIONS ------------------------------------- //
void send_number_of_rows_and_size(unsigned short* worldA, int worldWidth, int worldHeight, int maxWorkers, tWorkerInfo* masterIndex, int* maxSize) {

	unsigned short* i_ptr = worldA;
	int remainingRows = worldHeight;
	int rowsPerWorker = 0; 
	int worker = maxWorkers, i = 0, offset = 0;
	*maxSize = 0;
	while(worker != 0){

		rowsPerWorker = (remainingRows % worker == 0) ? (remainingRows / worker) : ((remainingRows / worker) + 1);	// To round
		if(rowsPerWorker * worldWidth > *maxSize)
			*maxSize = rowsPerWorker * worldWidth;

		// Send world width
		MPI_Send(&worldWidth, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);

		// Send size of piece to work with
		MPI_Send(&rowsPerWorker, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);

		// Save first position in the world of each worker and number of cells in it
		masterIndex[i].baseAddress = i_ptr;
		masterIndex[i].size = rowsPerWorker * worldWidth;
		masterIndex[i].offset = offset;
		
		// If the remaining rows to distribute are less that the size we are distributing:
		// It's the last partition -> Assign it to the last worker
		remainingRows -= rowsPerWorker;
		if(remainingRows < rowsPerWorker)
			rowsPerWorker = remainingRows;

		i_ptr += masterIndex[i].size;
		offset += rowsPerWorker * worldWidth;
		--worker;
		i++;
	}
}

void send_world_partitions(const unsigned short* worldA, const int workers, const int worldWidth, const int worldHeight, tWorkerInfo* masterIndex){

	for(int w = 0; w < workers; w++){

		int workerID = w + 1;
		unsigned short* start = masterIndex[w].baseAddress;
		int numberOfCells = masterIndex[w].size;

		// Send auxiliar row above
		unsigned short* rowFromAbove;
		if(workerID == 1)
			rowFromAbove = worldA + ( (worldWidth * worldHeight) - worldWidth );
		else
			rowFromAbove = masterIndex[w].baseAddress - worldWidth;
		MPI_Send(rowFromAbove, worldWidth, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);

		// Send world partition to work with
		MPI_Send(start, numberOfCells, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);

		// Send auxiliar row from under
		unsigned short* rowFromUnder;
		if(workerID == workers)
			rowFromUnder = worldA;
		else
			rowFromUnder = masterIndex[w + 1].baseAddress;
		MPI_Send(rowFromUnder, worldWidth, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);
	}
}

void receive_world_partitions(unsigned short* world, const int worldWidth, const int worldHeight, const int numWorkers, tWorkerInfo* masterIndex, const int maxSize){

	unsigned short* aux = malloc(sizeof(unsigned short) * maxSize);
	int workersReceived = numWorkers;
	int worker, offset, sizeReceived;
	MPI_Status status;

	memset(aux, 0, maxSize);
	while (workersReceived != 0){
		
		MPI_Recv(aux, maxSize, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_UNSIGNED_SHORT, &sizeReceived);
		worker = status.MPI_SOURCE - 1;

		// Moving the pointer to the starting position of the map
		offset = masterIndex[worker].offset;
		
		// Update new world        
		for(int i = 0; i < sizeReceived; i++)
			world[offset + i] = aux[i];

		workersReceived--;                             
	}                       

	free(aux);
}

void generat_cataclysm(unsigned short* world, const int worldWidth, const int worldHeight, tCoordinate* centralCell){

}

// Master execution
void executeMaster(const int worldWidth, const int worldHeight, const int numWorkers, const int totalIterations){

	// Create window
	SDL_Window* window = SDL_CreateWindow(
						"Práctica 3 de PSD", 
						SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
						worldWidth * CELL_SIZE, worldHeight * CELL_SIZE,
						SDL_WINDOW_SHOWN);			

	// Check if the window has been successfully created
	if(window == NULL){
		showError("Window could not be created!\n");
		exit(0);
	}
	
	// Create a renderer
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);


	// Sim logic
	unsigned short* worldA = (unsigned short*) malloc(sizeof(unsigned short) * worldWidth * worldHeight);
	unsigned short* worldB = (unsigned short*) malloc(sizeof(unsigned short) * worldWidth * worldHeight);
	tWorkerInfo* masterIndex = malloc(numWorkers * sizeof(tWorkerInfo));
	tCoordinate centralCell;
	int maxSize;

	// Inicializar mundos
	clearWorld(worldA, worldWidth, worldHeight);
	clearWorld(worldB, worldWidth, worldHeight);

	// Create a random world		
	initRandomWorld(worldA, worldWidth, worldHeight);

	// Calculate centrall cell for creating cataclysms
	centralCell.row = worldHeight / 2;
	centralCell.col = worldWidth / 2;

	// Mandar numero de filas y tamaño de fila
	send_number_of_rows_and_size(worldA, worldWidth, worldHeight, numWorkers, masterIndex, &maxSize);

	// Game loop
	int currentIteration = 0;
	while(currentIteration < totalIterations){
		
		// Clear renderer
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);

		printf("ITERACION %d\n", currentIteration);

		// Send map portions, draw map state and get new state
		if(currentIteration % 2 == 0){
			send_world_partitions(worldA, numWorkers, worldWidth, worldHeight, masterIndex);
			drawWorld(worldA, worldB, renderer, 0, worldHeight, worldWidth, worldHeight);
			receive_world_partitions(worldB, worldWidth, worldHeight, numWorkers, masterIndex, maxSize);
		}
		else{
			send_world_partitions(worldB, numWorkers, worldWidth, worldHeight, masterIndex);
			drawWorld(worldB, worldA, renderer, 0, worldHeight, worldWidth, worldHeight);
			receive_world_partitions(worldA, worldWidth, worldHeight, numWorkers, masterIndex, maxSize);
		}

		// Update surface
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(500);

		++currentIteration;
	}

	free(worldA);
	free(worldB);
	free(masterIndex);
}