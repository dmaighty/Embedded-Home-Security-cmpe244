#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <stdint.h>

/* Global values updated by the light sensor task */
extern volatile uint8_t  g_handNear;     /* 1 = dark/hand near, 0 = bright */
extern volatile uint32_t g_lightRaw;     /* raw 16-bit OPT3001 value */

/* Latched alarm flag:
 *  - Set to 1 by LightSensor when hand/object detected
 *  - Cleared to 0 by main_freertos.c (e.g. S1 or joystick)
 */
extern volatile uint8_t  g_alarmLatched;

/* Call this from main() to start the FreeRTOS light sensor + buzzer tasks */
void LightSensor_StartTask(void);

#endif
