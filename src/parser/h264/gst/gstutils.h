/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2002 Thomas Vander Stichele <thomas@apestaart.org>
 *
 * gstutils.h: Header for various utility functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef __GST_UTILS_H__
#define __GST_UTILS_H__

#include <glib.h>

G_BEGIN_DECLS
        

// Define PUT and GET functions for unaligned memory
#define _GST_GET(__data, __idx, __size, __shift) \
    (((guint##__size) (((const guint8 *) (__data))[__idx])) << (__shift))

//
// GST_READ_UINT64_LE:
// @data: memory location
//
// Read a 64 bit unsigned integer value in little endian format from the memory buffer.
//
#define _GST_READ_UINT64_BE(data)	(_GST_GET (data, 0, 64, 56) | \
					 _GST_GET (data, 1, 64, 48) | \
					 _GST_GET (data, 2, 64, 40) | \
					 _GST_GET (data, 3, 64, 32) | \
					 _GST_GET (data, 4, 64, 24) | \
					 _GST_GET (data, 5, 64, 16) | \
					 _GST_GET (data, 6, 64,  8) | \
					 _GST_GET (data, 7, 64,  0))
#define _GST_READ_UINT64_LE(data)	(_GST_GET (data, 7, 64, 56) | \
					 _GST_GET (data, 6, 64, 48) | \
					 _GST_GET (data, 5, 64, 40) | \
					 _GST_GET (data, 4, 64, 32) | \
					 _GST_GET (data, 3, 64, 24) | \
					 _GST_GET (data, 2, 64, 16) | \
					 _GST_GET (data, 1, 64,  8) | \
					 _GST_GET (data, 0, 64,  0))

#define GST_READ_UINT64_BE(data) __gst_slow_read64_be((const guint8 *)(data))
#define GST_READ_UINT64_LE(data) __gst_slow_read64_le((const guint8 *)(data))

static inline guint64 __gst_slow_read64_be (const guint8 * data) {
  return _GST_READ_UINT64_BE (data);
}
static inline guint64 __gst_slow_read64_le (const guint8 * data) {
  return _GST_READ_UINT64_LE (data);
}

//
// GST_READ_UINT32_BE:
// @data: memory location
//
// Read a 32 bit unsigned integer value in big endian format from the memory buffer.
//
//
// GST_READ_UINT32_LE:
// @data: memory location
//
// Read a 32 bit unsigned integer value in little endian format from the memory buffer.
//

#define _GST_READ_UINT32_BE(data)	(_GST_GET (data, 0, 32, 24) | \
					 _GST_GET (data, 1, 32, 16) | \
					 _GST_GET (data, 2, 32,  8) | \
					 _GST_GET (data, 3, 32,  0))
#define _GST_READ_UINT32_LE(data)	(_GST_GET (data, 3, 32, 24) | \
					 _GST_GET (data, 2, 32, 16) | \
					 _GST_GET (data, 1, 32,  8) | \
					 _GST_GET (data, 0, 32,  0))

#define GST_READ_UINT32_BE(data) __gst_slow_read32_be((const guint8 *)(data))
#define GST_READ_UINT32_LE(data) __gst_slow_read32_le((const guint8 *)(data))

static inline guint32 __gst_slow_read32_be (const guint8 * data) {
  return _GST_READ_UINT32_BE (data);
}
static inline guint32 __gst_slow_read32_le (const guint8 * data) {
  return _GST_READ_UINT32_LE (data);
}

//
// GST_READ_UINT24_BE:
// @data: memory location
//
// Read a 24 bit unsigned integer value in big endian format from the memory buffer.
//
#define _GST_READ_UINT24_BE(data)       (_GST_GET (data, 0, 32, 16) | \
                                         _GST_GET (data, 1, 32,  8) | \
                                         _GST_GET (data, 2, 32,  0))

#define GST_READ_UINT24_BE(data) __gst_slow_read24_be((const guint8 *)(data))

static inline guint32 __gst_slow_read24_be (const guint8 * data) {
  return _GST_READ_UINT24_BE (data);
}

//
// GST_READ_UINT24_LE:
// @data: memory location
//
// Read a 24 bit unsigned integer value in little endian format from the memory buffer.
//
#define _GST_READ_UINT24_LE(data)       (_GST_GET (data, 2, 32, 16) | \
                                         _GST_GET (data, 1, 32,  8) | \
                                         _GST_GET (data, 0, 32,  0))

#define GST_READ_UINT24_LE(data) __gst_slow_read24_le((const guint8 *)(data))

static inline guint32 __gst_slow_read24_le (const guint8 * data) {
  return _GST_READ_UINT24_LE (data);
}

//
// GST_READ_UINT16_BE:
// @data: memory location
//
// Read a 16 bit unsigned integer value in big endian format from the memory buffer.
//
// GST_READ_UINT16_LE:
// @data: memory location
//
// Read a 16 bit unsigned integer value in little endian format from the memory buffer.
//
#define _GST_READ_UINT16_BE(data)	(_GST_GET (data, 0, 16,  8) | _GST_GET (data, 1, 16,  0))
#define _GST_READ_UINT16_LE(data)	(_GST_GET (data, 1, 16,  8) | _GST_GET (data, 0, 16,  0))

#define GST_READ_UINT16_BE(data) __gst_slow_read16_be((const guint8 *)(data))
#define GST_READ_UINT16_LE(data) __gst_slow_read16_le((const guint8 *)(data))

static inline guint16 __gst_slow_read16_be (const guint8 * data) {
  return _GST_READ_UINT16_BE (data);
}

static inline guint16 __gst_slow_read16_le (const guint8 * data) {
  return _GST_READ_UINT16_LE (data);
}

//
// GST_READ_UINT8:
// @data: memory location
//
// Read an 8 bit unsigned integer value from the memory buffer.
//
#define GST_READ_UINT8(data)            (_GST_GET (data, 0,  8,  0))

//
// GST_READ_FLOAT_LE:
// @data: memory location
//
// Read a 32 bit float value in little endian format from the memory buffer.
//
// Returns: The floating point value read from @data
//
static inline gfloat
GST_READ_FLOAT_LE(const guint8 *data)
{
  union
  {
    guint32 i;
    gfloat f;
  } u;

  u.i = GST_READ_UINT32_LE (data);
  return u.f;
}

//
// GST_READ_FLOAT_BE:
// @data: memory location
//
// Read a 32 bit float value in big endian format from the memory buffer.
//
// Returns: The floating point value read from @data
//
static inline gfloat
GST_READ_FLOAT_BE(const guint8 *data)
{
  union
  {
    guint32 i;
    gfloat f;
  } u;

  u.i = GST_READ_UINT32_BE (data);
  return u.f;
}

//
// GST_READ_DOUBLE_LE:
// @data: memory location
//
// Read a 64 bit double value in little endian format from the memory buffer.
//
// Returns: The double-precision floating point value read from @data
//
static inline gdouble
GST_READ_DOUBLE_LE(const guint8 *data)
{
  union
  {
    guint64 i;
    gdouble d;
  } u;

  u.i = GST_READ_UINT64_LE (data);
  return u.d;
}

//
// GST_READ_DOUBLE_BE:
// @data: memory location
//
// Read a 64 bit double value in big endian format from the memory buffer.
//
// Returns: The double-precision floating point value read from @data
//
static inline gdouble
GST_READ_DOUBLE_BE(const guint8 *data)
{
  union
  {
    guint64 i;
    gdouble d;
  } u;

  u.i = GST_READ_UINT64_BE (data);
  return u.d;
}

G_END_DECLS

#endif /* __GST_UTILS_H__ */
