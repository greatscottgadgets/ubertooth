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
