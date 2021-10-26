#ifndef FLASHPAGE_H_
#define FLASHPAGE_H_
#include <stm32f1xx_hal.h>

uint32_t flashWrite(uint32_t flashStartAdress,uint32_t *data,uint16_t numberOfWorlds);
void flashRead(uint32_t flashStartAdress,uint32_t *data,uint16_t numberOfWorlds);

#endif

