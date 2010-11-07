/*
	LPCUSB, an USB device driver for LPC microcontrollers	
	Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "type.h"
#include "serial_fifo.h"

void fifo_init(fifo_t *fifo, U8 *buf)
{
	fifo->head = 0;
	fifo->tail = 0;
	fifo->buf = buf;
}


BOOL fifo_put(fifo_t *fifo, U8 c)
{
	int next;
	
	// check if FIFO has room
	next = (fifo->head + 1) % VCOM_FIFO_SIZE;
	if (next == fifo->tail) {
		// full
		return FALSE;
	}
	
	fifo->buf[fifo->head] = c;
	fifo->head = next;
	
	return TRUE;
}


BOOL fifo_get(fifo_t *fifo, U8 *pc)
{
	int next;
	
	// check if FIFO has data
	if (fifo->head == fifo->tail) {
		return FALSE;
	}
	
	next = (fifo->tail + 1) % VCOM_FIFO_SIZE;
	
	*pc = fifo->buf[fifo->tail];
	fifo->tail = next;

	return TRUE;
}


int fifo_avail(fifo_t *fifo)
{
	return (VCOM_FIFO_SIZE + fifo->head - fifo->tail) % VCOM_FIFO_SIZE;
}


int fifo_free(fifo_t *fifo)
{
	return (VCOM_FIFO_SIZE - 1 - fifo_avail(fifo));
}

