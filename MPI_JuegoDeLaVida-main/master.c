#include "master.h"

// ------------------------------------ MASTER AUXILIARY FUNCIONS ------------------------------------ //
unsigned short* getBaseAddressByIndex(const int index, const unsigned short* world, const int WIDTH) {
	return world + (index * WIDTH);
}

// ------------------------------------- MASTER ESTATIC FUNCIONS ------------------------------------- //
void send_number_of_rows_and_size(unsigned short* worldA, int worldWidth, int worldHeight, int maxWorkers, tWorkerInfo* masterIndex) {

	unsigned short* i_ptr = worldA;
	int remainingRows = worldHeight;
	int rowsPerWorker = (worldHeight % maxWorkers == 0) ? (worldHeight / maxWorkers) : ((worldHeight / maxWorkers) + 1);	// To round
	int worker = 1;
	while(worker <= maxWorkers){

		// Send world width
		MPI_Send(&worldWidth, 1, MPI_INT, worker, 0, MPI_COMM_WORLD);

		// Send size of piece to work with
		MPI_Send(&rowsPerWorker, 1, MPI_INT, worker, 0, MPI_COMM_WORLD);

		// Save first position in the world of each worker and number of cells in it
		masterIndex[worker - 1].baseAddress = i_ptr;
		masterIndex[worker - 1].size = rowsPerWorker * worldWidth;

		// If the remaining rows to distribute are less that the size we are distributing:
		// It's the last partition -> Assign it to the last worker
		remainingRows -= rowsPerWorker;
		if(remainingRows < rowsPerWorker)
			rowsPerWorker = remainingRows;

		i_ptr += masterIndex[worker - 1].size;
		++worker;
	}
}

void send_board_partitions(const unsigned short* worldA, const int workers, const int worldWidth, const int worldHeight, tWorkerInfo* masterIndex){

	for(int w = 0; w < workers; w++){
		int workerID = w + 1;
		unsigned short* start = masterIndex[w].baseAddress;
		int numberOfCells = masterIndex[w].size;

		// Send world partition to work with
		MPI_Send(start, numberOfCells, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);

		// Send auxiliar row above
		unsigned short* rowFromAbove;
		if(workerID == 1)
			rowFromAbove = worldA + ( (worldWidth * worldHeight) - worldWidth );
		else
			rowFromAbove = masterIndex[w].baseAddress - worldWidth;
		MPI_Send(rowFromAbove, worldWidth, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);


		// Send auxiliar row from under
		unsigned short* rowFromUnder;
		if(workerID == workers)
			rowFromUnder = worldA;
		else
			rowFromUnder = masterIndex[w + 1].baseAddress;
		MPI_Send(rowFromUnder, worldWidth, MPI_UNSIGNED_SHORT, workerID, 0, MPI_COMM_WORLD);
	}
}

void sendEstaticPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers) {

	// Repartir tablero entre los workers
	unsigned short* prt_i = worldA;
	int iniFila = 0, iniUp = 0, iniDown = 0, tamanio = 0, i = 1;
	int workersRestantes = workers, filasRestantes = worldHeight, numeroFilas;
	while(workersRestantes > 0) {

		// Mandar datos

		numeroFilas = filasRestantes / workersRestantes;

		// --------------------- Buffer Extra de Arriba --------------------- //
		//sii estamos en 0 => cojemos la ultima fila como la superior
		iniUp = (iniFila == 0) ? worldHeight - 1 : (iniFila - 1);
		prt_i = getBaseAddressByIndex(iniUp, worldA, worldWidth);
		MPI_Send(prt_i, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);	
		// --------------------- Buffer Extra de Arriba --------------------- //


		// ----------------------- Buffer de Trabajo ----------------------- //
		tamanio = numeroFilas * worldWidth;
		prt_i = getBaseAddressByIndex(iniFila, worldA, worldWidth);
		MPI_Send(prt_i, tamanio, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
		// ----------------------- Buffer de Trabajo ----------------------- //


		// --------------------- Buffer Extra de Abajo --------------------- //
		//sii estamos en  worldHeight - 1 => cojemos la primera fila como la *inferior
		iniDown = (iniFila + numeroFilas == worldHeight - 1) ? 0 : (iniFila + numeroFilas + 1);
		prt_i = getBaseAddressByIndex(iniDown, worldA, worldWidth);
		MPI_Send(prt_i, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);	
		// --------------------- Buffer Extra de Abajo --------------------- //

		// Actualizar estado
		iniFila += numeroFilas;
		filasRestantes -= numeroFilas;
		workersRestantes--;
		i++;	
	}
}

void recvEstaticPanel() {
	int a;
	MPI_Recv(&a, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, NULL);
}

void receive_world_partitions(unsigned short* newWorld, int totalsize, int nworkers){
	int recv = nworkers;
	unsigned short* auxWorld = malloc(totalsize * sizeof(unsigned short));
	MPI_Status status;

	while(recv != 0){

		MPI_Recv(auxWorld, totalsize, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);

		newWorld;
		recv--;
	}
}


// ------------------------------------- MASTER DINAMIC FUNCIONS ------------------------------------- //
void sendDinamicPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers) {

    int nFilasSend = 0, nFilasRecv = worldHeight, size = worldWidth;
	int* prt_i;
    // i es tanto el worquer como la fila
    for(int i = 0; i < workers; i++){

        MPI_Send(size, 1, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
        prt_i = getBaseAddressByIndex(i, worldA, worldWidth);
        MPI_Send(prt_i, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
        nFilasSend++;
	}


}