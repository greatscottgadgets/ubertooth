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

#include "iap.h"

IAP::ReturnCode IAP::prepare_sectors_for_write_operation(const uint32_t start_sector_number,
                                               const uint32_t end_sector_number) {
    uint32_t input[] = {
        50,
        start_sector_number,
        end_sector_number
    };
    uint32_t output[] = {
        INVALID_COMMAND
    };
    
    entry_point(input, output);
    
    return static_cast<ReturnCode>(output[0]);
}

IAP::ReturnCode IAP::copy_ram_to_flash(const uint32_t destination_flash_address,
                             const uint32_t source_ram_address,
                             const uint32_t number_of_bytes_to_write,
                             const uint32_t cpu_clock_frequency_khz) {
    uint32_t input[] = {
        51,
        destination_flash_address,
        source_ram_address,
        number_of_bytes_to_write,
        cpu_clock_frequency_khz
    };
    uint32_t output[] = {
        INVALID_COMMAND
    };
    
    entry_point(input, output);
    
    return static_cast<ReturnCode>(output[0]);
}

IAP::ReturnCode IAP::erase_sectors(const uint32_t start_sector_number,
                         const uint32_t end_sector_number,
                         const uint32_t cpu_clock_frequency_khz) {
    uint32_t input[] = {
        52,
        start_sector_number,
        end_sector_number,
        cpu_clock_frequency_khz
    };
    uint32_t output[] = {
        INVALID_COMMAND
    };
    
    entry_point(input, output);
    
    return static_cast<ReturnCode>(output[0]);
}

IAP::ReturnCode IAP::blank_check_sectors(const uint32_t start_sector_number,
                               const uint32_t end_sector_number) {
    uint32_t input[] = {
        53,
        start_sector_number,
        end_sector_number
    };
    uint32_t output[] = {
        INVALID_COMMAND,
        0,
        0
    };
    
    entry_point(input, output);
    
    return static_cast<ReturnCode>(output[0]);
}

IAP::ReturnCode IAP::compare(const uint32_t destination_address,
                   const uint32_t source_address,
                   const uint32_t number_of_bytes_to_compare) {
    uint32_t input[] = {
        56,
        destination_address,
        source_address,
        number_of_bytes_to_compare
    };
    uint32_t output[] = {
        INVALID_COMMAND,
        0
    };
    
    entry_point(input, output);
    
    return static_cast<ReturnCode>(output[0]);
}

const IAP::EntryPoint IAP::entry_point = reinterpret_cast<IAP::EntryPoint>(0x1FFF1FF1);
