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
#define _XOPEN_SOURCE 500
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


pkfs_config_t *config;

void *pkfs_init(struct fuse_conn_info *conn)
{
  config = calloc(1, sizeof(pkfs_config_t));
  initialize_config();
  return NULL;
}

static void initialize_directory_stats(struct stat **stbuf)
{
  (*stbuf)->st_mode = S_IFDIR | 0755;
  (*stbuf)->st_nlink = 2;
}

static void initialize_file_stats(struct stat **stbuf, int size)
{
  time_t current_time = time(NULL);

  (*stbuf)->st_size = size;
  (*stbuf)->st_uid = geteuid();
  (*stbuf)->st_gid = getegid();
  (*stbuf)->st_mode = S_IFREG | 0444;
  (*stbuf)->st_nlink = 1;
  (*stbuf)->st_ctime = current_time;
  (*stbuf)->st_mtime = current_time;
  (*stbuf)->st_atime = current_time;
}

int pkfs_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;
  char *uid = NULL;
  memset(stbuf, 0, sizeof(struct stat));

  uid_from_path(path, &uid);

  if (strcmp(path, "/") == 0) {
    initialize_directory_stats(&stbuf);
  } else if (ldap_user_check(uid) == 0) {
    pubkeys_t *pk = calloc(1, sizeof(pubkeys_t));
    int size = (get_public_keys(uid, pk) != 0) ? 0 : pk->size;
    initialize_file_stats(&stbuf, size);
    free(pk->keys);
    free(pk);
  } else {
    res = -ENOENT;;
  }

  free(uid);
  return res;
}

static int pkfs_open(const char *path, struct fuse_file_info *fi)
{
  int fd;
  char *pubkey_temp_file = NULL;
  char *uid = NULL;

  pubkeys_t *pk = calloc(1, sizeof(pubkeys_t));

  uid_from_path(path, &uid);

  if (get_public_keys(uid, pk) != 0) {
    free(pk);
    return -ENOENT;
  }

  pubkey_temp_file = strdup("/tmp/pkfs-XXXXXX");
  fd = mkstemp(pubkey_temp_file);

  write(fd, pk->keys, strlen(pk->keys));
  close(fd);
  fi->fh = (unsigned long)pubkey_temp_file;

  free(pk->keys);
  free(pk);
  free(uid);
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

static int pkfs_read(const char *path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi)
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

static int pkfs_write(const char* path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info* fi)
{
  return -EROFS;
}

static int pkfs_truncate(const char* path, off_t offset)
{
  return -EROFS;
}

static int pkfs_create(const char * path, mode_t mode,
             struct fuse_file_info * fi)
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
