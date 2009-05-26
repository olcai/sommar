#include "config.h"
#include <avr/eeprom.h>

volatile config_t config;

config_t EEMEM config_eeprom =
{
  1,                                    /* interval */
  "04",                                 /* group */
  { 
    CONFIG_MODE_BASE                    /* flags.mode */
  }
};

void config_load(void)
{
  eeprom_read_block((void*)&config, &config_eeprom, sizeof(config_t));
}

void config_save(void)
{
  eeprom_write_block((void*)&config, &config_eeprom, sizeof(config_t));
}

