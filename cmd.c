#include <string.h>
#include <avr/pgmspace.h>

#include "cmd.h"
#include "config.h"

/*
 * cmd_t is a mapping between a function's properties and it's
 * pointers and arguments.
 */
typedef struct
{
  PGM_P string;
  cmd_callback_t* f;
  const uint8_t cmd_len;
  const uint8_t arg_len;
} cmd_t;

/*
 * these defines only work when name is a constant value.
 * CMD_TEXT is the string we want the command to be associated with.
 * CMD_BYNAME gives the pointer to the command name.
 * CMD_DEFINE connects name with a string and a function pointer.
 * CMD_NUM is the number of defined commands.
 */
#define CMD_TEXT(name) static char const cmd_str_ ## name [] PROGMEM
#define CMD_BYNAME(name) (cmd_str_ ## name)
#define CMD_DEFINE(name, arglen) { CMD_BYNAME(name), cmd_##name, sizeof(CMD_BYNAME(name))-1, arglen }
#define CMD_NUM (sizeof(cmds)/sizeof(cmd_t))

CMD_TEXT(PING)  = "ping";
CMD_TEXT(TIME)  = "time";
CMD_TEXT(INT)   = "int";
CMD_TEXT(VALUE) = "value";

/* Define commands and argument length */
cmd_t const PROGMEM cmds[] =
{
  CMD_DEFINE(PING,  0),
  CMD_DEFINE(TIME,  6),
  CMD_DEFINE(INT,   6),
  CMD_DEFINE(VALUE, 0)
};

/*
 * Parses the string data points to, checks if it is any of the
 * defined commands, and calls the apropriate function.
 */
bool_t cmd_parse(const uint8_t* data, uint8_t size)
{
  uint8_t i;
  for(i=0; i < CMD_NUM; i++)
  {
    uint8_t cmd_len = pgm_read_byte(&cmds[i].cmd_len);
    uint8_t arg_len = pgm_read_byte(&cmds[i].arg_len);
    /*
     * Makes str point to the position in the string where the
     * command should be if it is valid
     */
    const uint8_t* str = data + size - cmd_len - arg_len - CONFIG_GRP_LEN;
    
#if 1
    /* can data even contain the command (and argument)?  */
    if(str < data);
    {
      /* check if it is broadcast or out group number and if it is a valid command */
      if((strncmp((const char*)str, "00", CONFIG_GRP_LEN) == 0 || 
          strncmp((const char*)str, (const char*)config.group, CONFIG_GRP_LEN) == 0))
      {
         if(strncmp_P((const char*)str + CONFIG_GRP_LEN, (PGM_P)pgm_read_word(&cmds[i].string), cmd_len) == 0)
#endif
#if 0
    /* can data even contain the command (and argument)?  */
    if((str < data) 
      /* and check that it starts with either witd broadcast id or
       * our group number */
       && (strncmp((const char*)str, "00", CONFIG_GRP_LEN) == 0 || 
           strncmp((const char*)str, (const char*)config.group, CONFIG_GRP_LEN) == 0)
       /* and is the command after if it is, check if it is the correct command */
       && strncmp_P((const char*)str + CONFIG_GRP_LEN, (PGM_P)pgm_read_word(&cmds[i].string), cmd_len) == 0)
#endif
    {
      /* Call the function with the pointer associated with the command */
      ((cmd_callback_t*)pgm_read_word(&cmds[i].f))(data+size-arg_len);
      return TRUE;
    }
    }
    }
  }
  return FALSE;
}
