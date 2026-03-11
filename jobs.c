#include "jobs.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include "argparse.h"

typedef struct Scheduler{
    Job* jobs_head, *jobs_tail;
    sem_t sem_jobs; // number of jobs
    sem_t mutex_jobs; // used for inserting or removing jobs
    sem_t mutex_is_running;
    int is_running;
    int is_pushing_batch;

    pthread_t* workers;
    int num_workers;
} Scheduler;

static Scheduler scheduler;

static void scheduler_set_is_running(int val){
    sem_wait(&scheduler.mutex_is_running);
    scheduler.is_running=val;
    sem_post(&scheduler.mutex_is_running);
}

static int scheduler_get_is_running(){
    int val;
    sem_wait(&scheduler.mutex_is_running);
    val=scheduler.is_running;
    sem_post(&scheduler.mutex_is_running);
    return val;
}

static Job* scheduler_job_pop(){
    Job* job=NULL;
    sem_wait(&scheduler.mutex_jobs);
    if(scheduler.jobs_head->next!=NULL){
        job=scheduler.jobs_head->next;
        scheduler.jobs_head->next=job->next;
        if(scheduler.jobs_head->next==NULL)
            scheduler.jobs_tail=scheduler.jobs_head;
    }
    sem_post(&scheduler.mutex_jobs);
    return job;
}

static void* event_loop(void* arg){
    arg=(char*)arg+1; // avoid unused params
    while(scheduler_get_is_running()){
        sem_wait(&scheduler.sem_jobs);
        Job* job=scheduler_job_pop();
        if(job)
            job->fn(job->arg);
    }
    return NULL;
}

void scheduler_init(){
    scheduler.is_pushing_batch=0;
    sem_init(&scheduler.sem_jobs, 0, 0);
    sem_init(&scheduler.mutex_jobs, 0, 1);
    sem_init(&scheduler.mutex_is_running, 0, 1);
    scheduler.jobs_head=(Job*)malloc(sizeof(Job));
    *scheduler.jobs_head=(Job){NULL, NULL, NULL};
    scheduler.jobs_tail=scheduler.jobs_head;
    // start the workers
    scheduler_set_is_running(1);
    scheduler.num_workers=argument.jobs;
    scheduler.workers=(pthread_t*)malloc(sizeof(pthread_t)*scheduler.num_workers);
    for(int i=0;i<scheduler.num_workers;++i){
        pthread_create(&scheduler.workers[i], NULL, &event_loop, NULL);
    }
}

void scheduler_destroy(){
    scheduler_set_is_running(0);
    for(int i=0;i<scheduler.num_workers;++i)
        sem_post(&scheduler.sem_jobs);
    // wait for workers to terminate
    for(int i=0;i<scheduler.num_workers;++i){
        pthread_join(scheduler.workers[i], NULL);
    }

    // free memory
    free(scheduler.workers);
    for(Job* job=scheduler.jobs_head; job; ){
        Job* next=job->next;
        free(job);
        job=next;
    }

    sem_destroy(&scheduler.sem_jobs);
    sem_destroy(&scheduler.mutex_jobs);
    sem_destroy(&scheduler.mutex_is_running);
}

void scheduler_job_push(Fn fn, void* arg){
    // initialize the job
    Job* job=(Job*)malloc(sizeof(Job));
    *job=(Job){fn, arg, NULL};
    // push job
    if(scheduler.is_pushing_batch==0)
        sem_wait(&scheduler.mutex_jobs);
    scheduler.jobs_tail->next=job;
    scheduler.jobs_tail=job;
    if(scheduler.is_pushing_batch==0)
        sem_post(&scheduler.mutex_jobs);
    // post sem_jobs
    sem_post(&scheduler.sem_jobs);
}

void scheduler_begin_push_batch(){
    sem_wait(&scheduler.mutex_jobs);
    scheduler.is_pushing_batch=1;
}

void scheduler_end_push_batch(){
    scheduler.is_pushing_batch=0;
    sem_post(&scheduler.mutex_jobs);
}