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


static void SW_Task(void * pvParameters);
static void LED_Task(void * pvParameters);
TaskHandle_t SW_Handle = NULL;

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(SW_Task, "Switch Task", 200, NULL, 3, &SW_Handle);
	xTaskCreate(LED_Task, "LED Task", 200, NULL, 2, NULL);
	vTaskStartScheduler();

	while(1) {
	}
}

void SW_Task(void * pvParameters) {
	printf("Starting task 1\r\n\n");
	while(1) {
		// waiting for a switch
		printf("Waiting for switch press....\r\n");
		while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 1);
		// suspend task
		printf("Switch pressed, suspending switch task...\r\n\n");
		vTaskSuspend(NULL);
		// wait for a switch release
		while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0);
	}
}

void LED_Task(void * pvParameters) {
	printf("Starting task 1\r\n\n");
	while(1) {
		// toggle LED2
		printf("Starting LED task...\r\n");
		HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
		// resume switch task
		printf("Resuming switch task....\r\n\n");
		vTaskResume(SW_Handle);
	}
}
