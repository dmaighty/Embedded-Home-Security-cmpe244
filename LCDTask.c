#include "LCDTask.h"

extern Graphics_Context g_sContext;

void LCDTask(void *arg0) {
    while(1) {
        switch(current_state) {

        case DISARMED:
            drawResultScreen("DISARMED");
            if(xSemaphoreTake(disarmedToArmed, portMAX_DELAY) == pdTRUE) {
                break;
            }

        case ARMED:
            drawResultScreen("ARMED");
            while(1) {
                if(xSemaphoreTake(armedToEntry, pdMS_TO_TICKS(100)) == pdTRUE) {
                    break;
                }
                else if(xSemaphoreTake(armedToDisarmed, pdMS_TO_TICKS(100)) == pdTRUE) {
                    break;
                }
            }
            break;

        case ENTRY:
            xSemaphoreGive(enterPinMode);
            while(1) {
                if(xSemaphoreTake(entryToAlarm, pdMS_TO_TICKS(100))) {
                    break;
                }
                else if(xSemaphoreTake(entryToDisarmed, pdMS_TO_TICKS(100))) {
                    break;
                }
            }
            break;


        case ALARM:
            while(1) {
                drawResultScreen("ALARM");
                if(xSemaphoreTake(alarmToDisarmed, pdMS_TO_TICKS(100)) == pdTRUE) {
                    break;
                }
                else {
                    Graphics_clearDisplay(&g_sContext);
                    vTaskDelay(100);
                }

            }

            break;
        }
    }
}



