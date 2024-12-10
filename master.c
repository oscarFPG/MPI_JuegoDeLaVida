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
void send_dynamic_sizes(tWorkerInfo* masterIndex, const int numWorkers, const int worldWidth, const int grainSize){

	int offset = 0;
	for(int w = 0; w < numWorkers; w++){

		// Send world width
		MPI_Send(&worldWidth, 1, MPI_INT, w + 1, 0, MPI_COMM_WORLD);

		// Send size of piece to work with
		MPI_Send(&grainSize, 1, MPI_INT, w + 1, 0, MPI_COMM_WORLD);

		masterIndex[w].offset = offset;
		offset += grainSize;
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
	MPI_Status status;

	// Inicializar mundos
	clearWorld(worldA, worldWidth, worldHeight);
	clearWorld(worldB, worldWidth, worldHeight);

	// Create a random world		
	initRandomWorld(worldA, worldWidth, worldHeight);

	// Show first world state
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(renderer);
	drawWorld(worldA, worldA, renderer, 0, worldHeight, worldWidth, worldHeight);
	SDL_RenderPresent(renderer);
	SDL_UpdateWindowSurface(window);
	SDL_Delay(400);

	// Send to workers sizes of their partition
	send_dynamic_sizes(masterIndex, numWorkers, worldWidth, grainSize);

	// Game loop
	unsigned short* aux = malloc(sizeof(unsigned short) * grainSize);
	unsigned short* world_ptr = NULL, *newWorld_ptr = NULL, *writer_ptr = NULL;
	int rowsPerWorker;
	int workerID, workerIndex, remainingRows, sizeReceived;
	
	int currentIteration = 0;
	int cataclysmCycle = 0;
	int numberOfReceives;
	int offset = masterIndex[numWorkers - 1].offset;
	while(currentIteration < totalIterations){

		send_flag_to_workers(1, numWorkers);

		printf("ITERACION %d\n", currentIteration);
		if(autoMode == 0){
			printf("Press ENTER to continue execution...\n");
			getchar();
		}

		world_ptr = (currentIteration % 2 == 0) ? worldA : worldB;
		writer_ptr = (currentIteration % 2 == 0) ? worldA : worldB;
		newWorld_ptr = (currentIteration % 2 != 0) ? worldA : worldB;

		remainingRows = worldHeight;
		numberOfReceives = 0;

		// Enviar primera seccion de mapa a todos los workers
		unsigned short* rowFromAbove = world_ptr + ( (worldWidth * worldHeight) - worldWidth );
		unsigned short* rowFromUnder = world_ptr + (grainSize * worldWidth);
		for(int w = 0; w < numWorkers; w++){
			
			workerID = w + 1;
			count = 1;

			if(workerID == numWorkers && remainingRows <= grainSize)
				rowFromUnder = world_ptr;

			send_world_partition(rowFromAbove, writer_ptr, rowFromUnder, worldWidth, grainSize * worldWidth, workerID);

			// Actualizar punteros y variables
			rowFromAbove = rowFromUnder - worldWidth;
			writer_ptr = rowFromUnder;
			rowFromUnder += (grainSize * worldWidth);
			numberOfReceives++;
			remainingRows -= grainSize;
		}

		// Esperar a recibir y enviar al mismo worker
		while(numberOfReceives != 0){

			// Recibir
			MPI_Recv(aux, grainSize * worldWidth, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_UNSIGNED_SHORT, &sizeReceived);
			workerID = status.MPI_SOURCE;
			numberOfReceives--;
			printf("He recibido datos del worker %d - quedan por enviar %d recibir %d filas\n", workerID, remainingRows, numberOfReceives);

			// Actualizar nuevo mundo
			int k = masterIndex[workerID - 1].offset;
			for(int i = 0; i < sizeReceived; i++){
				if(currentIteration % 2 == 0)
					worldB[k + i] = aux[i];//aux mal y no vuelca datos
				else
					worldA[k + i] = aux[i];
			}
			

			// Enviar al mismo worker mas trabajo, si queda
			if(remainingRows > 0){
				
				int flag = 1;
				MPI_Send(&flag, 1, MPI_INT, workerID, 0, MPI_COMM_WORLD);

				unsigned short* rowFromAbove = writer_ptr - worldWidth;
			
				// Auxiliar row from under
				unsigned short* rowFromUnder;
				if(writer_ptr + ((grainSize - 1) * worldWidth) >= world_ptr + ( (worldWidth * worldHeight) - worldWidth ))
					rowFromUnder = world_ptr;
				else
					rowFromUnder = writer_ptr + (grainSize * worldWidth);

				send_world_partition(rowFromAbove, writer_ptr, rowFromUnder, worldWidth, grainSize * worldWidth, workerID);

				offset += grainSize;
				masterIndex[workerID - 1].offset = offset;
				numberOfReceives++;
				writer_ptr += (grainSize * worldWidth);
				remainingRows -= grainSize;
			}

		}

		printf("HEMOS ENVIADO Y RECIBIDO TODO\n");

		if(currentIteration % 2 == 0)
			drawWorld(worldA, worldB, renderer, 0, worldHeight, worldWidth, worldHeight);
		else
			drawWorld(worldB, worldA, renderer, 0, worldHeight, worldWidth, worldHeight);

		// Clear renderer
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);
		
		// Update surface
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		SDL_Delay(400);

		++currentIteration;
		++cataclysmCycle;
	}

	printf("HEMOS SALIDO DEL WHILE DE ITERATION\n");
	send_flag_to_workers(END_PROCESSING, numWorkers);
	free(worldA);
	free(worldB);
	free(masterIndex);
	free(aux);
}
// --------------------------------------------------- MASTER EXECUTE --------------------------------------------------- //