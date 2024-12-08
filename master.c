#include "master.h"

// ----------------------------------------------- OUR AUXILIARY FUNTIONS ----------------------------------------------- //
void send_world_partition(unsigned short* rowAbove, unsigned short* partition, unsigned short* rowUnder, const int auxSize, const int partitionSize, const int workerID){
	MPI_Send(rowAbove, auxSize, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);
	MPI_Send(partition, partitionSize, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);
	MPI_Send(rowUnder, auxSize, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);
}

void generate_cataclysm(unsigned short* world, const int worldWidth, const int worldHeight, const int fila, const int columna){

	tCoordinate cell;
	unsigned short cellValue;

	// Eliminar las celdas de arriba
	cell.col = columna;
	for(int f = fila - 1; 0 <= f; f--){
		cell.row = f;
		cellValue = getCellAtWorld(&cell, world, worldWidth);
		if(cellValue == CELL_LIVE || cellValue == CELL_NEW)
			setCellAt(&cell, world, worldWidth, CELL_CATACLYSM);
	}

	// Eliminar las celdas de abajo
	cell.col = columna;
	for(int f = fila + 1; f < worldHeight; f++){
		cell.row = f;
		cellValue = getCellAtWorld(&cell, world, worldWidth);
		if(cellValue == CELL_LIVE || cellValue == CELL_NEW)
			setCellAt(&cell, world, worldWidth, CELL_CATACLYSM);
	}

	// Eliminar las celdas de la izquierda
	cell.row = fila;
	for(int c = columna - 1; 0 <= c; c--){
		cell.col = c;
		cellValue = getCellAtWorld(&cell, world, worldWidth);
		if(cellValue == CELL_LIVE || cellValue == CELL_NEW)
			setCellAt(&cell, world, worldWidth, CELL_CATACLYSM);
	}

	// Eliminar las celdas de la derecha
	cell.row = fila;
	for(int c = columna + 1; c < worldHeight; c++){
		cell.col = c;
		cellValue = getCellAtWorld(&cell, world, worldWidth);
		if(cellValue == CELL_LIVE || cellValue == CELL_NEW)
			setCellAt(&cell, world, worldWidth, CELL_CATACLYSM);
	}

	// Eliminar celda central
	cell.row = fila;
	cell.col = columna;
	cellValue = getCellAtWorld(&cell, world, worldWidth);
	if(cellValue == CELL_LIVE || cellValue == CELL_NEW)
		setCellAt(&cell, world, worldWidth, CELL_CATACLYSM);
}
// ----------------------------------------------- OUR AUXILIARY FUNTIONS ----------------------------------------------- //

// -------------------------------------------------- ESTATIC FUNTIONS -------------------------------------------------- //
void send_static_sizes(unsigned short* world, int worldWidth, int worldHeight, int maxWorkers, tWorkerInfo* masterIndex, int* maxSize) {

	unsigned short* i_ptr = world;
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

void send_all_world_partitions(const unsigned short* world, const int workers, const int worldWidth, const int worldHeight, tWorkerInfo* masterIndex){

	for(int w = 0; w < workers; w++){

		int workerID = w + 1;
		unsigned short* start = masterIndex[w].baseAddress;
		int numberOfCells = masterIndex[w].size;

		// Auxiliar row above
		unsigned short* rowFromAbove;
		if(workerID == 1)
			rowFromAbove = world + ( (worldWidth * worldHeight) - worldWidth );
		else
			rowFromAbove = masterIndex[w].baseAddress - worldWidth;
		
		// Auxiliar row from under
		unsigned short* rowFromUnder;
		if(workerID == workers)
			rowFromUnder = world;
		else
			rowFromUnder = masterIndex[w + 1].baseAddress;

		send_world_partition(rowFromAbove, start, rowFromUnder, worldWidth, numberOfCells, workerID);
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
// -------------------------------------------------- ESTATIC FUNTIONS -------------------------------------------------- //

// -------------------------------------------------- DINAMIC FUNTIONS -------------------------------------------------- //
void send_dynamic_sizes(unsigned short* world, int worldWidth, int worldHeight, const int maxWorkers, tWorkerInfo* masterIndex, const int grainSize){

	unsigned short* i_ptr = world;
	int remainingRows = worldHeight;
	int rowsPerWorker = grainSize; //Mirar si puede quedarse trabajador sin trabajo en la primera iteracion
	int worker = 1, offset = 0;
	while(worker <= maxWorkers) {

		if(remainingRows == 0){
			MPISend(NULL, NULL, NULL, worker, 0, MPI_COMM_WORLD);//Cerrar,Dormir,...
			masterIndex[i].baseAddress = world;
			masterIndex[i].size = -1;
			masterIndex[i].offset = -1;
		}
		else {

			if(remainingRows < grainSize)
				rowsPerWorker = remainingRows;
			else
				rowsPerWorker = grainSize;

			// Send world width
			MPI_Send(&worldWidth, 1, MPI_INT, worker, 0, MPI_COMM_WORLD);

			// Send size of piece to work with
			MPI_Send(&rowsPerWorker, 1, MPI_INT, worker, 0, MPI_COMM_WORLD);

			// Save first position in the world of each worker and number of cells in it
			masterIndex[i].baseAddress = i_ptr;
			masterIndex[i].size = rowsPerWorker * worldWidth;
			masterIndex[i].offset = offset;

			remainingRows -= rowsPerWorker;
			i_ptr += masterIndex[i].size;
			offset += masterIndex[i].size;
		}
		worker++;	
	}
}
// -------------------------------------------------- DINAMIC FUNTIONS -------------------------------------------------- //

// -------------------------------------------------- MASTER EXECUTION -------------------------------------------------- //
void masterStaticExecution(const int worldWidth, const int worldHeight, const int numWorkers, const int totalIterations, const int autoMode){

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
	int maxSize;

	// Inicializar mundos
	clearWorld(worldA, worldWidth, worldHeight);
	clearWorld(worldB, worldWidth, worldHeight);

	// Create a random world		
	initRandomWorld(worldA, worldWidth, worldHeight);

	// Show first world state
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(renderer);
	drawWorld(worldA, worldB, renderer, 0, worldHeight, worldWidth, worldHeight);
	SDL_RenderPresent(renderer);
	SDL_UpdateWindowSurface(window);
	SDL_Delay(400);

	// Send to workers sizes of their partition
	send_static_sizes(worldA, worldWidth, worldHeight, numWorkers, masterIndex, &maxSize);

	// Game loop
	int currentIteration = 0;
	int cataclysmCycle = 0;
	while(currentIteration < totalIterations){
		
		printf("ITERACION %d\n", currentIteration);
		if(autoMode == 0){
			printf("Press ENTER to continue execution...\n");
			getchar();
		}

		// Clear renderer
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);

		// Send map portions, draw map state and get new state
		if(currentIteration % 2 == 0){

			// Make cataclysm
			if(cataclysmCycle == ITER_CATACLYSM){
				cataclysmCycle = 0;
				if((rand() % 100) < PROB_CATACLYSM)
					generate_cataclysm(worldA, worldWidth, worldHeight, worldHeight / 2, worldWidth / 2);
			}

			send_all_world_partitions(worldA, numWorkers, worldWidth, worldHeight, masterIndex);
			drawWorld(worldA, worldB, renderer, 0, worldHeight, worldWidth, worldHeight);
			receive_world_partitions(worldB, worldWidth, worldHeight, numWorkers, masterIndex, maxSize);
		}
		else{

			// Make cataclysm
			if(cataclysmCycle == ITER_CATACLYSM){
				cataclysmCycle = 0;
				if((rand() % 100) <= PROB_CATACLYSM)
					generate_cataclysm(worldB, worldWidth, worldHeight, worldHeight / 2, worldWidth / 2);
			}

			send_all_world_partitions(worldB, numWorkers, worldWidth, worldHeight, masterIndex);
			drawWorld(worldB, worldA, renderer, 0, worldHeight, worldWidth, worldHeight);
			receive_world_partitions(worldA, worldWidth, worldHeight, numWorkers, masterIndex, maxSize);
		}

		// Update surface
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(400);

		++currentIteration;
		++cataclysmCycle;
	}

	free(worldA);
	free(worldB);
	free(masterIndex);
}

void masterDynamicExecution(const int worldWidth, const int worldHeight, const int numWorkers, const int totalIterations, const int autoMode, const int grainSize){

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
	tWorkerInfo* masterIndex = malloc(sizeof(tWorkerInfo) * numWorkers);

	// Inicializar mundos
	clearWorld(worldA, worldWidth, worldHeight);
	clearWorld(worldB, worldWidth, worldHeight);

	// Create a random world		
	initRandomWorld(worldA, worldWidth, worldHeight);

	// Show first world state
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(renderer);
	drawWorld(worldA, worldB, renderer, 0, worldHeight, worldWidth, worldHeight);
	SDL_RenderPresent(renderer);
	SDL_UpdateWindowSurface(window);
	SDL_Delay(400);

	// Send to workers sizes of their partition
	send_dynamic_sizes(worldA, worldWidth, worldHeight, numWorkers, masterIndex, grainSize);

	// Game loop
	unsigned short* aux = malloc(sizeof(unsigned short) * grainSize);
	unsigned short* world_ptr = NULL, *writer_ptr = NULL;
	int currentIteration = 0;
	int cataclysmCycle = 0;
	int rowsPerWorker;
	int workerID, workerIndex, remainingRows, sizeReceived;
	MPI_Status status;
	while(currentIteration < totalIterations){
		
		world_ptr = (currentIteration % 2 == 0) ? worldA : worldB;
		remainingRows = worldHeight;

		printf("ITERACION %d\n", currentIteration);
		if(autoMode == 0){
			printf("Press ENTER to continue execution...\n");
			getchar();
		}

		// Clear renderer
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);

		// ------------ Send the First Orde of portions to all workers ------------ //
		unsigned short* world_Conteo = NULL;
		int numberRecv = 0;
		for(int w = 0; w < numWorkers; w++){

			if(remainingRows == 0) {
				MPISend(NULL, NULL, NULL, worker, 0, MPI_COMM_WORLD);//Cerrar,Dormir,...
			}
			else {

				if(remainingRows < grainSize)
					rowsPerWorker = remainingRows;
				else
					rowsPerWorker = grainSize;
				
				// Auxiliar row above
				workerID = w + 1;
				unsigned short* start = masterIndex[w].baseAddress;
				int numberOfCells = rowsPerWorker * worldWidth;

				// Auxiliar row above
				unsigned short* rowFromAbove;
				if(workerID == 1)
					rowFromAbove = world_ptr + ( (worldWidth * worldHeight) - worldWidth );
				else
					rowFromAbove = masterIndex[w].baseAddress - worldWidth;
				
				// Auxiliar row from under
				unsigned short* rowFromUnder;
				if(remainingRows < grainSize)
					rowFromUnder = world_ptr;
				else
					rowFromUnder = start + (rowsPerWorker * worldWidth );

				send_world_partition(rowFromAbove, start, rowFromUnder, worldWidth, numberOfCells, workerID);
				printf("%d cells sent to worker %d\n", rowsPerWorker, w + 1);

				//Actu values
				remainingRows -= rowsPerWorker;
				numberRecv++;
				world_Conteo += (rowsPerWorker * worldWidth);
			}
		}
		// ------------ Send the First Orde of portions to all workers ------------ //

		// Wait to the faster worker to end, then receive world partition and send next portion to the same one
		while(remainingRows != 0 && numberRecv != 0){
			
			MPI_Recv(aux, grainSize, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			numberRecv--;

			// Detect who sent that portion and the siz of it
			workerIndex = status.MPI_SOURCE - 1;
			workerID = workerIndex + 1;
			MPI_Get_count(&status, MPI_UNSIGNED_SHORT, &sizeReceived);
			printf("Update of %d cells received from worker %d\n", sizeReceived, workerIndex);
			
			// Update that world portion //and the new position of the worker
			writer_ptr = masterIndex[workerIndex].baseAddress;
			for(int i = 0; i < sizeReceived; i++)
				writer_ptr[i] = aux[i];
			
			if(remainingRows != 0) {
				masterIndex[workerIndex].baseAddress = world_Conteo;

				if(remainingRows < grainSize) {
					rowsPerWorker = remainingRows;
					

				}
				else {
					rowsPerWorker = grainSize;

				}
				masterIndex[workerIndex].size = rowsPerWorker;
				//masterIndex[i].offset = offset;
				

				// Auxiliar row above
				unsigned short* start = masterIndex[workerIndex].baseAddress;
				int numberOfCells = rowsPerWorker * worldWidth;

				// Auxiliar row above
				unsigned short* rowFromAbove = masterIndex[workerIndex].baseAddress - worldWidth;
				
				// Auxiliar row from under
				unsigned short* rowFromUnder;
				if(remainingRows < grainSize)
					rowFromUnder = world_ptr;
				else
					rowFromUnder = start + (rowsPerWorker * worldWidth );

				send_world_partition(rowFromAbove, start, rowFromUnder, worldWidth, numberOfCells, workerID);
				printf("%d cells sent to worker %d\n", rowsPerWorker, w + 1);

				//Actu values
				remainingRows -= rowsPerWorker;
				numberRecv++;
				world_Conteo += (rowsPerWorker * worldWidth);
				//offset += rowsPerWorker * worldWidth;
			}
		}
		// Wait to the faster worker to end, then receive world partition and send next portion to the same one

		// Update surface
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(400);

		++currentIteration;
		++cataclysmCycle;
	}

	free(worldA);
	free(worldB);
	free(masterIndex);
	free(aux);
}
// -------------------------------------------------- MASTER EXECUTION -------------------------------------------------- //