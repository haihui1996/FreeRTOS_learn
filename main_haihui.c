/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#define Task_1_Priority		(tskIDLE_PRIORITY + 2)
#define Task_2_Priority		(tskIDLE_PRIORITY + 1)
#define Task_3_Priority		(tskIDLE_PRIORITY + 3)

#define mainTIMER_SEND_FREQUENCY_MS pdMS_TO_TICKS( 2000UL )
#define mainVALUE_SENT_FROM_TIMER			( 200UL )
static void Task_1_CreatTask(void * taskdata )
{
	while(1)
	{
		printf("I am task 1\r\n");
		vTaskDelay(10);
	}
	vTaskDelete(NULL);
}


static void prvQueueSendTimerCallback(TimerHandle_t xTimerHandle)
{
	const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */

	/* Avoid compiler warnings resulting from the unused parameter. */
	(void)xTimerHandle;

	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	//xQueueSend(xQueue, &ulValueToSend, 0U);
}
void main_haihui(void)
{
	const TickType_t xTimerPeriod = mainTIMER_SEND_FREQUENCY_MS;

	printf("\r\nhelloworld");
	xTaskCreate((TaskFunction_t *)Task_1_CreatTask,
		"task_1",
		configMINIMAL_STACK_SIZE,
		NULL,
		Task_1_Priority,
		NULL);

	xTimerCreate("Timer",				/* The text name assigned to the software timer - for debug only as it is not used by the kernel. */
		xTimerPeriod,		/* The period of the software timer in ticks. */
		pdFALSE,			/* xAutoReload is set to pdFALSE, so this is a one shot timer. */
		NULL,				/* The timer's ID is not used. */
		prvQueueSendTimerCallback);/* The function executed when the timer expires. */
	vTaskStartScheduler();
	while (1);
	return;
}