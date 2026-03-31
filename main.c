#define ARGPARSE_IMPLEMENTATION
#define TEST_IMPLEMENTATION
#define FILE_READER_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encoder.h"
#include "argparse.h"
#include "test.h"
#include "file_reader.h"
#include "jobs.h"

void test(void* a){
	a=(void*)((char*)a+1);
}

int main(int argc, char* argv[]){
	// parse arguments
	arg_parse(argc, argv);

	// init scheduler for thread pool
	scheduler_init();

	// read files into a single buffer
	size_t buffer_size=0;
	uint8_t* buffer=file_read_from_argument_multithread(&buffer_size);

	if(buffer!=NULL){
		// encode
		uint8_t** encoded;
		int encoded_len;
		int* encoded_lens;
		encode_multithread(buffer, (int)buffer_size, &encoded, &encoded_lens, &encoded_len);
		//encode_multithread_fixed_batch_size(buffer, (int)buffer_size, &encoded, &encoded_size, 1024);

		// writes to stdout
		encode_write_uncombined_chunks(STDOUT_FILENO, encoded, encoded_lens, encoded_len);
	}

	scheduler_destroy();
	return 0;
}
