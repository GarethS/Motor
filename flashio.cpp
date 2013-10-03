/*
	Copyright (c) Gareth Scott 2011, 2012

	flashio.cpp 

*/

#include <stdbool.h>
#include <stdint.h>
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/flash.h"   // Using "flash.h" pulls in file from FreeRTOS
#include "flashio.h"

flashio::flashio() {
}

void flashio::saveData(uint32_t* pData, unsigned long intCount/* 4 bytes per integer on 32-bit system */) {
    uint32_t* remainingDataPointer = pData;
    unsigned long remainingByteCount = intCount * sizeof(uint32_t);
    unsigned int pageCount = 0;
    while (remainingByteCount > 0) {
        // TODO - Before erasing, check to see if the current flash page is identical to data to be programmed.
        //         If so, no need to program, just skip over this page.
        ROM_FlashErase(FLASH_START_ADDRESS + (pageCount * FLASH_PAGE_SIZE));
        if (remainingByteCount >= FLASH_PAGE_SIZE) {
            ROM_FlashProgram(remainingDataPointer, FLASH_START_ADDRESS + (pageCount * FLASH_PAGE_SIZE), FLASH_PAGE_SIZE);
            remainingDataPointer += FLASH_PAGE_SIZE;
            remainingByteCount -= FLASH_PAGE_SIZE;
        } else {
            ROM_FlashProgram(remainingDataPointer, FLASH_START_ADDRESS + (pageCount * FLASH_PAGE_SIZE), remainingByteCount);
            break;            
        }
        ++pageCount;
    }
}
