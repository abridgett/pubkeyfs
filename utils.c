/*
 * Public Key File System (PKFS)
 *
 * Copyright (C) Kelsey Hightower, 2012
 */
#include <libconfig.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include "utils.h"

#define PKFS_CONFIG_FILE "/etc/pkfs.conf"


static void set_config_value(config_t *cf, const char *key, char **value)
{
  const char *s = NULL;
  config_lookup_string(cf, key, &s);
  *value = strdup(s);
}

void initialize_config(pkfs_config_t *pkfs_config)
{
  syslog(LOG_DEBUG, "Initializing pkfs config");

  config_t config;
  config_t *cf = &config;
  config_init(cf);

  if (config_read_file(cf, PKFS_CONFIG_FILE) == CONFIG_FALSE) {
    syslog(LOG_ERR, "Cannot load %s", PKFS_CONFIG_FILE);
    return;
  }

  set_config_value(cf, "uri",  &pkfs_config->uri);
  set_config_value(cf, "dn",   &pkfs_config->dn);
  set_config_value(cf, "pass", &pkfs_config->pass);
  set_config_value(cf, "base", &pkfs_config->base);
  set_config_value(cf, "key_attr", &pkfs_config->key_attr);

  config_destroy(cf);
}

void uid_from_path(const char *path, char **uid)
{
  if (strncmp(path, "/", 1) == 0) {
    *uid = strdup(path + 1);
  } else {
    *uid = strdup(path);
  }
}