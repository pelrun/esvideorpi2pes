/*
 * Copyright (C) 2014       James Churchill, pelrun@gmail.com
 * Copyright (C) 2004-2013  Lorenzo Pallara, l.pallara@avalpa.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "nal.h"

#define PES_HEADER_SIZE 14
#define SYSTEM_CLOCK 90000.0
#define PTS_MAX 8589934592LL

// Access unit delimiter NALU
const uint8_t aud_nal[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xf0 };

void stamp_ts (uint32_t ts, uint8_t* buffer)
{
	if (buffer)
  {
		buffer[0] = ((ts >> 29) & 0x0F) | 0x01;
		buffer[1] = (ts >> 22) & 0xFF;
		buffer[2] = ((ts >> 14) & 0xFF ) | 0x01;
		buffer[3] = (ts >> 7) & 0xFF;
		buffer[4] = ((ts << 1) & 0xFF ) | 0x01;
	}
}

int main(int argc, char *argv[])
{
	int fps;
	FILE* file_es;

	uint32_t pts;
	uint32_t pts_step = 0;
	uint32_t frame_counter = 0;

	uint8_t stream_id = 224;
	uint8_t pes_header[PES_HEADER_SIZE];
  uint8_t es_data[64*1024];
  uint32_t es_data_length;

	/* Open pes file */
	if (argc > 1) {
		file_es = fopen(argv[1], "rb");
	}

  if (argc > 2) {
    // rpi doesn't include timing info in stream, need to supply it explicitly
		fps = atol(argv[2]);
    pts_step = SYSTEM_CLOCK / fps;
	}

	if (argc > 3) {
		stream_id = atoi(argv[3]);
	}

	if (argc < 3){
		fprintf(stderr, "Usage: 'esvideorpi2pes video.h264 fps [stream_id] '\n");
		fprintf(stderr, "Video stream id default is 224\n");
		return 2;
	}

	if (file_es == 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}

  /* Init. default pack header */
	pes_header[0] = 0x00;
	pes_header[1] = 0x00;
	pes_header[2] = 0x01;
	pes_header[3] = stream_id;
	pes_header[4] = 0x00;
	pes_header[5] = 0x00;
	pes_header[6] = 0x80; /* no scrambling, no priority, no alignment defined (FIXME: set this?), no copyright, no copy */
	pes_header[7] = 0x80; /* pts only, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
	pes_header[8] = 0x05; /* pts size*/

  while (read_next_access_unit(file_es, es_data, &es_data_length))
  {
    pts += pts_step; // start the pts one frame delay in

    stamp_ts(pts % PTS_MAX, pes_header + 9);
    pes_header[9] &= 0x0F;
    pes_header[9] |= 0x30;

    fwrite(pes_header, 1, PES_HEADER_SIZE, stdout);

    fwrite(es_data, 1, es_data_length, stdout);

    frame_counter++;
  }

  return 0;
}
