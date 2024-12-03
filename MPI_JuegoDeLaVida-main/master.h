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
    unsigned short* baseAddress;
    int size;
}tWorkerInfo;

// Master auxiliary funcions
unsigned short* getBaseAddressByIndex(const int index, const unsigned short* world, const int WIDTH);

// Master estatic funcions
void send_number_of_rows_and_size(unsigned short* worldA, int worldWidth, int worldHeight, int workers, tWorkerInfo* masterIndex);
void send_board_partitions(const unsigned short* worldA, const int workers, const int worldWidth, const int worldHeight, tWorkerInfo* masterIndex);
void sendEstaticPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers);
void recvEstaticPanel();

// Master dinamic funcions
void sendDinamicPanel(unsigned short* worldA, int worldWidth, int worldHeight, int workers);