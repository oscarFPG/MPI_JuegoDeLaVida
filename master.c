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

void send_flag_to_workers(const int flag, const int workers){

	for(int w = 0; w < workers; w++){
		MPI_Send(&flag, 1, MPI_INT, w + 1, 0, MPI_COMM_WORLD);
	}
}

void get_next_row_indexes(int* rowAbove, int* workRow, int* rowUnder,
						const int worldHeight, const int grainSize){

	// Update above row index
	if(*rowAbove == worldHeight - 1)
		*rowAbove = 0;
	else
		*rowAbove = *rowUnder - 1;

	// Update working row index
	*workRow = *rowUnder;

	// Update under row index
	if(*rowUnder == worldHeight - 1)
		*rowUnder = 0;
	else
		*rowUnder = *rowUnder + grainSize;
}


// -------------------------------------------------- ESTATIC FUNCTIONS -------------------------------------------------- //
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


// -------------------------------------------------- DINAMIC FUNTIONS -------------------------------------------------- //
void send_dynamic_sizes(const int numWorkers, const int worldWidth, const int grainSize){

	for(int w = 0; w < numWorkers; w++){

		// Send world width
		MPI_Send(&worldWidth, 1, MPI_INT, w + 1, 0, MPI_COMM_WORLD);

		// Send size of piece to work with
		MPI_Send(&grainSize, 1, MPI_INT, w + 1, 0, MPI_COMM_WORLD);
	}
}

void send_initial_world_portions(unsigned short* world, const int worldWidth, const int worldHeight,
								const int grainSize, const int numWorkers){
	
	unsigned short* rowAbove;
	unsigned short* rowStart;
	unsigned short* rowUnder;
	int rowAboveIndex = worldHeight - 1;
	int rowStartIndex = 0;
	int rowUnderIndex = grainSize;
	for(int w = 0; w < numWorkers; w++){

		rowAbove = world + (worldWidth * rowAboveIndex);
		rowStart = world + (worldWidth * rowStartIndex);
		rowUnder = world + (worldWidth * rowUnderIndex);
		send_world_partition(rowAbove, rowStart, rowUnder, worldWidth, grainSize * worldWidth, w + 1);
		get_next_row_indexes(&rowAboveIndex, &rowStartIndex, &rowUnderIndex, worldHeight, grainSize);
	}
}

// -------------------------------------------------- MASTER EXECUTION -------------------------------------------------- //
void masterStaticExecution(const int worldWidth, const int worldHeight, const int numWorkers, const int totalIterations, const int autoMode, char* filename){

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
		
		send_flag_to_workers(1, numWorkers);

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

	send_flag_to_workers(END_PROCESSING, numWorkers);
	free(worldA);
	free(worldB);
	free(masterIndex);

	if(filename != NULL)
		saveImage(renderer, filename, worldWidth * CELL_SIZE, worldHeight * CELL_SIZE);
}

void masterDynamicExecution(const int worldWidth, const int worldHeight, const int numWorkers, const int totalIterations, const int autoMode, const int grainSize){

	/*
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
	*/

	// Sim logic
	unsigned short* worldA = (unsigned short*) malloc(sizeof(unsigned short) * worldWidth * worldHeight);
	unsigned short* worldB = (unsigned short*) malloc(sizeof(unsigned short) * worldWidth * worldHeight);
	tWorkerInfo* masterIndex = malloc(sizeof(tWorkerInfo) * numWorkers);
	MPI_Status status;
	int flag;

	// Inicializar mundos
	clearWorld(worldA, worldWidth, worldHeight);
	clearWorld(worldB, worldWidth, worldHeight);

	// Create a random world		
	initRandomWorld(worldA, worldWidth, worldHeight);

	int count = 1;
	for(int i = 0; i < worldWidth * worldHeight; i++){
		printf("| %hu |", worldA[i]);
		if(count == 6){
			count = 1;
			printf("\n");
		}
		else{
			count++;
			printf(" ");
		}
	}

	// Show first world state
	//SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
	//SDL_RenderClear(renderer);
	//drawWorld(worldA, worldB, renderer, 0, worldHeight, worldWidth, worldHeight);
	//SDL_RenderPresent(renderer);
	//SDL_UpdateWindowSurface(window);
	//SDL_Delay(400);

	// Send to workers sizes of their partition
	send_dynamic_sizes(numWorkers, worldWidth, grainSize);

	// Game loop
	unsigned short* aux = malloc(sizeof(unsigned short) * grainSize * worldWidth);
	unsigned short* world_ptr;
	unsigned short* newWorld_ptr;
	int aboveRowIndex;
	int underRowIndex;
	int currentIteration = 0;
	int cataclysmCycle = 0;
	int currentRow = 0;
	while(currentIteration < totalIterations){

		send_flag_to_workers(1, numWorkers);

		printf("ITERACION %d\n", currentIteration);
		if(autoMode == 0){
			printf("Press ENTER to continue execution...\n");
			getchar();
		}

		// Actualizar la informacion de los workers -> Fila que contienen
		for(int w = 0; w < numWorkers; w++){
			masterIndex[w].row = (w * grainSize);
			currentRow += grainSize;
		}

		// Send world portion to all workers
		world_ptr = (currentIteration % 2 == 0) ? worldA : worldB;
		newWorld_ptr = (currentIteration % 2 == 0) ? worldB : worldA;
		send_initial_world_portions(world_ptr, worldWidth, worldHeight, grainSize, numWorkers);


		// Mandar hasta que hayamos llegado hasta el final del tablero
		int remainingRows = worldHeight - (numWorkers * grainSize);
		int receivesLeft = numWorkers;	// Debemos esperar, como minimo, a los n workers anteriores
		int workerID, sizeReceived;
		MPI_Status status;

		aboveRowIndex = currentRow - 1;
		underRowIndex = currentRow + grainSize;
		while(remainingRows > 0){
			
			MPI_Recv(aux, grainSize * worldWidth, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_UNSIGNED_SHORT, &sizeReceived);
			workerID = status.MPI_SOURCE;
			remainingRows -= grainSize;
			receivesLeft--;

			printf("Fila recibida por el worker %d, quedan %d receives\n", workerID, receivesLeft);
			for(int i = 0; i < sizeReceived; i++){
				newWorld_ptr[currentRow * worldWidth + i] = aux[i];
				printf("| %hu |", aux[i]);
			}
			printf("\n");

			printf("Nuevas posiciones %d - %d - %d\n", aboveRowIndex, currentRow, underRowIndex);
			get_next_row_indexes(&aboveRowIndex, &currentRow, &underRowIndex,
								worldHeight, grainSize);

			flag = 1;
			MPI_Send(&flag, 1, MPI_INT, workerID, 0, MPI_COMM_WORLD);

			masterIndex[workerID - 1].row = currentRow;
			send_world_partition(world_ptr + (aboveRowIndex * worldWidth),
								world_ptr + (currentRow * worldWidth),
								world_ptr + (underRowIndex * worldWidth),
								worldWidth, grainSize * worldWidth, workerID
								);
			remainingRows -= grainSize;
			receivesLeft++;
			printf("Mandamos de nuevo al worker %d\n", workerID);
		}

		while(receivesLeft > 0){

			MPI_Recv(aux, grainSize * worldWidth, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_UNSIGNED_SHORT, &sizeReceived);
			workerID = status.MPI_SOURCE;
			remainingRows -= grainSize;
			receivesLeft--;

			printf("Fila recibida por el worker %d al final: quedan %d receives\n", workerID, receivesLeft);
			for(int i = 0; i < sizeReceived; i++){
				newWorld_ptr[masterIndex[workerID - 1].row * worldWidth + i] = aux[i];
				printf("| %hu |", aux[i]);
			}
			printf("\n");
		}

		// Clear renderer
		//SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		//SDL_RenderClear(renderer);
		
		// Update surface
		//SDL_RenderPresent(renderer);
		//SDL_UpdateWindowSurface(window);
		//SDL_Delay(400);

		++currentIteration;
		++cataclysmCycle;
	}

	send_flag_to_workers(END_PROCESSING, numWorkers);
	free(worldA);
	free(worldB);
	free(masterIndex);
	free(aux);
}