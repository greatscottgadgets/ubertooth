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

#ifndef _DFU_H_
#define _DFU_H_

#include <stdint.h>

#include <usbstruct.h>

#include "flash.h"

/* Descriptor Types */
#define DESC_DFU_FUNCTIONAL         0x21

/* Hack to clean up the namespace pollution from lpc17.h */
#undef Status

class DFU {
public:
    static const uint32_t detach_timeout_ms = 15000;
    static const uint32_t transfer_size = 0x100;
    
    enum Attribute {
        WILL_DETACH            = (1 << 3),
        MANIFESTATION_TOLERANT = (1 << 2),
        CAN_UPLOAD             = (1 << 1),
        CAN_DNLOAD             = (1 << 0),
    };
    
    DFU(Flash& flash);
    
    bool request_handler(TSetupPacket *pSetup, uint32_t *piLen, uint8_t **ppbData);
    
    bool in_dfu_mode() const;

    bool dfu_virgin() const;

private:
    enum Status {
        OK              = 0x00,
        ERRTARGET       = 0x01,
        ERRFILE         = 0x02,
        ERRWRITE        = 0x03,
        ERRERASE        = 0x04,
        ERRCHECK_ERASED	= 0x05,
        ERRPROG         = 0x06,
        ERRVERIFY       = 0x07,
        ERRADDRESS      = 0x08,
        ERRNOTDONE      = 0x09,
        ERRFIRMWARE     = 0x0A,
        ERRVENDOR       = 0x0B,
        ERRUSBR         = 0x0C,
        ERRPOR          = 0x0D,
        ERRUNKNOWN      = 0x0E,
        ERRSTALLEDPKT   = 0x0F,
    };
    
    enum State {
        APPIDLE                 = 0x00,
        APPDETACH               = 0x01,
        DFUIDLE                 = 0x02,
        DFUDNLOAD_SYNC          = 0x03,
        DFUDNBUSY               = 0x04,
        DFUDNLOAD_IDLE          = 0x05,
        DFUMANIFEST_SYNC        = 0x06,
        DFUMANIFEST             = 0x07,
        DFUMANIFEST_WAIT_RESET  = 0x08,
        DFUUPLOAD_IDLE          = 0x09,
        DFUERROR                = 0x0A,
    };
    
    enum Request {
        DETACH     = 0,
        DNLOAD     = 1,
        UPLOAD     = 2,
        GETSTATUS  = 3,
        CLRSTATUS  = 4,
        GETSTATE   = 5,
        ABORT      = 6,
    };
    
    Flash& flash;
    Status status;
    State state;
    bool virginity;
    
    void set_state(const State new_state);
    uint8_t get_state() const;
    
    void set_status(const Status new_status);
    uint8_t get_status() const;
    
    bool error(const Status new_status);
    
    uint8_t get_status_string_id() const;
    uint32_t get_poll_timeout() const;
    
    bool request_detach(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_dnload(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_upload(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_getstatus(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_clrstatus(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_getstate(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
    bool request_abort(TSetupPacket *pSetup, uint32_t *piLen, uint8_t* ppbData);
};

#endif /* _DFU_H_ */

