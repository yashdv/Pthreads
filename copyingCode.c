/*
 * @author: Yash Vadalia
 *
 * ASSUMPTIONS:
 *
 * The a.out after compilation must be in the folder containing the files to
 * be copied.
 *
 * Input is from command line. the format is "./a.out <file1> <file2> <file3>
 * ... <file n> <destination path>" (qoutes for clarity). Other formats are not
 * allowed.
 *
 * n < 100.
 *
 * Number of threads created to do task cannot be altered by any input from
 * user.
 */

#include<stdio.h>
#include<semaphore.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>

#define blocksz 500
#define thread_cnt 10

sem_t sem[100];
sem_t sem2;

int block_cnt[100] = {0};
int size[100];
int blockdone[100] = {0};
int whichfile;
int ARGC;
char name[100][100];
char dest[100];
FILE **fc;

struct info
{
    int whichblock;
    int fileno;
};

void *copyx(void *arg)
{
    struct info *inf = (struct info *)arg;
    FILE *fp;
    char c;
    int i;
    int byte = blocksz;
    int myblock = inf->whichblock;
    int myfile = inf->fileno;
    int shift = (myblock-1) * blocksz;

    if(myblock == block_cnt[myfile] && size[myfile]%blocksz != 0)
        byte = size[myfile]%blocksz;

    fp = fopen(name[myfile],"r");
    fseek(fp, shift, SEEK_SET);

    sem_wait(&sem[myfile]);
    fseek(fc[myfile], shift, SEEK_SET);

    for(i=1; i<=byte; i++)
    {
        c = fgetc(fp);
        fprintf(fc[myfile],"%c",c);
    }

    sem_post(&sem[myfile]);
    fclose(fp);

    if(blockdone[whichfile] < block_cnt[whichfile])
    {
        struct info temp;

        sem_wait(&sem2);
        blockdone[whichfile]++;
        temp.whichblock = blockdone[whichfile];
        temp.fileno = whichfile;
        sem_post(&sem2);

        copyx((void *)&temp);
    }
    else if(blockdone[whichfile] == block_cnt[whichfile] && whichfile < ARGC-3)
    {
        struct info temp;

        sem_wait(&sem2);
        whichfile++;
        blockdone[whichfile]++;
        temp.whichblock = blockdone[whichfile];
        temp.fileno = whichfile;
        sem_post(&sem2);

        copyx((void *)&temp);
    }
}

int main(int argc, char *argv[])
{
    int i;
    int sumblock = 0;
    int whichblockmain;
    int whichfilemain;
    FILE *fp;
    struct info inf[100];
    char t[100];

    ARGC = argc;

    for(i=1; i<argc-1; i++)
    {
        strcpy(name[i-1],argv[i]);

        fp = fopen(name[i-1],"r");
        fseek(fp, 0L, SEEK_END);
        size[i-1] = ftell(fp);
        fclose(fp);

        block_cnt[i-1] = size[i-1]/blocksz + 1;
        if(size[i-1]%blocksz == 0)
            block_cnt[i-1]--;

        sumblock += block_cnt[i-1];
    }
    strcpy(name[i-1],argv[i]);

    i = strlen(name[argc-2]);
    if(name[argc-2][i-1] != '/')
        name[argc-2][i] = '/';

    for(i=1; i<argc-1; i++)
        sem_init(&sem[i-1],0,1);
    sem_init(&sem2,0,1);

    fc = (FILE **) malloc(sizeof(FILE*)*(argc-2));

    for(i=0; i<argc-2; i++)
    {
        strcpy(t,"\0");
        strcpy(t,name[argc-2]);
        strcat(t,name[i]);
        fc[i] = fopen(t,"w");
    }

    pthread_t thread[thread_cnt];

    whichfilemain = 0;
    whichblockmain = 0;
    
    for(i=1; i<=thread_cnt; i++)
    {
        whichblockmain++;

        if(whichblockmain > block_cnt[whichfilemain])
        {
            whichfilemain++;
            whichblockmain = 1;
        }

        inf[i].whichblock = whichblockmain;
        inf[i].fileno = whichfilemain;
        blockdone[whichfilemain]++;
    }
    whichfile = whichfilemain;

    for(i=1; i<=thread_cnt && i<=sumblock; i++)
        pthread_create(&thread[i],NULL,copyx,(void *)&inf[i]);

    for(i=1; i<=thread_cnt && i<=sumblock; i++)
        pthread_join(thread[i],NULL);

    for(i=0; i<argc-2; i++)
        fclose(fc[i]);

    for(i=1; i<argc-1; i++)
        sem_init(&sem[i],0,1);

    sem_destroy(&sem2);

    return 0;
}
