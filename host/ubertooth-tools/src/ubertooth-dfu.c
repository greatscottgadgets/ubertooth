/* -*- c -*- */
/*
 * Copyright 2015 Dominic Spill
 * 
 * This file is part of Project Ubertooth
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
 * along with libbtbb; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>  // sleep
#include "ubertooth.h"
#include "dfu.h"

/*
 * DFU suffix / signature / CRC
 */

/* CRC32 implementation for DFU suffix */
uint32_t crc32(uint8_t *data, uint32_t data_len) {
	uint32_t crc = 0xffffffff;
	uint32_t i;
	for(i=0; i<data_len; i++)
		crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xff];
	return crc;
}

int check_suffix(FILE* signedfile, DFU_suffix* suffix) {
	uint8_t *data;
	uint32_t crc, data_length;
	
	printf("Checking firmware signature\n");
	fseek(signedfile, 0, SEEK_END);
	data_length = ftell(signedfile) - 4; // Ignore 4 byte CRC
	fseek(signedfile, -16, SEEK_END); // Start of SFU suffix
	fread(suffix, 1, 16, signedfile);
	
	if(suffix->bLength != 16) {
		fprintf(stderr, "Unknown DFU suffix length: %d\n", suffix->bLength);
		return 1;
	}
	
	// We only know about dfu version 1.0/1.1
	// This needs to be smarter to support other versions if/when they exist
	if((suffix->bcdDFU != 0x0100) && (suffix->bcdDFU != 0x0101)) {
		fprintf(stderr, "Unknown DFU version: %04x\n", suffix->bcdDFU);
		return 1;
	}
	
	// Suffix bytes are reversed
	if(!((suffix->ucDfuSig[0]==0x55) &&
		 (suffix->ucDfuSig[1]==0x46) &&
		 (suffix->ucDfuSig[2]==0x44))) {
		fprintf(stderr, "DFU Signature mismatch: not a DFU file\n");
		return 1;
	}
	
	fseek(signedfile, 0, SEEK_SET);
	data = malloc(data_length);
	if(data == NULL) {
		fprintf(stderr, "Cannot allocate buffer for CRC check\n");
		return 1;
	}
	
	data_length = fread(data, 1, data_length, signedfile);
	crc = crc32(data, data_length);
	free(data);
	if(crc != suffix->dwCRC) {
		fprintf(stderr, "CRC mismatch: calculated: 0x%x, found: 0x%x\n", crc, suffix->dwCRC);
		return 1;
	}
	return 0;
}

/* Add a DFU suffix to infile and write to outfile
 * suffix must already contain VID/PID
 */
int sign(FILE* infile, FILE* outfile, uint16_t idVendor, uint16_t idProduct) {
	DFU_suffix* suffix;
	uint32_t data_length, buffer_length;
	uint8_t* buffer;
	
	fseek(infile, 0, SEEK_END);
	data_length = ftell(infile);
	buffer_length = data_length + sizeof(DFU_suffix); // Add suffix
	buffer = malloc(buffer_length);
	if(buffer == NULL) {
		fprintf(stderr, "Cannot allocate buffer to calculate CRC\n");
		return 1;
	}
	
	fseek(infile, 0, SEEK_SET);
	data_length = fread(buffer, 1, data_length, infile);
	
	suffix = (DFU_suffix *) (buffer + data_length);
	suffix->idVendor    = idVendor;
	suffix->idProduct   = idProduct;
	suffix->bcdDevice   = 0;
	suffix->bcdDFU      = 0x0100;
	suffix->ucDfuSig[0] = 0x55;
	suffix->ucDfuSig[1] = 0x46;
	suffix->ucDfuSig[2] = 0x44;
	suffix->bLength     = 16;
	suffix->dwCRC = crc32(buffer, data_length + 12);
	
	fwrite(buffer, 1, buffer_length, outfile);
	free(buffer);
	return 0;
}

/*
 * USB helper function - find devices
 */
static struct libusb_device_handle* find_ubertooth_dfu_device() {
	struct libusb_context *ctx = NULL;
	struct libusb_device **usb_list = NULL;
	struct libusb_device_handle *devh = NULL;
	struct libusb_device_descriptor desc;
	int usb_devs, i, r, ret;
	
	r = libusb_init(NULL);
	
	usb_devs = libusb_get_device_list(ctx, &usb_list);
	for(i = 0 ; i < usb_devs ; ++i) {
		r = libusb_get_device_descriptor(usb_list[i], &desc);
		if(r < 0)
			fprintf(stderr, "couldn't get usb descriptor for dev #%d!\n", i);
		if ((desc.idVendor == TC13_VENDORID && desc.idProduct == TC13_PRODUCTID) ||
			(desc.idVendor == U1_DFU_VENDORID && desc.idProduct == U1_DFU_PRODUCTID))
		{
			// FOUND AN UBERTOOTH
			ret = libusb_open(usb_list[i], &devh);
			if (ret)
				show_libusb_error(ret);
			else
				break;
		}
	}
	return devh;
}

void stop_device(struct libusb_device_handle *devh)
{
	if (devh != NULL)
		libusb_release_interface(devh, 0);
	libusb_close(devh);
	libusb_exit(NULL);
}

/*
 * DFU functions
 */
#define DFU_IN LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE
#define DFU_OUT LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE

int dfu_get_state(libusb_device_handle* devh) {
	int rv;
	uint8_t state;
	rv = libusb_control_transfer(devh, DFU_IN, REQ_GETSTATE, 0, 0, &state, 1, 1000);
	if (rv < 0) {
		if (rv == LIBUSB_ERROR_PIPE)
			fprintf(stderr, "control message unsupported\n");
		else
			show_libusb_error(rv);
		return rv;
	}
	return state;
}

int dfu_clear_status(libusb_device_handle* devh) {
	int rv;
	rv = libusb_control_transfer(devh, DFU_OUT, REQ_CLRSTATUS, 0, 0, NULL, 0, 1000);
	if (rv < 0) {
		if (rv == LIBUSB_ERROR_PIPE)
			fprintf(stderr, "control message unsupported\n");
		else
			show_libusb_error(rv);
		return rv;
	}
	return 0;
}

int dfu_abort(libusb_device_handle* devh) {
	int rv;
	rv = libusb_control_transfer(devh, DFU_OUT, REQ_ABORT, 0, 0, NULL, 0, 1000);
	if (rv < 0) {
		if (rv == LIBUSB_ERROR_PIPE)
			fprintf(stderr, "control message unsupported\n");
		else
			show_libusb_error(rv);
		return rv;
	}
	return 0;
}

int enter_dfu_mode(libusb_device_handle* devh) {
	uint8_t state;
	int rv;
	while(1) {
        state = dfu_get_state(devh);
        if(state == STATE_DFU_IDLE)
            break;
		switch(state) {
			case STATE_DFU_DNLOAD_SYNC:
			case STATE_DFU_DNLOAD_IDLE:
			case STATE_DFU_MANIFEST_SYNC:
			case STATE_DFU_UPLOAD_IDLE:
			case STATE_DFU_MANIFEST:
				rv = dfu_abort(devh);
				if(rv < 0) {
					fprintf(stderr, "Unable to abort transaction from state:%d\n", state);
					return rv;
				}
				break;
			case STATE_APP_DETACH:
			case STATE_DFU_DNBUSY:
			case STATE_DFU_MANIFEST_WAIT_RESET:
				sleep(1);
				break;
			case STATE_APP_IDLE:
				// We don't support this state in the application
				//detach(devh);
				break;
			case STATE_DFU_ERROR:
				dfu_clear_status(devh);
				break;
		}
	}
	return 0;
}

int detach(libusb_device_handle* devh) {
	int state, rv;
	rv = enter_dfu_mode(devh);
	if(rv < 0) {
		fprintf(stderr, "Detach failed: could not enter DFU mode\n");
		return rv;
	}
	state = dfu_get_state(devh);
	if(state == STATE_DFU_IDLE) {
		rv = libusb_control_transfer(devh, DFU_OUT, REQ_DETACH, 0, 0, NULL, 0, 1000);
		if (rv != LIBUSB_SUCCESS) {
			if (rv == LIBUSB_ERROR_PIPE) {
				fprintf(stderr, "control message unsupported\n");
			} else
				show_libusb_error(rv);
			return rv;
		} else
			printf("Detached\n");
	} else {
		fprintf(stderr, "In unexpected state: %d", state);
		return 1;
	}
	return 0;
}

int upload(libusb_device_handle* devh, FILE* upfile) {
	int address, length, block, rv;
	uint8_t buffer[BLOCK_SIZE];
	address = BOOTLOADER_OFFSET + BOOTLOADER_SIZE;
	length = (256 * 1024) - address;
	block = address / BLOCK_SIZE;
	
    if ((address & (BLOCK_SIZE - 1)) != 0) {
		fprintf(stderr, "Upload failed: must start at block boundary\n");
		return -1;
    }
	
	rv = enter_dfu_mode(devh);
	if(rv < 0) {
		fprintf(stderr, "Upload failed: could not enter DFU mode\n");
		return rv;
	}
	
	while(length > 0) {
		rv = libusb_control_transfer(devh, DFU_IN, REQ_UPLOAD, block, 0,
									 buffer, BLOCK_SIZE, 1000);
		if (rv < 0) {
			if (rv == LIBUSB_ERROR_PIPE)
				fprintf(stderr, "control message unsupported\n");
			else {
				show_libusb_error(rv);
				return rv;
			}
		}
		fprintf(stdout, ".");
		if(rv == BLOCK_SIZE)
			fwrite(buffer, 1, BLOCK_SIZE, upfile);
		else {
			fprintf(stdout, "\n");
			fprintf(stderr, "Upload failed: did not read full block\n");
			return -1;
		}
		block++;
		length -= rv;
	}
	fprintf(stdout, "\n");
	return 0;
}

int dfu_get_status(libusb_device_handle* devh) {
	uint8_t buffer[6];
	int rv;
	rv = libusb_control_transfer(devh, DFU_IN, REQ_GETSTATUS, 0, 0, buffer, 6, 1000);
	if (rv < 0) {
		if (rv == LIBUSB_ERROR_PIPE)
			fprintf(stderr, "control message unsupported\n");
		else {
			show_libusb_error(rv);
			return rv;
		}
	}
	return 0;
}

int download(libusb_device_handle* devh, FILE* downfile) {
	int address, length, block, rv;
	uint8_t buffer[BLOCK_SIZE];
	address = BOOTLOADER_OFFSET + BOOTLOADER_SIZE;
	block = address / BLOCK_SIZE;
	fseek(downfile, 0, SEEK_SET);
	
    if ((address & (SECTOR_SIZE - 1)) != 0) {
		fprintf(stderr, "Download failed: must start at sector boundary\n");
		return -1;
    }
	
	rv = enter_dfu_mode(devh);
	if(rv < 0) {
		fprintf(stderr, "Download failed: could not enter DFU mode\n");
		return rv;
	}
	while((length=fread(buffer, 1, BLOCK_SIZE, downfile))) {
		for(;length<BLOCK_SIZE;length++)
			buffer[length] = 0xFF;
		rv = libusb_control_transfer(devh, DFU_OUT, REQ_DNLOAD, block, 0,
									 buffer, BLOCK_SIZE, 1000);
		if (rv < 0) {
			if (rv == LIBUSB_ERROR_PIPE)
				fprintf(stderr, "control message unsupported\n");
			else {
				show_libusb_error(rv);
				return rv;
			}
		}
		dfu_get_status(devh);
		fprintf(stdout, ".");
		fflush(stdout);
		block++;
	}
	fprintf(stdout, "\n");
	return 0;
}

static int get_outfile(char *infile, char **outfile) {
	char *suffix = strrchr(infile, '.');
	if (suffix != NULL && strcmp(suffix, ".bin") == 0) {
		*outfile = strdup(infile);
		if (*outfile == NULL)
			return 0;
		suffix = strrchr(*outfile, '.');
		strcpy(suffix, ".dfu");
		return 1;
	} else {
		return 0;
	}
}

static void usage()
{
	printf("ubertooth-dfu - Ubertooth firmware update tool\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-s <filename> add DFU suffix to binary firmware file\n");
	printf("\t-u <filename> upload - read firmware from device\n");
	printf("\t-d <filename> download - write DFU file to device\n");
	printf("\t-r reset Ubertooth after other operations complete\n");
	printf("\t-U <0-7> set ubertooth device to use\n");
}

#define FUNC_DOWNLOAD (1<<0)
#define FUNC_UPLOAD   (1<<1)
#define FUNC_SIGN     (1<<2)
#define FUNC_RESET    (1<<3)

int main(int argc, char **argv) {
	FILE *downfile, *upfile, *infile, *outfile;
	char *outfile_name;
	libusb_device_handle* devh = NULL;
	uint8_t functions = 0;
	int opt, ubertooth_device = -1;
	int r;
	
	while ((opt=getopt(argc,argv,"hd:u:s:rU:")) != EOF) {
		switch(opt) {
		case 'd':
			downfile = fopen(optarg, "r+b");
			if (downfile == NULL) {
				perror(optarg);
				return 1;
			}
			functions |= FUNC_DOWNLOAD;
			break;
		case 'u':
			upfile = fopen(optarg, "w+b");
			if (upfile == NULL) {
				perror(optarg);
				return 1;
			}
			functions |= FUNC_UPLOAD;
			break;
		case 's':
			infile = fopen(optarg, "r+b");
			if (infile == NULL) {
				perror(optarg);
				return 1;
			}
			r = get_outfile(optarg, &outfile_name);
			if (r == 0) {
				printf("Error: -s requires a .bin\n");
				return 1;
			}
			outfile = fopen(outfile_name, "w+b");
			if (outfile == NULL) {
				perror(optarg);
				return 1;
			}
			free(outfile_name);
			functions |= FUNC_SIGN;
			break;
		case 'r':
			functions |= FUNC_RESET;
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		default:
		case 'h':
			usage();
			return 1;
		}
	}
	
	if(functions & FUNC_SIGN) {
		sign(infile, outfile, U1_DFU_VENDORID, U1_DFU_PRODUCTID);
		fclose(infile);
		fclose(outfile);
	}
	
	if(functions & (FUNC_UPLOAD|FUNC_DOWNLOAD|FUNC_RESET)) {
		// Find Ubertooth and switch it to DFU mode
		int rv, count= 0;
		devh = find_ubertooth_dfu_device();
		if(devh == NULL) {
			devh = ubertooth_start(ubertooth_device);
			cmd_flash(devh);
			fprintf(stdout, "Switching to DFU mode...\n");
			while(((devh = find_ubertooth_dfu_device()) == NULL) && (count++) < 5)
				sleep(1);
			if(devh==NULL) {
				fprintf(stderr, "Unable to find Ubertooth\n");
				return 1;
			}
		}
		rv = libusb_claim_interface(devh, 0);
		if (rv < 0) {
			fprintf(stderr, "usb_claim_interface error %d\n", rv);
			fprintf(stderr, "Correct permissions to access Ubertooth?\n");
			stop_device(devh);
			return 1;
		}
	}
	
	if(functions & FUNC_UPLOAD) {
		int rv;
		rv = upload(devh, upfile);
		fclose(upfile);
		if(rv) {
			fprintf(stderr, "Firmware update failed\n");
			return rv;
		}
	}
	
	if(functions & FUNC_DOWNLOAD) {
		int rv;
		DFU_suffix suffix;
		rv = check_suffix(downfile, &suffix);
		if(rv) {
			fprintf(stderr, "Signature check failed, firmware will not be update\n");
			return rv;
		}
		rv = download(devh, downfile);
		fclose(downfile);
		if(rv) {
			fprintf(stderr, "Firmware update failed\n");
			return rv;
		}
	}
	
	if(functions & FUNC_RESET) {
		detach(devh);
	}
	
	if(functions & (FUNC_UPLOAD|FUNC_DOWNLOAD|FUNC_RESET)) {
		stop_device(devh);
	}
	return 0;
}
