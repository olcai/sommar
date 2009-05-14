/* AVR system */

#include <util/delay.h>
#include <avr/interrupt.h>

#include "system.h"
#include "lcd_lib.h"
#include "config.h"

void panic(const char* msg)
{
    unsigned char panic_msg[] = "KERNEL PANIC!";
    cli();
    PORTC=0xFF;
    LCDinit(); 
    LCDclr();
    LCDGotoXY(0,0);
    LCDcursorOFF(); 
    LCDstring(panic_msg, sizeof(panic_msg)-1);
    LCDGotoXY(0,1);
    while(*msg)
	LCDsendChar(*(msg++));
    while(1)
    {
        PORTC = ~PORTC;
        _delay_ms(500);
    }
}
