#include "worker.h"

void receive_sizes_of_work(int* worldWidth, int* numberOfRows){
    MPI_Recv(worldWidth, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, NULL);
    MPI_Recv(numberOfRows, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, NULL);
}

void receive_world_partition(unsigned short* partition, const int partitionSize, const int worldWidth){

    // Receive partition above
    MPI_Recv(partition, worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, NULL);

    // Receive partition to work with
    MPI_Recv(partition + worldWidth, partitionSize, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, NULL);

    // Receive partition under
    MPI_Recv(partition + partitionSize + worldWidth, worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, NULL);
}

void update_world_portion(unsigned short* world, unsigned short* newWorld, const int worldWidth, const int worldHeigth){

    tCoordinate cell;
    for(int row = 0; row < worldHeigth; row++){
        for(int col = 0; col < worldWidth; col++){
            cell.row = row;
            cell.col = col;
            updateCell(&cell, world, newWorld, worldWidth, worldHeigth);
        }
    }
}

void send_world_partition_to_master(unsigned short* world, const int size){
    MPI_Send(world, size, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD);
}


// Worker execution
void executeWorker(){

    unsigned short* workerWorld = NULL;
    unsigned short* newWorldPortion = NULL;
    int worldWidth, numberOfRows, worldPortionSize = 0;
    int END;

    receive_sizes_of_work(&worldWidth, &numberOfRows);

    // Alocate memory for the worker partition
    worldPortionSize = numberOfRows * worldWidth;
    workerWorld = malloc(sizeof(unsigned short) * (worldPortionSize + worldWidth + worldWidth));

    // Define the start of the working portion and allocate memory to store the new world generated
    newWorldPortion = malloc(sizeof(unsigned short) * worldPortionSize);

    do{
        MPI_Recv(&END, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, NULL);

        if(END != END_PROCESSING) {
                 // Receive world partition
        receive_world_partition(workerWorld, worldPortionSize, worldWidth);

        // Update the working size of the world
        update_world_portion((workerWorld + worldWidth), newWorldPortion, worldWidth, numberOfRows);

        // Send to master
        //send_world_partition_to_master(workerWorld + worldWidth, worldPortionSize);
        send_world_partition_to_master(newWorldPortion, worldPortionSize);
        }      
    }
    while(END != END_PROCESSING);

    // Free memory
    free(workerWorld);
    free(newWorldPortion);
}