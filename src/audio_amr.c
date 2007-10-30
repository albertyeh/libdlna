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

#define AMR_KNOWN_EXTENSIONS "amr,3gp,mp4"

/* Profile for audio media class content */
static dlna_profile_t amr = {
  .id = "AMR_3GPP",
  .mime = MIME_AUDIO_MPEG_4,
  .label = LABEL_AUDIO_MONO
};

/* Profile for audio media class content */
static dlna_profile_t three_gpp = {
  .id = "AMR_3GPP",
  .mime = MIME_AUDIO_3GP,
  .label = LABEL_AUDIO_MONO
};

/* Profile for audio media class content */
static dlna_profile_t amr_wbplus = {
  .id = "AMR_WBplus",
  .mime = MIME_AUDIO_3GP,
  .label = LABEL_AUDIO_2CH
};

int
audio_is_valid_amr (AVCodecContext *ac)
{
  if (!ac)
    return 0;

  if (ac->codec_id != CODEC_ID_AMR_NB)
    return 0;
  
  /* only mono is supported */
  if (ac->channels != 1)
    return 0;

  /* only supports 8 kHz sampling rate */
  if (ac->sample_rate != 8000)
    return 0;

  /* valid CBR bitrates: 4.75, 5.15, 5.9, 6.7, 7.4, 7.95, 10.2, 12.2 Kbps */
  switch (ac->bit_rate)
  {
  case 4750:
  case 5150:
  case 5900:
  case 6700:
  case 7400:
  case 7950:
  case 10200:
  case 12200:
    return 1;
  default:
    break;
  }

  return 0;
}

int
audio_is_valid_amr_wb (AVCodecContext *ac)
{
  if (!ac)
    return 0;

  if (ac->codec_id != CODEC_ID_AMR_WB)
    return 0;
  
  /* valid sampling rates: 8, 16, 24, 32 and 48 kHz */
  if (ac->sample_rate != 8000 &&
      ac->sample_rate != 16000 &&
      ac->sample_rate != 24000 &&
      ac->sample_rate != 32000 &&
      ac->sample_rate != 48000)
    return 0;

  /* supported bit rates: 5.2 Kbps - 48 Kbps */
  if (ac->bit_rate < 5200 || ac->bit_rate > 48000)
    return 0;

  /* only mono and stereo are supported */
  if (ac->channels > 2)
    return 0;

  return 1;
}

static dlna_profile_t *
probe_amr (AVFormatContext *ctx)
{
  AVCodecContext *codec;
  
  /* check for valid file extension */
  if (!match_file_extension (ctx->filename, AMR_KNOWN_EXTENSIONS))
    return NULL;

  codec = audio_profile_get_codec (ctx);
  if (!codec)
    return NULL;
  
  /* check for AMR NB/WB audio codec */
  if (audio_is_valid_amr (codec))
  {
    if (!strcasecmp (get_file_extension (ctx->filename), "3gp"))
      return set_profile (&three_gpp);
    return set_profile (&amr);
  }

  if (audio_is_valid_amr_wb (codec))
    return set_profile (&amr_wbplus);
  
  return NULL;
}

dlna_registered_profile_t dlna_profile_audio_amr = {
  .id = DLNA_PROFILE_AUDIO_AMR,
  .probe = probe_amr,
  .next = NULL
};
