/*---------------------------------------
-* File:    malloc
-* AUTHOR： kite
-* E_MAIL:  1216104245@qq.com
-* TIME：   2017/03/06
-* VERSION：V1.0
-* WARNING：
-*---------------------------------------*/

#include "malloc.h"	    

__align(8) unsigned char membase[MEM_SIZE];//内存池(8字节对齐)
//内存控制块
unsigned short	int mmt[MMTS];//内存管理表


void malloc_init(void)
{
	unsigned int i=0;
	for(i=0;i<MMTS;i++)
	{
		mmt[i]=0;
	}
}

void * my_malloc(unsigned int size)
{
	unsigned int need_blocks=0;//需要的内存块数
	unsigned int free_blocks=0;//连续空内存块数
	unsigned int blocks=0;//
	unsigned int i;
	
	need_blocks=size /MEM_BANK_MIN_SIZE;
	if(size%MEM_BANK_MIN_SIZE)
		need_blocks++;
	for(i=0;i<MMTS;i++)
	{
		if(mmt[i]==0)
			free_blocks++;
		else
			free_blocks=0;
		if(need_blocks==free_blocks)//找到了连续need_blocks个空内存块
		{
			i=(i+1)-free_blocks;
			for(blocks=i;blocks<(i+free_blocks);blocks++)
			{
				mmt[blocks]=1;//标记为占用
			}
			mmt[i]=need_blocks;//存放占用的大小
			return (membase+i*MEM_BANK_MIN_SIZE);//返回获取到的地址
		}
	}
	return 0x00000000L;
}

unsigned char my_free(void *mem_pointer)
{
	unsigned int free;
	unsigned int num;
	unsigned int i;
	free=(unsigned int)mem_pointer-(unsigned int)&membase[0];
	free/=MEM_BANK_MIN_SIZE;
	num=mmt[free];
	for(i=free;i<free+num;i++)
	{
		mmt[i]=0;
	}
	mem_pointer=0x00000000L;
	return 1;
}







