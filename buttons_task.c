#include <stdbool.h>
#include <ti/drivers/GPIO.h>
#include <FreeRTOS.h>
#include <task.h>

#include "ti_drivers_config.h"

// flag used by FSM
extern volatile bool button_pressed;

void ButtonTask(void *arg)
{
    bool last = false;

    for (;;) {
        bool now = (GPIO_read(CONFIG_GPIO_S1) == 0); // pressed = low

        if (now && !last) {
            button_pressed = true;
        }

        last = now;
        vTaskDelay(pdMS_TO_TICKS(20));  // debounce
    }
}
