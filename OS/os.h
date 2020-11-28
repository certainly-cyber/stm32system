#ifndef __OS_H
#define __OS_H	 
#include "sys.h"


#define OS_EXCEPT_STK_SIZE 		128u					//��ջ��С
#define OS_MAX_Task 			32u					//���������(���ȼ���)
#define IDLE_STK_SIZE 			64u					//���������ջ��С
#define OS_MAX_Event 			32u					//����¼���
#define OS_PRIO_SELF			0xFFu				//��������
#define System_Ticks  			100u		//ÿ1000/System_Ticks ms����һ���ж�

/*---------------------------------------
-* �¼�״̬
-*---------------------------------------*/
#define  OS_STAT_PEND_OK                0u  
#define  OS_STAT_PEND_TO                1u

#define  OS_STAT_MUTEX_DLY              2u
#define  OS_STAT_MUTEX_NO_DLY           3u

/*---------------------------------------
-* Description�����ñ�������չ���ܻ�ó���״̬�֣������ھֲ�����cpu_sr
-*---------------------------------------*/
#define  OS_ENTER_CRITICAL()  {cpu_sr = OS_CPU_SR_Save();}
#define  OS_EXIT_CRITICAL()   {OS_CPU_SR_Restore(cpu_sr);}

typedef struct 	OS_Tcb		   
{
	unsigned int *StkPtr;//����ջ��
	unsigned int DLy;//������ʱʱ��
	unsigned char OSTCBStatPend;//����״̬
   
}TCB; //������ƿ�

typedef struct 	OS_Ecb		   
{
	unsigned int Cnt;//������
	unsigned char OSEventTbl;//�¼��ȴ���
	
	unsigned int Prio;//���ȼ�
}ECB; //�¼����ƿ�

extern void OSCtxSw(void);
extern void OSStartHighRdy(void);
extern unsigned int OS_CPU_SR_Save(void);
extern void OS_CPU_SR_Restore(unsigned int cpu_sr);

void OS_TaskSuspend(unsigned char prio) ;
void OS_TaskResume(u8 prio);
void Task_Create(void (*task)(void),unsigned int *stk,unsigned char prio);
void OSSetPrioRdy(unsigned char prio);
void OSDelPrioRdy(unsigned char prio);
void OS_Start(void);
void OSTimeDly(unsigned int ticks);
void OS_Sched(void);


ECB * OS_SemCreate(unsigned char cnt);
void OS_SemPend(ECB  *pevent,unsigned char time);
void OS_SemPost(ECB  *pevent);
void OS_SemDel(ECB  *pevent);

ECB * OS_MutexCreate(void);
void OS_MutexPend(ECB  *pevent);
void OS_MutexPost(ECB  *pevent);
void OS_MutexDel(ECB  *pevent);


#endif


