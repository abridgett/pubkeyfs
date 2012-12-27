/*
 * Public Key File System (PKFS)
 *
 * Copyright (C) Kelsey Hightower, 2012
 */
#ifndef __PKFS_UTILS_H__
#define __PKFS_UTILS_H__

struct pkfs_config {
  const char *base;
  const char *dn;
  const char *uri;
  const char *pass;
  const char *key_attr;
  long timeout;
};

typedef struct pkfs_config pkfs_config_t;

void initialize_config(void);
void uid_from_path(const char *path, char **uid);

#endif
