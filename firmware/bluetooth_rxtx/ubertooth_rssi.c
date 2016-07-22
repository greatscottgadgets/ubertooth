/*
 * Copyright 2015 Hannes Ellinger
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

#include "ubertooth_rssi.h"

#include <string.h>

#define RINGBUFFER_SIZE 40

size_t pointer = 0;
size_t size = 0;
int8_t values[RINGBUFFER_SIZE] = {0};

int8_t max = -120;
int8_t min = 0;

void rssi_reset(void)
{
	memset(values, 0, size);
	pointer = 0;
	size = 0;

	max = -120;
	min = 0;
}

void rssi_add(int8_t v)
{
	max = (v > max) ? v : max;
	min = (v < min) ? v : min;

	values[pointer] = v;
	pointer = (pointer + 1) % RINGBUFFER_SIZE;

	size = size < RINGBUFFER_SIZE ? size + 1 : RINGBUFFER_SIZE;
}

int8_t rssi_get_avg()
{
	int32_t sum = 0;

	for (size_t i = 0; i < size; i++)
	{
		sum += values[i];
	}

	uint8_t avg = (int8_t)(sum / (int)size);

	return avg;
}

int8_t rssi_get_signal()
{
	int32_t sum = 0;
	size_t num = 0;
	int8_t avg = ((int16_t)min + (int16_t)max) / 2;

	for (size_t i = 0; i < size; i++)
	{
		if (values[i] > avg)
		{
			sum += values[i];
			num++;
		}
	}

	int8_t signal = (int8_t)(sum / (int)num);

	return signal;
}

int8_t rssi_get_noise()
{
	int32_t sum = 0;
	size_t num = 0;
	int8_t avg = ((int16_t)min + (int16_t)max) / 2;

	for (size_t i = 0; i < size; i++)
	{
		if (values[i] <= avg)
		{
			sum += values[i];
			num++;
		}
	}

	int8_t noise = (int8_t)(sum / (int)num);

	return noise;
}

uint8_t rssi_get_cnt()
{
	return (uint8_t)size;
}

int8_t rssi_get_min()
{
	return min;
}

int8_t rssi_get_max()
{
	return max;
}
