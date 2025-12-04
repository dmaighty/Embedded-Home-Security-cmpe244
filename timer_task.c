#include <FreeRTOS.h>
#include <task.h>
#include "shared.h"

// counts down timer_seconds every 1 sec
void TimerTask(void *arg)
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (current_state == ENTRY && timer_seconds > 0)
        {
            timer_seconds--;
        }
    }
}
