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
static void UART_Task(void * pvParameters);
static void waitTask(void * pvParameters);
EventGroupHandle_t LED_EventGroup = NULL;
TaskHandle_t waitTaskHandle = NULL;
#define buttonBit (1 << 3)
#define UART_Bit (1 << 2)
#define syncBits (buttonBit | UART_Bit)


// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(buttonTask, "Button Task", 200, NULL, 3, NULL);
	xTaskCreate(UART_Task, "UART Task", 200, NULL, 2, NULL);
	xTaskCreate(waitTask, "Wait Task", 200, NULL, 1, &waitTaskHandle);
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
			printf("Button pressed, setting event group bit...\r\n\n");
			xEventGroupSync(LED_EventGroup, buttonBit, syncBits, portMAX_DELAY);
			printf("Button task synchronized...\r\n\n");
			printf("\r\nSynchronisation complete, deleting wait task\r\n\n");
			vTaskDelete(waitTaskHandle);
			break;
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	while(1) {
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0 ){
			printf("Button pressed at %lums\r\n", xTaskGetTickCount() * 1000 / configTICK_RATE_HZ );
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void UART_Task(void * pvParameters) {
	uint8_t ch;
	printf("Starting UART task\r\n");
	while(1) {
		if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != 0) {
			HAL_UART_Receive(&huart1, &ch, 1, 100);
			if(ch == 's') {
				printf("Character 's' received, setting LED off event group bit...\r\n");
				xEventGroupSync(LED_EventGroup, UART_Bit, syncBits, portMAX_DELAY);
				printf("UART task synchronized...\r\n\n");
				break;
			}
			else {
				printf("Invalid option, enter 's' to set the event group bit\r\n\n");
			}
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	while(1) {
		if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != 0) {
			HAL_UART_Receive(&huart1, &ch, 1, 100);
			printf("Character received: %c\r\n", ch);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void waitTask(void * pvParameters) {
	//EventBits_t controlBits;
	printf("Starting LED task\r\n\n");
	while(1) {
		printf("Synchronizing...\r\n");
		HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


