#include "worker.h"

void receive_sizes_of_work(int rank, int* worldWidth, int* numberOfRows){

    MPI_Recv(worldWidth, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, NULL);
    MPI_Recv(numberOfRows, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, NULL);
    printf("The worker %d with auxilizar size of %d and working size of %d\n", rank, *worldWidth, *numberOfRows);
}

void receive_world_partition(unsigned short* partition, const int totalSize){
    MPI_Recv(partition, totalSize, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, NULL);
}