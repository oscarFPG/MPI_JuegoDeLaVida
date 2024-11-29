#include "mpi.h"
#include "world.h"

// Enables/Disables the log messages from worker processes
#define DEBUG_WORKER 0


void work(int rank);

void receive_sizes_of_work(int rank, int* auxSize, int* workSize);