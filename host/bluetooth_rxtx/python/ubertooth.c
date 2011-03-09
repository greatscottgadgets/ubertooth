/*
 * Copyright 2010 Michael Ossmann
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

#include <Python.h>
#include "../ubertooth.h"
#include <bluetooth_packet.h>

#define NUM_BANKS 2
#define BANK_LEN 400

char symbols[NUM_BANKS][BANK_LEN];
u8 bank    = 0;
u8 channel = 39;
u8 rx_buf1[BUFFER_SIZE];
u8 rx_buf2[BUFFER_SIZE];
u8 *empty_buf  = NULL;
u8 *full_buf   = NULL;
u8 really_full = 0;
struct libusb_transfer *rx_xfer   = NULL;
struct libusb_device_handle *devh = NULL;

static void cb_xfer(struct libusb_transfer *xfer)
{
	int r;
	u8 *tmp;

	if (xfer->status != LIBUSB_TRANSFER_COMPLETED)
    {
		libusb_free_transfer(xfer);
		rx_xfer = NULL;
	
    	return;
	}

	tmp         = full_buf;
	full_buf    = empty_buf;
	empty_buf   = tmp;
	really_full = 1;

	rx_xfer->buffer = empty_buf;

	while (1)
    {
		r = libusb_submit_transfer(rx_xfer);
		if (r >= 0)
            break;
	}
}

int rx_lap(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks)
{
    static int state = 0;

    switch (state)
    {
        case 0: goto LABEL0;
        case 1: goto LABEL1;
    }
    
    int r;
    static int i, j, k, m;
    static int xfer_blocks;
    int num_xfers;
    u32 time;
    u32 clkn;
    u8 clkn_high;
    packet pkt;
    char syms[BANK_LEN * NUM_BANKS];

    LABEL0:
    if (xfer_size > BUFFER_SIZE)
        xfer_size = BUFFER_SIZE;
        
    xfer_blocks = xfer_size / 64;
    xfer_size   = xfer_blocks * 64;
    num_xfers   = num_blocks / xfer_blocks;
    num_blocks  = num_xfers * xfer_blocks;

    empty_buf   = &rx_buf1[0];
    full_buf    = &rx_buf2[0];
    really_full = 0;
    rx_xfer     = libusb_alloc_transfer(0);
    libusb_fill_bulk_transfer(rx_xfer, devh, DATA_IN, empty_buf, xfer_size, cb_xfer, NULL, TIMEOUT);

    cmd_rx_syms(devh, num_blocks);

    r = libusb_submit_transfer(rx_xfer);
    
    if (r < 0)
        return -1;

    while (1)
    {
        while (!really_full)
        {
            r = libusb_handle_events(NULL);
            
            if (r < 0)
                return -1;
        }

        for (i = 0; i < xfer_blocks; i++)
        {
            time = full_buf[4 + 64 * i]
                    | (full_buf[5 + 64 * i] << 8)
                    | (full_buf[6 + 64 * i] << 16)
                    | (full_buf[7 + 64 * i] << 24);
        
            clkn_high = full_buf[3 + 64 * i];

            for (j = 0; j < 50; j++)
            {
                for (k = 0; k < 8; k++)
                {
                    symbols[bank][j * 8 + k] = (full_buf[j + 14 + i * 64] & 0x80) >> 7;
                    full_buf[j + 14 + i * 64] <<= 1;
                }
            }
            
            m = 0;
            
            for (j = 0; j < NUM_BANKS; j++)
                for (k = 0; k < BANK_LEN; k++)
                    syms[m++] = symbols[(j + 1 + bank) % NUM_BANKS][k];
            bank = (bank + 1) % NUM_BANKS;

            r = sniff_ac(syms, BANK_LEN);
            
            if  (r > -1)
            {
                clkn = (clkn_high << 19) | ((time + r * 10) / 6250);

                init_packet(&pkt, &syms[r], BANK_LEN * NUM_BANKS - r);

                state = 1;
                return pkt.LAP;

                LABEL1:;
            }
        }
        
        really_full = 0;
        fflush(stderr);
    }
}

typedef struct {
    PyObject_HEAD
} ubertooth_LapSniffer;

// define the LAP sniffers __iter__() method
PyObject* ubertooth_LapSniffer_iter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

// define the LAP sniffers next() method
PyObject* ubertooth_LapSniffer_iternext(PyObject *self)
{
    ubertooth_LapSniffer *p = (ubertooth_LapSniffer *)self;
    
    int lap;
    
    while (1)
    {
        lap = rx_lap(devh, 512, 0);
        
        if (lap == -1)
        {
            PyErr_SetString(PyExc_RuntimeError, "something went wrong");
            return NULL;
        }
        
        PyObject *tmp = Py_BuildValue("i", lap);
        return tmp;
    }
}

// provide Python with information about our __iter__() and next methods()
static PyTypeObject ubertooth_LapSnifferType = {
    PyObject_HEAD_INIT(NULL)
    0,
    "ubertooth._LapSniffer",
    sizeof(ubertooth_LapSniffer),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER,
    "Internal myiter iterator object.",
    0,
    0,
    0,
    0,
    ubertooth_LapSniffer_iter,
    ubertooth_LapSniffer_iternext
};

// calling this will create our iterator
static PyObject *
ubertooth_lapsniffer(PyObject *self, PyObject *args)
{
    ubertooth_LapSniffer *p;
    
    //if (!PyArg_ParseTuple(args, "l", &blah))
    //    return NULL;
    
    p = PyObject_New(ubertooth_LapSniffer, &ubertooth_LapSnifferType);
    
    if (!p)
        return NULL;
    
    if (!PyObject_Init((PyObject *)p, &ubertooth_LapSnifferType))
    {
        Py_DECREF(p);
        return NULL;
    }
    
    //p->channel = 32;
    
    return (PyObject *)p;
}

// define the methods
static PyMethodDef UbertoothMethods[] = {
    {"lap_sniffer", ubertooth_lapsniffer, METH_VARARGS, "Yields the LAP from Ubertooth"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initubertooth(void)
{
    PyObject* m;
    
    ubertooth_LapSnifferType.tp_new = PyType_GenericNew;
    
    if (PyType_Ready(&ubertooth_LapSnifferType) < 0)
        return;

    m = Py_InitModule("ubertooth", UbertoothMethods);

    Py_INCREF(&ubertooth_LapSnifferType);
    PyModule_AddObject(m, "_LapSniffer", (PyObject *)&ubertooth_LapSnifferType);
    
    // bring up the interface
    devh = ubertooth_start();
    
    if (devh == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "failed to bring up interface");
        return NULL;
    }
}
