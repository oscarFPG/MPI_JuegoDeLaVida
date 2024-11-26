#include "master.h"

// ------------------------------------ MASTER AUXILIARY FUNCIONS ------------------------------------ //
unsigned short* getBaseAddressByIndex(const int index, const unsigned short* world, const int WIDTH) {
	return world + (index * WIDTH);
}

void initializeGame(unsigned short* worldA, unsigned short* worldB, int worldWidth, int worldHeight) {

	// Create empty worlds
	worldA = (unsigned short*) malloc (worldWidth * worldHeight * sizeof (unsigned short));
	worldB = (unsigned short*) malloc (worldWidth * worldHeight * sizeof (unsigned short));
	clearWorld(worldA, worldWidth, worldHeight);
	clearWorld(worldB, worldWidth, worldHeight);
			
	// Create a random world		
	initRandomWorld(worldA, worldWidth, worldHeight);
}
// ------------------------------------ MASTER AUXILIARY FUNCIONS ------------------------------------ //

// ------------------------------------- MASTER ESTATIC FUNCIONS ------------------------------------- //
void sendBasicEstaticInfo(unsigned short* worldA, int worldWidth, int worldHeight, int workers, tWorkerInfo* masterIndex) {

	//Mandar la informacion basica que necesitan los workers
	int * prt_i;
	int iniFila = 0, tamanio = 0;
	int numeroFilas, filasRestantes = worldHeight, size = worldWidth;
	int workersRestantes = workers, i = 1;
	while(workersRestantes >= 0) {

		numeroFilas = filasRestantes / workersRestantes;

		MPI_Send(&numeroFilas, 1, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
		MPI_Send(&size, 1, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
		
		//Index information
		tamanio = numeroFilas * worldWidth;
		prt_i = getBaseAddressByIndex(iniFila, worldA, worldWidth);
		masterIndex[i].ptr_ini = prt_i;
		masterIndex[i].size = tamanio;

		printf("worker %d  size %d ptr %x\n", i, masterIndex[i].size, masterIndex[i].ptr_ini);
		// Actualizar estado
		filasRestantes -= numeroFilas;
		workersRestantes--;
		iniFila += numeroFilas;
		i++;
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

void recvEstaticPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers, tWorkerInfo* masterIndex) {

	//while()
}
// ------------------------------------- MASTER ESTATIC FUNCIONS ------------------------------------- //

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
// ------------------------------------- MASTER DINAMIC FUNCIONS ------------------------------------- //