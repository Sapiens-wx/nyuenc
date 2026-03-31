#pragma once
#ifdef TEST_IMPLEMENTATION
#include <stdio.h>
#include "encoder.h"
#endif

void test_encode()
#ifndef TEST_IMPLEMENTATION
;
#else
{
	char str[1024];
    while(1){
        printf("enter string to test: ");
        if(!scanf("%s", str))
            break;
        uint8_t* encoded;
        int  encoded_size;
        encode((uint8_t*)str, strlen(str), &encoded, &encoded_size);
        char* encoded_str=encoded_to_str(encoded, encoded_size);
        printf("encoding [%s]\n", str);
        printf("encoded=[%s]\n", encoded_str);
        // release resources
        free(encoded);
        free(encoded_str);
    }
}
#endif

void test_encode_combine_encoded_chunks()
#ifndef TEST_IMPLEMENTATION
;
#else
{
    #define CHUNK_MAX_COUNT 16
    #define LINE_CHAR_COUNT 256
	char str[CHUNK_MAX_COUNT][LINE_CHAR_COUNT];
    while(1){
        printf("enter chunks to be encoded:\n");
        int chunk_count=0;
        do{
            str[chunk_count][0]=0;
            fgets(str[chunk_count], LINE_CHAR_COUNT, stdin);
            if(str[chunk_count][0]=='\n') break;
            ++chunk_count;
            if(chunk_count>=CHUNK_MAX_COUNT) break;
        } while(1);
        uint8_t* encoded_chunks[CHUNK_MAX_COUNT];
        int encoded_chunk_len[CHUNK_MAX_COUNT];
        printf("encoded str:\n");
        for(int i=0;i<chunk_count;++i){
            encode((const uint8_t*)str[i], strlen(str[i])-1, &encoded_chunks[i], &encoded_chunk_len[i]);
            char* encoded_str=encoded_to_str(encoded_chunks[i], encoded_chunk_len[i]);
            printf("[%d]: %s\n", i, encoded_str);
        }
        printf("combined encoded str:\n");
        uint8_t* combined_chunk;
        int combined_chunk_len;
        encode_combine_encoded_chunks(encoded_chunks, encoded_chunk_len, chunk_count,
            &combined_chunk, &combined_chunk_len);
        char* combined_chunk_str=encoded_to_str(combined_chunk, combined_chunk_len);
        printf("%s\n", combined_chunk_str);
    }
}
#endif