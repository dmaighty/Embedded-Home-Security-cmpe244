// draft for the finite state machine controller


#include <stdbool.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"
#include <FreeRTOS.h>
#include <task.h>
#include "shared.h"
#include "lcd_functions.h"

extern volatile uint8_t  g_handNear;
extern volatile uint8_t  g_alarmLatched;

void set_led(enum led color)
{
    // turn everything off first
    GPIO_write(CONFIG_GPIO_LED_GREEN, 0);
    GPIO_write(CONFIG_GPIO_LED_BLUE, 0);
    GPIO_write(CONFIG_GPIO_LED_RED, 0);

    switch (color)
    {
        case GREEN:
            GPIO_write(CONFIG_GPIO_LED_GREEN, 1);
            break;

        case BLUE:
            GPIO_write(CONFIG_GPIO_LED_BLUE, 1);
            break;

        case RED:
            GPIO_write(CONFIG_GPIO_LED_RED, 1);
            break;

        case ORANGE:
            //Red + Green, but we need to dim down green
            GPIO_write(CONFIG_GPIO_LED_RED, 1);
            GPIO_write(CONFIG_GPIO_LED_GREEN, 1);
            vTaskDelay(pdMS_TO_TICKS(5));
            GPIO_write(CONFIG_GPIO_LED_GREEN, 0);
            vTaskDelay(pdMS_TO_TICKS(1));
            break;

        default:
            break;
    }
}

// Main FSM task
void FSM_Task(void) {
    switch (current_state) {

        case DISARMED:
            set_led(GREEN);
            if (button_pressed) {
                xSemaphoreGive(disarmedToArmed);
                current_state = ARMED;
            }
            break;

        case ARMED:
            set_led(BLUE);
            motion_detected = g_handNear;
            if (button_pressed) {
                current_state = DISARMED;
                xSemaphoreGive(armedToDisarmed);
            }
            else if (motion_detected) {
                xSemaphoreGive(armedToEntry);
                current_state = ENTRY;
                g_alarmLatched = 1;
                timer_seconds = 120;
            }
            else if (entry_button_pressed){
                // this is to test the state, we will be using the motion detector
                xSemaphoreGive(armedToEntry);
                g_alarmLatched = 1;
                current_state = ENTRY;
                timer_seconds = 120;
            }
            break;

        case ENTRY:
            set_led(ORANGE);

            if (pin_correct) {
                pin_correct = false;
                g_alarmLatched = 0;
                xSemaphoreGive(entryToDisarmed);
                current_state = DISARMED;
            }
            else if (pin_wrong) {
                pin_wrong = false;
                xSemaphoreGive(entryToAlarm);
                current_state = ALARM;
            }
            else if (timer_seconds == 0) {
                current_state = ALARM;
                xSemaphoreGive(entryToAlarm);
            }
            break;

        case ALARM:
            set_led(RED);
            if (button_pressed) {
                xSemaphoreGive(alarmToDisarmed);
                g_alarmLatched = 0;
                current_state = DISARMED;
            }
            break;
    }

    // clear button flags
    button_pressed = false;
    entry_button_pressed = false;
}
