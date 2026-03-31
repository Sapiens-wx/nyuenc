#pragma once
#include <stdint.h>
#ifdef FILE_READER_IMPLEMENTATION
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "jobs.h"
#include "argparse.h"

static size_t sum_of_sizet_array(const size_t arr[], int len){
    size_t sum=0;
    for(int i=0;i<len;++i)
        sum+=arr[i];
    return sum;
}

/// @brief gets the size of each file in the argument variable
/// @return an array of file size
static size_t* get_file_sizes(){
    size_t *file_sizes=(size_t*)malloc(sizeof(size_t)*argument.files_cnt);
    for(int i=0;i<argument.files_cnt;++i){
        struct stat st;
        stat(argument.files[i], &st);
        file_sizes[i]=st.st_size; // -1 to ignore the terminating \n char
    }
    return file_sizes;
}

/// @brief allocates one single buffer for all the files in argument.files
/// @param file_sizes an array of file size
/// @param out_data_ptrs an array of pointers pointing at the start of each file's buffer (they are just pointers pointing to different part of the single buffer)
/// @return the address of the single buffer
static uint8_t* allocate_buffer(const size_t* file_sizes, uint8_t*** out_data_ptrs){
    size_t buffer_size=sum_of_sizet_array(file_sizes, argument.files_cnt);
    uint8_t* buffer=(uint8_t*)malloc(buffer_size);
    uint8_t** ptrs=(uint8_t**)malloc(sizeof(uint8_t*)*argument.files_cnt);
    ptrs[0]=buffer;
    for(int i=1;i<argument.files_cnt;++i){
        ptrs[i]=ptrs[i-1]+file_sizes[i-1];
    }
    // set outputs
    *out_data_ptrs=ptrs;
    return buffer;
}

static void read_to_buffer(const char* path, uint8_t* buffer, size_t max_size){
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror(path);
        return;
    }
    size_t bytes_read;
    uint8_t* buffer_it=buffer;
    while ((bytes_read = read(fd, buffer_it, max_size-(buffer_it-buffer))) > 0) {
        buffer_it+=bytes_read;
        if((size_t)(buffer_it-buffer)>=max_size) break;
    }
    close(fd);
}

#endif

/// @brief reads all files into a single buffer
/// @param out_buffer_size 
/// @return a pointer to the buffer. NULL if no files are given or if all files are of size 0
uint8_t* file_read_from_argument(size_t* out_buffer_size)
#ifndef FILE_READER_IMPLEMENTATION
;
#else
{
    if(argument.files_cnt==0) return NULL;
    size_t* file_sizes=get_file_sizes();
    uint8_t** data_ptrs;
    uint8_t* buffer=allocate_buffer(file_sizes, &data_ptrs);
    for(int i=0;i<argument.files_cnt;++i){
        if(file_sizes[i]>0)
            read_to_buffer(argument.files[i], data_ptrs[i], file_sizes[i]);
    }
    *out_buffer_size=sum_of_sizet_array(file_sizes, argument.files_cnt);
    if(*out_buffer_size==0) return NULL;

    // free resources
    free(file_sizes);
    free(data_ptrs);
    return buffer;
}
#endif

/// ===== multithread =====
#ifdef FILE_READER_IMPLEMENTATION
typedef struct ReadFileArg{
    int file_index_start_inclusive, file_index_end_exclusive;
    uint8_t** data_ptrs;
    size_t* file_sizes;
    sem_t* sem_completed_worker_cnt;
} ReadFileArg;

static void read_from_argument_multithread_worker(void* arg_void){
    ReadFileArg* arg=(ReadFileArg*)arg_void;
    for(int i=arg->file_index_start_inclusive;i<arg->file_index_end_exclusive;++i){
        read_to_buffer(argument.files[i], arg->data_ptrs[i], arg->file_sizes[i]);
    }
    sem_post(arg->sem_completed_worker_cnt);
}
#endif

/// @brief reads all files into a single buffer
/// @param out_buffer_size 
/// @return a pointer to the buffer. NULL if no files are given or if all files are of size 0
uint8_t* file_read_from_argument_multithread(size_t* out_buffer_size)
#ifndef FILE_READER_IMPLEMENTATION
;
#else
{
    if(argument.files_cnt==0) return NULL;
    size_t* file_sizes=get_file_sizes();
    uint8_t** data_ptrs;
    uint8_t* buffer=allocate_buffer(file_sizes, &data_ptrs);

    // pushing jobs
    int num_batches=argument.jobs;
    int batch_size=(argument.files_cnt+num_batches-1)/num_batches;
    int actual_num_batches=0;
    ReadFileArg* args=(ReadFileArg*)malloc(sizeof(ReadFileArg)*num_batches);
    sem_t* sem_completed_worker_cnt=(sem_t*)malloc(sizeof(sem_t));
    sem_init(sem_completed_worker_cnt, 0, 0);
    scheduler_begin_push_batch();
    for(int i=0;i<num_batches;++i){
        int actual_batch_size=i==num_batches-1?argument.files_cnt-i*batch_size:batch_size;
        int file_index_start_inclusive=i*batch_size;
        int file_index_end_exclusive=file_index_start_inclusive+actual_batch_size;
        if(file_index_start_inclusive<argument.files_cnt){
            args[i]=(ReadFileArg){file_index_start_inclusive, file_index_end_exclusive,
                data_ptrs, file_sizes, sem_completed_worker_cnt};
            scheduler_job_push(read_from_argument_multithread_worker, &args[i]);
            ++actual_num_batches;
        }
    }
    scheduler_end_push_batch();
    // wait for jobs to end
    for(int i=0;i<actual_num_batches;++i){
        sem_wait(sem_completed_worker_cnt);
    }
    // if all files are empty, return NULL
    *out_buffer_size=sum_of_sizet_array(file_sizes, argument.files_cnt);
    if(*out_buffer_size==0) return NULL;

    // free resources
    free(file_sizes);
    free(data_ptrs);
    free(args);
    sem_destroy(sem_completed_worker_cnt);
    return buffer;
}
#endif