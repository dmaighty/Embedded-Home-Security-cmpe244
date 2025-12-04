#include "lcd_functions.h"
#include <stdio.h>


/* LCD Display Objects provided by driver */
extern Graphics_Display g_sCrystalfontz128x128;
extern const Graphics_Display_Functions g_sCrystalfontz128x128_funcs;
Graphics_Context g_sContext;

volatile uint16_t resultBuff[2] = {0,0};
uint8_t enteredPin[4]       = {0, 0, 0, 0};
const uint8_t correctPin[4] = {1, 2, 3, 4};
uint8_t cursorIndex         = 0;
uint8_t attemptsLeft        = MAX_PIN_ATTEMPTS;
PinState pinState           = STATE_ENTER;



void graphicsInit(void)
{
    WDT_A_holdTimer();

    /* Initialize LCD hardware */
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Init graphics */
    Graphics_initContext(&g_sContext,
                         &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);
}

void adcInit(void)
{
    // Joystick pins: P6.0 - A15 (X), P4.4 - A9 (Y)
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0,
                                               GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4,
                                               GPIO_TERTIARY_MODULE_FUNCTION);

    ADC14_enableModule();

    ADC14_initModule(ADC_CLOCKSOURCE_MCLK,
                     ADC_PREDIVIDER_1,
                     ADC_DIVIDER_1,
                     0);

    // Multi-sequence mode, but we'll trigger manually each time
    ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, false);

    ADC14_configureConversionMemory(ADC_MEM0,
                                    ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                    ADC_INPUT_A15,
                                    ADC_NONDIFFERENTIAL_INPUTS);

    ADC14_configureConversionMemory(ADC_MEM1,
                                    ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                    ADC_INPUT_A9,
                                    ADC_NONDIFFERENTIAL_INPUTS);


    // ADC14_enableInterrupt(ADC_INT1);
    // Interrupt_enableInterrupt(INT_ADC14);
    // ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    ADC14_enableConversion();
}

void initJoystickButton(void)
{
    // P4.1 as input with pull-up (button active LOW)
    P4->DIR  &= ~BIT1;
    P4->REN  |=  BIT1;
    P4->OUT  |=  BIT1;
}

void ADC14_IRQHandler(void)
{
    uint64_t status = ADC14_getEnabledInterruptStatus();
    ADC14_clearInterruptFlag(status);

    if (status & ADC_INT1)
    {
        resultBuff[0] = ADC14_getResult(ADC_MEM0);  // X axis
        resultBuff[1] = ADC14_getResult(ADC_MEM1);  // Y axis
    }
}




void drawPinScreen(void)
{
    char line2[20];
    char line3[20];

    Graphics_clearDisplay(&g_sContext);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"ENTER PIN:",
                                AUTO_STRING_LENGTH,
                                64, 20,
                                OPAQUE_TEXT);

    // [ d d d d ]
    snprintf(line2, sizeof(line2),
             "[ %d %d %d %d ]",
             enteredPin[0],
             enteredPin[1],
             enteredPin[2],
             enteredPin[3]);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)line2,
                                AUTO_STRING_LENGTH,
                                64, 40,
                                OPAQUE_TEXT);

    snprintf(line3, sizeof(line3),
             "Attempts: %d",
             attemptsLeft);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)line3,
                                AUTO_STRING_LENGTH,
                                64, 60,
                                OPAQUE_TEXT);

    // Draw a caret under the current digit
    int16_t cursorX = 64;
    int16_t cursorY = 52;
    int16_t offset = (cursorIndex - 1.5f) * 12;
    cursorX += offset;

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"^",
                                AUTO_STRING_LENGTH,
                                cursorX,
                                cursorY,
                                OPAQUE_TEXT);

    Graphics_flushBuffer(&g_sContext);
}

void drawResultScreen(const char *msg)
{
    Graphics_clearDisplay(&g_sContext);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)msg,
                                AUTO_STRING_LENGTH,
                                64, 40,
                                OPAQUE_TEXT);

    Graphics_flushBuffer(&g_sContext);
}

void drawTimeRemaining(uint16_t time) {

    char timeString[4];
    sprintf(timeString, "%d", time);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Time Remaining",
                                AUTO_STRING_LENGTH,
                                64,
                                70,
                                OPAQUE_TEXT);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)timeString,
                                AUTO_STRING_LENGTH,
                                64,
                                80,
                                OPAQUE_TEXT);
    Graphics_flushBuffer(&g_sContext);
}

int pinsMatch(const uint8_t *a, const uint8_t *b)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}

JoystickEvent readJoystick(void)
{
    static uint8_t lastPressed = 0;
    uint8_t nowPressed = ((P4->IN & BIT1) == 0);   // active LOW - 1 when pressed

    if (nowPressed && !lastPressed) {
        lastPressed = nowPressed;
        return JOY_PRESS;
    }
    lastPressed = nowPressed;


    // Trigger ADC conversion for X/Y
    ADC14_toggleConversionTrigger();
    __delay_cycles(240000);   // small delay so conversion finishes

    uint16_t x = ADC14_getResult(ADC_MEM0);  // X axis
    uint16_t y = ADC14_getResult(ADC_MEM1);  // Y axis

    const uint16_t HIGH_THRESH = 15000;
    const uint16_t LOW_THRESH  = 1000;

    JoystickEvent phys = JOY_NONE;

    if (y > HIGH_THRESH) {
        phys = JOY_UP;
    } else if (y < LOW_THRESH) {
        phys = JOY_DOWN;
    } else if (x > HIGH_THRESH) {
        phys = JOY_RIGHT;
    } else if (x < LOW_THRESH) {
        phys = JOY_LEFT;
    } else {
        phys = JOY_NONE;
    }

    static JoystickEvent lastPhys = JOY_NONE;
    JoystickEvent event = JOY_NONE;

    if (lastPhys == JOY_NONE && phys != JOY_NONE) {
        event = phys;        
    } else {
        event = JOY_NONE;    
    }

    lastPhys = phys;
    return event;
}

void vEnterPin(void){
    // Initialize PIN state
    pinState     = STATE_ENTER;
    attemptsLeft = MAX_PIN_ATTEMPTS;
    cursorIndex  = 0;
    int i = 0;
    for (i = 0; i < 4; i++) enteredPin[i] = 0;

    drawPinScreen();

    while (current_state == ENTRY)
    {
        JoystickEvent ev = readJoystick();

        switch (pinState) {

        case STATE_ENTER:
            if (ev == JOY_LEFT && cursorIndex > 0) {
                cursorIndex--;
                drawPinScreen();
            } else if (ev == JOY_RIGHT && cursorIndex < 3) {
                cursorIndex++;
                drawPinScreen();
            } else if (ev == JOY_UP) {
                enteredPin[cursorIndex] = (enteredPin[cursorIndex] + 1) % 10;
                drawPinScreen();
            } else if (ev == JOY_DOWN) {
                enteredPin[cursorIndex] = (enteredPin[cursorIndex] + 9) % 10; // -1 mod 10
                drawPinScreen();
            } else if (ev == JOY_PRESS) {
                //  CONFIRM PIN
                pinState = STATE_CHECK;
            }
            drawTimeRemaining(timer_seconds);
            break;

        case STATE_CHECK:
            if (pinsMatch(enteredPin, correctPin)) {
                pinState = STATE_SUCCESS;
                drawResultScreen("PIN CORRECT!");
            } else {
                attemptsLeft--;
                if (attemptsLeft == 0) {
                    pinState = STATE_LOCKED;
                } else {
                    pinState = STATE_FAIL;
                    drawResultScreen("WRONG PIN");
                }
            }
            break;

        case STATE_SUCCESS:
            pin_correct = true;
            vTaskDelay(pdMS_TO_TICKS(20));
            break;

        case STATE_FAIL:
            // Reset and go back to ENTER
            for (i = 0; i < 4; i++) enteredPin[i] = 0;
            cursorIndex = 0;
            pinState    = STATE_ENTER;
            drawPinScreen();
            break;

        case STATE_LOCKED:
            pin_wrong = true;
            vTaskDelay(pdMS_TO_TICKS(20));
            break;
        }
        __delay_cycles(2400000);   // small debounce / poll delay
    }
}

void LCD_init(void){
    graphicsInit();
    adcInit();
    initJoystickButton();
}

void pinTask(void *arg){
    while(1){
        xSemaphoreTake(enterPinMode, portMAX_DELAY);
        vEnterPin();
    }
}
