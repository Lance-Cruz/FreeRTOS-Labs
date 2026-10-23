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


static void taskFunction(void * pvParameters);
static const char *task1String = "Task 1 Running\r\n\n";
static const char *task2String = "Task 2 Running\r\n\n";

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}

void userApp() {
	printf("Starting application\r\n\n");
	xTaskCreate(taskFunction, "Task 1", 200, (void *)task1String, 3, NULL);
	xTaskCreate(taskFunction, "Task 2", 200, (void *)task2String, 3, NULL);
	vTaskStartScheduler();

	while(1) {
	}
}

void taskFunction (void *pvParameters) {
	char *ptr;
	ptr = (char *)pvParameters;

	while(1) {
		printf("%s", ptr);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}



