#include <avr/interrupt.h>

#include "suart.h"
#include "config.h"
#include "system.h"
#include "lcd_lib.h"
volatile suart_t suart;

#define STATE_IDLE 0
#define STATE_TRANSMIT 1
#define STATE_TRANSMIT_STOP_BIT 2
#define STATE_RECEIVE 3
#define STATE_RECEIVE_STOP_BIT 5
#define STATE_DATA_PENDING 4

/* 
 * The lenght of each bitlength
 * multipel == timer prescaler
 * Skala ned klockan med lämplig multipel
   Formel: N = CPUFreq / (Baudrate * multipel)
   N är värdet som ska laddas in i Compare-registret
*/
#define SUART_BIT_TIME_LENGTH 48 /* (uint8_t) (F_CPU / (1200 * 64)) */

void suart_init()
{
  /* configure RX pin as input */
  SUART_RX_DDR &= ~_BV(SUART_RX_PIN);

  /* configure TX-pin as output and set high */
  SUART_TX_DDR |= _BV(SUART_TX_PIN);
  SUART_TX_PORT |= _BV(SUART_TX_PIN);

  suart.state = STATE_IDLE;

  /* configure INT0 to go wild when the line is low */
  MCUCR &= ~_BV(ISC01) & ~_BV(ISC00);
  GICR  |= _BV(INT0);  

  /* configure timer0 to scale with /64 and activate it*/
  TCCR0 = _BV(CS01) | _BV(CS00);
  /* clear match interrupt flag */
  TIFR |= _BV(OCF0); 
  OCR0 = SUART_BIT_TIME_LENGTH;
  /* activate timer match interrupt */
  TIMSK |= _BV(OCIE0);
}

bool_t suart_putc(uint8_t byte)
{
    /* is there any space in the buffer? */
    if((suart.tx.write_offset + 1) % UART_FIFO_SIZE == suart.tx.read_offset)
      return FALSE;

    suart.tx.data[suart.tx.write_offset] = byte;
    suart.tx.write_offset = (suart.tx.write_offset + 1) % UART_FIFO_SIZE;

    /* in timer we check for data to send if we are idle */
    return TRUE;
}

uint8_t suart_getc(void)
{
    uint8_t data;

    /* is the buffer empty */
    if(suart.rx.write_offset == suart.rx.read_offset)
    {
        panic("suart: rx empty");
    }

    data = suart.rx.data[suart.rx.read_offset];
    suart.rx.read_offset = (suart.rx.read_offset + 1) % UART_FIFO_SIZE;
    return data;
}

/* Interrupt Service Routines */
/* RX-interrupt */
ISR(INT0_vect)
{
  suart.state = STATE_RECEIVE;
  suart.bit_counter = 0;
  
  /* by clearing the byte here we do not need to sample the zeros */
  suart.rx.data[suart.rx.write_offset] = 0;

  /* no more rx-interrupt, we read the bits with timer0 */
  GICR &= ~_BV(INT0);

  /* restart the timer and wait 1.5 bit length so that we sample the bits
   * in the middle */
  TCNT0 = 0;
  OCR0 = SUART_BIT_TIME_LENGTH * 1.5;
  TIFR |= _BV(OCF0);
}

ISR(TIMER0_COMP_vect)
{
  switch(suart.state)
  {
    case STATE_TRANSMIT:
      /* is it time for the stop bit? */
      if(suart.bit_counter > 7)
      {
        suart.state = STATE_TRANSMIT_STOP_BIT;
        /* put out the stopit */
        SUART_TX_PORT |= _BV(SUART_TX_PIN);
      }
      /* put the bit next in line on tx-pin */
      else if (suart.tx.data[suart.tx.read_offset] & 1)
        SUART_TX_PORT |= _BV(SUART_TX_PIN);
      else
        SUART_TX_PORT &= ~_BV(SUART_TX_PIN);

      /* advance the bit queue */
      suart.bit_counter++;
      suart.tx.data[suart.tx.read_offset] >>= 1;
      break;
    
    case STATE_RECEIVE:
      /* now we are in the middle of the bit so continue to sample here */
      OCR0 = SUART_BIT_TIME_LENGTH;
        
      /* the buffer is by default all zero so we only need to 'save'
         the ones */
      if(bit_is_set(SUART_RX_PORT, SUART_RX_PIN))
        suart.rx.data[suart.rx.write_offset] |= _BV(suart.bit_counter);

      if(++suart.bit_counter > 7)
        suart.state = STATE_RECEIVE_STOP_BIT;
      break;

    case STATE_TRANSMIT_STOP_BIT:
      suart.state = STATE_IDLE;
      suart.tx.read_offset = (suart.tx.read_offset + 1) % UART_FIFO_SIZE;

      /* reset rx-interrupt flag and activate it */
      GIFR = _BV(INTF0);
      GICR |= _BV(INT0);
      break;

    case STATE_RECEIVE_STOP_BIT:
      /* check for good stop bit (ie 1) */
      if(bit_is_set(SUART_RX_PORT, SUART_RX_PIN))
      {
        if(suart.rx.data[suart.rx.write_offset] == PROTOCOL_STOPCHAR)
          suart.stopchars++;
        suart.rx.write_offset = (suart.rx.write_offset + 1) % UART_FIFO_SIZE;
        
        /* buffer can not be empty, we have an overrun so lets  
         * throw away the oldest byte */
        if(suart.rx.write_offset == suart.rx.read_offset)
          suart.rx.read_offset = (suart.rx.read_offset + 1) % UART_FIFO_SIZE;
      }
      else
        panic("suart: rx bad stop");

      /* reset rx-interrupt flag and activate it */
      GIFR = _BV(INTF0);
      GICR |= _BV(INT0);
      suart.state = STATE_IDLE;
      //TIMSK &= ~_BV(1);
      /* no break as now we will check if there is new data to send */
      break;
    
    case STATE_IDLE:
      /* is there any data to send? */
      if(suart.tx.write_offset != suart.tx.read_offset)
      {
        /* we got data to send so disable rx-interrupt */
        GICR &= ~_BV(INT0);
        suart.state = STATE_TRANSMIT;

        /* put out the start bit */
        SUART_TX_PORT &= ~_BV(SUART_TX_PIN);
        suart.bit_counter = 0;
      }
      break;

    default:
      panic("suart: invalid state");
#if 0
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
#endif
  }
  
  /* reset timer0 counter */
  TCNT0 = 0;
}

