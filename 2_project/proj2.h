#ifndef proj2_h
#define proj2_h
#include <stdbool.h>

typedef struct {
    int counter;
    int waiting_V;
    int waiting_N;
    sem_t sem_V;
    sem_t sem_N;
    sem_t filling;
    sem_t finish;
} mem;
#endif