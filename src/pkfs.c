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
static void cache_pubkeys_on_disk(pubkeys_t *pk, uint64_t *fh);
static void initialize_directory_stats(struct stat **stbuf);
static void initialize_file_stats(struct stat **stbuf, int size);


void *pkfs_init(struct fuse_conn_info *conn)
{
  config = calloc(1, sizeof(pkfs_config_t));
  initialize_config();
  return NULL;
}

int pkfs_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;
  char *uid = NULL;
  pubkeys_t *pk = NULL;
  memset(stbuf, 0, sizeof(struct stat));

  uid_from_path(path, &uid);

  if (strcmp(path, "/") == 0) {
    initialize_directory_stats(&stbuf);
  } else if (ldap_user_check(uid) == 0) {
    initialize_public_keys(&pk);
    int size = (get_public_keys(uid, pk) != 0) ? 0 : pk->size;
    initialize_file_stats(&stbuf, size);
    destroy_public_keys(pk);
  } else {
    res = -ENOENT;
  }

  free(uid);
  return res;
}

static int pkfs_open(const char *path, struct fuse_file_info *fi)
{
  int res = 0;
  char *uid = NULL;
  pubkeys_t *pk = NULL;

  uid_from_path(path, &uid);
  initialize_public_keys(&pk);

  if (get_public_keys(uid, pk) == 0) {
    cache_pubkeys_on_disk(pk, &(fi->fh));
  } else {
    res = -ENOENT;
  }

  destroy_public_keys(pk);
  free(uid);
  return res;
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

//==== Utility Functions ====================================================
static void cache_pubkeys_on_disk(pubkeys_t *pk, uint64_t *fh)
{
  int fd;
  char *tempfile = NULL;

  tempfile = strdup("/tmp/pkfs-XXXXXX");
  fd = mkstemp(tempfile);
  write(fd, pk->keys, pk->size);
  close(fd);
  *fh = (unsigned long)tempfile;
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

