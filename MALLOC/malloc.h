#ifndef __MALLOC_H
#define __MALLOC_H
#include "stm32f10x.h"

#define MEM_BANK_MIN_SIZE 8//�ܷ�����ڴ��С
#define MEM_SIZE (10*1024)//������ڴ�Ĵ�С
#define MMTS (MEM_SIZE/MEM_BANK_MIN_SIZE)//�ڴ�����Ĵ�С

void malloc_init(void);
void * my_malloc(unsigned int size);
unsigned char my_free(void *mem_pointer);

#endif













