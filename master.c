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
void sendBasicEstaticInfo(int worldWidth, int worldHeight, int workers, int* index) {

	//Mandar la informacion basica que necesitan los workers
    index[workers * 2];
	int numeroFilas, filasRestantes = worldHeight, size = worldWidth;
	int workersRestantes = workers, i = 0;;
	while(workersRestantes > 0) {
		numeroFilas = filasRestantes / workersRestantes;

		MPI_Send(numeroFilas, 1, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
		MPI_Send(size, 1, MPI_INTEGER, i, 0, MPI_COMM_WORLD);

        //guardo para cada worker: la fila donde empieza y la cantidad que tiene asignada
        index[i * 2] = i;
        index[(i * 2) + 1] = numeroFilas;

		filasRestantes -= numeroFilas;
		workersRestantes--;
		i++;
	}
}

void sendEstaticPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers) {

	// Repartir tablero entre los workers
	unsigned short* prt_i = worldA;
	int iniFila = 0, iniUp = 0, iniDown = 0, tamanio = 0, i = 0;
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
void recvEstaticPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers, int* index) {

}
// ------------------------------------- MASTER ESTATIC FUNCIONS ------------------------------------- //

// ------------------------------------- MASTER DINAMIC FUNCIONS ------------------------------------- //
void sendDinamicPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers) {

    int nFilasSend = 0, nFilasRecv = worldHeight;
    int index[workers];
    // i es tanto el worquer como la fila
    for(int i = 0; i < worker; i++){

        MPI_Send(size, 1, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
        prt_i = getBaseAddressByIndex(i, worldA, worldWidth);
        MPI_Send(prt_i, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
        nFilasSend++;
        index[i] = i;//guardo que fila le he dado al trabajador;
    }
}
// ------------------------------------- MASTER DINAMIC FUNCIONS ------------------------------------- //