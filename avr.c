/* AVR system */

#define F_CPU 3686400

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <ctype.h>

#include "lcd_lib.h"
#include "system.h"
#include "config.h"
#include "uart.h"
#include "suart.h"
#include "rtc.h"

// Basstation eller inte?
#define BASE 0
#define NODE 1

// Väntetid innan svar skickas i antal timer2 compare-matchningar
#define ANSWER_SEND_TIMER2_WAITS 16


volatile uint8_t operation_mode;
volatile char answer[24] = {'0', '4'};
volatile uint8_t command_parsed = 0;

void check_command(char* buffer);
void execute_command(char* buffer);
void send_radio_command(char* buffer);
void set_measure_time(char* buffer);

void setupled(void) {
  // Setup button input
//  DDRC&=~_BV(0);//set PORTD pin0 to zero as input
//  PORTC|=_BV(0);//Enable pull up
  
  // Setup LED output
//  PORTB|=_BV(1);//led OFF
//  DDRB|=_BV(1);//set PORTD pin1 to one as output
}

ISR(TIMER2_COMP_vect) {
  static char counter = 0;
  int i = 0;
  if (counter++ == ANSWER_SEND_TIMER2_WAITS) {
    if (!command_parsed) {
      panic("No answer");
    }
    if (operation_mode == BASE) {
      do {
        uart_putc(answer[i]);
      } while (answer[++i] != PROTOCOL_STOPCHAR);
    }
    else {
      do {
        suart_putc(answer[i]);
      } while (answer[++i] != PROTOCOL_STOPCHAR);
    }
    command_parsed = 0;
    counter = 0;
    TIMSK &= ~_BV(OCIE2);
  }
}

void cmd_ok(void) {
    answer[2] = 'o';
    answer[3] = 'k';
    answer[4] = PROTOCOL_STOPCHAR;
}

void check_command(char* buffer) {
  if (buffer[0] == '0' && (buffer[1] == '4' || buffer[1] == '0')) {
      execute_command(buffer + 2);
  }
  else {
    TIMSK &= ~_BV(OCIE2);
  }
  if (operation_mode == BASE) {
    send_radio_command(buffer);
  }
}

void execute_command(char* buffer) {
  if (!strncmp("PING", buffer, 4)) {
    cmd_ok();
     command_parsed= 1;
  }
  else if (!strncmp("TIME", buffer, 4)) {
    cmd_ok();
    if(rtc_settime(buffer + 4))
      command_parsed = 1;
  }
  else if (!strncmp("INT", buffer, 3)) {
    cmd_ok();
    //if (set_measure_time(buffer + 3))
    set_measure_time(buffer + 3);
      command_parsed = 1;
  }
  else if (!strncmp("VALUE", buffer, 5)) {
    //Answer NNHHMMSSvalue
    rtc_gettime((char*)answer+2);
    answer[8] = PROTOCOL_STOPCHAR;
    command_parsed = 1;
  }
  else {
    TIMSK &= ~_BV(OCIE2);
  }
}

void send_radio_command(char* buffer) {

}

void set_measure_time(char* buffer) {
  //TODO fix and move to separate file
}

void timer2_init(void) {
  //ClearTimerOnCompare, Systemclock/1024 (3600Hz)
  TCCR2 = _BV(WGM21) | _BV(CS22) | _BV(CS21) | _BV(CS20);
  TCNT2 = 0;
  OCR2 = 180;
}

int main(void) {
   /* set portC as output and all leds off */
  DDRC = 0xFF;
  PORTC = 0xff;

  operation_mode = NODE;
  char buffer[32];
  int i;
  const char str[] = "foo";
  const char *p = str;
  uart_init();
  suart_init();
  rtc_init();

  //Timer2 används för att hålla våran radio-timeslot.
  timer2_init();
 
  sei();

  while(*p)
  {
#if 1
      uart_putc(*p);
      p++;
#else
      suart_putc(*p);
      p++;
#endif
  }

 /* Setup LCD and display greeting */
  LCDinit(); 
  LCDclr();
  LCDGotoXY(0,0);
  //LCDcursorOFF(); 
  char hello[] = "System online";
  LCDstring(hello, 13);
  LCDGotoXY(0,1);

  while(1)
  {
    if(uart.flags.stopchar_received) {
      uart.flags.stopchar_received = 0;
      TCNT2 = 0;
      TIMSK |= _BV(OCIE2);
      uint8_t i = 0;
      while ((buffer[i] = uart_getc()) != PROTOCOL_STOPCHAR) {
        buffer[i] = toupper(buffer[i]);
        i++;
      }
      check_command(buffer);
    }
    if(suart.flags.stopchar_received) {
      if (operation_mode == NODE) {
        suart.flags.stopchar_received = 0;
        TCNT2 = 0;
        TIMSK |= _BV(OCIE2);
        uint8_t i = 0;
        while ((buffer[i] = suart_getc()) != PROTOCOL_STOPCHAR) {
          buffer[i] = toupper(buffer[i]);
          i++;
        }
        check_command(buffer);
      }
      else {
        suart.flags.stopchar_received = 0;
        while ((i = suart_getc()) != PROTOCOL_STOPCHAR) {
          uart_putc(i);
        }
      }
    }
    //TODO här skulle man kunna sleepa tills man får interrupt som säger att vi har data

  }
}

