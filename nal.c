/*
 * Copyright (C) 2014       James Churchill, pelrun@gmail.com
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

#define BUFFER_RELOAD_SIZE 16

#define CIRCBUF_SIZE   256     /*** Must be a power of 2 ***/

#if CIRCBUF_SIZE < 2
#error CIRCBUF_SIZE is too small.  It must be larger than 1.
#elif ((CIRCBUF_SIZE & (CIRCBUF_SIZE-1)) != 0)
#error CIRCBUF_SIZE must be a power of 2.
#endif

typedef struct {
  uint16_t in; // Next In Index
  uint16_t out; // Next Out Index
  size_t reload; // number of bytes to reload
  FILE *fp; // file to reload buffer from
  uint8_t *buf; // Buffer
} Circular_Buffer_t;

static Circular_Buffer_t nal_input_buf = { 0, 0, 0, NULL, NULL };

void buffer_init(Circular_Buffer_t *buf, FILE *fp, size_t reload_len)
{
  if (buf->buf == NULL)
  {
    buf->in = 0;
    buf->out = 0;
    buf->fp = fp;
    buf->reload = reload_len;
    buf->buf = malloc(CIRCBUF_SIZE);
  }
}

uint32_t buffer_length(Circular_Buffer_t *p)
{
  return ((uint32_t)(p->in - p->out) & (CIRCBUF_SIZE-1));
}

int16_t buffer_write_byte(Circular_Buffer_t *p, uint8_t c)
{
  if (buffer_length(p) >= CIRCBUF_SIZE)
  {
    return -1;
  }

  p->buf[p->in++ & (CIRCBUF_SIZE - 1)] = c; // Add data to the transmit buffer.

  return 0;
}

uint16_t buffer_reload(Circular_Buffer_t *p, size_t len)
{
  uint8_t i = 0;

  if (p->fp!=NULL)
  {
    for (i=0; i<len; i++)
    {
      int16_t ch = fgetc(p->fp);
      if (ch<0)
      {
        break;
      }
      buffer_write_byte(p, ch);
    }
  }

  return i;
}

uint8_t buffer_peek(Circular_Buffer_t *p, uint8_t index)
{
  return p->buf[(p->out+index) & (CIRCBUF_SIZE - 1)];
}

int16_t buffer_read_byte(Circular_Buffer_t *p)
{
  int16_t ch;

  if (buffer_length(p) == 0)
  {
    if (buffer_reload(p, BUFFER_RELOAD_SIZE) == 0)
    {
      // nothing to reload
      return -1;
    }
  }

  ch = buffer_peek(p,0);
  p->out++;

  return ch;
}

int8_t buffer_detect_start_code(Circular_Buffer_t *p)
{
  uint32_t buf_len = buffer_length(p);
  if (buf_len < 4)
  {
    if (buf_len + buffer_reload(p, BUFFER_RELOAD_SIZE) < 4)
    {
      // nothing to reload
      return -1;
    }
  }

  if (buffer_peek(p,0) == 0x00 && buffer_peek(p,1) == 0x00 && (buffer_peek(p,2) == 0x01 || (buffer_peek(p,2) == 0x00 && buffer_peek(p,3) == 0x01)))
  {
    return 1;
  }

  return 0;
}

size_t read_next_nalu(FILE *fp, uint8_t *out_buf, size_t *out_len)
{
  int8_t result;

  // idempotent, won't delete existing buffer
  buffer_init(&nal_input_buf, fp, BUFFER_RELOAD_SIZE);

  *out_len = 0;

  while ((result = buffer_detect_start_code(&nal_input_buf)) == 0)
  {
    // discard byte preceding the start code
    if (buffer_read_byte(&nal_input_buf) < 0)
    {
      return 0;
    }
  }

  if (result >= 0)
  {
    // now stream out nalu until next start code or error
    do
    {
      int16_t ch = buffer_read_byte(&nal_input_buf);
      if (ch < 0)
      {
        // end of stream
        break;
      }

      *out_buf++ = ch;
      *out_len = *out_len+1;
    }
    while (*out_len < 3 || buffer_detect_start_code(&nal_input_buf) == 0);
  }

  return *out_len;
}

// batch multiple nalus until we get a video frame
size_t read_next_access_unit(FILE *fp, uint8_t *out_buf, size_t *out_len)
{
  size_t length = 0;

  *out_len = 0;

  while (read_next_nalu(fp, out_buf, &length))
  {
    uint8_t nal_type;

    if (out_buf[2] == 0x01)
    {
      nal_type = out_buf[3] & 0xF;
    }
    else
    {
      nal_type = out_buf[4] & 0xF;
    }

    *out_len += length;

    if (nal_type == 1 || nal_type == 5)
    {
      // this is a video frame
      break;
    }

    out_buf += length;
  }

  return *out_len;
}
