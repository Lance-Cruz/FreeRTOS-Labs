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
static void UART_Task(void * pvParameters);
EventGroupHandle_t LED_EventGroup = NULL;
#define buttonBit (1 << 3)
#define uartBit (1 << 2)


// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(buttonTask, "Button Task", 200, NULL, 2, NULL);
	xTaskCreate(LED_Task, "Led Task", 200, NULL, 3, NULL);
	xTaskCreate(UART_Task, "Led Task", 200, NULL, 1, NULL);
	LED_EventGroup = xEventGroupCreate();
	vTaskStartScheduler();

	while(1) {
	}
}

void buttonTask(void * pvParameters) {
	printf("Starting button task\r\n");
	while(1) {
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0 ){
			// set event group bit
			xEventGroupSetBits(LED_EventGroup, buttonBit);
			printf("Button pressed, setting event group...\r\n\n");
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void UART_Task(void * pvParameters) {
	printf("Starting UART task\r\n");
	while(1) {
		uint8_t ch;
		if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != 0) {
			HAL_UART_Receive(&huart1, &ch, 1, 100);
			if(ch == 'o') {
				printf("Character 'o' received, setting LED off event group bit...\r\n");
				xEventGroupSetBits(LED_EventGroup, uartBit);
			}
			else {
				printf("Invalid option, enter 'o' to turn off the LED\r\n\n");
			}
		}
	}
}

void LED_Task(void * pvParameters) {
	EventBits_t controlBits;
	printf("Starting LED task\r\n\n");
	while(1) {
		printf("Waiting for event group bit to be set\r\n");
		controlBits = xEventGroupWaitBits(LED_EventGroup, buttonBit | uartBit, pdTRUE, pdFALSE, portMAX_DELAY);
		if(controlBits & buttonBit) {
			printf("Button event group set bit detected turning on LED\r\n\n");
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 1);
		}
		else if (controlBits & uartBit) {
			printf("LED off event group set bit detected turning off LED\r\n\n");
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 0);
		}
	}
}


