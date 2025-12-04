#include <stdbool.h>
#include <ti/drivers/GPIO.h>
#include <FreeRTOS.h>
#include <task.h>

#include "ti_drivers_config.h"
#include "shared.h"

void ButtonTask(void *arg)
{
    bool lastS1 = false;
    bool lastS2 = false;

    for (;;) {
        bool nowS1 = (GPIO_read(CONFIG_GPIO_S1) == 0);
        bool nowS2 = (GPIO_read(CONFIG_GPIO_S2) == 0);

        if (nowS1 && !lastS1) {
            button_pressed = true;
        }
        if (nowS2 && !lastS2) {
            entry_button_pressed = true;
        }

        lastS1 = nowS1;
        lastS2 = nowS2;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
