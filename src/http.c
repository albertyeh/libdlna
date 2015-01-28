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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "upnp_internals.h"
#include "services.h"
#include "vfs.h"
#include "devices.h"

#define PROTOCOL_TYPE_PRE_SZ  11   /* for the str length of "http-get:*:" */
#define PROTOCOL_TYPE_SUFF_SZ 2    /* for the str length of ":*" */

/* Internal HTTP Server */
dlna_http_callback_t *dlna_http_callback = NULL;

struct http_resource_s
{
  uint32_t id;
};

static char *
http_url (vfs_resource_t *resource)
{
  char *url;
  struct http_resource_s *cookie = resource->cookie;
  
  url = malloc (1024);
  sprintf (url, "http://%s:%d%s/%u%s",
                      dlnaGetServerIpAddress (),
                      dlnaGetServerPort (), VIRTUAL_DIR, 
                      cookie->id, resource->protocol_info->profile->ext);
  return url;
}

static dlna_protocol_t *static_http_protocol = NULL;

static dlna_stream_t *
dlna_http_stream_open (void *cookie, const char *url)
{
  vfs_item_t *item = (vfs_item_t *)cookie;
  dlna_item_t *dlna_item;
  dlna_stream_t *stream;
  vfs_resource_t *resource;

  if (strncmp (url, VIRTUAL_DIR, VIRTUAL_DIR_LEN))
    return NULL;
  /* ask for anything else ... */
/*
  uint32_t id;
  id = strtoul (page, NULL, 10);
*/
  if (item->type != DLNA_RESOURCE)
    return NULL;

  dlna_item = vfs_item_get(item);
  if (!dlna_item)
    return NULL;

  if (!dlna_item->filename)
    return NULL;

  char *page;
  page = strchr (url, '/') + 1;

  resource = vfs_resource_get (item);
  while (resource)
  {
    char *res_url = resource->url (resource);
    char *res_page;

    res_page = strchr (res_url, '/') + 1;
    if (!strcmp (res_page, page))
    {
      free (res_url);
      break;
    }
    resource = resource->next;
    free (res_url);
  }
  stream = stream_open (dlna_item->filename);
  if (stream && resource && resource->protocol_info->other)
  {
    char *other = resource->protocol_info->other (resource->protocol_info);
    char *mime = strdup (stream->mime);
    snprintf (stream->mime, 199, 
                "%s:%s;DLNA.ORG_PS=%d;DLNA.ORG_CI=%d;DLNA.ORG_OP=%02d;", 
                mime, other,
                resource->info.speed, resource->info.cnv, resource->info.op);
    free (other);
    free (mime);
  }
  return stream;
}

static vfs_resource_t *
dlna_http_resource_new (vfs_item_t *item)
{
  vfs_resource_t *resource;
  dlna_item_t *dlna_item;
  struct http_resource_s *cookie;

  resource = calloc (1, sizeof (vfs_resource_t));
  cookie = calloc (1, sizeof (struct http_resource_s));
  cookie->id = item->id;
  resource->cookie = cookie;
  resource->url = http_url;

  dlna_http_callback_t *callback;
  callback = calloc (1, sizeof (dlna_http_callback_t));
  callback->cookie = item;
  callback->open = dlna_http_stream_open;
  dlna_http_set_callback(callback);
  dlna_item = vfs_item_get (item);

  resource->protocol_info = calloc (1, sizeof (protocol_info_t));
  resource->protocol_info->protocol = static_http_protocol;
  resource->protocol_info->profile = dlna_item->profile;

  resource->size = dlna_item->filesize;
  dlna_properties_t *properties = dlna_item_properties (dlna_item);
  if (properties)
    memcpy (&resource->properties, properties, sizeof (dlna_properties_t));
  resource->info.protocolid = DLNA_PROTOCOL_INFO_TYPE_HTTP;
  resource->info.cnv = DLNA_ORG_CONVERSION_NONE;
  resource->info.op = DLNA_ORG_OPERATION_RANGE;
  resource->info.speed = DLNA_ORG_PLAY_SPEED_NORMAL;
  return resource;
}

static const char *
http_name ()
{
  return "http-get";
}

static const char *
http_net ()
{
  return "*";
}

dlna_protocol_t *
http_protocol_new (dlna_t *dlna dlna_unused)
{
  static_http_protocol = calloc (1, sizeof (dlna_protocol_t));
  static_http_protocol->type = DLNA_PROTOCOL_INFO_TYPE_HTTP;
  static_http_protocol->create_resource = dlna_http_resource_new;
  static_http_protocol->name = http_name;
  static_http_protocol->net = http_net;
  return static_http_protocol;
}

static inline void
set_service_http_info (struct File_Info *info,
                       const size_t length,
                       const char *content_type)
{
  info->file_length   = length;
  info->last_modified = 0;
  info->is_directory  = 0;
  info->is_readable   = 1;
  info->content_type  = ixmlCloneDOMString (content_type);
}

static int
dlna_http_get_info (void *cookie,
                    const char *filename,
                    struct File_Info *info)
{
  if (!cookie || !filename || !info)
    return HTTP_ERROR;

  dlna_log (DLNA_MSG_INFO,
            "%s, filename : %s\n", __FUNCTION__, filename);

  /* trap application-level HTTP callback */
  dlna_http_callback_t *http_callback;
  for (http_callback = dlna_http_callback; http_callback; http_callback = http_callback->next)
  {
    dlna_stream_t *stream = NULL;
    if (http_callback->open)
      stream = http_callback->open (http_callback->cookie, filename);
    if (stream)
    {
      set_service_http_info (info, stream->length, stream->mime);
      stream->close (stream);
      return HTTP_OK;
    }
  }
  
  return HTTP_ERROR;
}

static dlnaWebFileHandle
dlna_http_open (void *cookie,
                const char *filename,
                enum dlnaOpenFileMode mode)
{
  if (!cookie || !filename)
    return NULL;

  dlna_log (DLNA_MSG_INFO,
            "%s, filename : %s\n", __FUNCTION__, filename);

  if (mode != DLNA_READ)
    return NULL;

  /* trap application-level HTTP callback */
  dlna_http_callback_t *http_callback;
  for (http_callback = dlna_http_callback; http_callback; http_callback = http_callback->next)
  {
    dlna_stream_t *stream = NULL;
    if (http_callback->open)
      stream = http_callback->open (http_callback->cookie, filename);
    if (stream)
    {
      dlna_http_file_handler_t *dhdl;
      dhdl = calloc (1, sizeof (dlna_http_file_handler_t));
      dhdl->external = 1;
      dhdl->priv = stream;
      return (dlnaWebFileHandle)dhdl;
    }
  }
  
  return NULL;
}

static int
dlna_http_read (void *cookie,
                dlnaWebFileHandle fh,
                char *buf,
                size_t buflen)
{
  dlna_http_file_handler_t *dhdl;

  if (!cookie || !fh)
    return HTTP_ERROR;

  dhdl = (dlna_http_file_handler_t *) fh;
  
  dlna_log (DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  /* trap application-level HTTP callback */
  if (dhdl->external)
  {
    int res;
    dlna_stream_t *stream = dhdl->priv;
    if (stream->read)
      res = stream->read (stream, buf, buflen);
    if (res > 0)
      return res;
  }

  return -1;
}

static int
dlna_http_write (void *cookie dlna_unused,
                 dlnaWebFileHandle fh,
                 char *buf,
                 size_t buflen)
{
  dlna_http_file_handler_t *dhdl;

  dhdl = (dlna_http_file_handler_t *) fh;
  
  /* trap application-level HTTP callback */
  if (dhdl->external)
  {
    int res;
    dlna_stream_t *stream = dhdl->priv;
    if (stream->write)
      res = stream->write (stream, buf, buflen);
    if (res > 0)
      return res;
  }
  
  return 0;
}

static int
dlna_http_seek (void *cookie,
                dlnaWebFileHandle fh,
                off_t offset,
                int origin)
{
  dlna_http_file_handler_t *dhdl;
  
  if (!cookie || !fh)
    return HTTP_ERROR;

  dhdl = (dlna_http_file_handler_t *) fh;
  
  dlna_log (DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  /* trap application-level HTTP callback */
  if (dhdl->external)
  {
    int res;
    dlna_stream_t *stream = dhdl->priv;
    if (stream->lseek)
      res = stream->lseek (stream, offset, origin);
    if (res >= 0)
      return HTTP_OK;
  }

  return HTTP_ERROR;
}

static int
dlna_http_close (void *cookie,
                 dlnaWebFileHandle fh)
{
  dlna_http_file_handler_t *dhdl;
  
  if (!cookie || !fh)
    return HTTP_ERROR;

  dhdl = (dlna_http_file_handler_t *) fh;
  
  dlna_log (DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  /* trap application-level HTTP callback */
  if (dhdl->external)
  {
    dlna_stream_t *stream = dhdl->priv;
    if (stream->close)
      stream->close (stream);
    free (dhdl);
    return 0;
  }

  return HTTP_OK;
}

void
dlna_http_set_callback (dlna_http_callback_t *cb)
{
  if (!cb)
    return;

  cb->next = dlna_http_callback;
  dlna_http_callback = cb;
}


#ifndef HAVE_EXTERNAL_LIBUPNP
struct dlnaVirtualDirCallbacks virtual_dir_callbacks = {
  .cookie = NULL,
  .get_info = dlna_http_get_info,
  .open = dlna_http_open,
  .read = dlna_http_read,
  .write = dlna_http_write,
  .seek = dlna_http_seek,
  .close = dlna_http_close
};
#else
static void *http_cookie;

int dlnaSetVirtualDirCallbacks(
    struct dlnaVirtualDirCallbacks *callbacks,
    void *cookie)
{
  http_cookie = cookie;
  return UpnpSetVirtualDirCallbacks(callbacks);
}

static int
upnp_http_get_info (
                    const char *filename,
                    struct File_Info *info)
{
  return dlna_http_get_info(http_cookie, filename, info);
}

static dlnaWebFileHandle
upnp_http_open (
                const char *filename,
                enum dlnaOpenFileMode mode)
{
  return dlna_http_open(http_cookie, filename, mode);
}

static int
upnp_http_read (
                 dlnaWebFileHandle fh,
                 char *buf,
                 size_t buflen)
{
  return dlna_http_read(http_cookie, fh, buf, buflen);
}

static int
upnp_http_write (
                 dlnaWebFileHandle fh,
                 char *buf,
                 size_t buflen)
{
  return dlna_http_write(http_cookie, fh, buf, buflen);
}

static int
upnp_http_seek (
                dlnaWebFileHandle fh,
                off_t offset,
                int origin)
{
  return dlna_http_seek(http_cookie, fh, offset, origin);
}

static int
upnp_http_close (dlnaWebFileHandle fh)
{
  return dlna_http_close(http_cookie, fh);
}

struct UpnpVirtualDirCallbacks virtual_dir_callbacks = {
  .get_info = upnp_http_get_info,
  .open = upnp_http_open,
  .read = upnp_http_read,
  .write = upnp_http_write,
  .seek = upnp_http_seek,
  .close = upnp_http_close
};
#endif
