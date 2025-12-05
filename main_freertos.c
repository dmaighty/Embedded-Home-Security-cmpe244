#include <stdbool.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Board.h>
#include "ti_drivers_config.h"

#include "freertos/lcd_functions.h"
#include "freertos/LCDTask.h"
#include "freertos/shared.h"

extern void TimerTask(void *arg);

// application hooks, issues without this
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for(;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for(;;);
}

// externs
UART_Handle uart;

extern void ButtonTask(void *arg);
extern void FSM_Task_Wrapper(void *arg);

int main(void)
{
    // Init all necessary peripherals
    Board_init();
    GPIO_init();
    UART_init();
    LCD_init();

    // Creating Semaphores for task synchronization
    disarmedToArmed = xSemaphoreCreateBinary();
    armedToEntry = xSemaphoreCreateBinary();
    entryToDisarmed = xSemaphoreCreateBinary();
    entryToAlarm = xSemaphoreCreateBinary();
    enterPinMode = xSemaphoreCreateBinary();
    alarmToDisarmed = xSemaphoreCreateBinary();

    // optional UART for debugging
    UART_Params params;
    UART_Params_init(&params);
    params.baudRate = 115200;
    uart = UART_open(CONFIG_UART_0, &params);

    // Create tasks
    xTaskCreate(ButtonTask, "Button", 768, NULL, 1, NULL);
    xTaskCreate(FSM_Task_Wrapper, "FSM", 768, NULL, 4, NULL);
    xTaskCreate(TimerTask, "Timer", 768, NULL, 5, NULL);
    xTaskCreate(LCDTask, "LCD", 768, NULL, 1, NULL);
    xTaskCreate(pinTask, "Pin", 768, NULL, 3, NULL);


    vTaskStartScheduler();

    while (1) { }
}

