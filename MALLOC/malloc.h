#ifndef __MALLOC_H
#define __MALLOC_H
#include "stm32f10x.h"

#define MEM_BANK_MIN_SIZE 8//能分配的内存大小
#define MEM_SIZE (10*1024)//管理的内存的大小
#define MMTS (MEM_SIZE/MEM_BANK_MIN_SIZE)//内存管理表的大小

void malloc_init(void);
void * my_malloc(unsigned int size);
unsigned char my_free(void *mem_pointer);

#endif













