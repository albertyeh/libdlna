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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dlna_internals.h"
#include "upnp_internals.h"

#define UPNP_DMS_DESCRIPTION \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">" \
"  <specVersion>" \
"    <major>1</major>" \
"    <minor>0</minor>" \
"  </specVersion>" \
"  <device>" \
"    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>" \
"    <friendlyName>%s: 1</friendlyName>" \
"    <manufacturer>%s</manufacturer>" \
"    <manufacturerURL>%s</manufacturerURL>" \
"    <modelDescription>%s</modelDescription>" \
"    <modelName>%s</modelName>" \
"    <modelNumber>%s</modelNumber>" \
"    <modelURL>%s</modelURL>" \
"    <serialNumber>%s</serialNumber>" \
"    <UDN>uuid:%s</UDN>" \
"    <presentationURL>%s/%s</presentationURL>" \
"    <dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">DMS-1.00</dlna:X_DLNADOC>" \
"    <serviceList>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>" \
"        <SCPDURL>%s/%s</SCPDURL>" \
"        <controlURL>%s/%s</controlURL>" \
"        <eventSubURL>%s/%s</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>" \
"        <SCPDURL>%s/%s</SCPDURL>" \
"        <controlURL>%s/%s</controlURL>" \
"        <eventSubURL>%s/%s</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:AVTransport:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:AVTransport</serviceId>" \
"        <SCPDURL>%s/%s</SCPDURL>" \
"        <controlURL>%s/%s</controlURL>" \
"        <eventSubURL>%s/%s</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1</serviceType>\n" \
"        <serviceId>urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar</serviceId>\n" \
"        <SCPDURL>%s/%s</SCPDURL>" \
"        <controlURL>%s/%s</controlURL>" \
"        <eventSubURL>%s/%s</eventSubURL>" \
"      </service>" \
"    </serviceList>" \
"  </device>" \
"</root>"

char *
dlna_dms_description_get (dlna_t *dlna)
{
  char *model_name, *desc = NULL;
  size_t len;
 
  if (!dlna)
    return NULL;

  if (dlna->mode == DLNA_CAPABILITY_UPNP_AV_XBOX)
  {
    model_name =
      malloc (strlen (XBOX_MODEL_NAME) + strlen (dlna->model_name) + 4);
    sprintf (model_name, "%s (%s)", XBOX_MODEL_NAME, dlna->model_name);
  }
  else
    model_name = strdup (dlna->model_name);
  
  len = strlen (UPNP_DMS_DESCRIPTION) + strlen (dlna->friendly_name)
    + strlen (dlna->manufacturer) + strlen (dlna->manufacturer_url)
    + strlen (dlna->model_description) + strlen (model_name)
    + strlen (dlna->model_number) + strlen (dlna->model_url)
    + strlen (dlna->serial_number) + strlen (dlna->uuid) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (dlna->presentation_url) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (CMS_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (CMS_CONTROL_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (CMS_EVENT_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (CDS_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (CDS_CONTROL_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (CDS_EVENT_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (AVTS_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (AVTS_CONTROL_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (AVTS_EVENT_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (MSR_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (MSR_CONTROL_URL) +
    + strlen (SERVICES_VIRTUAL_DIR) + strlen (MSR_EVENT_URL) +
    1;

  desc = malloc (len);
  memset (desc, 0, len);
  sprintf (desc, UPNP_DMS_DESCRIPTION, dlna->friendly_name,
           dlna->manufacturer, dlna->manufacturer_url, dlna->model_description,
           model_name, dlna->model_number, dlna->model_url,
           dlna->serial_number, dlna->uuid,
           SERVICES_VIRTUAL_DIR, dlna->presentation_url,
           SERVICES_VIRTUAL_DIR, CMS_URL,
           SERVICES_VIRTUAL_DIR, CMS_CONTROL_URL,
           SERVICES_VIRTUAL_DIR, CMS_EVENT_URL,
           SERVICES_VIRTUAL_DIR, CDS_URL,
           SERVICES_VIRTUAL_DIR, CDS_CONTROL_URL,
           SERVICES_VIRTUAL_DIR, CDS_EVENT_URL,
           SERVICES_VIRTUAL_DIR, AVTS_URL,
           SERVICES_VIRTUAL_DIR, AVTS_CONTROL_URL,
           SERVICES_VIRTUAL_DIR, AVTS_EVENT_URL,
           SERVICES_VIRTUAL_DIR, MSR_URL,
           SERVICES_VIRTUAL_DIR, MSR_CONTROL_URL,
           SERVICES_VIRTUAL_DIR, MSR_EVENT_URL);

  return desc;
}

int
dlna_dms_init (dlna_t *dlna)
{
  if (!dlna)
    return DLNA_ST_ERROR;

  if (!dlna->inited)
    return DLNA_ST_ERROR;

  return upnp_init (dlna, DLNA_DEVICE_DMS);
}

int
dlna_dms_uninit (dlna_t *dlna)
{
  if (!dlna)
    return DLNA_ST_ERROR;

  if (!dlna->inited)
    return DLNA_ST_ERROR;

  return upnp_uninit (dlna);
}
