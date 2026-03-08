#pragma once
#include <stdint.h>
#include <stddef.h>

void encode(const uint8_t* chunk, const int size, uint8_t** out_encoded, int* out_size);
void encode_combine_encoded_chunks(uint8_t* const chunks[], const int chunk_len[], const int chunks_len,
    uint8_t** out_chunk, int* out_size);
char* encoded_to_str(const uint8_t* encoded, int len);

void encode_multithread(const uint8_t* chunk, const int size, uint8_t** out_encoded, int* out_size);