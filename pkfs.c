/*
 Public Key File System (PKFS)
 Copyright (C) 2012 Kelsey Hightower <kelsey.hightower@gmail.com>

 This program can be distributed under the terms of the MIT license.

-----------  ----    ---- ------------ ------------
************ ****   ****  ************ ************
---      --- ----  ----   ----         ----
************ *********    ************ ************
-----------  ---------    ------------ ------------
****         ****  ****   ****                *****
----         ----   ----  ----         ------------
****         ****    **** ****         ************

*/
#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <ldap.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "utils.h"
#include "ldapapi.h"

void *pkfs_init(struct fuse_conn_info *conn)
{
  struct pkfs_config *config;
  config = malloc(sizeof(struct pkfs_config));
  initialize_config(config);

  return config;
}

int pkfs_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;
  char *uid = NULL;
  memset(stbuf, 0, sizeof(struct stat));

  uid_from_path(path, &uid);

  // Gather LDAP connection details
  struct fuse_context *fc = fuse_get_context();
  struct pkfs_config *config = fc->private_data;

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else if (ldap_user_check(uid, config) == 0) {
    pubkeys_t *publickey = malloc(sizeof(pubkeys_t));

    if (get_public_keys(uid, publickey, config) != 0) {
      stbuf->st_size = 0;
    } else {
      stbuf->st_size = publickey->size;
    }

    free(publickey);
    time_t current_time = time(NULL);

    stbuf->st_uid = geteuid();
    stbuf->st_gid = getegid();
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_ctime = current_time;
    stbuf->st_mtime = current_time;
    stbuf->st_atime = current_time;
  } else {
    res = -ENOENT;;
  }

  return res;
}

static int pkfs_open(const char *path, struct fuse_file_info *fi)
{
  int fd;
  char *pubkey_temp_file = NULL;
  char *uid = NULL;

  struct fuse_context *fc = fuse_get_context();
  pkfs_config_t *config = fc->private_data;
  pubkeys_t *publickey = malloc(sizeof(pubkeys_t));

  uid_from_path(path, &uid);

  if (get_public_keys(uid, publickey, config) != 0) {
    free(publickey);
    return -ENOENT;
  }

  pubkey_temp_file = strdup("/tmp/pkfs-XXXXXX");
  fd = mkstemp(pubkey_temp_file);

  write(fd, publickey->keys, strlen(publickey->keys));
  fi->fh = (unsigned long)pubkey_temp_file;
  free(publickey);

  return 0;
}

static int pkfs_flush(const char *path, struct fuse_file_info *fi)
{
  return 0;
}

static int pkfs_release(const char *path, struct fuse_file_info *fi)
{
  unlink(CAST_PATH fi->fh);
  free(CAST_PATH fi->fh);
  return 0;
}

static int pkfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  int fd, res;

  fd = open(CAST_PATH fi->fh, O_RDONLY);
  if (fd == -1) return -errno;

  res = pread(fd, buf, size, offset);
  if (res == -1) res = -errno;

  close(fd);
  return res;
}

static int pkfs_access(const char *path, int mask)
{
  return 0;
}

static int pkfs_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
  return -EROFS;
}

static int pkfs_truncate(const char* path, off_t offset)
{
  return -EROFS;
}

static int pkfs_create(const char * path, mode_t mode, struct fuse_file_info * fi)
{
  return -EROFS;
}

static struct fuse_operations pkfs_oper = {
  .init     = pkfs_init,
  .getattr  = pkfs_getattr,
  .read     = pkfs_read,
  .open     = pkfs_open,
  .flush    = pkfs_flush,
  .release  = pkfs_release,
  .access   = pkfs_access,
  .write    = pkfs_write,
  .truncate = pkfs_truncate,
  .create   = pkfs_create,
};

int main(int argc, char *argv[])
{
  return fuse_main(argc, argv, &pkfs_oper, NULL);
}