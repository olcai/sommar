/* AVR system */

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <ctype.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <stdio.h>

#include "cmd.h"
#include "lcd_lib.h"
#include "system.h"
#include "config.h"
#include "uart.h"
#include "suart.h"
#include "rtc.h"
#include "adc.h"

// Basstation eller inte?
#define BASE 0
#define NODE 1

// Väntetid innan svar skickas i antal timer2 compare-matchningar
#define ANSWER_SEND_TIMER2_WAITS (GROUP_NUMBER * 4)


//volatile uint8_t operation_mode;
volatile char answer[24] = {'0', '0' + GROUP_NUMBER};
volatile char answer[24];
volatile uint8_t command_parsed = 0;

void check_command(char* buffer);
void execute_command(char* buffer);
void send_radio_command(char* buffer);
signed char set_measure_time(char* buffer);

ISR(TIMER2_COMP_vect) {
  static char counter = 0;
  int i = 0;
  if (counter++ == ANSWER_SEND_TIMER2_WAITS) {
    if (!command_parsed) {
      panic("No answer");
    }
    do {
      uart_putc(answer[i]);
    } while (answer[i++] != PROTOCOL_STOPCHAR);
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

/* innehållet i dessa funktioner är hämtade ifrån execute_command() + aktivera timer */

void cmd_PING(const uint8_t* arg)
{
  cmd_ok();
  command_parsed= 1;
}

void cmd_VALUE(const uint8_t* arg)
{
  //Get current time
  rtc_gettime((uint8_t*)answer+2);
  
  //Get adc value and convert to human readable
  uint16_t value = adc.data[0];
  //Do this to avoid division
  value *= 4;
  
  //This is ugly, but avoids division, and is almost right.
  if (value > 1000) {
    value = 1000;
  }

  // Write the number to answer and remove the last digit to simulate division by ten
  uint8_t lines = snprintf((char*)answer + 8, sizeof(answer) - 8, "_%02u", value);
  answer[8 + lines] = answer[8 + lines - 1];
  answer[8 + lines - 1] = '.';
  answer[8 + lines + 1] = '%';
  answer[8 + lines + 2] = PROTOCOL_STOPCHAR;

  command_parsed = 1;
}

void cmd_TIME(const uint8_t* arg)
{
  if(rtc_settime(arg))
  {
    cmd_ok();
    command_parsed = 1;
  }
}

void cmd_INT(const uint8_t* arg)
{
  if(adc_set_interval(arg))
  {
    cmd_ok();
    command_parsed = 1;
  }
}

void timer2_init(void) {
  //ClearTimerOnCompare, Systemclock/1024 (3600Hz)
  TCCR2 = _BV(WGM21) | _BV(CS22) | _BV(CS21) | _BV(CS20);
  TCNT2 = 0;
  //Interrupt var 50:e ms
  OCR2 = 180;
}

void suart_puts(const char* str)
{
  while(*str)
  {
    while(suart_putc(*str) == FALSE);
    str++;
  }
  while(suart_putc('\n') == FALSE);
}

void lcd_init(void)
{
  /* setup LCD on portB and display greeting */
  LCDinit(); 
  LCDclr();
  LCDGotoXY(0,0);
  LCDcursorOFF(); 

  LCDstring((uint8_t*)"System online", 13);
  LCDGotoXY(0,1);
  if(config.flags.mode == CONFIG_MODE_BASE)
    LCDstring((uint8_t*)"base", 4);
  else
    LCDstring((uint8_t*)"node", 4);
}

int main(void)
{
  const char* str;

  /* initialize stuff commen for both base and node */
  config_load();
  lcd_init();
  rtc_init();
  adc_init();
  uart_init();
  
  //Timer2 används för att hålla våran radio-timeslot (kanske bara behövs när vi är nod?.
  timer2_init();

#if 1
  str = "\n00init\nSystem is now online!\n";
  while(*str)
  {
    while(suart_putc(*str) == FALSE);
    str++;
  }
#endif

  /* set portC as output and all leds off */
  DDRC = 0xFF;
  PORTC = 0xff;

  /* lets initialize modules specific for the mode */
  if(config.flags.mode == CONFIG_MODE_BASE); // TODO: temp always suart to
    suart_init();

  /* in our answer the two first byte is always the group number */
  memcpy((void*)answer, (void*)config.group, CONFIG_GRP_LEN);

  /* all is initialized, lets roll */
  sei();
  
#if 0
  /* alternate output between '00foo' and '00bar' forever */
  while(1)
  {
    str = "00foo\n";
    while(*str)
    {
      while(uart_putc(*str) == FALSE);
      str++;
    }
    _delay_ms(1000);

    str = "00bar\n";
    while(*str)
    {
      while(uart_putc(*str) == FALSE);
      str++;
    }
    _delay_ms(1000);
  }
#endif

  /* configure the mode button pin as input */
  MODE_BUTTON_DDR &= ~_BV(MODE_BUTTON_PIN);

  /* loop until the mode button pin is low */
  while(bit_is_set(MODE_BUTTON_PORT, MODE_BUTTON_PIN))
  {
    uint8_t buffer[UART_FIFO_SIZE];

    /* uart data (radio), parse it */
    if(uart.stopchars)
    {
      uart.stopchars--;

      /* copy it to our stack */
      uint8_t i = 0;
      while ((buffer[i] = uart_getc()) != PROTOCOL_STOPCHAR)
        i++;
      /* 
       * node -> parse it
       * base -> pass along to the suart (to computer)
       */
      
      if(config.flags.mode == CONFIG_MODE_BASE)
      {
        int j;
        for(j=0;j<i;j++)
          while(suart_putc(buffer[j]) == FALSE);
      }
      else {
        if (cmd_parse(buffer, i)) {
          // enable the send timer
          TCNT2 = 0;
          TIMSK |= _BV(OCIE2);
        }
      }
    }

    /*
     * suart data (from computer)
     * We only get this as base, so answer the command
     * if it's addressed to us, and send to all nodes.
     */
    if(suart.stopchars) {
      suart.stopchars--;
      uint8_t i = 0;
      while ((buffer[i] = suart_getc()) != PROTOCOL_STOPCHAR)
        i++;
      if (cmd_parse(buffer, i)) {
        i = 0;
        if (command_parsed == 1) {
          do {
            suart_putc(answer[i]);
          } while (answer[i++] != PROTOCOL_STOPCHAR);
          command_parsed = 0;
        }
      }
      //Send to radio
      i = 0;
      do {
        uart_putc(buffer[i]);
      } while (buffer[i++] != PROTOCOL_STOPCHAR);
    }
  }

  /* we are closing down, do not disturb */
  cli();

  /* this is safe because we know that the mode is just one bit */
  config.flags.mode = !config.flags.mode;
  config_save();
  rtc_save();

  /* use the watchdog to get a nice clean reset */
  wdt_enable(WDTO_15MS);
  while(1);






#if 0

  //adc_dosample();
  //uart_putc(a);

  PORTC = 0xFF;

  /* loop until the mode button pin is low */
  while(bit_is_set(MODE_BUTTON_PORT, MODE_BUTTON_PIN))
  {
#if 0
    if(uart.flags.stopchar_received) {
      uart.flags.stopchar_received = 0;
      uint8_t i = 0;
      while ((buffer[i] = uart_getc()) != PROTOCOL_STOPCHAR) {
        buffer[i] = toupper(buffer[i]);
        i++;
      }
      check_command(buffer);
    }
#endif
#if 1
    if(suart.flags.stopchar_received) {
      if (config.flags.mode == CONFIG_MODE_NODE) {
        suart.flags.stopchar_received = 0;
        while(1);
//        TCNT2 = 0;
//        TIMSK |= _BV(OCIE2);
        uint8_t i = 0;
#if 0
p="SUART[";
while(*p)
  uart_putc(*(p++));
        while ((buffer[i] = suart_getc()) != PROTOCOL_STOPCHAR) {
uart_putc(buffer[i]);
          buffer[i] = toupper(buffer[i]);
          i++;

        }
p="]\n\r";
while(*p)
  uart_putc(*(p++));
        PORTC--;
#endif
        //check_command(buffer);
      }
      else {
        suart.flags.stopchar_received = 0;
        char c;
        while ((c = suart_getc()) != PROTOCOL_STOPCHAR) {
          uart_putc(c);
        }
        uart_putc(PROTOCOL_STOPCHAR);
      }
    }
#endif
    //TODO här skulle man kunna sleepa tills man får interrupt som säger att vi har data
  }

  cli();
  rtc_save();

  /* this is safe because we know that mode is one bit */
  config.flags.mode = !config.flags.mode;
  config_save();

  /* use the watchdog to get a nice cleab reset */
  wdt_enable(WDTO_15MS);
  while(1);
#endif
}

