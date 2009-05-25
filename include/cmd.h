#ifndef __cmd_h__
#define __cmd_h__

#include <stdint.h>

#include "bool.h"

typedef void(cmd_callback_t)(const uint8_t* arg);
bool_t cmd_parse(const uint8_t* data, uint8_t size);

/* for now (?) these are in avr.c */
void cmd_PING(const uint8_t* arg);
void cmd_VALUE(const uint8_t* arg);
void cmd_TIME(const uint8_t* arg);
void cmd_INT(const uint8_t* arg);
#endif
