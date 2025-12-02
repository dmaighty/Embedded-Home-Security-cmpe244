#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Board.h>

extern void *temperatureThread(void *arg0);
extern void *consoleThread(void *arg0);

/* Stack size in 16-bit words */
#define THREADSTACKSIZE    768 / sizeof(portSTACK_TYPE)

/*
 *  ======== main ========
 */
int main(void)
{
    TaskHandle_t consoleHandle;
    TaskHandle_t temperatureHandle;
    BaseType_t   retc;

    /* Call driver init functions */
    Board_init();

    retc = xTaskCreate((TaskFunction_t)consoleThread,     // pvTaskCode
                                "console",                // pcName
                                THREADSTACKSIZE,          // usStackDepth
                                NULL,                     // pvParameters
                                1,                        // uxPriority
                                &consoleHandle);          // pxCreatedTask
    if (retc != pdPASS) {
        /* xTaskCreate() failed */
        while (1);
    }

    retc = xTaskCreate((TaskFunction_t)temperatureThread, // pvTaskCode
                       "temperature",                     // pcName
                       THREADSTACKSIZE,                   // usStackDepth
                       NULL,                              // pvParameters
                       2,                                 // uxPriority
                       &temperatureHandle);               // pxCreatedTask
    if (retc != pdPASS) {
        /* xTaskCreate() failed */
        while (1);
    }

    /* Initialize the GPIO since multiple threads are using it */
    GPIO_init();

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    return (0);
}

//*****************************************************************************
//
//! \brief Application defined malloc failed hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationMallocFailedHook()
{
    /* Handle Memory Allocation Errors */
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined stack overflow hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    //Handle FreeRTOS Stack Overflow
    while(1)
    {
    }
}
