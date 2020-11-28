/*---------------------------------------
-* File:    malloc
-* AUTHOR�� kite
-* E_MAIL:  1216104245@qq.com
-* TIME��   2017/03/06
-* VERSION��V1.0
-* WARNING��
-*---------------------------------------*/

#include "malloc.h"	    

__align(8) unsigned char membase[MEM_SIZE];//�ڴ��(8�ֽڶ���)
//�ڴ���ƿ�
unsigned short	int mmt[MMTS];//�ڴ�����


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
	unsigned int need_blocks=0;//��Ҫ���ڴ����
	unsigned int free_blocks=0;//�������ڴ����
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
		if(need_blocks==free_blocks)//�ҵ�������need_blocks�����ڴ��
		{
			i=(i+1)-free_blocks;
			for(blocks=i;blocks<(i+free_blocks);blocks++)
			{
				mmt[blocks]=1;//���Ϊռ��
			}
			mmt[i]=need_blocks;//���ռ�õĴ�С
			return (membase+i*MEM_BANK_MIN_SIZE);//���ػ�ȡ���ĵ�ַ
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







