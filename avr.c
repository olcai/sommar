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
volatile uint8_t sendreadyflag = 0;

void check_command(char* buffer);
void execute_command(char* buffer);
void send_radio_command(char* buffer);
void set_time(char* buffer);
void set_measure_time(char* buffer);

#if 0
// Defines for Software UART
#define STATE_IDLE 0
#define STATE_TRANSMIT 1
#define STATE_TRANSMIT_STOP_BIT 2
#define STATE_RECEIVE 3
#define STATE_RECEIVE_STOP_BIT 5
#define STATE_DATA_PENDING 4

#define SUART_RX_PORT PIND
#define SUART_TX_PORT PORTD
#define SUART_RX_DDR  DDRD
#define SUART_TX_DDR  DDRD
#define SUART_RX_PIN  2
#define SUART_TX_PIN  4

/* Skala ned klockan med lämplig multipel
   Formel: N = CPUFreq / (Baudrate * multipel)
   N är värdet som ska laddas in i Compare-registret
*/
#define SUART_BIT_TIME_LENGTH 48 /* (uint8_t) (F_CPU / (1200 * 64)) */

typedef uint8_t word_t;

volatile int8_t led = 0;
volatile uint8_t state = STATE_IDLE;
volatile word_t tx_data;
volatile word_t tx_bit_count;
volatile word_t rx_data;
volatile word_t rx_data2[8];
volatile word_t rx_bit_count;

volatile int flag_new_data =0;;

ISR(INT0_vect)
{
  flag_new_data=0;
  //PORTC = ~PORTC;
  state = STATE_RECEIVE;

  /* no more rx-interrupt, we read the bits with timer0 */
  GICR &= ~_BV(INT0);

  /* wait 1.5 bit length */
  OCR0 = SUART_BIT_TIME_LENGTH * 1.5;
  /* Set timer to 0 */
  TCNT0 = 0;
  
  rx_data = rx_bit_count = 0;

  /* Clear timer interrupt bit and activate it */
  TIFR |= _BV(OCF0);
  TIMSK |= _BV(OCIE0);
}

ISR(TIMER0_COMP_vect)
{
  switch (state)
  {
    case (STATE_TRANSMIT):
    {
      // Kolla om vi sänt alla bitar
      if (tx_bit_count > 7)
          {
            state = STATE_TRANSMIT_STOP_BIT;
            // Skriv en etta som stoppbit
            SUART_TX_PORT |= _BV(SUART_TX_PIN);
          }
        /* Kolla om biten vi ska sända är en etta genom att köra
           bitwise AND på tx_data som innehåller tecknet vi sänder.
        */
        else if (tx_data & 1)
          // Skriv etta
          SUART_TX_PORT |= _BV(SUART_TX_PIN);
        else
          // Skriv nolla
          SUART_TX_PORT &= ~_BV(SUART_TX_PIN);
        // Räkna upp
        tx_bit_count++;
        // Skifta tx_data ett steg
        tx_data>>=1;
        break;
      }
    case (STATE_TRANSMIT_STOP_BIT):
      {
        state = STATE_IDLE;
        GIFR = _BV(INTF0);
        GICR |= _BV(INT0); /* rx-interrupt, yes please */

        // Avaktivera timerinterrupt
        TIMSK &= ~_BV(1);
        break;
      }
    case STATE_RECEIVE:
      /* now we are in the middle of the bit */
      OCR0 = SUART_BIT_TIME_LENGTH;
        
      /* sample the incoming bit */
      if(bit_is_set(SUART_RX_PORT, SUART_RX_PIN))
        rx_data |= _BV(rx_bit_count);
      else
        rx_data &= ~_BV(rx_bit_count);

      if(++rx_bit_count > 7)
        state = STATE_RECEIVE_STOP_BIT;
      break;

    case STATE_RECEIVE_STOP_BIT:
      /* Check for good stop bit */
      if(bit_is_set(SUART_RX_PORT, SUART_RX_PIN))
        flag_new_data = 1;
      /* Reset interrupts */
      GIFR = _BV(INTF0);
      GICR |= _BV(INT0);
      TIMSK &= ~_BV(1); 
      state = STATE_IDLE;
      break;
  // KERNEL PANIC
  default:
    PORTC=0;
    while(1)
      {
        PORTC=~PORTC;
        _delay_ms(500);
      }
  }
  // Återställ timer0
  TCNT0 = 0;
}

void initSoftwareUart()
{
  // Sätt RX-pin
  SUART_RX_DDR &= ~_BV(SUART_RX_PIN);
  // Sätt TX-pin
  SUART_TX_DDR |= _BV(SUART_TX_PIN);
  // Sätt TX-pin till 1
  SUART_TX_PORT |= _BV(SUART_TX_PIN);
  // Sätt timer0 till 0
  TCNT0 = 0;
  // Skala med multipel 64
  TCCR0 = _BV(CS01) | _BV(CS00);

  // aktivera rx-avbrott (falling edge på INT0)
  //  MCUCR |= _BV(ISC01);
  //  MCUCR &=  ~_BV(ISC00);
  // iavbroot på låg
  MCUCR &= ~_BV(ISC01) & ~_BV(ISC00);
  // Aktivera avbrottet
  GICR  |= _BV(INT0);  

  // Sätt statusflagga
  state = STATE_IDLE;
}

word_t suart_putc(const word_t c) {

  /* just half-duplex, so no rx-interrupt */
  GICR &= ~_BV(INT0); 

  if (state!=STATE_IDLE) {
    /* rc-interrupt is only enabled when we ar idle so it corect */
    return 0;
  }

  // Byt tillstånd
  state = STATE_TRANSMIT;
  tx_data = c;
  tx_bit_count = 0;
  // Sätt jämförelseregister
  OCR0 = 48;//SUART_TIMER;
  // Återställ timer0
  TCNT0 = 0;
  // Sätter sändlinan till 0 - startbit
  SUART_TX_PORT &= ~_BV(SUART_TX_PIN);
  //
  TIFR |= _BV(OCF0);
  // Aktivera timerinterrupt
  TIMSK |= _BV(OCIE0);
  return !0;
}
#endif 
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
    if (!sendreadyflag) {
      panic("No answer!");
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
    sendreadyflag = 0;
    counter = 0;
    TIMSK &= ~_BV(OCIE2);
  }
}

void check_command(char* buffer) {
  if (buffer[0] == '0' && (buffer[1] == '4' || buffer[1] == '0')) {
      execute_command(buffer + 2);
  }
  if (operation_mode == BASE) {
    send_radio_command(buffer);
  }
}

void execute_command(char* buffer) {
  PORTC--;
  if (!strncmp("PING", buffer, 4)) {
    answer[2] = 'o';
    answer[3] = 'k';
    answer[4] = PROTOCOL_STOPCHAR;
    sendreadyflag = 1;
  }
  if (!strncmp("TIME", buffer, 4)) {
    answer[2] = 'o';
    answer[3] = 'k';
    answer[4] = PROTOCOL_STOPCHAR;
    sendreadyflag = 1;
    set_time(buffer + 4);
  }
  if (!strncmp("INT", buffer, 3)) {
    answer[2] = 'o';
    answer[3] = 'k';
    answer[4] = PROTOCOL_STOPCHAR;
    sendreadyflag = 1;
    set_measure_time(buffer + 3);
  }
  if (!strncmp("VALUE", buffer, 5)) {
    //Answer NNHHMMSSvalue
    answer[2] = real_time[0];
    answer[3] = real_time[1];
    answer[4] = real_time[2];
    answer[5] = real_time[3];
    answer[6] = real_time[4];
    answer[7] = real_time[5];
    answer[8] = PROTOCOL_STOPCHAR;
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
   /* set portD as output and all leds off */
  DDRC = 0xFF;
  PORTC = 0xff;

  operation_mode = BASE;
  char buffer[32];
  int i;
  const char str[] = "foo";
  const char *p = str;
  uart_init();
  suart_init();
  init_rtc();

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
        uart.flags.stopchar_received = 0;
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

