#include "graph.h"
#include "mpi.h"

// Enables/Disables the log messages from the master process
#define DEBUG_MASTER 1

// Probability that a cataclysm may occur [0-100] :(
#define PROB_CATACLYSM 100

// Number of iterations between two possible cataclysms
#define ITER_CATACLYSM 2

// Our structs
typedef struct {
    unsigned short* baseAddress;
    int size;
    int offset;
}tWorkerInfo;


void send_static_sizes(unsigned short* world, int worldWidth, int worldHeight, int workers, tWorkerInfo* masterIndex, int* maxSize);
void send_dynamic_sizes(unsigned short* world, int worldWidth, int worldHeight, const int maxWorkers, tWorkerInfo* masterIndex, const int granSize);

void send_world_partition(unsigned short* rowAbove, unsigned short* partition, unsigned short* rowUnder, const int auxSize, const int partitionSize, const int workerID);
void send_all_world_partitions(const unsigned short* world, const int workers, const int worldWidth, const int worldHeight, tWorkerInfo* masterIndex);
void receive_world_partitions(unsigned short* world, const int worldWidth, const int worldHeight, const int numWorkers, tWorkerInfo* masterIndex, const int maxSize);
void generate_cataclysm(unsigned short* world, const int worldWidth, const int worldHeight, const int fila, const int columna);

void masterStaticExecution (const int worldWidth, const int worldHeight, const int numWorkers, const int totalIterations, const int autoMode);
void masterDynamicExecution(const int worldWidth, const int worldHeight, const int numWorkers, const int totalIterations, const int autoMode, const int grainSize);