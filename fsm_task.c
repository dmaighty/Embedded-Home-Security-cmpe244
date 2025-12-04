#include <FreeRTOS.h>
#include <task.h>

void FSM_Task(void);  // from your FSM file

void FSM_Task_Wrapper(void *arg)
{
    for (;;) {
        FSM_Task();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
