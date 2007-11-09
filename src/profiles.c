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
#include "containers.h"

extern dlna_registered_profile_t dlna_profile_image_jpeg;
extern dlna_registered_profile_t dlna_profile_image_png;
extern dlna_registered_profile_t dlna_profile_audio_ac3;
extern dlna_registered_profile_t dlna_profile_audio_amr;
extern dlna_registered_profile_t dlna_profile_audio_atrac3;
extern dlna_registered_profile_t dlna_profile_audio_lpcm;
extern dlna_registered_profile_t dlna_profile_audio_mp3;
extern dlna_registered_profile_t dlna_profile_audio_mpeg4;
extern dlna_registered_profile_t dlna_profile_audio_wma;
extern dlna_registered_profile_t dlna_profile_av_mpeg1;
extern dlna_registered_profile_t dlna_profile_av_mpeg2;
extern dlna_registered_profile_t dlna_profile_av_mpeg4_part2;
extern dlna_registered_profile_t dlna_profile_av_mpeg4_part10;
extern dlna_registered_profile_t dlna_profile_av_wmv9;

static void
dlna_register_profile (dlna_t *dlna, dlna_registered_profile_t *profile)
{
  void **p;

  if (!dlna)
    return;

  if (!dlna->inited)
    dlna = dlna_init ();
  
  p = &dlna->first_profile;
  while (*p != NULL)
  {
    if (((dlna_registered_profile_t *) *p)->id == profile->id)
      return; /* already registered */
    p = (void *) &((dlna_registered_profile_t *) *p)->next;
  }
  *p = profile;
  profile->next = NULL;
}

void
dlna_register_all_media_profiles (dlna_t *dlna)
{
  if (!dlna)
    return;
  
  if (!dlna->inited)
    dlna = dlna_init ();
  
  dlna_register_profile (dlna, &dlna_profile_image_jpeg);
  dlna_register_profile (dlna, &dlna_profile_image_png);
  dlna_register_profile (dlna, &dlna_profile_audio_ac3);
  dlna_register_profile (dlna, &dlna_profile_audio_amr);
  dlna_register_profile (dlna, &dlna_profile_audio_atrac3);
  dlna_register_profile (dlna, &dlna_profile_audio_lpcm);
  dlna_register_profile (dlna, &dlna_profile_audio_mp3);
  dlna_register_profile (dlna, &dlna_profile_audio_mpeg4);
  dlna_register_profile (dlna, &dlna_profile_audio_wma);
  dlna_register_profile (dlna, &dlna_profile_av_mpeg1);
  dlna_register_profile (dlna, &dlna_profile_av_mpeg2);
  dlna_register_profile (dlna, &dlna_profile_av_mpeg4_part2);
  dlna_register_profile (dlna, &dlna_profile_av_mpeg4_part10);
  dlna_register_profile (dlna, &dlna_profile_av_wmv9);
}

void
dlna_register_media_profile (dlna_t *dlna, dlna_media_profile_t profile)
{
  if (!dlna)
    return;
  
  if (!dlna->inited)
    dlna = dlna_init ();
  
  switch (profile)
  {
  case DLNA_PROFILE_IMAGE_JPEG:
    dlna_register_profile (dlna, &dlna_profile_image_jpeg);
    break;
  case DLNA_PROFILE_IMAGE_PNG:
    dlna_register_profile (dlna, &dlna_profile_image_png);
    break;
  case DLNA_PROFILE_AUDIO_AC3:
    dlna_register_profile (dlna, &dlna_profile_audio_ac3);
    break;
  case DLNA_PROFILE_AUDIO_AMR:
    dlna_register_profile (dlna, &dlna_profile_audio_amr);
    break;
  case DLNA_PROFILE_AUDIO_ATRAC3:
    dlna_register_profile (dlna, &dlna_profile_audio_atrac3);
    break;
  case DLNA_PROFILE_AUDIO_LPCM:
    dlna_register_profile (dlna, &dlna_profile_audio_lpcm);
    break;
  case DLNA_PROFILE_AUDIO_MP3:
    dlna_register_profile (dlna, &dlna_profile_audio_mp3);
    break;
  case DLNA_PROFILE_AUDIO_MPEG4:
    dlna_register_profile (dlna, &dlna_profile_audio_mpeg4);
    break;
  case DLNA_PROFILE_AUDIO_WMA:
    dlna_register_profile (dlna, &dlna_profile_audio_wma);
    break;
  case DLNA_PROFILE_AV_MPEG1:
    dlna_register_profile (dlna, &dlna_profile_av_mpeg1);
    break;
  case DLNA_PROFILE_AV_MPEG2:
    dlna_register_profile (dlna, &dlna_profile_av_mpeg2);
    break;
  case DLNA_PROFILE_AV_MPEG4_PART2:
    dlna_register_profile (dlna, &dlna_profile_av_mpeg4_part2);
    break;
  case DLNA_PROFILE_AV_MPEG4_PART10:
    dlna_register_profile (dlna, &dlna_profile_av_mpeg4_part10);
    break;
  case DLNA_PROFILE_AV_WMV9:
    dlna_register_profile (dlna, &dlna_profile_av_wmv9);
    break;
  default:
    break;
  }
}

dlna_t *
dlna_init (void)
{
  dlna_t *dlna;

  dlna = malloc (sizeof (dlna_t));
  dlna->inited = 1;
  dlna->verbosity = 0;
  dlna->first_profile = NULL;
  
  /* register all FFMPEG demuxers */
  av_register_all ();

  return dlna;
}

void
dlna_uninit (dlna_t *dlna)
{
  if (!dlna)
    return;

  dlna->inited = 0;
  if (dlna->verbosity)
    fprintf (stderr, "DLNA: uninit\n");
  dlna->first_profile = NULL;
  free (dlna);
}

void
dlna_set_verbosity (dlna_t *dlna, int level)
{
  if (!dlna)
    return;

  dlna->verbosity = level;
}

dlna_profile_t *
dlna_guess_media_profile (dlna_t *dlna, const char *filename)
{
  AVFormatContext *pFormatCtx;
  dlna_registered_profile_t *p;
  dlna_profile_t *profile = NULL;

  if (!dlna)
    return NULL;
  
  if (!dlna->inited)
    dlna = dlna_init ();
  
  if (av_open_input_file (&pFormatCtx, filename, NULL, 0, NULL) != 0)
  {
    if (dlna->verbosity)
      fprintf (stderr, "can't open file: %s\n", filename);
    return NULL;
  }

  if (av_find_stream_info (pFormatCtx) < 0)
  {
    if (dlna->verbosity)
      fprintf (stderr, "can't find stream info\n");
    return NULL;
  }

#ifdef HAVE_DEBUG
  dump_format (pFormatCtx, 0, NULL, 0);
#endif /* HAVE_DEBUG */
  
  p = dlna->first_profile;
  while (p)
  {
    dlna_profile_t *prof;
    
    if (p->extensions)
    {
      /* check for valid file extension */
      if (!match_file_extension (filename, p->extensions))
      {
        p = p->next;
        continue;
      }
    }
    
    prof = p->probe (pFormatCtx);
    if (prof)
    {
      profile = prof;

      switch (p->id)
      {
      case DLNA_PROFILE_IMAGE_JPEG:
      case DLNA_PROFILE_IMAGE_PNG:
        profile->class = DLNA_CLASS_IMAGE;
        break;
      case DLNA_PROFILE_AUDIO_AC3:
      case DLNA_PROFILE_AUDIO_AMR:
      case DLNA_PROFILE_AUDIO_ATRAC3:
      case DLNA_PROFILE_AUDIO_LPCM:
      case DLNA_PROFILE_AUDIO_MP3:
      case DLNA_PROFILE_AUDIO_MPEG4:
      case DLNA_PROFILE_AUDIO_WMA:
        profile->class = DLNA_CLASS_AUDIO;
        break;
      case DLNA_PROFILE_AV_MPEG1:
      case DLNA_PROFILE_AV_MPEG2:
      case DLNA_PROFILE_AV_MPEG4_PART2:
      case DLNA_PROFILE_AV_MPEG4_PART10:
      case DLNA_PROFILE_AV_WMV9:
        profile->class = DLNA_CLASS_AV;
        break;
      default:
        profile->class = DLNA_CLASS_UNKNOWN;
        break;
      }
      
      break;
    }
    p = p->next;
  }

  av_close_input_file (pFormatCtx);
  return profile;
}

char *
get_file_extension (const char *filename)
{
  char *str = NULL;

  str = strrchr (filename, '.');
  if (str)
    str++;

  return str;
}

int
match_file_extension (const char *filename, const char *extensions)
{
  const char *ext, *p;
  char ext1[32], *q;

  if (!filename)
    return 0;

  ext = strrchr (filename, '.');
  if (ext)
  {
    ext++;
    p = extensions;
    for (;;)
    {
      q = ext1;
      while (*p != '\0' && *p != ',' && (q - ext1 < sizeof (ext1) - 1))
        *q++ = *p++;
      *q = '\0';
      if (!strcasecmp (ext1, ext))
        return 1;
      if (*p == '\0')
        break;
      p++;
    }
  }
  
  return 0;
}

dlna_profile_t *
set_profile (dlna_profile_t *profile)
{
  dlna_profile_t *p;
  p = malloc (sizeof (dlna_profile_t *));
  memcpy (p, profile, sizeof (*profile));
  return p;
}

AVCodecContext *
audio_profile_get_codec (AVFormatContext *ctx)
{
  AVStream *stream = NULL;
  AVCodecContext *codec = NULL;
  int i;
  
  /* check there is no video stream in container
   (otherwise it would be part of AV profile) */
  for (i = 0; i < ctx->nb_streams; i++)
  {
    stream = ctx->streams[i];
    if (!stream)
      return NULL;
    
    codec = stream->codec;
    if (!codec)
      return NULL;

    if (codec->codec_type == CODEC_TYPE_VIDEO)
      return NULL;
  }

  /* find first audio stream */
  for (i = 0; i < ctx->nb_streams; i++)
  {
    stream = ctx->streams[i];
    if (!stream)
      return NULL;
    
    codec = stream->codec;
    if (!codec)
      return NULL;

    if (codec->codec_type == CODEC_TYPE_AUDIO)
      break;
  }

  return codec;
}

av_codecs_t *
av_profile_get_codecs (AVFormatContext *ctx)
{
  AVStream *stream = NULL;
  AVCodecContext *codec = NULL;
  av_codecs_t *codecs = NULL;
  int i;
 
  codecs = malloc (sizeof (av_codecs_t));
  
  /* find first audio stream */
  for (i = 0; i < ctx->nb_streams; i++)
  {
    stream = ctx->streams[i];
    if (!stream)
      goto av_profile_get_codecs_end;
    
    codec = stream->codec;
    if (!codec)
      goto av_profile_get_codecs_end;

    if (codec->codec_type == CODEC_TYPE_AUDIO)
    {
      codecs->as = stream;
      codecs->ac = codec;
      break;
    }
  }

  /* find first video stream */
  for (i = 0; i < ctx->nb_streams; i++)
  {
    stream = ctx->streams[i];
    if (!stream)
      goto av_profile_get_codecs_end;
    
    codec = stream->codec;
    if (!codec)
      goto av_profile_get_codecs_end;

    if (codec->codec_type == CODEC_TYPE_VIDEO)
    {
      codecs->vs = stream;
      codecs->vc = codec;
      break;
    }
  }

  /* check for at least one video stream and one audio stream in container */
  if (!codecs->ac || !codecs->vc)
    goto av_profile_get_codecs_end;
  
  return codecs;

 av_profile_get_codecs_end:
  if (codecs)
    free (codecs);
  return NULL;
}

char *
dlna_write_protocol_info (dlna_protocol_info_type_t type,
                          dlna_org_play_speed_t speed,
                          dlna_org_conversion_t ci,
                          dlna_org_operation_t op,
                          dlna_org_flags_t flags,
                          dlna_profile_t *p)
{
  char protocol[512];
  char dlna_info[448];

  if (type == DLNA_PROTOCOL_INFO_TYPE_HTTP)
    sprintf (protocol, "http-get:*:");

  strcat (protocol, p->mime);
  strcat (protocol, ":");
  
  sprintf (dlna_info, "%s=%d;%s=%d;%s=%.2x;%s=%s;%s=%.8x%.24x",
           "DLNA.ORG_PS", speed, "DLNA.ORG_CI", ci,
           "DLNA.ORG_OP", op, "DLNA.ORG_PN", p->id,
           "DLNA.ORG_FLAGS", flags, 0);
  strcat (protocol, dlna_info);

  return strdup (protocol);
}

audio_profile_t
audio_profile_guess (AVCodecContext *ac)
{
  audio_profile_t ap = AUDIO_PROFILE_INVALID;
  
  if (!ac)
    return ap;

  ap = audio_profile_guess_aac (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_ac3 (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_amr (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_atrac (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_g726 (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_lpcm (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_mp2 (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_mp3 (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_wma (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  return AUDIO_PROFILE_INVALID;
}
