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
#include "semphr.h"
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
extern TIM_HandleTypeDef htim6;

static void timerTask(void * pvParameters);
SemaphoreHandle_t timerSemaphore = NULL;
SemaphoreHandle_t resetSemaphore = NULL;
EventGroupHandle_t timerControlEventGroup = NULL;
#define pauseBit (1<<0)
#define resumeBit (1<<1)
#define clearBit (1<<2)
uint8_t ch;

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}

//TIM6 6 interrupts handler
void TIM6_Handler() {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(timerSemaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

//Button interrupt
void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(GPIO_Pin == BUTTON_EXTI13_Pin) {
		//give semaphore
		xSemaphoreGiveFromISR(resetSemaphore, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

//UART interrupt handler
void HAL_UART_RxCpltCallback (UART_HandleTypeDef * huart) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(huart->Instance == USART1 ) {
		switch(ch) {
		case 'p':
			xEventGroupSetBitsFromISR(timerControlEventGroup, pauseBit, &xHigherPriorityTaskWoken);
			break;

		case 'r':
			xEventGroupSetBitsFromISR(timerControlEventGroup, resumeBit, &xHigherPriorityTaskWoken);
			break;

		case 'c':
			xEventGroupSetBitsFromISR(timerControlEventGroup, clearBit, &xHigherPriorityTaskWoken);
			break;

		default:
			printf("Invalid character, press either 'p', 'r', or 'c'.\r\n");
			break;
		}
		HAL_UART_Receive_IT(&huart1, &ch, 1); //IMPORTANT
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(timerTask, "Timer Task", 200, NULL, 2, NULL);
	timerSemaphore = xSemaphoreCreateBinary();
	resetSemaphore = xSemaphoreCreateBinary();
	timerControlEventGroup = xEventGroupCreate();
	HAL_TIM_Base_Start_IT(&htim6);
	HAL_UART_Receive_IT(&huart1, &ch, 1);
	vTaskStartScheduler();

	while(1) {
	}
}

void timerTask(void * pvParameters) {
	EventBits_t timerBits;
	uint8_t sec = 0;
	printf("Starting timer task \r\n\n");
	printf("Timer: %02d\r\n", sec);
	while(1) {
		if(xSemaphoreTake(timerSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
			sec++;
			printf("Timer: %02d\r\n", sec);
		}

		if(xSemaphoreTake(resetSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
			HAL_TIM_Base_Stop_IT(&htim6);
			sec = 0;
			TIM6->CNT = 0;
			HAL_TIM_Base_Start_IT(&htim6);
			printf("Resetting timer to: %02d\r\n", sec);
		}

		timerBits = xEventGroupWaitBits(timerControlEventGroup, pauseBit | resumeBit | clearBit, pdTRUE, pdFALSE, pdMS_TO_TICKS(50));
		if(timerBits & pauseBit) {
			HAL_TIM_Base_Stop_IT(&htim6);
			printf("Pausing timer\r\n");
		}
		else if(timerBits & resumeBit) {
			HAL_TIM_Base_Start_IT(&htim6);
			printf("Resuming timer\r\n");
		}
		else if(timerBits & clearBit) {
			HAL_TIM_Base_Stop_IT(&htim6);
			sec = 0;
			TIM6->CNT = 0;
			HAL_TIM_Base_Start_IT(&htim6);
			printf("Resetting timer to: %02d\r\n", sec);
		}

	}
}


