#include "encoder.h"
#include <stdlib.h>
#include <stdio.h>

static void write_encoded_value(uint8_t val, uint8_t cnt, uint8_t* buffer, size_t* len){
    size_t buffer_len=*len;
    buffer[buffer_len++]=val;
    buffer[buffer_len++]=cnt;
    *len=buffer_len;
}

void encode(const uint8_t* chunk, const size_t size, uint8_t** out_encoded, size_t* out_size){
    if(size==0){
        *out_encoded=NULL;
        *out_size=0;
        return;
    }
    uint8_t* buffer=(uint8_t*)malloc(sizeof(uint8_t)*size);
    size_t buffer_len=0;
    uint8_t cur_val=chunk[0];
    uint8_t cur_cnt=1;
    for(size_t i=1;i<size;++i){
        if(chunk[i]==cur_val){ // duplicate value
            ++cur_cnt;
        } else{ // encounters a new value. encode the previous value
            write_encoded_value(cur_val, cur_cnt, buffer, &buffer_len);
            cur_val=chunk[i];
            cur_cnt=1;
        }
    }
    write_encoded_value(cur_val, cur_cnt, buffer, &buffer_len);

    // output value
    *out_encoded=realloc(buffer, sizeof(uint8_t)*buffer_len);
    *out_size=buffer_len;
}

char* encoded_to_str(const uint8_t* encoded, size_t len){
    size_t str_len=len*2;
    char* str=(char*)malloc(str_len);
    char* str_it=str;
    for(size_t i=0;i<len;i+=2){
        str_it[0]=encoded[i];
        ++str_it;
        str_it+=snprintf(str_it, 4, "%d", encoded[i+1]);
    }
    return str;
}