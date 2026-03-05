#define ARGPARSE_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encoder.h"
#include "argparse.h"

int main(int argc, char* argv[]){
	// parse arguments
	arg_parse(argc, argv);

	char str[1024];
	scanf("%s", str);
	uint8_t* encoded;
	size_t encoded_size;
	encode((uint8_t*)str, strlen(str), &encoded, &encoded_size);
	char* encoded_str=encoded_to_str(encoded, encoded_size);
	printf("encoding [%s]\n", str);
	printf("encoded=[%s]\n", encoded_str);
	// release resources
	free(encoded);
	free(encoded_str);
	return 0;
}
