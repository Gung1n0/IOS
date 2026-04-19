#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "proj2.h"

#define shared_mem "sh_mem"

void create_file()
{
    FILE *file;
    file = fopen("./proj2.out", "w");
    fclose(file);
}
void append_file(char *string, int *counter, int last_write)
{
    FILE *file;
    file = fopen("proj2.out", "a");
    int i = 0;
    if (last_write)
        i = fprintf(file, "%i: %s", *counter, string);
    else
        i = fprintf(file, "%i: %s\n", *counter, string);
    *counter = *counter + 1;
    if (i == EOF)
        fprintf(stderr, "failed to write");
    fclose(file);
}

mem *create_shmem()
{
    //                  memory      mode + flags(if exists return -1), R/W by owner
    int memory = shm_open(shared_mem, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (memory == -1)
    {
        shm_unlink(shared_mem);
        memory = shm_open(shared_mem, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (memory == -1)
            return NULL;
    }

    ftruncate(memory, sizeof(mem));

    mem *Mem = mmap(NULL, sizeof(mem), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0);
    if (Mem == MAP_FAILED)
        return NULL;
    else
        return Mem;
}

void init_mem(mem *shm)
{
    shm->counter = 1;
    shm->waiting = NULL;
    shm->done = 0;
    shm->sem = false;
}
int check_values(int N, int V, int K, int TV, int TN, int O)
{

    if (!(V > 0 && V < 10))
    {
        fprintf(stderr, "wrong cart count\n");
        return 1;
    }
    if (!(N > 0 && N < 10000))
    {
        fprintf(stderr, "wrong visitor count\n");
        return 1;
    }
    if (!(K > 3 && K < 41))
    {
        fprintf(stderr, "wrong cart capacity\n");
        return 1;
    }
    if (!(TV > -1 && TV < 1001))
    {
        fprintf(stderr, "wrong time\n");
        return 1;
    }
    if (!(TN > -1 && TN < 1001))
    {
        fprintf(stderr, "wrong max time\n");
        return 1;
    }
    if (!(O > 0 && O < 101))
    {
        fprintf(stderr, "wrong distance\n");
        return 1;
    }
}
int init_queue(queue *q, int x)
{
    int *temp = malloc(sizeof(int) * x);
    if (temp == NULL)
        return 0;
    else
    {
        q->items = temp;
        q->front = 0;
        q->rear = 0;
        return 1;
    }
}
void enqueue(queue *q, int value)
{
    q->items[q->rear] = value;
    q->rear++;
}
int deque(queue *q)
{
    int temp = q->items[q->front];
    q->front++;
    return temp;
}
bool is_empty(queue *q)
{
    return (q->rear == q->front);
}
void free_malloc(queue *q)
{
    free(q->items);
}
int main(int argc, char const *argv[])
{
    /*if (!(argc == 7))
    {
        fprintf(stderr, "too few arguments");
        exit(1);
    }*/

    // int V = atoi(argv[1]), N = atoi(argv[2]), K = atoi(argv[3]), TV = atoi(argv[4]), TN = atoi(argv[5]), O = atoi(argv[6]);
    int V = 3, N = 2, K = 4, TV = 1, TN = 1, O = 1;

    int check = check_values(N, V, K, TV, TN, O);
    if (check == 1)
        exit(1);

    create_file();

    queue qn;
    if (!init_queue(&qn, N))
    {
        fprintf(stderr, "malloc failed");
        exit(1);
    }
    queue qv;
    if (!init_queue(&qv, V))
    {
        fprintf(stderr, "malloc failed");
        exit(1);
    }

    mem *shmem;
    shmem = create_shmem();
    if (shmem == NULL)
    {
        fprintf(stderr, "shared memory allocation failed");
        exit(1);
    }
    init_mem(shmem);

    sem_init(&shmem->sem,1, 0);
    sem_init(&shmem->sem_eqN,1, 0);

    int id = fork();
    if (id == 0) // MAIN process
    {
        int idV = 0;
        int idN = 0;
        // vytvorenie vozíkov
        for (int i = 0; i < V; i++)
        {
            int temp = fork();
            if (temp == -1)
                exit(1);
            else if (temp == 0)
            {
                idV = i + 1;
                enqueue(&qv, idV);
                char str[100];
                sprintf(str, "V %i: started", idV);
                append_file(str, &shmem->counter, false);
                break;
            }
        }
        if (idV != 0)
        {
            printf("process s idv %i\n", idV);
        }

        // vytvorenie navštevnikov
        if (idV == 0 && id == 0)
        {
            for (int i = 0; i < N; i++)
            {
                int temp = fork();
                if (temp == -1)
                    exit(1);
                else if (temp == 0)
                {
                    idN = i + 1;
                    enqueue(&qn, idN);
                    char str[100];
                    sprintf(str, "N %i: started", idN);
                    append_file(str, &shmem->counter, false);
                    break;
                }
            }
            if (idN != 0)
            {
                printf("process s idn %i\n", idN);
            }
        }
        if (idV != 0)
        {
            //cakam na semafor ci možem
        for (int i = 0; i < V; i++)
        sem_wait(&shmem->sem);

        //priel semafor že možem
        deque(&qv); //tu som skončil, treba to deque načítať ktorý, atĎ
        }
    }
    else if (id > 0)
    {
        // dispečer
        int cart_capacity_counter = 0;
        append_file("D: started", &shmem->counter, false);
        while (true)
        {

            if (!is_empty(&qv) && shmem->waiting == 0)
            {
                append_file("D: next cart", &shmem->counter, false);
                //posielam signal že možem
                sem_post(&shmem->sem);
                usleep(O);
            }
            else if (is_empty(&qv))
            {
                append_file("D: closing", &shmem->counter, false);
                break;
            }
        }
    }
    else
    {
        fprintf(stderr, "fork failed");
        exit(1);
    }

    if (id > 0)
    {
        free_malloc(&qv);
        free_malloc(&qn);
        wait(NULL); // wait
        sem_destroy(&shmem->sem);
        munmap(shmem, sizeof(mem));
        shm_unlink(shared_mem);
    }
    return 0;
}