/*
	Copyright (c) Gareth Scott 2011, 2012

	flashio.cpp 

*/

#include <stdbool.h>
#include <stdint.h>
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "driverlib/flash.h"   // Using "flash.h" pulls in file from FreeRTOS
#include "flashio.h"

flashio::flashio() {
}

void flashio::saveData(uint32_t* pData, unsigned long count) {
    uint32_t* remainingDataPointer = pData;
    unsigned long remainingCount = count;
    unsigned int pageCount = 0;
    while (remainingCount > 0) {
        if (remainingCount >= FLASH_PAGE_SIZE) {
            FlashProgram(remainingDataPointer, FLASH_START_ADDRESS + (pageCount * FLASH_PAGE_SIZE), FLASH_PAGE_SIZE);
            remainingDataPointer += FLASH_PAGE_SIZE;
            remainingCount -= FLASH_PAGE_SIZE;
        } else {
            FlashProgram(remainingDataPointer, FLASH_START_ADDRESS + (pageCount * FLASH_PAGE_SIZE), remainingCount);
            break;            
        }
        ++pageCount;
    }
}
