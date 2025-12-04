#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <stdbool.h>

// --------------------
// FSM State Variables
// --------------------
enum state {
    DISARMED,
    ARMED,
    ENTRY,
    ALARM
};

extern volatile enum state current_state;

// --------------------
// Flags from Tasks
// --------------------
extern volatile bool button_pressed;
extern volatile bool motion_detected;
extern volatile bool pin_correct;
extern volatile bool pin_wrong;
extern volatile bool reset_pressed;
extern volatile bool entry_button_pressed;


// --------------------
// Counters / Timers
// --------------------
extern volatile uint8_t attempts;
extern volatile uint16_t timer_seconds;

// --------------------
// LED color enum
// --------------------
enum led {
    RED,
    ORANGE,
    GREEN,
    BLUE
};

#endif
