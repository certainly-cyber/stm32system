////#include "stm32f10x.h"
//#include "led.h"
//#include "os.h"

//#define TASK_1_STK_SIZE 512
//#define TASK_2_STK_SIZE 512
//#define TASK_3_STK_SIZE 512

//unsigned int TASK_1_STK[TASK_1_STK_SIZE];
//unsigned int TASK_2_STK[TASK_2_STK_SIZE];
//unsigned int TASK_3_STK[TASK_3_STK_SIZE];

////ECB * s_msg;			//信号量
//ECB * m_msg;			//互斥信号量

//void Task1(void)//高优先级
//{
//	OSTimeDly(50);
//	while(1)
//	{
//		OS_MutexPend(m_msg);
//		LED0=!LED0;
//		OS_MutexPost(m_msg);
//		OSTimeDly(200);
//	}
//}
//void Task2(void)//中优先级
//{
//	OSTimeDly(100);
//	while(1)
//	{
//		LED1=!LED1;
//		OSTimeDly(50);
//	}
//}
//void delay(unsigned int j )//无意义延时
//{
//	unsigned int i = 0;
//	unsigned int k = j;
//	for(i=0;i<50000;i++)
//	{
//		while(--j);
//		j=k;
//	}
//}
//void Task3(void)//低优先级
//{
//	unsigned int i = 0;
//	while(1)
//	{
//		i++;
//		if(i==1)
//		{
//			OS_MutexPend(m_msg);
//			LED1=0;
//			delay(1000);//一直霸占资源,7s左右//OSTimeDly(500);	
//			OS_MutexPost(m_msg);
//		}
//		if(i==500)
//			i=10;
//		OSTimeDly(150);
//	}
//}

//int main(void)
//{	
//	
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//	LED_Init();
//	Task_Create(Task1,&TASK_1_STK[TASK_1_STK_SIZE-1],0);
//	Task_Create(Task2,&TASK_2_STK[TASK_2_STK_SIZE-1],1);
//	Task_Create(Task3,&TASK_3_STK[TASK_3_STK_SIZE-1],2);
//	
//	//s_msg=OS_SemCreate(0);//开始时，信号量计数器为0
//	m_msg =OS_MutexCreate();	//创建互斥信号量
//	OS_Start(); 
//	return 0;
//}
//#include "stm32f10x.h"  
#include "led.h"  
#include "os.h"  
#define TASK_1_STK_SIZE 512  
#define TASK_2_STK_SIZE 512  
  
unsigned int TASK_1_STK[TASK_1_STK_SIZE];  
unsigned int TASK_2_STK[TASK_2_STK_SIZE];  
ECB * s_msg;            //信号量  
  
void Task1(void)  
{  
    while(1)  
    {  
        OS_SemPend(s_msg,0);//请求信号量  
        LED0=!LED0;  
		printf("进程一执行完毕\r\n");
        OSTimeDly(200);  
    }  
}  
void Task2(void)  
{  
    unsigned int i = 0;  
    while(1)  
    {  
        i++;  
        if(10==i)  
        {  
            i=0;  
            OS_SemPost(s_msg);  
        }  
        LED1=!LED1;  
		printf("进程二执行完毕\r\n");
        OSTimeDly(150);  
    }  
}  
  
int main(void)  
{     
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
    LED_Init();  
	uart_init(115200);
    Task_Create(Task1,&TASK_1_STK[TASK_1_STK_SIZE-1],0);  
    Task_Create(Task2,&TASK_2_STK[TASK_2_STK_SIZE-1],1);  
      
    s_msg=OS_SemCreate(0);//开始时，信号量计数器为0  
    OS_Start();   
    return 0;  
}  



