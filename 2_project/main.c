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
    shm->waiting_V = 0;
    shm->waiting_N = 0;
}
void clean(mem *shmem)
{
    wait(NULL); // wait
    sem_destroy(&shmem->sem_V);
    sem_destroy(&shmem->sem_N);
    munmap(shmem, sizeof(mem));
    shm_unlink(shared_mem);
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

    mem *shmem;
    shmem = create_shmem();
    if (shmem == NULL)
    {
        fprintf(stderr, "shared memory allocation failed");
        exit(1);
    }
    init_mem(shmem);

    sem_init(&shmem->sem_V, 1, 0);
    sem_init(&shmem->sem_N, 1, 0);
    sem_init(&shmem->filling, 1, 0);
    sem_init(&shmem->finish, 1, 0);

    char str[100];

    int id = fork();
    int idV = 0;
    int idN = 0;
    if (id == 0) // MAIN process
    {
        // vytvorenie vozíkov
        for (int i = 0; i < V; i++)
        {
            int temp = fork();
            if (temp == -1)
                exit(1);
            else if (temp == 0)
            {
                idV = i + 1;
                sprintf(str, "V %i: started", idV);
                append_file(str, &shmem->counter, false);
                shmem->waiting_V++;
                break;
            }
        }
        if (idV != 0)
        {
            // printf("process s idv %i\n", idV);
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
                    sprintf(str, "N %i: started", idN);
                    append_file(str, &shmem->counter, false);
                    usleep(TN);
                    sprintf(str, "N %i: queue", idN);
                    shmem->waiting_N++;
                    break;
                }
            }
        }
        bool stop = false;
        if (idV != 0)
        {
            sprintf(str, "V %i: boarding started", idV);

            sem_wait(&shmem->sem_V);
            append_file(str, &shmem->counter, false);
            shmem->waiting_V--;
            for (int i = 0; i < K; i++)
                sem_post(&shmem->sem_N);
            
        }
        else if (idV == 0 && id == 0)
        {
            sprintf(str, "N %i: boarding", idN);
            sem_wait(&shmem->sem_N);
            append_file(str, &shmem->counter, false);
            shmem->waiting_N--;

            sprintf(str, "N %i: leaving", idN);
            sem_wait(&shmem->finish);
            append_file(str, &shmem->counter, false);
        }
    }
    else if (id > 0)
    {
        // dispečer
        append_file("D: started", &shmem->counter, false);
        for (int i = 0; i < V; i++)
        {
            append_file("D: next cart", &shmem->counter, false);
            if (shmem->waiting_V == 0)
                sem_post(&shmem->sem_V);
            sem_wait(&shmem->filling);
            usleep(O);
        }
        append_file("D: closing", &shmem->counter, false);
    }
    else
    {
        fprintf(stderr, "fork failed");
        exit(1);
    }

    if (id > 0)
        clean(shmem);
    exit(0);
}