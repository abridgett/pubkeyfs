/*
 * Public Key File System (PKFS)
 * Copyright (C) 2012 Kelsey Hightower <kelsey.hightower@gmail.com>
 *
 * This program can be distributed under the terms of the MIT license.
 *
 */
#ifndef __PKFS_LDAPAPI_H__
#define __PKFS_LDAPAPI_H__

#include "utils.h"

#define MAX_FILTER 256
#define CAST_PATH (char*)(ptrdiff_t)

struct pkfs_pubkey {
  char *key;
  int size;
};

int ldap_user_check(const char *uid, struct pkfs_config *config);

int get_public_key(const char *uid, struct pkfs_pubkey *pubkey, struct pkfs_config *config);

#endif
