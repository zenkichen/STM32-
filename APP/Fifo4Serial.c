#include "Fifo4serial.h" 

/**********************************************
�������ܣ���������
���룺��
�������
���ߣ�Сӥfighting
�������ڣ�2012��3��25��
����޸�ʱ�䣺2012��3��25��
��ע�������ṹ��
**********************************************/
Fifo4Serial USART1Send,USART1Recieve ;
Fifo4Serial USART2Send,USART2Recieve ;
Fifo4Serial USART3Send,USART3Recieve ;

/**********************************************
�������ܣ����г�ʼ��
���룺��
�������
���ߣ�Сӥfighting
�������ڣ�2012��3��25��
����޸�ʱ�䣺2012��3��25��
��ע����
**********************************************/
void QueueInit(Fifo4Serial *Q)
{
	Q->front = 0 ;
	Q->rear = 0 ;
	Q->bufferCount = 0 ;
}
/**********************************************
�������ܣ�����
���룺��
�������
���ߣ�Сӥfighting
�������ڣ�2012��3��25��
����޸�ʱ�䣺2012��3��25��
��ע������ѭ������
**********************************************/
char QueueIn(Fifo4Serial *Q,char dat)
{
	if(((Q->rear) % QUEUE_BUFFER == Q->front) && (Q->bufferCount == QUEUE_BUFFER ))
		return QUEUE_FULL ;
	Q->base[Q->rear] = dat ;
	Q->rear = (Q->rear + 1) % QUEUE_BUFFER ;
	Q->bufferCount++ ;
	return(QUEUE_OK) ;
}
/**********************************************
�������ܣ�����
���룺��
�������
���ߣ�Сӥfighting
�������ڣ�2012��3��25��
����޸�ʱ�䣺2012��3��25��
��ע����
**********************************************/
char QueueOut(Fifo4Serial *Q,char *dat)
{
 	if((Q->front == Q->rear) && (Q->bufferCount == 0)) 
		return(QUEUE_EMPTY) ;
	else {
		*dat = Q->base[Q->front] ;
		Q->front = (Q->front + 1) % QUEUE_BUFFER ;
		Q->bufferCount-- ;
		return(QUEUE_OK) ;
	}
}
