#include <delays.h>
#include "keypad.h"

/*******************************************************************************
* PUBLIC FUNCTION: c_read_keypad
*
* PARAMETERS:
* ~ void
*
* RETURN:
* ~ numeric value for the pressed key. Return 0xFF if no key is pressed.
*
* DESCRIPTIONS:
* Read from the keypad.
*
*******************************************************************************/
int V;
int I;

unsigned char keypad_read(){
    if (PORTDbits.RD3 == 1) 
    {
            I = 0b11110000;
            V = PORTC & I;
            V = V >> 4;

            if (V == 0x00) return '1'; // Key '1' is pressed
            if (V == 0x01) return '2'; // Key '2' is pressed
            if (V == 0x02) return '3'; // Key '3' is pressed
            if (V == 0x04) return '7'; // Key 'A' is pressed, we will store as 10
            if (V == 0x07) return 'C'; // Key '1' is pressed
            if (V == 0x06) return '9'; // Key '2' is pressed
            if (V == 0x08) return '4'; // Key '3' is pressed
            if (V == 0x0B) return 'B'; // Key 'A' is pressed, we will store as 10
            if (V == 0x0A) return '6'; // Key '1' is pressed
            if (V == 0x0F) return 'D'; // Key '2' is pressed
            if (V == 0x03) return 'A'; // Key '3' is pressed
            if (V == 0x05) return '8'; // Key 'A' is pressed, we will store as 10
            if (V == 0x09) return '5'; // Key '2' is pressed
            if (V == 0x0D) return 'F'; // Key '3' is pressed
            if (V == 0x0C) return '0'; // Key 'A' is pressed, we will store as 10
            if (V == 0x0E) return 'E'; // Key 'A' is pressed, we will store as 10
        }
    
        return 0xFF; // if no key press, the register is 0xFF;
    }


    


/*******************************************************************************
* PUBLIC FUNCTION: c_wait_keypad
*
* PARAMETERS:
* ~ void
*
* RETURN:
* ~ numeric value for the pressed key.
*
* DESCRIPTIONS:
* Wait until the key is pressed and released.
*
*******************************************************************************/
unsigned char keypad_wait(void)
{
	// The pressed key.
	unsigned char c_pressed_key = 0xFF;
    
    c_pressed_key = keypad_read();

	// Wait until the key is pressed.
	/*do {
		c_pressed_key = keypad_read();
	}	
	while (c_pressed_key == 0xFF);*/
	
	// Wait until the key is released.
	while (keypad_read() != 0xFF);
	
	return c_pressed_key;
}	