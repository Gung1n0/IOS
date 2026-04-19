#ifndef proj2_h
#define proj2_h
#include <stdbool.h>
sem_t sem;
typedef struct {
int counter;
int *waiting;
int done;
bool sem;
bool sem_eqN;
} mem;

typedef struct
{
    int *items;
    int front;
    int rear;
} queue;
#endif