#include "worker.h"

void receive_sizes_of_work(int rank, int* worldWidth, int* numberOfRows){

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

void executeWorker(const int rank){

    unsigned short* workerWorld = NULL;
    int portionSize = 0;
    int worldWidth, numberOfRows;

    receive_sizes_of_work(rank, &worldWidth, &numberOfRows);
    
    printf("worker %d with %d rows and %d width\n", rank, numberOfRows, worldWidth);

    portionSize = numberOfRows * worldWidth;
    workerWorld = malloc(sizeof(unsigned short) * (portionSize + worldWidth + worldWidth));
    receive_world_partition(workerWorld, portionSize, worldWidth);
    
    if(rank == 1) {
        printf("Mundo worker %d\n", rank);
        int count = 1;
        for(int j = 0; j < (portionSize + worldWidth + worldWidth); j++){
            printf("| %hu |", workerWorld[j]);
            if(count == worldWidth){
                count = 1;
                printf("\n");
            }
            else{
                count++;
                printf(" ");
            }
        }
        printf("\n\n");
    }
    

    // Update the working size of the world
    unsigned short* iniWorkingPortion = (workerWorld + worldWidth);
    unsigned short* newPortion = malloc(sizeof(unsigned short) * portionSize);
   
    tCoordinate cell;
    for(int row = 1; row < numberOfRows; row++){
        for(int col = 0; col < worldWidth; col++){
            cell.row = row;
            cell.col = col;
            updateCell(&cell, iniWorkingPortion, newPortion, worldWidth, numberOfRows);
        }
    }
    
    //updateWorld(iniWorkingPortion, newPortion, worldWidth, numberOfRows + 2, numberOfRows, 1);

    if(rank == 1) {
        printf("Mundo Update worker %d\n", rank);
        int count = 1;
        for(int j = 0; j < portionSize; j++){
            printf("| %hu |", newPortion[j]);
            if(count == worldWidth){
                count = 1;
                printf("\n");
            }
            else{
                count++;
                printf(" ");
            }
        }
        printf("\n\n");
    }

    return;
    // Send to master
}