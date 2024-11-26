#include "mpi.h"
#include "world.h"

// Enables/Disables the log messages from worker processes
#define DEBUG_WORKER 0


void work(int rank);

void recvBasicEstaticInfo(int rank, int* numeroFilas, int* size);