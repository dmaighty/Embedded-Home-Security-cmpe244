#include "shared.h"

// FSM initial state
volatile enum state current_state = DISARMED;

// Flags set by tasks
volatile bool button_pressed = false;
volatile bool motion_detected = false;
volatile bool pin_correct = false;
volatile bool pin_wrong = false;
volatile bool reset_pressed = false;
volatile bool entry_button_pressed = false;


// Counters
volatile uint8_t attempts = 0;
volatile uint16_t timer_seconds = 0;
