/*
 * Public Key File System (PKFS)
 * Copyright (C) 2012 Kelsey Hightower <kelsey.hightower@gmail.com>
 *
 * This program can be distributed under the terms of the MIT license.
 *
 */
#include <libconfig.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include "utils.h"

void initialize_config(struct pkfs_config *config)
{
  syslog(LOG_DEBUG, "Initializing pkfs config");

  int error;
  config_t cfg;
  config_t *cf;

  cf = &cfg;
  config_init(cf);

  error = config_read_file(cf, "/etc/pkfs.conf");

  if (error == CONFIG_FALSE) {
    syslog(LOG_ERR, "Cannot load /etc/pkfs.conf");
    return;
  }

  const char *uri = NULL;
  config_lookup_string(cf, "uri", &uri);
  strncpy(config->uri, uri, MAX_CONFIG);

  const char *dn = NULL;
  config_lookup_string(cf, "dn", &dn);
  strncpy(config->dn, dn, MAX_CONFIG);

  const char *pass = NULL;
  config_lookup_string(cf, "pass", &pass);
  strncpy(config->pass, pass, MAX_CONFIG);

  const char *base = NULL;
  config_lookup_string(cf, "base", &base);
  strncpy(config->base, base, MAX_CONFIG);

  const char *key_attribute = NULL;
  config_lookup_string(cf, "key_attr", &key_attribute);
  strncpy(config->key_attribute, key_attribute, MAX_CONFIG);

  config_destroy(cf);
}

void uid_from_path(const char *path, char uid[])
{
  if (strncmp(path, "/", 1) == 0) {
    strncpy(uid, path + 1, UID_MAX - 1);
  } else {
    strncpy(uid, path, UID_MAX - 1);
  }
}

