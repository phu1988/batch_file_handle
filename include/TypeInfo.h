#ifndef TYPE_INFO_H_
#define TYPE_INFO_H_
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "LogInstance.h"

#define int64 long long
#define uint64 unsigned long long
#define ValType double
#define CharToVal atof

#define MY_SHM_KEY 0x9999
#define MY_SHM_LEN 1073741824
#define MY_INPUT_PROCESS_NUM 1
#define MY_OUTPUT_PROCESS_NUM 10
#define MY_SPLIT_ONE_PROC 32

#endif
