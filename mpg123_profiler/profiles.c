/*
 * Copyright (C) 2014-2016 Marc Chalain <marc.chalain@gmail.com>
 *
 * This file is part of uplaymusic.
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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mpg123.h>

#include "dlna_internals.h"
#include "sound_module.h"
#include "network.h"

#define dbgprintf(...)

static dlna_properties_t *item_get_properties (dlna_item_t *item);
static dlna_metadata_t *item_get_metadata (dlna_item_t *item);
static int item_prepare_stream (dlna_item_t *item);
static int item_read_stream (dlna_item_t *item);
static void item_close_stream (dlna_item_t *item);

struct 
{
  enum mpg123_version version;
  int layer;
  enum mpg123_mode channels;
} default_profiles_info[] = {
  {
    .version = MPG123_1_0,
    .layer = 3,
    .channels = MPG123_STEREO,
  },
  {
    .version = MPG123_2_0,
    .layer = 3,
    .channels = MPG123_STEREO,
  },
  {
    .version = MPG123_2_5,
    .layer = 3,
    .channels = MPG123_STEREO,
  },
  {
    .version = MPG123_1_0,
    .layer = 3,
    .channels = MPG123_MONO,
  },
};
static dlna_profile_t *default_profiles[] = {
  & (dlna_profile_t) {
    .id = "MP3",
    .mime = MIME_AUDIO_MPEG,
    .label = LABEL_AUDIO_2CH,
    .media_class = DLNA_CLASS_AUDIO,
  },
  & (dlna_profile_t) {
    .id = "MP3X",
    .mime = MIME_AUDIO_MPEG,
    .label = LABEL_AUDIO_2CH,
    .media_class = DLNA_CLASS_AUDIO,
  },
  & (dlna_profile_t) {
    .id = "MP3X",
    .mime = MIME_AUDIO_MPEG,
    .label = LABEL_AUDIO_2CH,
    .media_class = DLNA_CLASS_AUDIO,
  },
  & (dlna_profile_t) {
    .id = "MP3",
    .mime = MIME_AUDIO_MPEG,
    .label = LABEL_AUDIO_MONO,
    .media_class = DLNA_CLASS_AUDIO,
  },
	NULL
};

typedef struct mpg123_profiler_data_s mpg123_profiler_data_t;
struct mpg123_profiler_data_s
{
  char **mimes;
  dlna_profile_t *profile;
  enum mpg123_version version;
  int layer;
  enum mpg123_channelcount channels;
  struct sound_module *sound;
  mpg123_profiler_data_t *next;
  mpg123_profiler_data_t *previous;
};

typedef struct profile_data_s profile_data_t;
struct profile_data_s
{
  mpg123_profiler_data_t *profiler;
  dlna_properties_t *prop;
  dlna_metadata_t *meta;
  int fd;
  ssize_t buffsize;
  void *buffer;
  uint32_t offset;
  uint32_t length;
};

static mpg123_profiler_data_t *g_profiler = NULL;
static mpg123_handle *g_profiler_handle = NULL;

#define ONLY_ONE
int
mpg123_profiler_init ()
{
  int ret = 0;
  mpg123_profiler_data_t *profiler = NULL;
  mpg123_profiler_data_t *previous = NULL;
  const char **decoderslist;

  mpg123_init ();

  decoderslist = mpg123_decoders();
  while (*decoderslist)
  {
    int i;

    g_profiler_handle = mpg123_new(*decoderslist, &ret);
    decoderslist ++;
    if (ret)
      continue;
    for (i = 0; default_profiles[i]; i++)
    {
      profiler = calloc (1, sizeof (mpg123_profiler_data_t));
      mpg123_param(g_profiler_handle, MPG123_RESYNC_LIMIT, -1, 0);
      profiler->profile = default_profiles[i];
      profiler->version = default_profiles_info[i].version;
      profiler->layer = default_profiles_info[i].layer;
      profiler->channels = default_profiles_info[i].channels;
      profiler->sound = sound_module_get();

      if (!g_profiler)
      {
        g_profiler = profiler;
      }
      else if (profiler)
      {
        previous->next = profiler;
        profiler->previous = previous;
      }
      previous = profiler;
    }
#ifdef ONLY_ONE
    if (profiler)
      break;
#endif
  }
	return ret;
}

static int
dlna_list_length (void *list)
{
  void **l = list;
  int n = 0;
  while (*l++)
    n++;

  return n;
}

static void *
dlna_list_add (char **list, char *element)
{
  char **l = list;
  int n = dlna_list_length (list) + 1;
  int i;

  for (i = 0; i < n; i++)
    if (l[i] && element && !strcmp (l[i], element))
      return l;
  
  l = realloc (l, (n + 1) * sizeof (char *));
  l[n] = NULL;
  l[n - 1] = element;
  
  return l;
}

static char **
mpg123_profiler_get_supported_mime_types ()
{
  mpg123_profiler_data_t *profiler;

  if (g_profiler->mimes)
    return g_profiler->mimes;
  else
    g_profiler->mimes = calloc (1, sizeof (char *));
  
  profiler = g_profiler;
  while (profiler)
  {
    g_profiler->mimes = dlna_list_add (g_profiler->mimes, (char *)profiler->profile->mime);
    profiler = profiler->next;
  }
  g_profiler->mimes = dlna_list_add (g_profiler->mimes, NULL);
  return g_profiler->mimes;
}

static void
mpg123_profiler_free ()
{
  mpg123_profiler_data_t *profiler;
  while (g_profiler)
  {
    profiler = g_profiler->next;
    if (g_profiler->mimes)
      free (g_profiler->mimes);
    free (g_profiler);
    g_profiler = profiler;
  }
  mpg123_delete (g_profiler_handle);
  mpg123_exit ();
}

static void
profile_free(dlna_item_t *item);
static dlna_properties_t *
item_get_properties (dlna_item_t *item);
static dlna_metadata_t *
item_get_metadata (dlna_item_t *item);

static char*
dup_mpg123_string (mpg123_string *string)
{
  char *ret = NULL;
  if (string && string->fill)
    ret = strndup (string->p,string->size);
  return ret;
}

dlna_profile_t *
mpg123_profiler_guess_media_profile (dlna_stream_t *reader, void **cookie)
{
  dlna_profile_t *profile;
  dlna_properties_t *prop;
  dlna_metadata_t *meta;
  profile_data_t *data;
  mpg123_profiler_data_t *profiler;
  int  encoding = MPG123_ENC_SIGNED_32;
  long rate = 44100;
  off_t length;
  uint32_t time, time_s, time_m, time_h;
  struct mpg123_frameinfo mpg_info;
  int metaflags;
  mpg123_id3v1 *v1 = NULL;
  mpg123_id3v2 *v2 = NULL;
  enum mpg123_channelcount channels;

  profiler = g_profiler;
  mpg123_replace_reader_handle (g_profiler_handle, reader->read, reader->lseek, reader->cleanup);

	if(mpg123_open_handle(g_profiler_handle, reader) != MPG123_OK)
	{
    dbgprintf ("%s 1: %s\n", __FUNCTION__, mpg123_strerror (g_profiler_handle));
		return NULL;
	}
  if (mpg123_scan(g_profiler_handle) != MPG123_OK)
  {
    dbgprintf ("%s 2: %s\n", __FUNCTION__, mpg123_strerror (g_profiler_handle));
    mpg123_close(g_profiler_handle);
    return NULL;
  }
  length = mpg123_length(g_profiler_handle);
  if (length < MPG123_OK)
  {
    dbgprintf ("%s 3: %s\n", __FUNCTION__, mpg123_strerror (g_profiler_handle));
    mpg123_close(g_profiler_handle);
    return NULL;
  }
  if (mpg123_info (g_profiler_handle, &mpg_info) != MPG123_OK)
  {
    dbgprintf ("%s 4: %s\n", __FUNCTION__, mpg123_strerror (g_profiler_handle));
    mpg123_close(g_profiler_handle);
    return NULL;
  }
  rate = mpg_info.rate;
  channels = (mpg_info.mode == MPG123_M_MONO)? MPG123_MONO:MPG123_STEREO;

  while (profiler)
  {
    if (mpg_info.version == profiler->version && mpg_info.layer == profiler->layer && channels == profiler->channels)
      break;
    profiler = profiler->next;
  }
  if (!profiler)
  {
    dbgprintf ("found MPEG version %d layer %d nb channels %d\n", mpg_info.version, mpg_info.layer, channels);
    mpg123_close(g_profiler_handle);
    return NULL;
  }

  profile = profiler->profile;
  data = calloc (1, sizeof (profile_data_t));
  data->profiler = profiler;

  /* check the possible output */
	mpg123_format_none(g_profiler_handle);
	mpg123_format(g_profiler_handle, rate, (int)channels, encoding);

	if (mpg123_getformat(g_profiler_handle, &rate, (int *)&channels, &encoding) != MPG123_OK)
	{
    dbgprintf ("%s 5: %s\n", __FUNCTION__, mpg123_strerror (g_profiler_handle));
    mpg123_close(g_profiler_handle);
		return NULL;
	}

  data->buffsize = mpg123_outblock(g_profiler_handle);

  /* properties setup */
  prop = calloc (1, sizeof (dlna_properties_t));

  prop->sample_frequency = rate;
  prop->channels = channels;
  prop->bps = (encoding & MPG123_ENC_8)? 8:(encoding & MPG123_ENC_16)? 16: (encoding & MPG123_ENC_32)? 32: 32;

  data->length = length;
  time = length / rate;

  time_h = time / 60 / 60;
  time_m = (time / 60) % 60;
  time_s = time % 60;
  snprintf(prop->duration, 63, "%02u:%02u:%02u", time_h, time_m, time_s);
  data->prop = prop;
  
  /* metadata setup */
  metaflags = mpg123_meta_check(g_profiler_handle);
  if((metaflags & MPG123_ID3) && mpg123_id3(g_profiler_handle, &v1, &v2) == MPG123_OK)
  {
    meta = calloc (1, sizeof (dlna_metadata_t));
    meta->title = dup_mpg123_string (v2->title);
    if (!meta->title && v1->title)
      meta->title = strndup (v1->title,sizeof(v1->title));
    meta->author = dup_mpg123_string (v2->artist);
    if (!meta->author && v1->artist)
      meta->author = strndup (v1->artist,sizeof(v1->artist));
    meta->album = dup_mpg123_string (v2->album);
    if (!meta->album && v1->album)
      meta->album = strndup (v1->album,sizeof(v1->album));
    meta->comment = dup_mpg123_string (v2->comment);
    if (!meta->comment && v1->comment)
      meta->comment = strndup (v1->comment,sizeof(v1->comment));
    meta->genre = dup_mpg123_string (v2->genre);
    if (!meta->genre)
      meta->genre = strdup ("default");
    mpg123_meta_free (g_profiler_handle);
    data->meta = meta;
  }

  *cookie = data;
  mpg123_close(g_profiler_handle);

  profile->free = profile_free;
  profile->get_metadata = item_get_metadata;
  profile->get_properties = item_get_properties;
  profile->prepare_stream = item_prepare_stream;
  profile->read_stream = item_read_stream;
  profile->close_stream = item_close_stream;

  return profile;
}

static void
profile_free(dlna_item_t *item)
{
  profile_data_t *cookie = (profile_data_t *)item->profile_cookie;

  free (cookie);
  item->profile_cookie = NULL;
}

static dlna_properties_t *
item_get_properties (dlna_item_t *item)
{
  profile_data_t *cookie = (profile_data_t *)item->profile_cookie;
  return cookie->prop;
}

static dlna_metadata_t *
item_get_metadata (dlna_item_t *item)
{
  profile_data_t *cookie = (profile_data_t *)item->profile_cookie;
  return cookie->meta;
}

static int
item_prepare_stream (dlna_item_t *item)
{
  profile_data_t *cookie = (profile_data_t *)item->profile_cookie;
  mpg123_profiler_data_t *profiler = cookie->profiler;
  int  channels = 2, encoding = MPG123_ENC_SIGNED_32;
  long rate = 44100;

  profiler = g_profiler;
  mpg123_replace_reader_handle (g_profiler_handle, item->stream->read, item->stream->lseek, item->stream->cleanup);

  if (mpg123_open_handle (g_profiler_handle, item->stream))
  {
    dbgprintf ("%s: %s\n", __FUNCTION__, mpg123_strerror (g_profiler_handle));
    return -1;
  }

	mpg123_format_none(g_profiler_handle);
	mpg123_format(g_profiler_handle, rate, channels, encoding);

	if (mpg123_getformat(g_profiler_handle, &rate, &channels, &encoding) != MPG123_OK)
	{
    dbgprintf ("%s: %s\n", __FUNCTION__, mpg123_strerror (g_profiler_handle));
    mpg123_close(g_profiler_handle);
		return -1;
	}

  profiler->sound->open (channels, encoding, rate);
  cookie->buffer = calloc (1, cookie->buffsize);
  cookie->offset = 0;
  return 0;
}

static int
item_read_stream (dlna_item_t *item)
{
  profile_data_t *cookie = (profile_data_t *)item->profile_cookie;
  mpg123_profiler_data_t *profiler = cookie->profiler;
  size_t done = 0;
  int err;

  err = mpg123_read( g_profiler_handle, cookie->buffer, cookie->buffsize, &done );

  if (err > MPG123_OK || err == MPG123_ERR)
  {
    dbgprintf ("%s: %s\n", __FUNCTION__, mpg123_strerror (g_profiler_handle));
    return -1;
  }
  if (err == MPG123_DONE || ( err == MPG123_NEED_MORE && cookie->offset >= cookie->length))
  {
    return -1;
  }
  if (err == MPG123_OK)
  {
    cookie->offset += done;
    err = profiler->sound->write (cookie->buffer, cookie->buffsize);
    if (err <  0)
       dbgprintf ("%s: %d %d\n", __FUNCTION__, err, done);
  }
  return 1;
}

static void
item_close_stream (dlna_item_t *item)
{
  profile_data_t *cookie = (profile_data_t *)item->profile_cookie;
  mpg123_profiler_data_t *profiler = cookie->profiler;

  mpg123_close(g_profiler_handle);
  profiler->sound->close ();
}

dlna_profile_t *
mpg123_profiler_get_media_profile (char *profileid)
{
  mpg123_profiler_data_t *profiler;

  profiler = g_profiler;
  while (profiler)
  {
    if (!strcmp(profileid, profiler->profile->id))
    {
      return profiler->profile;
    }
    profiler = profiler->next;
  }
  return NULL;
}


const dlna_profiler_t mpg123_profiler = {
  .guess_media_profile = mpg123_profiler_guess_media_profile,
  .get_media_profile = mpg123_profiler_get_media_profile,
  .get_supported_mime_types = mpg123_profiler_get_supported_mime_types,
  .free = mpg123_profiler_free,
};
