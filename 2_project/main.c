#include <stdlib.h>
#include <stdio.h>
#include "proj2.h"
void create_file()
{
FILE *file;
file=fopen("proj2.out", "w"); // "a" ked iba píšem
fclose(file);
}
void append_file(char *string)
{
FILE *file;
file=fopen("proj2.out", "a");
    int i = 0;
    i = fprintf(file,"%s", string);
    if(i == EOF)
    fprintf(stderr, "failed to write");
fclose(file);
}

int main(int argc, char const *argv[])
{
    int counter = 1;
    if (argc < 7)
    return 0;
    
    int V = atoi(argv[1]), N = atoi(argv[2]), K = atoi(argv[3]), TV=atoi(argv[4]), TN=atoi(argv[5]), O=atoi(argv[6]);
    if (!(V> 0 && V <10))
    {
        fprintf(stderr,"zlý počet vozíkov\n");
        return 0;
    }
    if (!(N> 0 &&  N<10000))
    {
        fprintf(stderr,"zlý počet navštevnikov\n");
        return 0;
    }
    if (!(K> 3 && K <41))
    {
        fprintf(stderr,"zlá kapacita vozíku\n");
        return 0;
    }
    if (!(TV> -1 && TV <1001))
    {
        fprintf(stderr,"zlá doba jízdy\n");
        return 0;
    }
    if (!(TN> -1 &&  TN<1001))
    {
        fprintf(stderr,"zlá maximálna doba\n");
        return 0;
    }
    if (!(O> 0 && O <101))
    {
        fprintf(stderr,"zlý odstup\n");
        return 0;
    }

    create_file();

    return 0;
}
