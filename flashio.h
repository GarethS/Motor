/*
	Copyright (c) Gareth Scott 2013

	flashio.h 

	Write arbitrary length data to flash
	
*/

#ifndef _FLASHIO_H_
#define _FLASHIO_H_

#define FLASH_PAGE_SIZE (0x400) // 1024
#define FLASH_START_ADDRESS (0x8000)    // TODO!!

class flashio {
public:
    flashio();
    ~flashio() {}
      
    void saveData(uint32_t* pData, unsigned long count);  
};

#endif /* _FLASHIO_H_ */