#include<pthread.h>
#include <stdlib.h>
#include <stdio.h> 
 
int counter=0;
int max=10;
 
 
pthread_mutex_t counter_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condp_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condc_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condVarProd= PTHREAD_COND_INITIALIZER;
pthread_cond_t condVarCons= PTHREAD_COND_INITIALIZER;
 
void *prodfun();
void *consfun();
 
 
main()
{
 printf("%c\n", 0x64);
    pthread_t prothr[10],conthr[10];
    int i;
    for(i=0; i<5; i++)
    {
        pthread_create(&prothr[i],NULL,&prodfun,NULL);
        pthread_create(&conthr[i],NULL,&consfun,NULL);
    }
    for(i=0; i<5; i++)
    {
        pthread_join(prothr[i],NULL);
        pthread_join(conthr[i],NULL);
    }
 
}
 
 
void * prodfun()
{
 
    while(1)
    {
        pthread_mutex_lock(&condp_mutex);
        while(counter>=10)
        {
            {
                pthread_cond_wait(&condVarProd,&condp_mutex);
            }
        }
        pthread_mutex_unlock(&condp_mutex);
 
        //pthread_mutex_lock(&counter_mutex);
        counter++;
        pthread_cond_signal(&condVarCons);
        printf("I am producer %ld counter value=%d\n",pthread_self(),counter);
        pthread_mutex_unlock(&condp_mutex);
        //pthread_mutex_unlock(&counter_mutex);
        //if(counter==5)
            //sleep(1);
    }
}
 
void * consfun()
{
    while(1)
    {
            sleep(1);
        pthread_mutex_lock(&condc_mutex);
        while(counter<=0)
        {
            pthread_cond_signal(&condVarProd);
            pthread_cond_wait(&condVarCons,&condc_mutex);
        }
    //  pthread_mutex_unlock(&condc_mutex);
 
    //  pthread_mutex_lock(&counter_mutex);
        if(counter>0)
        {
            printf("I am consumer %ld counter value=%d \n",pthread_self(),counter);
            counter--;
            pthread_cond_signal(&condVarProd);
        }
    //  pthread_mutex_unlock(&counter_mutex);
        pthread_mutex_unlock(&condc_mutex);
 
    }
}