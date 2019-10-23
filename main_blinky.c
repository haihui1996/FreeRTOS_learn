/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/******************************************************************************
 * NOTE: Windows will not be running the FreeRTOS demo threads continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Windows port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Windows
 * port for further information:
 * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
 *
 * NOTE 2:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.  Console output
 * is used in place of the normal LED toggling.
 *
 * NOTE 3:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, are defined
 * in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, one software timer, and two tasks.  It then
 * starts the scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  It uses vTaskDelayUntil() to create a periodic task that sends
 * the value 100 to the queue every 200 milliseconds (please read the notes
 * above regarding the accuracy of timing under Windows).
 *
 * The Queue Send Software Timer:
 * The timer is a one-shot timer that is reset by a key press.  The timer's
 * period is set to two seconds - if the timer expires then its callback
 * function writes the value 200 to the queue.  The callback function is
 * implemented by prvQueueSendTimerCallback() within this file.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() waits for data to arrive on the queue.
 * When data is received, the task checks the value of the data, then outputs a
 * message to indicate if the data came from the queue send task or the queue
 * send software timer.
 *
 * Expected Behaviour:
 * - The queue send task writes to the queue every 200ms, so every 200ms the
 *   queue receive task will output a message indicating that data was received
 *   on the queue from the queue send task.
 * - The queue send software timer has a period of two seconds, and is reset
 *   each time a key is pressed.  So if two seconds expire without a key being
 *   pressed then the queue receive task will output a message indicating that
 *   data was received on the queue from the queue send software timer.
 *
 * NOTE:  Console input and output relies on Windows system calls, which can
 * interfere with the execution of the FreeRTOS Windows port.  This demo only
 * uses Windows system call occasionally.  Heavier use of Windows system calls
 * can crash the port.
 */

/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTASK_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 200UL )
#define mainTIMER_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 2000UL )

/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH					( 2 )

/* The values sent to the queue receive task from the queue send task and the
queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK			( 100UL )
#define mainVALUE_SENT_FROM_TIMER			( 200UL )

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );

/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/
static TaskHandle_t handler1, handler2;
static void Task_1_CreatTask(void* taskdata)
{
	unsigned int i;
	(void)taskdata;

	i = 0;
	while (1)
	{
		i++;
		printf("I am task 1\r\n");
		if (i == 5) {
			i = 0;
			vTaskSuspend(NULL);
		}
		vTaskDelay(1000);

	}
	vTaskDelete(NULL);
}
static void Task_2_CreatTask(void* taskdata)
{
	unsigned int i;
	(void)taskdata;

	i = 0;
	while (1)
	{
		i++;
		printf("I am task 2\r\n");
		if (i == 10)
		{
			i = 0;
			vTaskResume(handler1);
		}
		vTaskDelay(1000);
	}
	vTaskDelete(NULL);
}

/*
	链表操作
*/
static void Task_3_CreatTask(void* taskdata)
{
	/* 定义链表 */
	List_t list_test;
	/* 定义列表项 */
	ListItem_t ListItem1, ListItem2, ListItem3, ListItem4;

	(void)taskdata;
	/* 初始化列表 */
	vListInitialise(&list_test);

	/* 初始化列表项 */
	vListInitialiseItem(&ListItem1);
	vListInitialiseItem(&ListItem2);
	vListInitialiseItem(&ListItem3);
	vListInitialiseItem(&ListItem4);

	/* 列表项赋值 */
	ListItem1.xItemValue = 100;
	ListItem2.xItemValue = 200;
	ListItem3.xItemValue = 300;
	ListItem4.xItemValue = 400;

	//printf("ListItem1:%#x\r\n", (int)&ListItem1);
	//printf("ListItem2:%#x\r\n", (int)&ListItem2);
	//printf("ListItem3:%#x\r\n", (int)&ListItem3);

	/* 无论按什么顺序插入，最终的排列都是按照ItemValue由小到大 */
	vListInsert(&list_test, &ListItem3);
	vListInsert(&list_test, &ListItem1);
	vListInsert(&list_test, &ListItem4);
	//vListInsert(&list_test, &ListItem2);

	printf("list1:%d\r\n", (list_test.xListEnd.pxNext->xItemValue));
	printf("list2:%d\r\n", (list_test.xListEnd.pxNext->pxNext->xItemValue));
	printf("list3:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->xItemValue));
	//printf("list4:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->pxNext->xItemValue));

#if 0
	uxListRemove(&ListItem3);
	printf("***********************************\r\n");
	printf("list1:%d\r\n", (list_test.xListEnd.pxNext->xItemValue));
	printf("list2:%d\r\n", (list_test.xListEnd.pxNext->pxNext->xItemValue));
	printf("list3:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->xItemValue));
	printf("list4:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->pxNext->xItemValue));
#endif
	/*
		vListInsertEnd()将Item加在pxIndex当前指向的列表项之前
	*/
	printf("***********************************\r\n");
	printf("PIdx:%ld\n", list_test.pxIndex->xItemValue);
	list_test.pxIndex = list_test.pxIndex->pxNext;
	vListInsertEnd(&list_test, &ListItem2);
	printf("list1:%d\r\n", (list_test.xListEnd.pxNext->xItemValue));
	printf("list2:%d\r\n", (list_test.xListEnd.pxNext->pxNext->xItemValue));
	printf("list3:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->xItemValue));
	printf("list4:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->pxNext->xItemValue));
	
	uxListRemove(&ListItem2);
	printf("***********************************\r\n");
	list_test.pxIndex = list_test.pxIndex->pxNext;
	vListInsertEnd(&list_test, &ListItem2);
	printf("list1:%d\r\n", (list_test.xListEnd.pxNext->xItemValue));
	printf("list2:%d\r\n", (list_test.xListEnd.pxNext->pxNext->xItemValue));
	printf("list3:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->xItemValue));
	printf("list4:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->pxNext->xItemValue));
	//printf("list4:%d\r\n", (list_test.xListEnd.pxNext->pxNext->pxNext->pxNext->pxNext->xItemValue));
	
	while (1);

}
/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_blinky( void )
{
const TickType_t xTimerPeriod = mainTIMER_SEND_FREQUENCY_MS;

	/* Create the queue. */
	xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

	if( xQueue != NULL )
	{
		/* Start the two tasks as described in the comments at the top of this
		file. */
#if 0
		xTaskCreate( prvQueueReceiveTask,			/* The function that implements the task. */
					"Rx", 							/* The text name assigned to the task - for debug only as it is not used by the kernel. */
					configMINIMAL_STACK_SIZE, 		/* The size of the stack to allocate to the task. */
					NULL, 							/* The parameter passed to the task - not used in this simple case. */
					mainQUEUE_RECEIVE_TASK_PRIORITY,/* The priority assigned to the task. */
					NULL );							/* The task handle is not required, so NULL is passed. */

		xTaskCreate( prvQueueSendTask, "TX", configMINIMAL_STACK_SIZE, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );
#else
		//xTaskCreate((TaskFunction_t*)Task_1_CreatTask,
		//	"task_1",
		//	configMINIMAL_STACK_SIZE,
		//	NULL,
		//	1,
		//	&handler1);

		//xTaskCreate((TaskFunction_t*)Task_2_CreatTask,
		//	"task_2",
		//	configMINIMAL_STACK_SIZE,
		//	NULL,
		//	1,
		//	&handler2);
		xTaskCreate((TaskFunction_t*)Task_3_CreatTask,
			"task_3",
			configMINIMAL_STACK_SIZE,
			NULL,
			1,
			NULL);
#endif
		/* Create the software timer, but don't start it yet. */
		xTimer = xTimerCreate( "Timer",				/* The text name assigned to the software timer - for debug only as it is not used by the kernel. */
								xTimerPeriod,		/* The period of the software timer in ticks. */
								pdFALSE,			/* xAutoReload is set to pdFALSE, so this is a one shot timer. */
								NULL,				/* The timer's ID is not used. */
								prvQueueSendTimerCallback );/* The function executed when the timer expires. */

		/* Start the tasks and timer running. */
		vTaskStartScheduler();
	}

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created.  See the memory management section on the
	FreeRTOS web site for more details. */
	for( ;; );
}
/*-----------------------------------------------------------*/




static void prvQueueSendTask( void *pvParameters )
{
TickType_t xNextWakeTime;
const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;

	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil( &xNextWakeTime, xBlockTime );

		/* Send to the queue - causing the queue receive task to unblock and
		write to the console.  0 is used as the block time so the send operation
		will not block - it shouldn't need to block as the queue should always
		have at least one space at this point in the code. */
		xQueueSend( xQueue, &ulValueToSend, 0U );
	}
}
/*-----------------------------------------------------------*/

static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle )
{
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */

	/* Avoid compiler warnings resulting from the unused parameter. */
	( void ) xTimerHandle;

	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	xQueueSend( xQueue, &ulValueToSend, 0U );
}
/*-----------------------------------------------------------*/

static void prvQueueReceiveTask( void *pvParameters )
{
uint32_t ulReceivedValue;

	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	for( ;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h.  It will not use any CPU time while it is in the
		Blocked state. */
		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

		/*  To get here something must have been received from the queue, but
		is it an expected value?  Normally calling printf() from a task is not
		a good idea.  Here there is lots of stack space and only one task is
		using console IO so it is ok.  However, note the comments at the top of
		this file about the risks of making Windows system calls (such as 
		console output) from a FreeRTOS task. */
		if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
		{
			//printf( "Message received from task\r\n" );
		}
		else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
		{
			printf( "Message received from software timer\r\n" );
		}
		else
		{
			printf( "Unexpected message\r\n" );
		}

		/* Reset the timer if a key has been pressed.  The timer will write
		mainVALUE_SENT_FROM_TIMER to the queue when it expires. */
		if( _kbhit() != 0 )
		{
			/* Remove the key from the input buffer. */
			( void ) _getch();

			/* Reset the software timer. */
			xTimerReset( xTimer, portMAX_DELAY );
		}
	}
}
/*-----------------------------------------------------------*/


