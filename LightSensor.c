#include "LightSensor.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* ===== GLOBAL VARIABLES ===== */
volatile uint8_t  g_handNear      = 0;
volatile uint32_t g_lightRaw      = 0;
volatile uint8_t  g_alarmLatched  = 0;   /* NEW: latched alarm */
static uint32_t   baseline_raw    = 0;

/* ===== I2C AND OPT3001 DEFINES ===== */
#define I2C_MODULE          EUSCI_B1_BASE
#define OPT3001_ADDR        0x44

#define OPT3001_REG_RESULT  0x00
#define OPT3001_REG_CONFIG  0x01

/* ===== BUZZER DEFINES (EDU MKII: P2.7, TA0.4) ===== */
#define BUZZER_PORT         GPIO_PORT_P2
#define BUZZER_PIN          GPIO_PIN7

/* Timer_A0 runs from SMCLK = 12 MHz / 48 = 250 kHz */
#define BUZZER_TIMER_BASE   TIMER_A0_BASE
#define BUZZER_TIMER_CLK_HZ 250000u
#define BUZZER_DIVIDER      TIMER_A_CLOCKSOURCE_DIVIDER_48


#define TASK_STACK(bytes) \
    (configMINIMAL_STACK_SIZE + ((bytes) / sizeof(StackType_t)))

/* ============================================================
 * I2C INIT FOR OPT3001
 * ============================================================*/
static void I2C_Init_OPT3001(void)
{
    /* P6.5 = SCL, P6.4 = SDA */
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6,
                                               GPIO_PIN4 | GPIO_PIN5,
                                               GPIO_PRIMARY_MODULE_FUNCTION);

    /* SMCLK = 12 MHz, 100 kHz I2C */
    {
        eUSCI_I2C_MasterConfig cfg = {
            EUSCI_B_I2C_CLOCKSOURCE_SMCLK,
            12000000,
            EUSCI_B_I2C_SET_DATA_RATE_100KBPS,
            0,
            EUSCI_B_I2C_NO_AUTO_STOP
        };

        I2C_initMaster(I2C_MODULE, &cfg);
    }

    I2C_setSlaveAddress(I2C_MODULE, OPT3001_ADDR);
    I2C_setMode(I2C_MODULE, EUSCI_B_I2C_TRANSMIT_MODE);
    I2C_enableModule(I2C_MODULE);
}

/* ============================================================
 * OPT3001 REGISTER WRITE
 * ============================================================*/
static void OPT3001_WriteReg(uint8_t reg, uint16_t value)
{
    while (I2C_isBusBusy(I2C_MODULE));

    I2C_masterSendMultiByteStart(I2C_MODULE, reg);
    I2C_masterSendMultiByteNext (I2C_MODULE, (uint8_t)(value >> 8));
    I2C_masterSendMultiByteFinish(I2C_MODULE, (uint8_t)(value & 0xFF));

    while (I2C_isBusBusy(I2C_MODULE));
}

/* ============================================================
 * OPT3001 REGISTER READ (C89-compatible)
 * ============================================================*/
static uint16_t OPT3001_ReadReg(uint8_t reg)
{
    uint8_t hi, lo;

    while (I2C_isBusBusy(I2C_MODULE));

    /* send register pointer */
    I2C_masterSendSingleByte(I2C_MODULE, reg);
    while (I2C_isBusBusy(I2C_MODULE));

    /* switch to receive mode */
    I2C_setMode(I2C_MODULE, EUSCI_B_I2C_RECEIVE_MODE);

    hi = I2C_masterReceiveSingleByte(I2C_MODULE);
    while (I2C_isBusBusy(I2C_MODULE));

    lo = I2C_masterReceiveSingleByte(I2C_MODULE);
    while (I2C_isBusBusy(I2C_MODULE));

    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

/* ============================================================
 * BUZZER INIT & CONTROL
 * ============================================================*/
/* ============================================================
 * BUZZER INIT & CONTROL  (PWM via Timer_A0 CCR4)
 * ============================================================*/
static void Buzzer_Init(void)
{
    /* P2.7 as TA0.4 output */
    GPIO_setAsPeripheralModuleFunctionOutputPin(
        BUZZER_PORT, BUZZER_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    Timer_A_stopTimer(BUZZER_TIMER_BASE);

    /* Up-mode config, dummy period initially (1 kHz) */
    {
        Timer_A_UpModeConfig upCfg = {
            TIMER_A_CLOCKSOURCE_SMCLK,
            BUZZER_DIVIDER,
            (BUZZER_TIMER_CLK_HZ / 1000u) - 1u,   /* CCR0: 1 kHz */
            TIMER_A_TAIE_INTERRUPT_DISABLE,
            TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,
            TIMER_A_DO_CLEAR
        };
        Timer_A_initUpMode(BUZZER_TIMER_BASE, &upCfg);
    }

    /* CCR4: toggle/reset output mode (square wave) */
    {
        Timer_A_CompareModeConfig cmpCfg = {
            TIMER_A_CAPTURECOMPARE_REGISTER_4,
            TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
            TIMER_A_OUTPUTMODE_TOGGLE_RESET,
            (BUZZER_TIMER_CLK_HZ / 1000u) / 2u     /* 50% duty */
        };
        Timer_A_initCompare(BUZZER_TIMER_BASE, &cmpCfg);
    }

    Timer_A_stopTimer(BUZZER_TIMER_BASE);
}

/* freqHz = 0 -> OFF, otherwise loud tone at freqHz */
static void Buzzer_SetFrequency(uint16_t freqHz)
{
    uint16_t period;

    if (freqHz == 0u) {
        /* Stop timer and force pin low */
        Timer_A_stopTimer(BUZZER_TIMER_BASE);
        GPIO_setAsOutputPin(BUZZER_PORT, BUZZER_PIN);
        GPIO_setOutputLowOnPin(BUZZER_PORT, BUZZER_PIN);
        return;
    }

    period = (uint16_t)(BUZZER_TIMER_CLK_HZ / (uint32_t)freqHz);

    /* Back to TA0.4 function */
    GPIO_setAsPeripheralModuleFunctionOutputPin(
        BUZZER_PORT, BUZZER_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Update period (CCR0) and duty (CCR4 = 50%) */
    Timer_A_setCompareValue(
        BUZZER_TIMER_BASE,
        TIMER_A_CAPTURECOMPARE_REGISTER_0,
        (period - 1u));

    Timer_A_setCompareValue(
        BUZZER_TIMER_BASE,
        TIMER_A_CAPTURECOMPARE_REGISTER_4,
        (period / 2u));

    Timer_A_startCounter(BUZZER_TIMER_BASE, TIMER_A_UP_MODE);
}


/* ============================================================
 * LIGHT SENSOR TASK (CALIBRATION + HAND-NEAR + LATCH ALARM)
 * ============================================================*/
static void vLightTask(void *arg)
{
    (void)arg;

    uint32_t sum;
    int i;
    uint16_t margin;
    uint16_t thresholdLow;

    /* Init sensor */
    I2C_Init_OPT3001();
    OPT3001_WriteReg(OPT3001_REG_CONFIG, 0xCC10);  /* continuous, auto-range */
    vTaskDelay(pdMS_TO_TICKS(150));

    /* ===== CALIBRATE BASELINE (NORMAL LIGHT) ===== */
    sum = 0;
    for (i = 0; i < 32; i++) {
        uint16_t rawCal = OPT3001_ReadReg(OPT3001_REG_RESULT);
        sum += rawCal;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    baseline_raw = sum / 32;

    /* margin: how much drop from baseline counts as "hand near" */
    margin = (uint16_t)(baseline_raw / 4u + 50u);   /* 25% + offset */

    /* threshold below which we say â€œdark / hand nearâ€� */
    if (baseline_raw > margin) {
        thresholdLow = (uint16_t)(baseline_raw - margin);
    } else {
        thresholdLow = (uint16_t)(baseline_raw / 2u);
    }

    for (;;)
    {
        uint16_t raw = OPT3001_ReadReg(OPT3001_REG_RESULT);

        /* ignore obvious junk */
        if (raw != 0) {
            g_lightRaw = raw;

            if (raw < thresholdLow) {
                g_handNear = 1;   /* darker than baseline -> object/hand */
                /* LATCH the alarm: once set, stays 1 until main clears */
                // g_alarmLatched = 1;
            } else {
                g_handNear = 0;   /* normal/bright */
                /* DO NOT clear g_alarmLatched here */
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));   /* ~5 Hz updates */
    }
}

/* ============================================================
 * BUZZER TASK
 *  - Beeps continuously while g_alarmLatched == 1
 *  - Stops only when main_freertos.c clears g_alarmLatched to 0
 * ============================================================*/
/* ============================================================
 * BUZZER TASK
 *  - While g_alarmLatched == 1:
 *      alternate between two frequencies (woo-woo siren)
 * ============================================================*/
static void vBuzzerTask(void *arg)
{
    (void)arg;
    Buzzer_Init();

    for (;;)
    {
        if (g_alarmLatched) {
            /* Simple 2-tone police-style siren */
            Buzzer_SetFrequency(800);    /* low tone */
            vTaskDelay(pdMS_TO_TICKS(250));

            Buzzer_SetFrequency(1500);   /* high tone */
            vTaskDelay(pdMS_TO_TICKS(250));
        } else {
            /* Alarm cleared -> buzzer off */
            Buzzer_SetFrequency(0);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}


/* ============================================================
 * PUBLIC API â€” START TASKS
 * ============================================================*/
void LightSensor_StartTask(void)
{
    xTaskCreate(vLightTask,
                "LightTask",
                TASK_STACK(512),
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);

    xTaskCreate(vBuzzerTask,
                "BuzzerTask",
                TASK_STACK(256),
                NULL,
                tskIDLE_PRIORITY + 1,
                NULL);
}
