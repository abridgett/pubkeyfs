/*
 * Public Key File System (PKFS)
 * Copyright (C) 2012 Kelsey Hightower <kelsey.hightower@gmail.com>
 *
 * This program can be distributed under the terms of the MIT license.
 *
 */
#ifndef __PKFS_UTILS_H__
#define __PKFS_UTILS_H__

#define UID_MAX 128
#define MAX_CONFIG 256

struct pkfs_config {
  char base[MAX_CONFIG];
  char dn[MAX_CONFIG];
  char uri[MAX_CONFIG];
  char pass[MAX_CONFIG];
  char key_attribute[MAX_CONFIG];
};

void initialize_config(struct pkfs_config *config);

void uid_from_path(const char *path, char uid[]);

#endif
