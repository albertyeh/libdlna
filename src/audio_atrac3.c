/*
 * libdlna: reference DLNA standards implementation.
 * Copyright (C) 2007 Benjamin Zores <ben@geexbox.org>
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

#include <stdlib.h>
#include <string.h>

#include "dlna.h"
#include "profiles.h"

#define ATRAC3_KNOWN_EXTENSIONS "acm,wav"
#define ATRAC3_MIME_TYPE "audio/x-sony-oma"
#define ATRAC3_LABEL "2-ch multi"

/* Profile for audio media class content */
static dlna_profile_t atrac3 = {
  .id = "ATRAC3plus",
  .mime = ATRAC3_MIME_TYPE,
  .label = ATRAC3_LABEL
};

static dlna_profile_t *
probe_atrac3 (AVFormatContext *ctx)
{
  AVStream *stream;
  AVCodecContext *codec;
  
  /* check for valid file extension */
  if (!match_file_extension (ctx->filename, ATRAC3_KNOWN_EXTENSIONS))
    return NULL;

  /* should only have 1 stream */
  if (ctx->nb_streams > 1)
    return NULL;

  stream = ctx->streams[0];
  codec = stream->codec;
  
  /* which obviously should be an audio one */
  if (codec->codec_type != CODEC_TYPE_AUDIO)
    return NULL;

  /* check for ATRAC3 codec */
  if (codec->codec_id != CODEC_ID_ATRAC3)
    return NULL;

  return set_profile (&atrac3);
}

dlna_registered_profile_t dlna_profile_audio_atrac3 = {
  .id = DLNA_PROFILE_AUDIO_ATRAC3,
  .probe = probe_atrac3,
  .next = NULL
};