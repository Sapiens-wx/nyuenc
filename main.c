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

int main(int argc, char* argv[]){
	// parse arguments
	arg_parse(argc, argv);

	// read files into a single buffer
	size_t buffer_size=0;
	uint8_t* buffer=file_read_from_argument(&buffer_size);

	// encode
	uint8_t* encoded;
	int encoded_size;
	encode(buffer, (int)buffer_size, &encoded, &encoded_size);

	char* encoded_str = encoded_to_str(encoded, encoded_size);
	printf("encoded:\n%s\n", encoded_str);

	return 0;
}
