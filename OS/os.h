#ifndef __OS_H
#define __OS_H	 
#include "sys.h"


#define OS_EXCEPT_STK_SIZE 		128u					//堆栈大小
#define OS_MAX_Task 			32u					//最大任务数(优先级数)
#define IDLE_STK_SIZE 			64u					//空闲任务堆栈大小
#define OS_MAX_Event 			32u					//最大事件数
#define OS_PRIO_SELF			0xFFu				//任务自身
#define System_Ticks  			100u		//每1000/System_Ticks ms进入一次中断

/*---------------------------------------
-* 事件状态
-*---------------------------------------*/
#define  OS_STAT_PEND_OK                0u  
#define  OS_STAT_PEND_TO                1u

#define  OS_STAT_MUTEX_DLY              2u
#define  OS_STAT_MUTEX_NO_DLY           3u

/*---------------------------------------
-* Description：利用编译器扩展功能获得程序状态字，保存在局部变量cpu_sr
-*---------------------------------------*/
#define  OS_ENTER_CRITICAL()  {cpu_sr = OS_CPU_SR_Save();}
#define  OS_EXIT_CRITICAL()   {OS_CPU_SR_Restore(cpu_sr);}

typedef struct 	OS_Tcb		   
{
	unsigned int *StkPtr;//任务栈顶
	unsigned int DLy;//任务延时时钟
	unsigned char OSTCBStatPend;//任务状态
   
}TCB; //任务控制块

typedef struct 	OS_Ecb		   
{
	unsigned int Cnt;//计数器
	unsigned char OSEventTbl;//事件等待表
	
	unsigned int Prio;//优先级
}ECB; //事件控制块

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


