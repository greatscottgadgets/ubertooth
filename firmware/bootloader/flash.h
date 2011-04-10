#ifndef __FLASH_H_
#define __FLASH_H_

#include "iap.h"

class Flash {
public:
    bool write(const uint32_t flash_address, uint32_t source_address, uint32_t length) {
        IAP::ReturnCode result = IAP::CMD_SUCCESS;
        
        const uint32_t sector = sector_number(flash_address);
        if( at_sector_boundary(flash_address) ) {
            if( result == IAP::CMD_SUCCESS) {
                result = iap.prepare_sectors_for_write_operation(sector, sector);
            }
            if( result == IAP::CMD_SUCCESS) {
                result = iap.erase_sectors(sector, sector, cclk_khz);
            }
        }
        
        if( result == IAP::CMD_SUCCESS) {
            result = iap.prepare_sectors_for_write_operation(sector, sector);
        }
        if( result == IAP::CMD_SUCCESS ) {
            iap.copy_ram_to_flash(flash_address, source_address, length, cclk_khz);
        }
        
        return (result == IAP::CMD_SUCCESS);
    }
    
    bool valid_address(const uint32_t address) const {
        return (address < 0x80000);
    }
    
private:
    IAP iap;
    
    static const uint32_t cclk_khz = 100000;
    
    bool at_sector_boundary(const uint32_t address) const {
        return (address & (sector_size(address) - 1)) == 0;
    }
    
    uint32_t sector_size(const uint32_t address) const {
        if( address < 0x10000 ) {
            return 4096;
        } else {
            return 32768;
        }
    }
                              
    uint32_t sector_number(const uint32_t address) const {
        if( address < 0x10000 ) {
            return address >> 12;
        } else {
            return 14 + (address >> 15);
        }
    }
};

#endif//__FLASH_H_

