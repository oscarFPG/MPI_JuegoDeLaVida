#include "graph.h"
#include "mpi.h"

// Enables/Disables the log messages from the master process
#define DEBUG_MASTER 1

// Probability that a cataclysm may occur [0-100] :(
#define PROB_CATACLYSM 100

// Number of iterations between two possible cataclysms
#define ITER_CATACLYSM 5

//Our struct
typedef struct {
    unsigned short* ptr_ini;
    int size;

}tWorkerInfo;

//Master auxiliary funcions
unsigned short* getBaseAddressByIndex(const int index, const unsigned short* world, const int WIDTH);
void initializeGame(unsigned short* worldA, unsigned short* worldB, int worldWidth, int worldHeight);

//Master estatic funcions
void sendBasicEstaticInfo(unsigned short* worldA, int worldWidth, int worldHeight, int workers, tWorkerInfo* masterIndex);
void sendEstaticPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers);
void recvEstaticPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers, tWorkerInfo* masterIndex);

//Master dinamic funcions
void sendDinamicPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers);