#include "proj2.h"

int main(int argc, char const *argv[])
{
    //kontrola správnych argumentov
    if (!(argc == 7))
    {
        fprintf(stderr, "too few arguments");
        exit(1);
    }
    //nastavenie hodnôt
    int V = atoi(argv[1]), N = atoi(argv[2]), K = atoi(argv[3]), TV = atoi(argv[4]), TN = atoi(argv[5]), O = atoi(argv[6]);

    //kontrola hodnôt
    int check = check_values(N, V, K, TV, TN, O);
    if (check == 1)
        exit(1);

    //vytvorenie súboru
    create_file();

    //vytvorenie zdielanej pamäte
    mem *shmem;
    shmem = create_shmem();
    if (shmem == NULL)
    {
        fprintf(stderr, "shared memory allocation failed");
        exit(1);
    }
    //inicializácia pamäte
    init_mem(shmem);
    //inicializácia semafórov
    init_sem(shmem);
    shmem->total_N = N;

    //nastavenie seedu
    srand(time(NULL));
    //vytvorenie premmenných
    char str[100];
    int id = fork();
    int idV = 0;
    int idN = 0;
    if (id == 0) // MAIN process
    {
        //vytvorenie vozíkov
        cart_create(V, &idV, str, shmem);
        if (idV != 0)
        {
            //cyklus vozíkov
            cart_cycle(shmem, str, &idV, K, TV);
        }

        // vytvorenie navštevnikov
        if (idV == 0 && id == 0)
        {
            create_visitor(N, &idN, str, shmem, TN);
        }
        //cyklus návštevníkov
        if (idV == 0 && idN != 0)
        {
            visitor_cycle(shmem, str, &idN);
        }
    }
    else if (id > 0)
    {
        // dispečer
        disp(shmem, str, O, V);
    }
    else
    {
        //v prípade zlyhania vytvorenia dispečera
        fprintf(stderr, "fork failed");
        exit(1);
    }

    //upratuje iba hlavný process
    if (id == 0 && idV == 0 && idN == 0)
    {
        for (int i = 0; i < V + N; i++)
            wait(NULL);
        clean(shmem);
    }
    exit(0);
}