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
#include "event_groups.h"

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

static void buttonTask(void * pvParameters);
static void LED_Task(void * pvParameters);
EventGroupHandle_t buttonEventGroup = NULL;
#define buttonBit (1 << 3)


// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(buttonTask, "Button Task", 200, NULL, 2, NULL);
	xTaskCreate(LED_Task, "Led Task", 200, NULL, 3, NULL);
	buttonEventGroup = xEventGroupCreate();
	vTaskStartScheduler();

	while(1) {
	}
}

void buttonTask(void * pvParameters) {
	printf("Starting button task\r\n");
	while(1) {
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0 ){
			// set event group bit
			xEventGroupSetBits(buttonEventGroup, buttonBit);
			printf("Button pressed, setting event group...\r\n\n");
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void LED_Task(void * pvParameters) {
	EventBits_t controlBits;
	printf("Starting LED task\r\n\n");
	while(1) {
		printf("Waiting for event group bit to be set\r\n");
		controlBits = xEventGroupWaitBits(buttonEventGroup, buttonBit, pdTRUE, pdFALSE, portMAX_DELAY);
		printf("Event group bit set %06x, toggling LED...\r\n\n", controlBits);
		HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
	}
}


