/*
 * Public Key File System (PKFS)
 *
 * Copyright (C) Kelsey Hightower, 2012
 */
#ifndef __PKFS_UTILS_H__
#define __PKFS_UTILS_H__

#define MAX_UID 128

struct pkfs_config {
  char *base;
  char *dn;
  char *uri;
  char *pass;
  char *key_attr;
};

typedef struct pkfs_config pkfs_config_t;

void initialize_config(pkfs_config_t *config);
void uid_from_path(const char *path, char **uid);

#endif