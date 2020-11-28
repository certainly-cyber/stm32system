#include "os.h"
#include "malloc.h"	 
#include "led.h"
unsigned char OS_PrioCur;//��¼��ǰ���е��������ȼ�
unsigned char OS_PrioHighRdy;
volatile unsigned int OSRdyTbl;//���������
unsigned int OS_Tisks;
TCB TCB_Task[OS_MAX_Task];//������ƿ�

unsigned int  CPU_ExceptStk[OS_EXCEPT_STK_SIZE];//�������ջ
unsigned int * CPU_ExceptStkBase;//ָ������������һ��Ԫ��
unsigned int IDLE_STK[IDLE_STK_SIZE];//���������ջ
TCB *p_TCB_Cur;//ָ��ǰ�����tcb
TCB *p_TCBHightRdy;//ָ����߼������tcb

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
-* Description���������н���ʱ����˺�����������ÿ����������һ����ѭ�����������
-* Arguments:  	void				
-*---------------------------------------*/
void Task_End(void)
{
	while(1);
}

/*---------------------------------------
-* File: Arguments
-* Description������һ���µ�����,��ʼ������Ϊ���ָ�״̬
-* Arguments:  	task		��������
-* 				stk			�����ջ
-* 				prio		�������ȼ�
-*---------------------------------------*/
void Task_Create(void (*task)(void),unsigned int *stk,unsigned char prio)
{
	unsigned int * p_stk;
	p_stk=stk;
	p_stk=(unsigned int *) ((unsigned int)(p_stk)&0xFFFFFFF8u);
	//���¼Ĵ���˳���PendSV�˳�ʱ�Ĵ����ָ�˳��һ��
	*(--p_stk)=(unsigned int)0x01000000uL;//xPSR״̬�Ĵ�������24λTHUMBģʽ������λһ 
	*(--p_stk)=(unsigned int)task;//entry point//�������
	*(--p_stk)=(unsigned int)Task_End ;//R14(LR);
	*(--p_stk)=(unsigned int)0x12121212uL;//R12
	*(--p_stk)=(unsigned int)0x03030303uL;//R3
	*(--p_stk)=(unsigned int)0x02020202uL;//R2
	*(--p_stk)=(unsigned int)0x01010101uL;//R1
	*(--p_stk)=(unsigned int)0x00000000uL;//R0
	//PendSV����ʱδ�Զ�������ں˼Ĵ�����R4~R11
	*(--p_stk)=(unsigned int)0x11111111uL;//R11
	*(--p_stk)=(unsigned int)0x10101010uL;//R10
	*(--p_stk)=(unsigned int)0x09090909uL;//R9
	*(--p_stk)=(unsigned int)0x08080808uL;//R8
	*(--p_stk)=(unsigned int)0x07070707uL;//R7
	*(--p_stk)=(unsigned int)0x06060606uL;//R6
	*(--p_stk)=(unsigned int)0x05050505uL;//R5
	*(--p_stk)=(unsigned int)0x04040404uL;//R4
	
	TCB_Task[prio].StkPtr =p_stk;//����������ƿ���Ӧ��ָ��ջ����ָ�룬ָ���˸��������ջ��
	TCB_Task[prio].DLy =0;
	
	OSSetPrioRdy(prio);//���� ���ȼ�
}

/*---------------------------------------
-* File: OS_IDLE_Task
-* Description���������񣬷�ֹCPU���¿ɸ�
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
-* Description��������Ⱥ���������������ȼ����񲢵���
-* Arguments:  	void				
-*---------------------------------------*/
void OS_Sched(void)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();                                  //�����ٽ���
	OSGetHighRdy();    							//�ҳ���������������ȼ���ߵ�����
	if(OS_PrioHighRdy!=OS_PrioCur)              //������ǵ�ǰ�������񣬽����������
	{
		p_TCBHightRdy=&TCB_Task[OS_PrioHighRdy];
		//p_TCB_Cur=&TCB_Task[OS_PrioCur];
		OS_PrioCur= OS_PrioHighRdy;//����OS_PrioCur
		OSCtxSw();//��������,�ڻ��������
	}
	OS_EXIT_CRITICAL();                                 //�˳��ٽ���
}

/*---------------------------------------
-* File: System_init
-* Description��ϵͳ(�δ�)ʱ�ӳ�ʼ��
-* Arguments:  	void
-*---------------------------------------*/
void System_init(void)
{
	unsigned int reload;
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//ѡ���ⲿʱ��  HCLK/8 
	reload=SystemCoreClock/8000000;							//ÿ���ӵļ������� ��λΪK	   
	reload*=1000000/System_Ticks;		//����System_Ticks�趨���ʱ��
								//reloadΪ24λ�Ĵ���,���ֵ:16777216,��72M��,Լ��1.86s����	 
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//����SYSTICK�ж�
	SysTick->LOAD=reload; 		//ÿ1/System_Ticks���ж�һ��	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//����SYSTICK    
}

/*---------------------------------------
-* File: SysTick_Handler
-* Description��ϵͳ(�δ�)ʱ���ж�,ÿ1000/System_Ticks ms�ж�һ��
-* Arguments:  	void
-*---------------------------------------*/
void SysTick_Handler(void)
{
	unsigned int cpu_sr;
	unsigned char i=0;
	//OSIntNesting++;//����Cortex-M3,�����л�����pendsv�ж�ʵ�ֵģ���pendsv�жϵ����ȼ����
	//��ʹ�ж�Ƕ�׷������������Ҳû�£������������л�Ҳ���ᷢ����ֻ�Ǵ�����pendsv�ж��˶���
	for(;i<OS_MAX_Task;i++)
    {
		OS_ENTER_CRITICAL();
		if(TCB_Task[i].DLy)
		{
			TCB_Task[i].DLy-=1000/System_Ticks;
			if(TCB_Task[i].DLy==0)			//��ʱʱ�ӵ���
			{
				OSSetPrioRdy(i);            //�������¾���
			}
		}
		OS_EXIT_CRITICAL();
	}
	
	OS_Sched();//������pendsv�жϽ��е���
}

/*---------------------------------------
-* File: OS_Start
-* Description������һ����������ϵͳ��ʼ����
-* Arguments:  	void
-*---------------------------------------*/
void OS_Start(void)
{
	System_init();
	CPU_ExceptStkBase=CPU_ExceptStk+OS_EXCEPT_STK_SIZE-1;//Cortex-M3ջ��������
	Task_Create(OS_IDLE_Task,&IDLE_STK[IDLE_STK_SIZE-1],OS_MAX_Task-1);//��������
	OSGetHighRdy();//�����߼��ľ�������
	OS_PrioCur= OS_PrioHighRdy;
	//p_TCB_Cur=&TCB_Task[OS_PrioCur];
	p_TCBHightRdy=&TCB_Task[OS_PrioHighRdy];
	OSStartHighRdy();
}

/*---------------------------------------
-* File: OSTimeDly
-* Description��ϵͳ��ʱ
-* Arguments:  	ticks		��ʱʱ��
-*---------------------------------------*/
void OSTimeDly(unsigned int ticks)
{
	if(ticks> 0)
	{
		unsigned int cpu_sr;
		OS_ENTER_CRITICAL();                                  //�����ٽ���
		OSDelPrioRdy(OS_PrioCur);                             //���������
		TCB_Task[OS_PrioCur].DLy= ticks;                      //����TCB��������ʱ������
		OS_EXIT_CRITICAL();                                   //�˳��ٽ���
		OS_Sched();                                           //�������
		//return ;
	}
}

/*---------------------------------------
-* File: OS_SemCreate
-* Description�������ź���
-* Arguments:  	cnt		�ź�����ʼֵ
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
-* Description�������ź���
-* Arguments:  	pevent		�ź���ָ��
-* 			  	time		�ȴ�ʱ�䣬0Ϊ���޵ȴ�
-*---------------------------------------*/
void OS_SemPend(ECB  *pevent,unsigned char time)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();                                  //�����ٽ���
	if(pevent->Cnt > 0)
	{
		pevent->Cnt--;
		OS_EXIT_CRITICAL();                                   //�˳��ٽ��� 
		return ;
	}
	TCB_Task[OS_PrioCur].OSTCBStatPend =OS_STAT_PEND_TO;//�ź���״̬
	TCB_Task[OS_PrioCur].DLy= time;
	pevent->OSEventTbl|=0x01<<OS_PrioCur;//��ӵ��ȴ���
	OSDelPrioRdy(OS_PrioCur);//�Ӿ�������ɾ��
	OS_EXIT_CRITICAL();
	OS_Sched();
	OS_ENTER_CRITICAL();
	if(TCB_Task[OS_PrioCur].OSTCBStatPend==OS_STAT_PEND_TO)//�����ǵȴ���ʱ�Ż�ȡ����Ȩ
	{
		pevent->OSEventTbl&=~(0x01<<OS_PrioCur);//�ӵȴ�����ɾ��
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_PEND_OK;//��־Ϊ����״̬
	}
	OS_EXIT_CRITICAL();
}

/*---------------------------------------
-* File: OS_SemPost
-* Description���ͷ��ź���
-* Arguments:  	pevent		�ź���ָ��
-*---------------------------------------*/
void OS_SemPost(ECB  *pevent)
{
	unsigned int cpu_sr;
	unsigned char	OS_ECB_Prio;
	OS_ENTER_CRITICAL();
	if(pevent->OSEventTbl!=0)//�������ڵȴ�
	{
		
		for(OS_ECB_Prio=0;//�ҳ��ȴ��������ȼ���ߵ�����
			(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
			OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//�ӵȴ�����ɾ��
		OSSetPrioRdy(OS_ECB_Prio);//��ӵ���������
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
-* Description��ɾ���ź���,����֮ǰ��Ҫɾ���������ź�������������
-* Arguments:  	pevent		�ź���ָ��
-*---------------------------------------*/
void OS_SemDel(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->OSEventTbl !=0)//�������ڵȴ����ź���
		return ;
	my_free(pevent);
	OS_EXIT_CRITICAL();
}

/*---------------------------------------
-* File: OS_MutexCreate
-* Description�����������ź���
-* Arguments:  	void
-*---------------------------------------*/
ECB * OS_MutexCreate(void)
{
	ECB * p;
	p=my_malloc(sizeof (ECB));
	p->OSEventTbl =0;
	p->Prio=OS_MAX_Task;//��ʼ��ռ����Դ�������ȼ�Ϊ�����ڵ����ȼ���
	return p;
}

/*---------------------------------------
-* File: OS_MutexPend
-* Description�����󻥳��ź���
-* Arguments:  	pevent		�����ź���ָ��
-*---------------------------------------*/
void OS_MutexPend(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->Prio==OS_MAX_Task)//��Դû��ռ��
	{
		pevent ->Prio=OS_PrioCur;			//��¼ռ����Դ��������ȼ�
		pevent ->Cnt=OS_PrioCur;			//��¼������Դ��������ȼ�
		OS_EXIT_CRITICAL();
		return ;
	}
	else if(pevent->Cnt < OS_PrioCur)		//�����һ��������Դ����������ȼ��ȴ˴������(���ȼ���ֵС)
	{
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//��ӵ��ȴ���
		OSDelPrioRdy(OS_PrioCur);			//�Ӿ�������ɾ��
		OS_EXIT_CRITICAL();
		OS_Sched();							//���ȼ�����ߣ�ֱ�ӵ���
		return ;
	}
	else									//�����һ��������Դ����������ȼ��ȴ˴������
	{
		pevent->OSEventTbl|=0x01<<OS_PrioCur;//��ӵȴ���Դ�����񵽵ȴ���
		while(!(OSRdyTbl&(0x01<<(pevent ->Cnt))))//�����һ��������Դ������������״̬
		{
			OS_EXIT_CRITICAL();
			OS_Sched();//ֱ�ӵ��ȣ�����������,ֱ��ռ����Դ�����ھ���״̬
			OS_ENTER_CRITICAL();
		}	
		OSDelPrioRdy(pevent ->Cnt);		//�Ӿ�������ɾ��ռ����Դ������(��һ��"��"���ȼ�������Դ����)
		pevent ->Cnt=OS_PrioCur;			//��¼������Դ��������ȼ�	
		while(pevent ->Prio!=OS_MAX_Task)	//�����Դû���ͷ�
		{
				p_TCBHightRdy=&TCB_Task[pevent ->Prio];//���������Դ��������ȼ�Ϊ�˵ȴ���������ȼ�
				TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_MUTEX_DLY;//���Ϊ�����ڵȴ���Դ�ͷ�
				OS_EXIT_CRITICAL();
				OSCtxSw();					//һֱ����ռ����Դ��������ȼ����ȴ���Դ�ͷ�
				OS_ENTER_CRITICAL();
		}
		TCB_Task[OS_PrioCur].OSTCBStatPend=OS_STAT_MUTEX_NO_DLY;//ȡ�����
		pevent ->Prio=OS_PrioCur;			//����ռ����Դ��������ȼ�
		OS_EXIT_CRITICAL();
		return ;
	}
}

/*---------------------------------------
-* File: OS_SemPost
-* Description���ͷŻ����ź���
-* Arguments:  	pevent		�����ź���ָ��
-*---------------------------------------*/
void OS_MutexPost(ECB  *pevent)
{
	unsigned char	OS_ECB_Prio;
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	OS_PrioCur=pevent->Prio;					//OS_PrioCur����Ϊ�ָ������������ȼ�
	pevent ->Prio=OS_MAX_Task;				//���ȼ�ָ��һ�������ڵ����ȼ��������Դ�ͷŵ���
	if(pevent->OSEventTbl)
	{
		for(OS_ECB_Prio=0;						//�ҳ��ȴ��������ȼ���ߵ�����
				(OS_ECB_Prio<OS_MAX_Event)&&(!((pevent->OSEventTbl)&(0x01<<OS_ECB_Prio)));
				OS_ECB_Prio++);
		pevent->OSEventTbl&=~(0x01<<OS_ECB_Prio);//�ӵȴ�����ɾ��
		OSSetPrioRdy(OS_ECB_Prio);				//��ӵȴ��������ȼ���ߵ����񵽾�������
		OSSetPrioRdy(OS_PrioCur);				//�ָ�ռ����Դ�����񵽾�������(�������ȼ�ʱ��ѵ����ȼ���������Դ������ɾ����)
	}
	OS_EXIT_CRITICAL();
	OS_Sched();
}

/*---------------------------------------
-* File: OS_MutexDel
-* Description��ɾ���ź���,����֮ǰ��Ҫɾ���������ź�������������
-* Arguments:  	pevent		�ź���ָ��
-*---------------------------------------*/
void OS_MutexDel(ECB  *pevent)
{
	unsigned int cpu_sr;
	OS_ENTER_CRITICAL();
	if(pevent ->OSEventTbl !=0)//�������ڵȴ����ź���
		return ;
	my_free(pevent);
	OS_EXIT_CRITICAL();
}

