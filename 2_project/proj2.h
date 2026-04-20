#ifndef proj2_h
#define proj2_h
#include <stdbool.h>

typedef struct {
    int counter;
    int waiting_V;
    int waiting_N;
    int finished_N;
    int total_N; 
    sem_t sem_V;
    sem_t sem_N;
    sem_t filling;
    sem_t finish;
    sem_t sem_V_start;
    sem_t write;
    sem_t ready;
    bool end;
} mem;
#endif