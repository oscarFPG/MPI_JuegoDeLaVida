#include "worker.h"

void recvBasicEstaticInfo(int rank, int* numeroFilas, int* size){

    MPI_Status status;

    MPI_Recv(numeroFilas, 1, MPI_INTEGER, MASTER, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(size, 1, MPI_INTEGER, MASTER, 0, MPI_COMM_WORLD, &status);

    printf("worker %d  size %d ptr %x\n", rank  , numeroFilas, size);
}

void work(int rank){

    int numeroFilas, size;
    recvBasicEstaticInfo(rank, &numeroFilas, &size);
    //recvEstaticInfo();
    //sendInfo();
}