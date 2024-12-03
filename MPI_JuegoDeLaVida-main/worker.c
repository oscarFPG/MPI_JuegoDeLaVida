#include "worker.h"

void receive_sizes_of_work(int rank, int* worldWidth, int* numberOfRows){

    MPI_Recv(worldWidth, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, NULL);
    MPI_Recv(numberOfRows, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, NULL);
}

void receive_world_partition(unsigned short* partition, unsigned short* rowAbove, unsigned short* rowUnder, const int totalSize, const int partitionSize){

    // Receive partition to work with
    MPI_Recv(partition, totalSize, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, NULL);

    // Receive partition above
    MPI_Recv(partition + totalSize, partitionSize, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, NULL);

    // Receive partition under
    MPI_Recv(partition + totalSize + partitionSize, partitionSize, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, NULL);

    for(int i = 0; i < partitionSize + totalSize + partitionSize; i++){
			printf("| %hu | ", partition[i]);
	}
}