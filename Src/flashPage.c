#include "flashPage.h"
#include "string.h"
#include "stdio.h"



static uint32_t getPage(uint32_t adress){
	for(int indx = 0; indx < 128 ; indx++){
		if( (adress <(0x08000000 + (FLASH_PAGE_SIZE*(indx+1))) )&& (adress >=(0x08000000 + (FLASH_PAGE_SIZE*(indx)))) ){
			return (0x08000000 + FLASH_PAGE_SIZE*indx);
		}
	}
	return 0;
}

uint32_t flashWrite(uint32_t startPageAdress,uint32_t *data,uint16_t numberOfWorlds){

	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t pageError;
	uint16_t counter = 0;

	HAL_FLASH_Unlock();

	uint32_t startPage = getPage(startPageAdress);
	uint32_t endPageAdress = startPageAdress + (numberOfWorlds*4);
	uint32_t endPage  = getPage(endPageAdress);

	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = startPage;
	EraseInitStruct.NbPages = ((endPage - startPage)/FLASH_PAGE_SIZE)+1;

	if(HAL_FLASHEx_Erase(&EraseInitStruct,&pageError) != HAL_OK){
		return HAL_FLASH_GetError ();
	}

	/*Flash'a datalar 32 bit olarak yazýldý*/
	while(counter < numberOfWorlds){
		if( HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,startPageAdress,data[counter]) == HAL_OK){
			startPageAdress +=4;
			counter ++;
		}

		else{
			return HAL_FLASH_GetError();
		}
	}

	HAL_FLASH_Lock();
	return 0;

}

void flashRead(uint32_t flashStartAdress,uint32_t *data,uint16_t numberOfWorlds){

	while(1){

		*data = *(__IO uint32_t *)flashStartAdress;
		flashStartAdress +=4;
		data ++;
		if(!(numberOfWorlds--))
		break;
	}

}














