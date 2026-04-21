#ifndef proj2_h
#define proj2_h
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#define shared_mem "sh_mem"

//štruktúra zdielanej pamäte
typedef struct
{
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
// vytvorí alebo vymaže výstupný súbor.
void create_file()
{
    FILE *file;
    file = fopen("./proj2.out", "w");
    fclose(file);
}

// zapíše riadok do výstupného súboru
void append_file(char *string, int *counter, int last_write)
{
    FILE *file;
    file = fopen("proj2.out", "a");
    int failed = 0;
    // ak ide o posledný zápis, neposunie sa na nový riadok
    if (last_write)
        failed = fprintf(file, "%i: %s", *counter, string);
    else
        failed = fprintf(file, "%i: %s\n", *counter, string);
    // po vypísaní zvýši počítadlo o 1
    *counter = *counter + 1;
    // v prípade že zapís zlyhal vypíše na stderr
    if (failed == EOF)
        fprintf(stderr, "failed to write");
    fclose(file);
}
// synchronizuje zápis do súboru
// použije semafor write, ktorý je inicializovaný na 1
// prvý write vždy prejde a ak už nejaký process zapisuje, tak všetky ostatné čakajú
void wait_for_write(mem *shmem, char *str)
{
    sem_wait(&shmem->write);
    append_file(str, &shmem->counter, false);
    sem_post(&shmem->write);
}

//vytvorenie zdielanej pamäte
mem *create_shmem()
{
    //pamäť, mode + flags(if exists return -1), R/W by owner
    int memory = shm_open(shared_mem, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    //ak existuje tak ju zruší a vytvorí novú
    if (memory == -1)
    {
        shm_unlink(shared_mem);
        memory = shm_open(shared_mem, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (memory == -1)
            return NULL;
    }

    //nastavenie velkosti zdielanej pamäte
    ftruncate(memory, sizeof(mem));

    //alokovanie pamate, permisions(read + write), flags(viditelna všetkými procesmi)
    mem *Mem = mmap(NULL, sizeof(mem), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0);
    if (Mem == MAP_FAILED)
        return NULL;
    else
        return Mem;
}
//inicializovanie semafórov
void init_mem(mem *shm)
{
    shm->counter = 1;
    shm->waiting_V = 0;
    shm->waiting_N = 0;
    shm->finished_N = 0;
    shm->total_N = 0;
    shm->end = false;
}

// upratanie po skončení súboru
void clean(mem *shmem)
{
    //zmaže posledný znak (\n)
    FILE *file;
    file = fopen("proj2.out", "a");
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    if (size > 0)
        ftruncate(fileno(file), size - 1);
    fclose(file);

    sem_destroy(&shmem->sem_V);
    sem_destroy(&shmem->sem_N);
    sem_destroy(&shmem->sem_V_start);
    sem_destroy(&shmem->filling);
    sem_destroy(&shmem->finish);
    sem_destroy(&shmem->write);
    sem_destroy(&shmem->ready);
    munmap(shmem, sizeof(mem));
    shm_unlink(shared_mem);
}

//kontrola vstupných hodnôt
int check_values(int N, int V, int K, int TV, int TN, int O)
{
    //kontrola Vozíkov
    if (!(V > 0 && V < 10))
    {
        fprintf(stderr, "wrong cart count\n");
        return 1;
    }
    //kontrola Navšetvníkov
    if (!(N > 0 && N < 10000))
    {
        fprintf(stderr, "wrong visitor count\n");
        return 1;
    }
    //kontrola Kapacity
    if (!(K > 3 && K < 41))
    {
        fprintf(stderr, "wrong cart capacity\n");
        return 1;
    }
    //kontrola času vozíka
    if (!(TV > -1 && TV < 1001))
    {
        fprintf(stderr, "wrong time\n");
        return 1;
    }
    //kontrola času návštevníka
    if (!(TN > -1 && TN < 1001))
    {
        fprintf(stderr, "wrong max time\n");
        return 1;
    }
    //kontrola časového odstupu
    if (!(O > 0 && O < 101))
    {
        fprintf(stderr, "wrong distance\n");
        return 1;
    }
    return 0;
}

//vracia náhodnú hodnotu v rozsahu
int get_random(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

//funkcia / cyklus dispečera
void disp(mem *shmem, char *str, int O, int V)
{
    //ked sa vytvorí process napíše started
    sprintf(str, "D: started");
    wait_for_write(shmem, str);

    //ak neboli všetci obslúžený tak pokračuje v cykle
    do
    {
        //zavolá další vozík signálom sem_v_start
        sprintf(str, "D: next cart");
        wait_for_write(shmem, str);
        sem_post(&shmem->sem_V_start);
        //čaká kým vozík neskončí
        sem_wait(&shmem->filling);
        //čaká pre bezpečný výstup
        usleep(O);
    } while (shmem->finished_N < shmem->total_N);
    sprintf(str, "D: closing");
    wait_for_write(shmem, str);
    //vyšle signál, ktorý signalizuje, že vozíky môžu skončiť
    shmem->end = true;

    //vyšle signál pre všektky vozíky nech skončia
    for (int i = 0; i < V; i++)
        sem_post(&shmem->sem_V_start);
}
//funkcia / cyklus pre návštevníka
void visitor_cycle(mem *shmem, char *str, int *idN)
{
    //keď je zavolaný, tak sa zaradí do rady
    sprintf(str, "N %i: queue", *idN);
    wait_for_write(shmem, str);

    //čaká kým vozík vyšle signál boarding started
    sprintf(str, "N %i: boarding", *idN);
    sem_wait(&shmem->sem_N);
    wait_for_write(shmem, str);
    //odpočíta sa od čakajúcich
    shmem->waiting_N--;
    sem_post(&shmem->sem_V);

    //čaká na skončenie jazdy a potom sa ukončí
    sprintf(str, "N %i: leaving", *idN);
    sem_wait(&shmem->finish);
    wait_for_write(shmem, str);
    shmem->finished_N++;
    sem_post(&shmem->sem_V);
}

//vytvorenie návštevníkov
void create_visitor(int N, int *idN, char *str, mem *shmem, int TN)
{
    for (int i = 0; i < N; i++)
    {
        //kontorla či bolo vytvorenie procesu úspešné
        int temp = fork();
        if (temp == -1)
            exit(1);
        else if (temp == 0)
        {
            //ak bolo vytvorenie procesu úspešné, tak mu pridelí ID návšetvníka
            *idN = i + 1;
            sprintf(str, "N %i: started", *idN);
            wait_for_write(shmem, str);
            //čaká náhodnú dobu v intervale <0,TN>
            usleep(get_random(0, TN));
            //pripočíta sa k čakajúcim
            shmem->waiting_N++;
            //vyšle signál že je vytvorený
            sem_post(&shmem->ready);
            break;
        }
    }
}
//funkcia / cyklus pre vozík
void cart_cycle(mem *shmem, char *str, int *idV, int K, int TV)
{
    //pokým dispečer neskončil tak pokračuj
    while (!shmem->end)
    {
        sprintf(str, "V %i: boarding started", *idV);
        //čaká na signál dispečera na začatie nástupu
        sem_wait(&shmem->sem_V_start);
        //ak už skončil tak ukonči
        if (shmem->end)
            break;
        wait_for_write(shmem, str);
        //odčítanie od čakajúcich vozíkov (vozík odišiel)
        shmem->waiting_V--;
        //výpočet kolko návštevníkov ešte ostáva
        int remaining = shmem->total_N - shmem->finished_N;
        int board;

        //nastavenie kapacity vozíku
        sem_wait(&shmem->write);
        if (remaining < K)
            board = remaining;
        else
            board = K;
        sem_post(&shmem->write);

        //čaká pokým všetci navšetvníci nezaradili do fronty
        for (int i = 0; i < board; i++)
            sem_wait(&shmem->ready);

        //čaká pokým  všetci navšetvníci nenastúpili
        for (int i = 0; i < board; i++)
            sem_post(&shmem->sem_N);
        for (int j = 0; j < board; j++)
            sem_wait(&shmem->sem_V);

        sprintf(str, "V %i: boarding complete", *idV);
        wait_for_write(shmem, str);
        //čaká náhodnú dobu v intervale <TN/2,TN>
        usleep(get_random(TV / 2, TV));

        sprintf(str, "V %i: leaving started", *idV);
        wait_for_write(shmem, str);

        //počká kým všetci návštevníci nevystúia
        for (int i = 0; i < board; i++)
            sem_post(&shmem->finish);
        for (int j = 0; j < board; j++)
            sem_wait(&shmem->sem_V);

        sprintf(str, "V %i: leaving complete", *idV);
        wait_for_write(shmem, str);
        //keď všetci vystúpia tak vráti vozík naspäť do fronty a čaká čo povie dispečer 
        shmem->waiting_V++;
        sem_post(&shmem->filling);
    }
    //ak dispečer rozhodne o konci, tak vypíše koniec a ukončí proces
    sprintf(str, "V %i: closed", *idV);
    wait_for_write(shmem, str);
}

// vytvorenie vozíkov
void cart_create(int V, int *idV, char *str, mem *shmem)
{
    for (int i = 0; i < V; i++)
    {
        //kontorla či bolo vytvorenie procesu úspešné
        int temp = fork();
        if (temp == -1)
            exit(1);
        else if (temp == 0)
        {
            //ak bolo vytvorenie procesu úspešné, tak mu pridelí ID vozíka
            *idV = i + 1;
            sprintf(str, "V %i: started", *idV);
            wait_for_write(shmem, str);
            shmem->waiting_V++;
            break;
        }
    }
}
//inicializovanie semafórov 
void init_sem(mem *shmem)
{
    //zo začiatku sú semafóry vypnuté
    sem_init(&shmem->sem_V, 1, 0);
    sem_init(&shmem->sem_V_start, 1, 0);
    sem_init(&shmem->sem_N, 1, 0);
    sem_init(&shmem->filling, 1, 0);
    sem_init(&shmem->finish, 1, 0);
    sem_init(&shmem->write, 1, 1);//zo začiatku je semafór zapnutý
    sem_init(&shmem->ready, 1, 0);
}