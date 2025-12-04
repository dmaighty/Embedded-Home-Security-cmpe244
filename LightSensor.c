#include "LightSensor.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* ===== GLOBAL VARIABLES ===== */
volatile uint8_t  g_handNear = 0;
volatile uint32_t g_lightRaw = 0;
static uint32_t   baseline_raw = 0;

/* ===== I2C AND OPT3001 DEFINES ===== */
#define I2C_MODULE      EUSCI_B1_BASE
#define OPT3001_ADDR    0x44

#define OPT3001_REG_RESULT   0x00
#define OPT3001_REG_CONFIG   0x01

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

    eUSCI_I2C_MasterConfig cfg = {
        EUSCI_B_I2C_CLOCKSOURCE_SMCLK,
        12000000,
        EUSCI_B_I2C_SET_DATA_RATE_100KBPS,
        0,
        EUSCI_B_I2C_NO_AUTO_STOP
    };

    I2C_initMaster(I2C_MODULE, &cfg);
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
    I2C_masterSendMultiByteNext (I2C_MODULE, (value >> 8));
    I2C_masterSendMultiByteFinish(I2C_MODULE, (value & 0xFF));

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

    return ((uint16_t)hi << 8) | lo;
}

/* ============================================================
 * LIGHT SENSOR TASK (CALIBRATION + HAND-NEAR DETECTION)
 * ============================================================*/
static void vLightTask(void *arg)
{
    (void)arg;

    uint32_t sum;
    int i;
    uint16_t margin;

    /* Init sensor */
    I2C_Init_OPT3001();
    OPT3001_WriteReg(OPT3001_REG_CONFIG, 0xCC10);
    vTaskDelay(pdMS_TO_TICKS(150));

    /* ===== CALIBRATE BASELINE ===== */
    sum = 0;
    for (i = 0; i < 32; i++) {
        sum += OPT3001_ReadReg(OPT3001_REG_RESULT);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    baseline_raw = sum / 32;

    /* small margin (bright-detect threshold) */
    margin = baseline_raw / 4 + 50;

    /* ===== MAIN LOOP ===== */
    while (1)
    {
        uint16_t raw = OPT3001_ReadReg(OPT3001_REG_RESULT);

        g_lightRaw = raw;

        if (raw > baseline_raw + margin) {
            /* Bright (flashlight or uncovered) */
            g_handNear = 0;
        } else {
            /* Darkness or hand covering */
            g_handNear = 1;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ============================================================
 * PUBLIC API — CALL THIS FROM main()
 * ============================================================*/
void LightSensor_StartTask(void)
{
    xTaskCreate(vLightTask,
                "LightTask",
                TASK_STACK(512),
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);
}
