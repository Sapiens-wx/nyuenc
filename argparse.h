#pragma once

typedef struct Argument{
    int jobs;
    const char* files[128];
    int files_cnt;
} Argument;

extern Argument argument;

void arg_parse(int argc, char* argv[])
#ifndef ARGPARSE_IMPLEMENTATION
;
#else
{
    argument.files_cnt=0;
    argument.jobs=1;
    for(int i=1;i<argc;++i){
        if(strcmp("-j", argv[i])==0){ // set jobs
            if(i+1>=argc || sscanf(argv[i+1], "%d", &argument.jobs)==0){
                argument.jobs=1;
                printf("[WARNING] fail to parse -j param. default [jobs] to 1\n");
            } else ++i;
        } else{ // set files
            argument.files[argument.files_cnt]=argv[i];
            argument.files_cnt++;
        }
    }
}
#endif

#ifdef ARGPARSE_IMPLEMENTATION
Argument argument;
#endif