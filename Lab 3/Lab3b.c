/*
 * userApp.c
 *
 *  Created on: Dec 8, 2023
 *      Author: Niall.OKeeffe@atu.ie
 */

#include "userApp.h"
#include <stdio.h>

//Add the relevant FreeRTOS header files here
#include "FreeRTOS.h"
#include "task.h"

//--------------------------------------------------------------
//used for real time stats, do not delete code from this section
extern TIM_HandleTypeDef htim7;
extern volatile unsigned long ulHighFrequencyTimerTicks;
void configureTimerForRunTimeStats(void)
{
    ulHighFrequencyTimerTicks = 0;
    HAL_TIM_Base_Start_IT(&htim7);
}

unsigned long getRunTimeCounterValue(void)
{
	return ulHighFrequencyTimerTicks;
}
//end of real time stats code
//----------------------------------------------------------------

extern UART_HandleTypeDef huart1;


static void task1(void * pvParameters);
static void task2(void * pvParameters);
TaskHandle_t task2Handle = NULL;

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}

void userApp() {
	printf("Starting application\r\n\n");
	xTaskCreate(task2, "Task 2", 200, NULL, 1, &task2Handle);
	xTaskCreate(task1, "Task 1", 200, NULL, 3, NULL);
	vTaskStartScheduler();

	while(1) {
	}
}

void task1(void * pvParameters) {
	printf("Starting task 1\r\n");
	while(1) {
		//print priority of task 1 and task 2
		printf("Task1 running priority %lu, Task2 running priority %lu \r\n", uxTaskPriorityGet(NULL), uxTaskPriorityGet(task2Handle));

		//increment task 2 priority by 1
		printf("Incrementing task 2 priority\r\n\n");
		vTaskPrioritySet(task2Handle, uxTaskPriorityGet(task2Handle) + 1);

	}
}

void task2(void * pvParameters) {
	printf("\nStarting task2\r\n\n");
	while(1) {
		//print task2 priority
		printf("Task2 running priority %lu \r\n", uxTaskPriorityGet(NULL));

		//rest task2 priority to 1
		printf("Restoring task 2 to priority 1\r\n\n");
		vTaskPrioritySet(NULL, 1);


	}
}


