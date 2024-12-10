#include "mpi.h"
#include "world.h"

// Enables/Disables the log messages from worker processes
#define DEBUG_WORKER 1


void executeWorker();

void receive_sizes_of_work(int* auxSize, int* workSize);
void receive_world_partition(unsigned short* partition, const int partitionSize, const int worldWidth);
void update_world_portion(unsigned short* world, unsigned short* newWorld, const int worldWidth, const int worldHeigth);
void send_world_partition_to_master(unsigned short* world, const int size);