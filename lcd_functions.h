#ifndef LCD_FUNCTIONS_H_
#define LCD_FUNCTIONS_H_


/* Necessary for LCD */
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "shared.h"
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>


#define MAX_PIN_ATTEMPTS 3

/* Pin state machine */
typedef enum {
    STATE_ENTER,
    STATE_CHECK,
    STATE_SUCCESS,
    STATE_FAIL,
    STATE_LOCKED
} PinState;



/* Joystick events */
typedef enum {
    JOY_NONE,
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT,
    JOY_PRESS
} JoystickEvent;
// PIN globals





extern Graphics_Context g_sContext;



extern volatile uint16_t resultBuff[2];
extern uint8_t enteredPin[4];
extern const uint8_t correctPin[4];
extern uint8_t cursorIndex;
extern uint8_t attemptsLeft;
extern PinState pinState;

void graphicsInit(void);
void adcInit(void);
void initJoystickButton(void);
void ADC14_IRQHandler(void);
static void drawPinScreen(void);
void drawResultScreen(const char *msg);
void drawTimeRemaining(uint16_t time);
int pinsMatch(const uint8_t *a, const uint8_t *b);
JoystickEvent readJoystick(void);
void vEnterPin(void);
void pinTask(void *arg);
void LCD_init(void);

#endif /* LCD_FUNCTIONS_H_ */
