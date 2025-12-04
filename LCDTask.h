#ifndef FREERTOS_LCDTASK_H_
#define FREERTOS_LCDTASK_H_

#include <stdbool.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include "shared.h"
#include "lcd_functions.h"

void LCDTask(void *arg0);

#endif
