/*
 * libdlna: reference DLNA standards implementation.
 * Copyright (C) 2007-2008 Benjamin Zores <ben@geexbox.org>
 * Copyright (C) 2014-2016 Marc Chalain <marc.chalain@gmail.com>
 *
 * This file is part of libdlna.
 *
 * libdlna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libdlna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libdlna; if not, write to the Free Software
 * Foundation, Inc, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libavformat/avformat.h>

#include "containers.h"
#include "profiles.h"

#define MPEG_PACK_HEADER 0xba
#define MPEG_TS_SYNC_CODE 0x47
#define MPEG_TS_PACKET_LENGTH 188
#define MPEG_TS_PACKET_LENGTH_DLNA 192 /* prepends 4 bytes to TS packet */

static const struct {
  const char *name;
  const dlna_container_type_t type;
} avf_format_mapping[] = {
  { "image2",                     CT_IMAGE         },
  { "asf",                        CT_ASF           },
  { "amr",                        CT_AMR           },
  { "aac",                        CT_AAC           },
  { "ac3",                        CT_AC3           },
  { "mp3",                        CT_MP3           },
  { "wav",                        CT_WAV           },
  { "mov,mp4,m4a,3gp,3g2,mj2",    CT_MOV           },
  { "mpeg",                       CT_FF_MPEG       },
  { "mpegts",                     CT_FF_MPEG_TS    },
  { NULL, 0 }
};

static dlna_container_type_t
mpeg_find_container_type (const char *filename)
{
  unsigned char buffer[2*MPEG_TS_PACKET_LENGTH_DLNA+1];
  int fd, i;

  /* read file header */
  fd = open (filename, O_RDONLY);
  read (fd, buffer, 2 * MPEG_TS_PACKET_LENGTH_DLNA); 
  close (fd);

  /* check for MPEG-TS container */
  for (i = 0; i < MPEG_TS_PACKET_LENGTH; i++)
  {
    if (buffer[i] == MPEG_TS_SYNC_CODE)
    {
      if (buffer[i + MPEG_TS_PACKET_LENGTH] == MPEG_TS_SYNC_CODE)
        return CT_MPEG_TRANSPORT_STREAM;
    }
  }

  /* check for DLNA specific MPEG-TS container */
  for (i = 0; i < MPEG_TS_PACKET_LENGTH_DLNA; i++)
  {
    if (buffer[i] == MPEG_TS_SYNC_CODE)
    {
      if (buffer[i + MPEG_TS_PACKET_LENGTH_DLNA] == MPEG_TS_SYNC_CODE)
      {
        if (buffer[i] == 0x00 && buffer [i+1] == 0x00 &&
            buffer [i+2] == 0x00 && buffer [i+3] == 0x00)
          return CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS; /* empty timestamp */
        else
          return CT_MPEG_TRANSPORT_STREAM_DLNA; /* valid timestamp */
      }
    }
  }

  /* check for MPEG-PS and MPEG-(P)ES container */
  if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x01)
    return (buffer[3] == MPEG_PACK_HEADER) ?
      CT_MPEG_PROGRAM_STREAM : CT_MPEG_ELEMENTARY_STREAM;

  return CT_UNKNOWN;
}

static dlna_container_type_t
mov_find_container_type (const char *filename)
{
  if (!filename)
    return CT_UNKNOWN;

  if (!strcasecmp (get_file_extension (filename), "3gp") ||
      !strcasecmp (get_file_extension (filename), "3gpp") ||
      !strcasecmp (get_file_extension (filename), "3g2"))
    return CT_3GP;

  return CT_MP4;
}

dlna_container_type_t
stream_get_container (AVFormatContext *ctx)
{
  int i;

  for (i = 0; avf_format_mapping[i].name; i++)
    if (!strcmp (ctx->iformat->name, avf_format_mapping[i].name))
    {
      switch (avf_format_mapping[i].type)
      {
      case CT_FF_MPEG:
      case CT_FF_MPEG_TS:
        return mpeg_find_container_type (ctx->filename);
      case CT_MOV:
        return mov_find_container_type (ctx->filename);
      default:
        return avf_format_mapping[i].type;
      }
    }

  /* unknown or invalid container */
  return CT_UNKNOWN;
}
