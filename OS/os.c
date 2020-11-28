#include "os.h"
#include "malloc.h"	 
#include "led.h"
unsigned char OS_PrioCur;//记录当前运行的任务优先级
unsigned char OS_PrioHighRdy;
volatile unsigned int OSRdyTbl;//任务就绪表
unsigned int OS_Tisks;
TCB TCB_Task[OS_MAX_Task];//任务控制块

unsigned int  CPU_ExceptStk[OS_EXCEPT_STK_SIZE];//主任务堆栈
unsigned int * CPU_ExceptStkBase;//指向的是数组最后一个元素
unsigned int IDLE_STK[IDLE_STK_SIZE];//空闲任务堆栈
TCB *p_TCB_Cur;//指向当前任务的tcb
TCB *p_TCBHightRdy;//指向最高级任务的tcb

__inline void OSSetPrioRdy(unsigned char prio)
{
	OSRdyTbl|=0x01<<prio;
}

__inline void OSDelPrioRdy(unsigned char prio)
{
	OSRdyTbl&=~(0x01<<prio);
}

__inline void OSGetHighRdy(void)				
{									
	unsigned char	OS_NEXT_Prio;	
	for(OS_NEXT_Prio=0;
	(OS_NEXT_Prio<OS_MAX_Task)&&(!(OSRdyTbl&(0x01<<OS_NEXT_Prio)));
	OS_NEXT_Prio++);
	OS_PrioHighRdy=OS_NEXT_Prio;	
}

/*---------------------------------------
-* File: Task_End
-* Description：任务运行结束时进入此函数，理论上每个子任务都是一个死循环，不会结束
-* Arguments:  	void				
-*---------------------------------------*/
void Task_End(void)
{
	while(1);
}

/*---------------------------------------
-* File: Arguments
-* Description：创建一个新的任务,初始化任务为待恢复状态
-* Arguments:  	task		任务函数名
-* 				stk			任务堆栈
-* 				prio		任务优先级
-*---------------------------------------*/
void Task_Create(void (*task)(void),unsigned int *stk,unsigned char prio)
{
	unsigned int * p_stk;
	p_stk=stk;
	p_stk=(unsigned int *) ((unsigned int)(p_stk)&0xFFFFFFF8u);
	//以下寄存器顺序和PendSV退出时寄存器恢复顺序一致
	*(--p_stk)=(unsigned int)0x01000000uL;//xPSR状态寄存器、第24位THUMB模式必须置位一 
	*(--p_stk)=(unsigned int)task;//entry point//函数入口
	*(--p_stk)=(unsigned int)Task_End ;//R14(LR);
	*(--p_stk)=(unsigned int)0x12121212uL;//R12
	*(--p_stk)=(unsigned int)0x03030303uL;//R3
	*(--p_stk)=(unsigned int)0x02020202uL;//R2
	*(--p_stk)=(unsigned int)0x01010101uL;//R1
	*(--p_stk)=(unsigned int)0x00000000uL;//R0
	//PendSV发生时未自动保存的内核寄存器：R4~R11
	*(--p_stk)=(unsigned int)0x11111111uL;//R11
	*(--p_stk)=(unsigned int)0x10101010uL;//R10
	*(--p_stk)=(unsigned int)0x09090909uL;//R9
	*(--p_stk)=(unsigned int)0x08080808uL;//R8
	*(--p_stk)=(unsigned int)0x07070707uL;//R7
	*(--p_stk)=(unsigned int)0x06060606uL;//R6
	*(--p_stk)=(unsigned int)0x05050505uL;//R5
	*(--p_stk)=(unsigned int)0x04040404uL;//R4
	
	TCB_Task[prio].StkPtr =p_stk;//将该任务控制块中应当指向栈顶的指针，指向了该任务的新栈顶
	TCB_Task[prio].DLy =0;
	
	OSSetPrioRdy(prio);//设置 优先级
}

/*---------------------------------------
-* File: OS_IDLE_Task
-* Description：空闲任务，防止CPU无事可干
-* Arguments:  	void				
-*---------------------------------------*/
void OS_IDLE_Task(void)
{
	unsigned int IDLE_Count=0;
	while(1)
	{
		IDLE_Count++;
		//__ASM volatile("WFE");
	}
}

/*---------------------------------------
-* File: OS_Sched
-* Description：任务调度函数，查找最高优先级任务并调度
-* Arguments:  	void				
-*---------------------------------------*/
void OS_Sched(void)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();                                  //进入临界区
	OSGetHighRdy();    							//找出任务就绪表中优先级最高的任务
	if(OS_PrioHighRdy!=OS_PrioCur)              //如果不是当前运行任务，进行任务调度
	{
		p_TCBHightRdy=&TCB_Task[OS_PrioHighRdy];
		//p_TCB_Cur=&TCB_Task[OS_PrioCur];
		OS_PrioCur= OS_PrioHighRdy;//更新OS_PrioCur
		OSCtxSw();//调度任务,在汇编中引用
	}
	OS_EXIT_CRITICAL();                                 //退出临界区
}

/*---------------------------------------
-* File: System_init
-* Description：系统(滴答)时钟初始化
-* Arguments:  	void
-*---------------------------------------*/
void System_init(void)
{
	unsigned int reload;
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//选择外部时钟  HCLK/8 
	reload=SystemCoreClock/8000000;							//每秒钟的计数次数 单位为K	   
	reload*=1000000/System_Ticks;		//根据System_Ticks设定溢出时间
								//reload为24位寄存器,最大值:16777216,在72M下,约合1.86s左右	 
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//开启SYSTICK中断
	SysTick->LOAD=reload; 		//每1/System_Ticks秒中断一次	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//开启SYSTICK    
}

/*---------------------------------------
-* File: SysTick_Handler
-* Description：系统(滴答)时钟中断,每1000/System_Ticks ms中断一次
-* Arguments:  	void
-*---------------------------------------*/
void SysTick_Handler(void)
{
	unsigned int cpu_sr;
	unsigned char i=0;
	//OSIntNesting++;//对于Cortex-M3,任务切换是由pendsv中断实现的，而pendsv中断的优先级最低
	//即使中断嵌套发生了任务调度也没事，真正的任务切换也不会发生，只是触发了pendsv中断了而已
	for(;i<OS_MAX_Task;i++)
    {
		OS_ENTER_CRITICAL();
		if(TCB_Task[i].DLy)
		{
			TCB_Task[i].DLy-=1000/System_Ticks;
			if(TCB_Task[i].DLy==0)			//延时时钟到达
			{
				OSSetPrioRdy(i);            //任务重新就绪
			}
		}
		OS_EXIT_CRITICAL();
	}
	
	OS_Sched();//都是由pendsv中断进行调度
}

/*---------------------------------------
-* File: OS_Start
-* Description：创建一个空闲任务，系统开始运行
-* Arguments:  	void
-*---------------------------------------*/
void OS_Start(void)
{
	System_init();
	CPU_ExceptStkBase=CPU_ExceptStk+OS_EXCEPT_STK_SIZE-1;//Cortex-M3栈向下增长
	Task_Create(OS_IDLE_Task,&IDLE_STK[IDLE_STK_SIZE-1],OS_MAX_Task-1);//空闲任务
	OSGetHighRdy();//获得最高级的就绪任务
	OS_PrioCur= OS_PrioHighRdy;
	//p_TCB_Cur=&TCB_Task[OS_PrioCur];
	p_TCBHightRdy=&TCB_Task[OS_PrioHighRdy];
	OSStartHighRdy();
}

/*---------------------------------------
-* File: OSTimeDly
-* Description：系统延时
-* Arguments:  	ticks		延时时间
-*---------------------------------------*/
void OSTimeDly(unsigned int ticks)
{
	if(ticks> 0)
	{
		unsigned int cpu_sr;
		OS_ENTER_CRITICAL();                                  //进入临界区
		OSDelPrioRdy(OS_PrioCur);                             //将任务挂起
		TCB_Task[OS_PrioCur].DLy= ticks;                      //设置TCB中任务延时节拍数
		OS_EXIT_CRITICAL();                                   //退出临界区
		OS_Sched();                                           //任务调度
		//return ;
	}
}

/*---------------------------------------
-* File: OS_SemCreate
-* Description：创建信号量
-* Arguments:  	cnt		信号量初始值
-*---------------------------------------*/
ECB * OS_SemCreate(unsigned char cnt)
{
	ECB * p;
	p=my_malloc(sizeof (ECB));
	p->OSEventTbl =0;
	p->Cnt=cnt;
	return p;
}

/*---------------------------------------
-* File: OS_SemPend
-* Description：请求信号量
-* Arguments:  	pevent		信号量指针
-* 			  	time		等待时间，0为无限等待
-*---------------------------------------*/
void OS_SemPend(ECB  *pevent,unsigned char time)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();                                  //进入临界区
	if(pevent->Cnt > 0)
	{
		pevent->Cnt--;
		OS_EXIT_CRITICAL();                                   //退出临界区 
		return ;
	}
	TCB_Task[OS_PrioCur].OSTCBStatPend =OS_STAT_PEND_TO;//信号量状态
	TCB_Task[OS_PrioCur].DLy= time;
	pevent->OSEventTbl|=0x01<<OS_PrioCur;//添加到等待表
	OSDelPrioRdy(OS_PrioCur);//从就绪表中删除
	OS_EXIT_CRITICAL();
	OS_Sched();
	OS_ENTER_CRITICAL();
	if(TCB_Task[OS_PrioCur].OSTCBStatPend==OS_STAT_PEND_TO)//任务是等待超时才获取控制权
	{
		pevent->OSEventTbl&=~(0x01<<OS_PrioCur);//从等待表中删除
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_PEND_OK;//标志为正常状态
	}
	OS_EXIT_CRITICAL();
}

/*---------------------------------------
-* File: OS_SemPost
-* Description：释放信号量
-* Arguments:  	pevent		信号量指针
-*---------------------------------------*/
void OS_SemPost(ECB  *pevent)
{
	unsigned int cpu_sr;
	unsigned char	OS_ECB_Prio;
	OS_ENTER_CRITICAL();
	if(pevent->OSEventTbl!=0)//有任务在等待
	{
		
		for(OS_ECB_Prio=0;//找出等待表中优先级最高的任务
			(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
			OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//从等待表中删除
		OSSetPrioRdy(OS_ECB_Prio);//添加到就绪表中
		TCB_Task[OS_ECB_Prio].OSTCBStatPend =OS_STAT_PEND_OK;
		OS_EXIT_CRITICAL();
		OS_Sched();
		return ;
	}
	else if(pevent->Cnt<255)
	{
		pevent->Cnt++;
		OS_EXIT_CRITICAL();
		return ;
	}
	OS_EXIT_CRITICAL();
}

/*---------------------------------------
-* File: OS_SemDel
-* Description：删除信号量,在这之前需要删除操作此信号量的所以任务
-* Arguments:  	pevent		信号量指针
-*---------------------------------------*/
void OS_SemDel(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->OSEventTbl !=0)//有任务在等待该信号量
		return ;
	my_free(pevent);
	OS_EXIT_CRITICAL();
}

/*---------------------------------------
-* File: OS_MutexCreate
-* Description：创建互斥信号量
-* Arguments:  	void
-*---------------------------------------*/
ECB * OS_MutexCreate(void)
{
	ECB * p;
	p=my_malloc(sizeof (ECB));
	p->OSEventTbl =0;
	p->Prio=OS_MAX_Task;//初始化占用资源任务优先级为不存在的优先级数
	return p;
}

/*---------------------------------------
-* File: OS_MutexPend
-* Description：请求互斥信号量
-* Arguments:  	pevent		互斥信号量指针
-*---------------------------------------*/
void OS_MutexPend(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->Prio==OS_MAX_Task)//资源没被占用
	{
		pevent ->Prio=OS_PrioCur;			//记录占用资源任务的优先级
		pevent ->Cnt=OS_PrioCur;			//记录申请资源任务的优先级
		OS_EXIT_CRITICAL();
		return ;
	}
	else if(pevent->Cnt < OS_PrioCur)		//如果上一次申请资源的任务的优先级比此次任务高(优先级数值小)
	{
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//添加到等待表
		OSDelPrioRdy(OS_PrioCur);			//从就绪表中删除
		OS_EXIT_CRITICAL();
		OS_Sched();							//优先级不提高，直接调度
		return ;
	}
	else									//如果上一次申请资源的任务的优先级比此次任务低
	{
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//添加等待资源的任务到等待表
		while(!(OSRdyTbl&(0x01<<(pevent ->Cnt))))//如果上一个申请资源的任务处于休眠状态
		{
			OS_EXIT_CRITICAL();
			OS_Sched();//直接调度，不给于运行,直到占用资源任务处于就绪状态
			OS_ENTER_CRITICAL();
		}	
		OSDelPrioRdy(pevent ->Cnt);		//从就绪表中删除占用资源的任务(上一次"高"优先级申请资源任务)
		pevent ->Cnt=OS_PrioCur;			//记录申请资源任务的优先级	
		while(pevent ->Prio!=OS_MAX_Task)	//如果资源没被释放
		{
				p_TCBHightRdy=&TCB_Task[pevent ->Prio];//间接提升资源任务的优先级为此等待任务的优先级
				TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_MUTEX_DLY;//标记为任务在等待资源释放
				OS_EXIT_CRITICAL();
				OSCtxSw();					//一直提升占用资源任务的优先级，等待资源释放
				OS_ENTER_CRITICAL();
		}
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_MUTEX_NO_DLY;//取消标记
		pevent ->Prio=OS_PrioCur;			//更新占用资源任务的优先级
		OS_EXIT_CRITICAL();
		return ;
	}
}

/*---------------------------------------
-* File: OS_SemPost
-* Description：释放互斥信号量
-* Arguments:  	pevent		互斥信号量指针
-*---------------------------------------*/
void OS_MutexPost(ECB  *pevent)
{
	unsigned char	OS_ECB_Prio;
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	OS_PrioCur=pevent->Prio;					//OS_PrioCur更新为恢复被提升的优先级
	pevent ->Prio=OS_MAX_Task;				//优先级指向一个不存在的优先级，标记资源释放掉了
	if(pevent->OSEventTbl)
	{
		for(OS_ECB_Prio=0;						//找出等待表中优先级最高的任务
				(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
				OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//从等待表中删除
		OSSetPrioRdy(OS_ECB_Prio);				//添加等待表中优先级最高的任务到就绪表中
		OSSetPrioRdy(OS_PrioCur);				//恢复占用资源的任务到就绪表中(提升优先级时会把低优先级的申请资源的任务删除掉)
	}
	OS_EXIT_CRITICAL();
	OS_Sched();
}

/*---------------------------------------
-* File: OS_MutexDel
-* Description：删除信号量,在这之前需要删除操作此信号量的所以任务
-* Arguments:  	pevent		信号量指针
-*---------------------------------------*/
void OS_MutexDel(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->OSEventTbl !=0)//有任务在等待该信号量
		return ;
	my_free(pevent);
	OS_EXIT_CRITICAL();
}

