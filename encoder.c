#include "encoder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    uint8_t* buffer=(uint8_t*)malloc(sizeof(uint8_t)*size);
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