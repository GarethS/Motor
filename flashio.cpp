/*
	Copyright (c) Gareth Scott 2013

	flashio.cpp 

*/

#include <stdbool.h>
#include <stdint.h>
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/flash.h"   // Using "flash.h" pulls file from FreeRTOS. Not what we want.
#include "flashio.h"

flashio::flashio() {
}

// Move data from sRAM to Flash
bool flashio::saveData(uint32_t* pDataSource, uint32_t flashDestination, unsigned long intCount/* 4 bytes per integer on 32-bit ARM systems */) {
    uint32_t* remainingDataPointer = pDataSource;
    unsigned long remainingByteCount = intCount * sizeof(uint32_t);
    unsigned int pageCount = 0;
    while (remainingByteCount > 0) {
        uint32_t flashAddress = flashDestination + (pageCount * FLASH_PAGE_SIZE);
        // An optimization here would be if the flash memory contains the same contents as 'remainingDataPointer'
        //  then we don't need to call ROM_FlashProgram() below.
        if (_eraseBlockRequired(flashAddress, remainingDataPointer, remainingByteCount)) {
            ROM_FlashErase(flashAddress);
            if (!_flashErased(flashAddress)) {
                return false;
            }
        }
        if (remainingByteCount >= FLASH_PAGE_SIZE) {
            ROM_FlashProgram(remainingDataPointer, flashAddress, FLASH_PAGE_SIZE);
            remainingDataPointer += FLASH_PAGE_SIZE_INT;    // Note that remainingDataPointer is a pointer to 'unsigned int' so it increments by 4, not 1
            //remainingDataPointer += FLASH_PAGE_SIZE / 4;
            remainingByteCount -= FLASH_PAGE_SIZE;
        } else {
            ROM_FlashProgram(remainingDataPointer, flashAddress, remainingByteCount);
            break;            
        }
        ++pageCount;
    }
    return true;
}

// Move data from Flash into sRAM
void flashio::retrieveData(uint32_t* pDataDestination, uint32_t flashSource, unsigned long intCount) {
    // similar to memcpy()
#if 0
    uint8_t* pFlashAddress = (uint8_t*)flashSource;
    uint8_t* pDestination = (uint8_t*)pDataDestination;
    uint32_t counter = intCount * 4;
    for (uint32_t i = 0; i < counter; ++i) {
        pDestination[i] = pFlashAddress[i];
    }
#else
    uint32_t* pFlashAddress = (uint32_t*)flashSource;
    for (uint32_t i = 0; i < intCount; ++i) {
        pDataDestination[i] = pFlashAddress[i];
    }
#endif
}

// Return false if flash block is all 0xff (i.e. erased) or data to be programmed matches data in flash block; true otherwise
bool flashio::_eraseBlockRequired(uint32_t flashAddress, uint32_t* remainingDataPointer, unsigned long byteCount) {
    if (byteCount >= FLASH_PAGE_SIZE) {
        byteCount = FLASH_PAGE_SIZE;
    }
    uint32_t intCount = byteCount / sizeof(uint32_t);
    uint32_t* pFlashAddress = (uint32_t*)flashAddress;
    for (uint32_t i = 0; i < intCount; ++i) {
        // Note, since pFlashAddress points to a 32-bit integer, we're incrementing
        //  over 4 bytes each time through this loop
        uint32_t flashValue = pFlashAddress[i];
        if (!(flashValue == FLASH_ERASED_VALUE || flashValue == remainingDataPointer[i])) {
            return true;
        }
    }
    return false;
}

// Called after ROM_FlashErase() to check that flash erased correctly. If bits are 
//  starting to wear out this should detect it.
// Return false if flash bad; true otherwise
bool flashio::_flashErased(uint32_t flashAddress) {
    uint32_t intCount = FLASH_PAGE_SIZE / sizeof(uint32_t);
    uint32_t* pFlashAddress = (uint32_t*)flashAddress;
    for (uint32_t i = 0; i < intCount; ++i) {
        uint32_t flashValue = pFlashAddress[i];
        if (flashValue != FLASH_ERASED_VALUE) {
            return false;
        }
    }
    return true;
}