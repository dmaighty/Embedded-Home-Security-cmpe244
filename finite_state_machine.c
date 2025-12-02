// draft for the finite state machine controller


enum state{
    DISARMED,
    ARMED,
    ENTRY,
    ALARM
};

enum led{
    RED,
    ORANGE,
    GREEN,
    BLUE
};

enum state current_state = DISARMED;
void set_led(enum led color);

// Main FSM task
void FSM_Task(void) {
    switch (current_state) {
        case DISARMED:
            set_led(GREEN);
			// LCD show disarmed
            if (button_pressed) {
                current_state = ARMED;
                attempts = 0;
            }
            break;

        case ARMED:
            set_led(BLUE);
			// LCD show armed
            if (button_pressed) {
                current_state = DISARMED;
                set_led(GREEN); 
            }
            else if (motion_detected) {
                current_state = ENTRY;
                timer = 120; 
                attempts = 0;
            }
            break;

        case ENTRY:
            set_led(ORANGE);
			// LCD or UART: show "motion detected, enter pin"
            if (pin_correct) {
                current_state = DISARMED;
                set_led(GREEN); 
            }
            else if (pin_wrong) {
                attempts++;
                if (attempts >= 3) {
                    current_state = ALARM;
                    set_led(RED);
                    // Turn on alarm sound/buzzer
                }
                else {
                    set_led(ORANGE);
                    // LCD or UART show number of attempts remaining
                }
            }
            else if (timer == 0) {
                current_state = ALARM;
                set_led(RED);
                // Output LED and LCD updates here
                // Turn on alarm sound/buzzer
            }
            break;

        case ALARM:
			// LCD and UART: show "ALARM! Calling police!!"
            set_led(RED);
            if (reset) {
                current_state = DISARMED;
                set_led(GREEN);
                // Output LED and LCD updates here
            }
            break;
    }

    // Reset flags (these might be implemented in another task)
    motion_detected = false;
    button_pressed = false;
    pin_correct = false;
    pin_wrong = false;
}