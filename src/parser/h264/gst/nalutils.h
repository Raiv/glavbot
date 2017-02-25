/* Gstreamer
 * Copyright (C) <2011> Intel Corporation
 * Copyright (C) <2011> Collabora Ltd.
 * Copyright (C) <2011> Thibault Saunier <thibault.saunier@collabora.com>
 *
 * Some bits C-c,C-v'ed and s/4/3 from h264parse and videoparsers/h264parse.c:
 *    Copyright (C) <2010> Mark Nauwelaerts <mark.nauwelaerts@collabora.co.uk>
 *    Copyright (C) <2010> Collabora Multimedia
 *    Copyright (C) <2010> Nokia Corporation
 *
 *    (C) 2005 Michal Benes <michal.benes@itonis.tv>
 *    (C) 2008 Wim Taymans <wim.taymans@gmail.com>
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

/**
 * Common code for NAL parsing from h264 and h265 parsers.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "h264/gst/gstbytereader.h"
#include "h264/gst/gstbitreader.h"
#include <string.h>

guint ceil_log2 (guint32 v);

typedef struct
{
  const guint8 *data;
  guint size;

  guint n_epb;                  /* Number of emulation prevention bytes */
  guint byte;                   /* Byte position */
  guint bits_in_cache;          /* bitpos in the cache of next bit */
  guint8 first_byte;
  guint64 cache;                /* cached bytes */
} NalReader;

G_GNUC_INTERNAL
void nal_reader_init (NalReader * nr, const guint8 * data, guint size);

static inline gboolean
nal_reader_read (NalReader * nr, guint nbits)
{
  if (G_UNLIKELY (nr->byte * 8 + (nbits - nr->bits_in_cache) > nr->size * 8)) {
//    GST_DEBUG ("Can not read %u bits, bits in cache %u, Byte * 8 %u, size in "
//        "bits %u", nbits, nr->bits_in_cache, nr->byte * 8, nr->size * 8);
    return FALSE;
  }

  while (nr->bits_in_cache < nbits) {
    guint8 byte;
    gboolean check_three_byte;

    check_three_byte = TRUE;
  next_byte:
    if (G_UNLIKELY (nr->byte >= nr->size))
      return FALSE;

    byte = nr->data[nr->byte++];

    /* check if the byte is a emulation_prevention_three_byte */
    if (check_three_byte && byte == 0x03 && nr->first_byte == 0x00 &&
        ((nr->cache & 0xff) == 0)) {
      /* next byte goes unconditionally to the cache, even if it's 0x03 */
      check_three_byte = FALSE;
      nr->n_epb++;
      goto next_byte;
    }
    nr->cache = (nr->cache << 8) | nr->first_byte;
    nr->first_byte = byte;
    nr->bits_in_cache += 8;
  }

  return TRUE;
}

static inline gboolean
nal_reader_skip (NalReader * nr, guint nbits)
{
  g_assert (nbits <= 8 * sizeof (nr->cache));

  if (G_UNLIKELY (!nal_reader_read (nr, nbits)))
    return FALSE;

  nr->bits_in_cache -= nbits;

  return TRUE;
}

G_GNUC_INTERNAL
gboolean nal_reader_skip_long (NalReader * nr, guint nbits);

static inline guint
nal_reader_get_pos (const NalReader * nr)
{
  return nr->byte * 8 - nr->bits_in_cache;
}

static inline guint
nal_reader_get_remaining (const NalReader * nr)
{
  return (nr->size - nr->byte) * 8 + nr->bits_in_cache;
}

static inline guint
nal_reader_get_epb_count (const NalReader * nr)
{
  return nr->n_epb;
}

G_GNUC_INTERNAL
gboolean nal_reader_is_byte_aligned (NalReader * nr);

G_GNUC_INTERNAL
gboolean nal_reader_has_more_data (NalReader * nr);

#define NAL_READER_READ_BITS_H(bits) \
G_GNUC_INTERNAL \
gboolean nal_reader_get_bits_uint##bits (NalReader *nr, guint##bits *val, guint nbits)

NAL_READER_READ_BITS_H (8);
NAL_READER_READ_BITS_H (16);
NAL_READER_READ_BITS_H (32);

#define NAL_READER_PEEK_BITS_H(bits) \
G_GNUC_INTERNAL \
gboolean nal_reader_peek_bits_uint##bits (const NalReader *nr, guint##bits *val, guint nbits)

NAL_READER_PEEK_BITS_H (8);

G_GNUC_INTERNAL
gboolean nal_reader_get_ue (NalReader * nr, guint32 * val);

static inline gboolean
nal_reader_get_se (NalReader * nr, gint32 * val)
{
  guint32 value;

  if (G_UNLIKELY (!nal_reader_get_ue (nr, &value)))
    return FALSE;

  if (value % 2)
    *val = (value / 2) + 1;
  else
    *val = -(value / 2);

  return TRUE;
}

#define CHECK_ALLOWED_MAX(val, max) { \
  if (val > max) { \
    GST_WARNING ("value greater than max. value: %d, max %d", \
                     val, max); \
    goto error; \
  } \
}

#define CHECK_ALLOWED(val, min, max) { \
  if (val < min || val > max) { \
    GST_WARNING ("value not in allowed range. value: %d, range %d-%d", \
                     val, min, max); \
    goto error; \
  } \
}

#define READ_UINT8(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint8 (nr, &val, nbits)) { \
    GST_WARNING ("failed to read uint8, nbits: %d", nbits); \
    goto error; \
  } \
}

#define READ_UINT16(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint16 (nr, &val, nbits)) { \
  GST_WARNING ("failed to read uint16, nbits: %d", nbits); \
    goto error; \
  } \
}

#define READ_UINT32(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint32 (nr, &val, nbits)) { \
  GST_WARNING ("failed to read uint32, nbits: %d", nbits); \
    goto error; \
  } \
}

#define READ_UINT64(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint64 (nr, &val, nbits)) { \
    GST_WARNING ("failed to read uint32, nbits: %d", nbits); \
    goto error; \
  } \
}

#define READ_UE(nr, val) { \
  if (!nal_reader_get_ue (nr, &val)) { \
    GST_WARNING ("failed to read UE"); \
    goto error; \
  } \
}

#define READ_UE_ALLOWED(nr, val, min, max) { \
  guint32 tmp; \
  READ_UE (nr, tmp); \
  CHECK_ALLOWED (tmp, min, max); \
  val = tmp; \
}

#define READ_UE_MAX(nr, val, max) { \
  guint32 tmp; \
  READ_UE (nr, tmp); \
  CHECK_ALLOWED_MAX (tmp, max); \
  val = tmp; \
}

#define READ_SE(nr, val) { \
  if (!nal_reader_get_se (nr, &val)) { \
    GST_WARNING ("failed to read SE"); \
    goto error; \
  } \
}

#define READ_SE_ALLOWED(nr, val, min, max) { \
  gint32 tmp; \
  READ_SE (nr, tmp); \
  CHECK_ALLOWED (tmp, min, max); \
  val = tmp; \
}

static inline gint
scan_for_start_codes (const guint8 * data, guint size)
{
  GstByteReader br;
  gst_byte_reader_init (&br, data, size);
  /* NALU not empty, so we can at least expect 1 (even 2) bytes following sc */
  return gst_byte_reader_masked_scan_uint32 (&br, 0xffffff00, 0x00000100, 0, size);
}