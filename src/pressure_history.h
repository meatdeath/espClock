#ifndef __PRESSURE_HISTORY_H__
#define __PRESSURE_HISTORY_H__


#pragma pack(push,1)
typedef struct pressure_history_st {
    unsigned long time;
    float pressure;
} pressure_history_t;
#pragma pack(pop)

#define COLLECT_PRESSURE_HISTORY_PERIOD (60*60)

#define PRESSURE_HISTORY_SIZE       48
#define EEPROM_HISTORY_ADDR         (EEPROM_CONFIG_CLOCK_ADDR+EEPROM_CONFIG_CLOCK_SIZE)
#define EEPROM_HISTORY_ITEM_SIZE    sizeof(pressure_history_t)

extern String json_PressureHistory;
extern String html_PressureHistory;
extern String pressureLabelsStr;
extern String pressureValuesStr;

void generate_pressure_history(void);
void eeprom_add_history_item( unsigned long time, float pressure );
void eeprom_restore_pressure_history(unsigned long time);
void clear_log(void);
void pressureHistory_printDumpFromEeprom();
void config_printDumpFromEeprom();

#endif // __PRESSURE_HISTORY_H__