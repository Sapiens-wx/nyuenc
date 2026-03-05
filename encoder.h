#pragma once
#include <stdint.h>
#include <stddef.h>

void encode(const uint8_t* chunk, const size_t size, uint8_t** out_encoded, size_t* out_size);
char* encoded_to_str(const uint8_t* encoded, size_t len);