/*
 * Copyright 2010, 2011 Michael Ossmann
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

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

