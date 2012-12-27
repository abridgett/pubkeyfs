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

static void set_config_defaults(void);
static void set_config_from_file(config_t *cf);

extern pkfs_config_t *config;

void initialize_config(void)
{
  syslog(LOG_DEBUG, "Initializing pkfs config");

  config_t cf;
  config_init(&cf);

  if (config_read_file(&cf, PKFS_CONFIG_FILE) == CONFIG_FALSE) {
    syslog(LOG_ERR, "Cannot load %s", PKFS_CONFIG_FILE);
    return;
  }

  set_config_defaults();
  set_config_from_file(&cf);
}

void uid_from_path(const char *path, char **uid)
{
  if (strncmp(path, "/", 1) == 0) {
    *uid = strdup(path + 1);
  } else {
    *uid = strdup(path);
  }
}

/*============= Utility Functions ================================*/

static void set_config_defaults(void)
{
  config->uri = NULL;
  config->dn = NULL;
  config->pass = NULL;
  config->base = NULL;
  config->timeout = 30;
  config->key_attr = strdup("sshPublicKey");
}

static void set_config_from_file(config_t *cf)
{
  config_lookup_string(cf, "uri", &config->uri);
  config_lookup_string(cf, "dn", &config->dn);
  config_lookup_string(cf, "pass", &config->pass);
  config_lookup_string(cf, "base", &config->base);
  config_lookup_string(cf, "key_attr", &config->key_attr);
  config_lookup_int(cf, "timeout", &config->timeout);
}
