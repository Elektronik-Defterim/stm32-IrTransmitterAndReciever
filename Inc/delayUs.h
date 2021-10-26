#ifndef DELAYUS_H
#define DELAYUS_H

#ifdef __cplusplus
extern "C" {
#endif


#include "stm32f1xx_hal.h"

__STATIC_INLINE void delayUs(__IO uint32_t micros){
	 micros *= (SystemCoreClock / 1000000) / 9;
  while (micros--) ;
}

#ifdef __cplusplus
}
#endif

#endif
