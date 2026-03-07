#pragma once

typedef void (*Fn)(void*);

typedef struct Job{
    Fn fn;
    void* arg;
    struct Job* next;
} Job;

void scheduler_init();
void scheduler_destroy();
void scheduler_job_push(Fn fn, void* arg);
