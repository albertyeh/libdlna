/*
 * libdlna: reference DLNA standards implementation.
 * Copyright (C) 2007-2008 Benjamin Zores <ben@geexbox.org>
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

#ifndef PROFILES_H
#define PROFILES_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

/* UPnP library headers */
#include <ithread.h>

#include "dlna.h"
#include "ffmpeg_profiler.h"
#include "containers.h"

#if defined(__GNUC__)
#    define dlna_unused __attribute__((unused))
#else
#    define dlna_unused
#endif

typedef struct av_codecs_s {
  unsigned int nb_streams;
  /* audio stream and codec */
  AVStream *as;
  AVCodecContext *ac;
  /* video stream and codec */
  AVStream *vs;
  AVCodecContext *vc;
} av_codecs_t;

typedef struct registered_profile_s {
  ffmpeg_profiler_media_profile_t id;
  dlna_media_class_t class;
  char *extensions;
  dlna_profile_t **profiles;
  dlna_profile_t * (*probe) (AVFormatContext *ctx,
                             av_codecs_t *codecs);
  struct registered_profile_s *next;
} registered_profile_t;

char * get_file_extension (const char *filename);

/* audio profile checks */

typedef enum {
  AUDIO_PROFILE_INVALID = 0,

  /* Advanced Audio Codec variants */
  AUDIO_PROFILE_AAC,
  AUDIO_PROFILE_AAC_320,
  AUDIO_PROFILE_AAC_MULT5,
  AUDIO_PROFILE_AAC_BSAC,  
  AUDIO_PROFILE_AAC_BSAC_MULT5,
  AUDIO_PROFILE_AAC_HE_L2,
  AUDIO_PROFILE_AAC_HE_L2_320,
  AUDIO_PROFILE_AAC_HE_L3,
  AUDIO_PROFILE_AAC_HE_MULT5,
  AUDIO_PROFILE_AAC_HE_V2_L2,
  AUDIO_PROFILE_AAC_HE_V2_L2_320,
  AUDIO_PROFILE_AAC_HE_V2_L3,
  AUDIO_PROFILE_AAC_HE_V2_MULT5,
  AUDIO_PROFILE_AAC_LTP,  
  AUDIO_PROFILE_AAC_LTP_MULT5,
  AUDIO_PROFILE_AAC_LTP_MULT7,
  
  AUDIO_PROFILE_AC3,
  AUDIO_PROFILE_AC3_EXTENDED,
  
  AUDIO_PROFILE_AMR,
  AUDIO_PROFILE_AMR_WB,
  
  AUDIO_PROFILE_ATRAC,

  AUDIO_PROFILE_G726,

  AUDIO_PROFILE_LPCM,

  /* MPEG audio variants */
  AUDIO_PROFILE_MP2,
  AUDIO_PROFILE_MP3,
  AUDIO_PROFILE_MP3_EXTENDED,

  /* Windows Media Audio variants */
  AUDIO_PROFILE_WMA_BASELINE,
  AUDIO_PROFILE_WMA_FULL,
  AUDIO_PROFILE_WMA_PRO
} audio_profile_t;

audio_profile_t audio_profile_guess (AVCodecContext *ac);

audio_profile_t audio_profile_guess_aac (AVCodecContext *ac);
audio_profile_t audio_profile_guess_ac3 (AVCodecContext *ac);
audio_profile_t audio_profile_guess_amr (AVCodecContext *ac);
audio_profile_t audio_profile_guess_atrac (AVCodecContext *ac);
audio_profile_t audio_profile_guess_g726 (AVCodecContext *ac);
audio_profile_t audio_profile_guess_lpcm (AVCodecContext *ac);
audio_profile_t audio_profile_guess_mp2 (AVCodecContext *ac);
audio_profile_t audio_profile_guess_mp3 (AVCodecContext *ac);
audio_profile_t audio_profile_guess_wma (AVCodecContext *ac);

/* stream context check routines */
int stream_ctx_is_image (AVFormatContext *ctx,
                         av_codecs_t *codecs);
int stream_ctx_is_audio (av_codecs_t *codecs);
int stream_ctx_is_av (av_codecs_t *codecs);

typedef struct ffmpeg_profiler_data_s
{
  /* has the library's been inited */
  int inited;
  /* linked-list of registered DLNA profiles */
  void *first_profile;
} ffmpeg_profiler_data_t;

typedef struct ffmpeg_stream_s ffmpeg_stream_t;
struct ffmpeg_stream_s
{
  int id;
  ithread_mutex_t mutex;
  ithread_cond_t cond;
  ithread_t thread;
};
typedef struct ffmpeg_profile_s ffmpeg_profile_t;
struct ffmpeg_profile_s
{
  AVFormatContext *ctx;
  struct ffmpeg_stream_s stream[AVMEDIA_TYPE_NB];
};
#endif /* PROFILES_H */
