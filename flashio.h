/*
	Copyright (c) Gareth Scott 2013, 2014

	flashio.h 

	Write arbitrary length data to flash
	
*/

#ifndef _FLASHIO_H_
#define _FLASHIO_H_

enum flashRegion {FLASH_PROGRAM, FLASH_SYMBOL_TABLE};

#define FLASH_PAGE_SIZE                     (0x400) // 1024 bytes
#define FLASH_PAGE_SIZE_INT                 (FLASH_PAGE_SIZE / 4)
// Note: 0xfc00 - 0xf000 = 0xc00 bytes, 768 32-bit words for program. See: MAX_PROGRAM_ENTRY (currently 200)
//       0xffff - 0xfc00 = 0x3ff bytes, 256 32-bit words for symbol table. See: MAX_SYMBOL_TABLE_ENTRY (currently 60)
#define FLASH_START_ADDRESS_PROGRAM         (0xF000)    // Current code is located in flash at ~0x7200
#define FLASH_START_ADDRESS_SYMBOL_TABLE    (0xFC00)
#define FLASH_ERASED_VALUE                  (0xFFFFFFFF)

class flashio {
public:
    flashio();
    ~flashio() {}
      
    bool saveData(uint32_t* pDataSource, uint32_t flashDestination, unsigned long intCount);
    void retrieveData(uint32_t* pDataDestination, uint32_t flashSource, unsigned long intCount);
  
private:    
    bool _eraseBlockRequired(uint32_t flashAddress, uint32_t* remainingDataPointer, unsigned long byteCount);
    bool _flashErased(uint32_t flashAddress);
};

#endif /* _FLASHIO_H_ */