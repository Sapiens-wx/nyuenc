#define _XOPEN_SOURCE 700
#include "encoder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "argparse.h"
#include "jobs.h"

static void write_encoded_value(uint8_t val, uint8_t cnt, uint8_t* buffer, int* len){
    int buffer_len=*len;
    buffer[buffer_len++]=val;
    buffer[buffer_len++]=cnt;
    *len=buffer_len;
}

static int sum_of_array(const int arr[], int arr_len){
    int sum=0;
    for(int i=0;i<arr_len;++i)
        sum+=arr[i];
    return sum;
}

/// @brief encode a given chunk into out_encoded. out_encoded should be allocated by caller
/// @param out_size actual used size of out_encoded
static void encode_internal(const uint8_t* chunk, const int size, uint8_t* out_encoded, int* out_size){
    uint8_t cur_val=chunk[0];
    uint8_t cur_cnt=1;
    int out_encoded_len=0;
    for(int i=1;i<size;++i){
        if(chunk[i]==cur_val){ // duplicate value
            ++cur_cnt;
        } else{ // encounters a new value. encode the previous value
            write_encoded_value(cur_val, cur_cnt, out_encoded, &out_encoded_len);
            cur_val=chunk[i];
            cur_cnt=1;
        }
    }
    write_encoded_value(cur_val, cur_cnt, out_encoded, &out_encoded_len);
    *out_size=out_encoded_len;
}

/// @brief encodes [chunk] into [out_encoded]
/// @param chunk pointer to the data to be encoded
/// @param size size of the data
/// @param out_encoded 
/// @param out_size 
void encode(const uint8_t* chunk, const int size, uint8_t** out_encoded, int* out_size){
    if(size==0){
        *out_encoded=NULL;
        *out_size=0;
        return;
    }
    uint8_t* buffer=(uint8_t*)malloc(sizeof(uint8_t)*size*2);
    int buffer_len=0;
    encode_internal(chunk, size, buffer, &buffer_len);

    // output value
    *out_encoded=buffer;
    *out_size=buffer_len;
}

/// @brief combines an array of encoded chunks into one (merges encoded chars at the edges e.g., 
/// a3a5 -> a8)
/// @param chunks array of encoded chunks
/// @param chunk_len length of each encoded chunk
/// @param chunks_len length of the encoded chunks
void encode_combine_encoded_chunks(uint8_t* const chunks[], const int chunk_len[], const int chunks_len,
    uint8_t** out_chunk, int* out_size){
    int total_chunk_size=sum_of_array(chunk_len, chunks_len);
    // allocate memory for the final chunk
    uint8_t* total_chunk=(uint8_t*)malloc(sizeof(uint8_t)*total_chunk_size);
    uint8_t* total_chunk_it=total_chunk;

    uint8_t prev_key=0, prev_cnt=0;
    // start copying each chunk
    for(int i=0;i<chunks_len;++i){
        const uint8_t* cur_chunk=chunks[i];
        int cur_chunk_len=chunk_len[i];
        if(cur_chunk[0]==prev_key && prev_cnt!=0){ // has same value as the previous key (e.g. a3a5)
            memcpy(total_chunk_it-2, cur_chunk, sizeof(uint8_t)*cur_chunk_len);
            total_chunk_it[-1]+=prev_cnt; // accumulates the count to merge
            total_chunk_it+=cur_chunk_len-2;
        } else{
            memcpy(total_chunk_it, chunks[i], sizeof(uint8_t)*chunk_len[i]);
            total_chunk_it+=cur_chunk_len;
        }
        prev_key=cur_chunk[cur_chunk_len-2];
        prev_cnt=cur_chunk[cur_chunk_len-1];
    }

    // sets the output values
    *out_chunk=total_chunk;
    *out_size=total_chunk_it-total_chunk;
}

// converts an encoded chunk into ASCII format
char* encoded_to_str(const uint8_t* encoded, int len){
    int str_len=len*2;
    char* str=(char*)malloc(str_len);
    char* str_it=str;
    for(int i=0;i<len;i+=2){
        str_it[0]=encoded[i];
        ++str_it;
        str_it+=snprintf(str_it, 4, "%d", encoded[i+1]);
    }
    return str;
}

// ====encode with multi thread=====
typedef struct EncodeMultithreadArg{
    const uint8_t* chunk;
    int size;
    uint8_t** out_encoded;
    int* out_size;
    sem_t* sem;
}EncodeMultithreadArg;

static void encode_multithread_worker(void* arg_void){
    EncodeMultithreadArg* arg=(EncodeMultithreadArg*)arg_void;
    encode(arg->chunk, arg->size, arg->out_encoded, arg->out_size);
    sem_post(arg->sem);
}

/// @brief encode [chunk] with multithread
/// @param chunk the data to encode
/// @param size size of the data to encode
/// @param out_encoded pointer to a 2D array, containing all the encoded chunks
/// @param out_size pointer to an array, containing the size of each chunk in [*out_encoded]
/// @param num_batches number of batches
static void encode_batch_multithread(const uint8_t* chunk, const int size, uint8_t*** out_encoded, int** out_size, int num_batches){
    int batch_size=size/num_batches;

    uint8_t** out_encoded_chunks=(uint8_t**)malloc(sizeof(uint8_t*)*num_batches);
    int* out_encoded_chunk_len=(int*)malloc(sizeof(int)*num_batches);

    sem_t* sem_tasks_done=(sem_t*)malloc(sizeof(sem_t));
    sem_init(sem_tasks_done, 0, 0);

    EncodeMultithreadArg* args=(EncodeMultithreadArg*)malloc(sizeof(EncodeMultithreadArg)*num_batches);
    for(int i=0;i<num_batches;++i){
        const uint8_t* batch_chunk=chunk+i*batch_size;
        int actual_batch_size=i==num_batches-1?size-batch_size*i:batch_size;
        args[i]=(EncodeMultithreadArg){batch_chunk, actual_batch_size,
            &out_encoded_chunks[i], &out_encoded_chunk_len[i], sem_tasks_done};
    }
    scheduler_begin_push_batch();
    for(int i=0;i<num_batches;++i){
        scheduler_job_push(encode_multithread_worker, &args[i]);
    }
    scheduler_end_push_batch();

    // wait for async tasks to finish
    for(int i=0;i<num_batches;++i)
        sem_wait(sem_tasks_done);

    *out_encoded=out_encoded_chunks;
    *out_size=out_encoded_chunk_len;

    // free resources
    free(args);
    sem_destroy(sem_tasks_done);
    free(sem_tasks_done);

}

/// @brief encode [chunk] with multithread
/// @param chunk the data to encode
/// @param size size of the data to encode
/// @param out_encoded pointer to a buffer, containing the encoded chunk
/// @param out_size pointer to an integer, containing the size of the encoded chunk
/// @param num_batches number of batches
static void encode_batch_multithread_combine(const uint8_t* chunk, const int size, uint8_t** out_encoded, int* out_size, int num_batches){
    uint8_t** encoded_chunks;
    int* encoded_chunk_len;
    encode_batch_multithread(chunk, size, &encoded_chunks, &encoded_chunk_len, num_batches);
    encode_combine_encoded_chunks(encoded_chunks, encoded_chunk_len, num_batches, out_encoded, out_size);
    free(encoded_chunks);
    free(encoded_chunk_len);
}

void encode_multithread_combine(const uint8_t* chunk, const int size, uint8_t** out_encoded, int* out_size){
    int num_batches=argument.jobs;
    encode_batch_multithread_combine(chunk, size, out_encoded, out_size, num_batches);
}

/// @brief encode [chunk] with multithread
/// @param chunk the data to encode
/// @param size size of the data to encode
/// @param out_encoded pointer to a 2D array, containing all the encoded chunks
/// @param out_encoded_lens pointer to an array, containing the size of each chunk in [*out_encoded]
/// @param out_encoded_len length of the 2D array
void encode_multithread(const uint8_t* chunk, const int size, uint8_t*** out_encoded, int** out_encoded_lens, int* out_encoded_len){
    int num_batches=argument.jobs;
    encode_batch_multithread(chunk, size, out_encoded, out_encoded_lens, num_batches);
    *out_encoded_len=num_batches;
}

void encode_multithread_combine_fixed_batch_size(const uint8_t* chunk, const int size, uint8_t** out_encoded, int* out_size, int batch_size){
    int num_batches=(size+batch_size-1)/batch_size;
    encode_batch_multithread_combine(chunk, size, out_encoded, out_size, num_batches);
}

void encode_write_uncombined_chunks(int fd, uint8_t*const* chunks, const int* chunk_lens, const int chunk_len){
    ssize_t write_ret=0;
    uint8_t prev[2]={0,0};
    for(int i=0;i<chunk_len;++i){
        const int len=chunk_lens[i];
        if(len<=0) continue; // len==0
        const uint8_t* chunk=chunks[i];
        // merge with previous chunk
        if(prev[0]==chunk[0]){
            prev[1]+=chunk[1];
        }
        // if len==2, then continue merging with the next chunk
        // and does nothing
        if(len==2){
            if(prev[0]!=chunk[0] && prev[1]>0)
                write_ret = write(fd, prev, sizeof(prev));
            prev[0]=chunk[0];
            prev[1]=chunk[1];
        } else{
            // else write(prev)
            if(prev[1]>0)
                write_ret = write(fd, prev, sizeof(prev));
            // write the chunk[0], chunk[1]
            if(prev[0]!=chunk[0])
                write_ret = write(fd, chunk, 2);
            // if len>4, write the middle chunk (chunk+2, chunk+len-2)
            if(len>4)
                write_ret = write(fd, chunk+2, len-4);
            // if len>2, set prev to the last two bits of the chunk
            prev[0]=chunk[len-2];
            prev[1]=chunk[len-1];
        }
    }
    if(prev[1]>0)
        write_ret = write(fd, prev, sizeof(prev));
    if(write_ret==-1)
        perror("error using write()");
}

typedef struct ChunkWriteInfo{
    int offset;
    // number of char of the [last char of previous]/[first char of next chunk] if they are the same with this chunk.
    // e.g., chunks=[a3,a5,a2], prev_count=3, next_count=2. if 0, then no merging with the previous/next chunk
    uint8_t prev_count;
    uint8_t next_count;
} ChunkWriteInfo;

// update the variables of struct ChunkWriteInfo of a given chunk
static void chunk_info_init(uint8_t*const* chunks, const int* chunk_lens, const int chunk_len, int idx, ChunkWriteInfo* chunk_infos){
    if(idx>=chunk_len) return;
    uint8_t* cur_chunk=chunks[idx];
    ChunkWriteInfo* chunk_info=&chunk_infos[idx];
    chunk_info->offset=0;
    chunk_info->prev_count=0;
    chunk_info->next_count=0;
    if(idx>0){
        uint8_t* prev_chunk=chunks[idx-1];
        int prev_chunk_len=chunk_lens[idx-1];
        chunk_info->offset=chunk_infos[idx-1].offset;
        if(prev_chunk[prev_chunk_len-2]==cur_chunk[0]){
            ChunkWriteInfo* prev_chunk_info=&chunk_infos[idx-1];
            chunk_info->prev_count=prev_chunk[prev_chunk_len-1];
            if(prev_chunk_len==2)
                chunk_info->prev_count+=prev_chunk_info->prev_count;
            prev_chunk_info->next_count=chunk_info->prev_count;
            chunk_info->offset-=2;
        }
    }
}
static ChunkWriteInfo* get_chunk_infos(uint8_t*const* chunks, const int* chunk_lens, const int chunk_len){
    ChunkWriteInfo* chunk_infos=(ChunkWriteInfo*)calloc(chunk_len, sizeof(ChunkWriteInfo));
    for(int i=0;i<chunk_len;++i){
        chunk_info_init(chunks, chunk_lens, chunk_len, i, chunk_infos);
    }
    return chunk_infos;
}

typedef struct ChunkWriteArg{
    const uint8_t* chunk;
    ChunkWriteInfo* info;
    sem_t* sem;
    int fd;
    int chunk_len;
    int cumulative_len; // sum of the len of all previous chunks
} ChunkWriteArg;

static void encode_write_uncombined_chunks_worker(void* argv){
    ChunkWriteArg* arg=(ChunkWriteArg*)argv;
    uint8_t buf[2];
    int f_pos=arg->cumulative_len + arg->info->offset;
    ssize_t pwrite_ret=0;
    if(arg->chunk_len==2){
        if(arg->info->next_count==0){ // does not overlap with the next chunk
            buf[0]=arg->chunk[0];
            buf[1]=arg->chunk[1]+arg->info->prev_count;
            pwrite_ret = pwrite(arg->fd, buf, sizeof(buf), f_pos);
        }
    } else{
        buf[0]=arg->chunk[0];
        buf[1]=arg->chunk[1]+arg->info->prev_count;
        pwrite_ret = pwrite(arg->fd, buf, sizeof(buf), f_pos);
        if(arg->info->next_count){
            if(arg->chunk_len>4)
                pwrite_ret = pwrite(arg->fd, arg->chunk+2, arg->chunk_len-4, f_pos+2);
        }
        else
            pwrite_ret = pwrite(arg->fd, arg->chunk+2, arg->chunk_len-2, f_pos+2);
    }
    if(pwrite_ret==-1)
        perror("error using pwrite");
    sem_post(arg->sem);
}

/// @brief write uncombined encoded chunks to a file descriptor using multiple threads. thread count=chunk_len
void encode_write_uncombined_chunks_multithread(int fd, uint8_t*const* chunks, const int* chunk_lens, const int chunk_len){
    // calculate the offset of each chunk to write in the file
    ChunkWriteInfo* chunk_infos=get_chunk_infos(chunks, chunk_lens, chunk_len);
    // reserve enough space to write the encoded chunks
    int file_size=sum_of_array(chunk_lens, chunk_len)+chunk_infos[chunk_len-1].offset;
    ssize_t ftrunc_ret = ftruncate(fd, file_size);
    if(ftrunc_ret==-1)
        perror("error using ftrunc_ret");
    // init sem
    sem_t* sem_tasks_done=(sem_t*)malloc(sizeof(sem_t));
    sem_init(sem_tasks_done, 0, 0);
    // write using multithread
    int num_batches=chunk_len;
    // init args
    ChunkWriteArg* args=(ChunkWriteArg*)malloc(sizeof(ChunkWriteArg)*num_batches);
    scheduler_begin_push_batch();
    for(int i=0;i<num_batches;++i){
        args[i].chunk=chunks[i];
        args[i].chunk_len=chunk_lens[i];
        args[i].fd=fd;
        args[i].info=&chunk_infos[i];
        args[i].sem=sem_tasks_done;
        if(i==0)
            args[i].cumulative_len=0;
        else
            args[i].cumulative_len=args[i-1].cumulative_len+chunk_lens[i-1];
        scheduler_job_push(encode_write_uncombined_chunks_worker, &args[i]);
    }
    scheduler_end_push_batch();
    // wait for a jobs to complete
    for(int i=0;i<num_batches;++i)
        sem_wait(sem_tasks_done);

    free(chunk_infos);
    free(args);
    sem_destroy(sem_tasks_done);
}