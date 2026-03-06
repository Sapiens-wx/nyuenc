#define ARGPARSE_IMPLEMENTATION
#define TEST_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encoder.h"
#include "argparse.h"
#include "test.h"

int main(int argc, char* argv[]){
	// parse arguments
	arg_parse(argc, argv);

	test_encode_combine_encoded_chunks();
	return 0;
}
