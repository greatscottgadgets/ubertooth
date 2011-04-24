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

#ifndef __IAP_H_
#define __IAP_H_

#include <stdint.h>

class IAP {
public:
    enum ReturnCode {
        CMD_SUCCESS                             = 0,
        INVALID_COMMAND                         = 1,
        SRC_ADDR_ERROR                          = 2,
        DST_ADDR_ERROR                          = 3,
        SRC_ADDR_NOT_MAPPED                     = 4,
        DST_ADDR_NOT_MAPPED                     = 5,
        COUNT_ERROR                             = 6,
        INVALID_SECTOR                          = 7,
        SECTOR_NOT_BLANK                        = 8, 
        SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION = 9,
        COMPARE_ERROR                           = 10,
        BUSY                                    = 11,
    };

    ReturnCode prepare_sectors_for_write_operation(const uint32_t start_sector_number,
                                                   const uint32_t end_sector_number);
    
    ReturnCode copy_ram_to_flash(const uint32_t destination_flash_address,
                                 const uint32_t source_ram_address,
                                 const uint32_t number_of_bytes_to_write,
                                 const uint32_t cpu_clock_frequency_khz);

    ReturnCode erase_sectors(const uint32_t start_sector_number,
                             const uint32_t end_sector_number,
                             const uint32_t cpu_clock_frequency_khz);
    
    ReturnCode blank_check_sectors(const uint32_t start_sector_number,
                                   const uint32_t end_sector_number);
    
    ReturnCode compare(const uint32_t destination_address,
                       const uint32_t source_address,
                       const uint32_t number_of_bytes_to_compare);

private:
    typedef void(*EntryPoint)(uint32_t[], uint32_t[]);
    
    static const EntryPoint entry_point;
};

#endif//__IAP_H_
