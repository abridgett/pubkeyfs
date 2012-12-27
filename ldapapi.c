/*
 * Public Key File System (PKFS)
 *
 * Copyright (C) Kelsey Hightower, 2012
 */
#include <ldap.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ldapapi.h"
#include "utils.h"

extern pkfs_config_t *config;

static void init_pubkeys_from_ldap_values(char *vals[], pubkeys_t *pubkey)
{
  char *keys = NULL;
  size_t length = 0;
  size_t total_length_of_keys = 0;

  for(int i = 0; vals[i]; i++) {
    total_length_of_keys += strlen(vals[i]);
    if (vals[i][strlen(vals[i]) - 1] != '\n') {
      ++total_length_of_keys; // Add room for newline
    }
  }
  ++total_length_of_keys; // Add room for final '\0'

  keys = malloc(total_length_of_keys);
  keys[0] = '\0';

  for(int i = 0; vals[i]; i++) {
    strcat(keys, vals[i]);
    length = strlen(keys);

    if (keys[length - 1] != '\n') {
       keys[length] = '\n';
       keys[length + 1] = '\0';
    }
  }

  pubkey->keys = strdup(keys);
  pubkey->size = strlen(keys);
  free(keys);
}

static LDAP *get_ldap_connection(void)
{
  int ldap_error;
  int desired_version = LDAP_VERSION3;
  LDAP *ldap_conn = NULL;

  ldap_initialize(&ldap_conn, config->uri);
  ldap_set_option(ldap_conn, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
  ldap_error = ldap_bind_s(ldap_conn, config->dn, config->pass,
                 LDAP_AUTH_SIMPLE);

  if (ldap_error < 0) {
    syslog(LOG_ERR, "LDAP connection error");
    return NULL;
  }

  return ldap_conn;
}

int ldap_user_check(const char *uid)
{
  int count = 0;

  struct timeval timeout;
  timeout.tv_sec = config->timeout;
  timeout.tv_usec = 0;

  int ldap_error;
  LDAP *ldap_conn = NULL;
  LDAPMessage *result = NULL;

  char *attrs[] = { "uid", NULL };

  char filter[MAX_FILTER];
  sprintf(filter, "(uid=%s)", uid);

  ldap_conn = get_ldap_connection();
  ldap_error = ldap_search_ext_s(ldap_conn, config->base, LDAP_SCOPE_SUBTREE,
                 filter, attrs, 0, NULL, NULL, &timeout, 1, &result);

  if (ldap_error != 0) {
    syslog(LOG_ERR, "%s", ldap_err2string(ldap_error));
    ldap_msgfree(result);
    return -1;
  }
  count = ldap_count_entries(ldap_conn, result);

  int res = count > 0 ? 0 : 1;

  ldap_msgfree(result);
  ldap_unbind(ldap_conn);
  return res;
}

int get_public_keys(const char *uid, pubkeys_t *pubkeys)
{
  LDAP *ldap_conn = NULL;
  LDAPMessage *result = NULL;
  LDAPMessage *entry  = NULL;
  BerElement* ber = NULL;

  struct timeval timeout;
  timeout.tv_sec = config->timeout;
  timeout.tv_usec = 0;

  int ldap_error;

  char **vals;
  char *attr;
  char *attrs[] = { config->key_attr, NULL };

  char filter[MAX_FILTER];
  sprintf(filter, "(uid=%s)", uid);

  ldap_conn = get_ldap_connection();
  ldap_error = ldap_search_ext_s(ldap_conn, config->base, LDAP_SCOPE_SUBTREE,
                 filter, attrs, 0, NULL, NULL, &timeout, 1, &result);

  if (ldap_error != 0) {
    syslog(LOG_ERR, "%s", ldap_err2string(ldap_error));
    ldap_unbind(ldap_conn);
    return -1;
  }

  entry = ldap_first_entry(ldap_conn, result);
  attr  = ldap_first_attribute(ldap_conn, entry, &ber);
  vals  = (char **)ldap_get_values(ldap_conn, entry, attr);

  init_pubkeys_from_ldap_values(vals, pubkeys);

  ber_free(ber, 0);
  ldap_memfree(attr);
  ldap_value_free(vals);
  ldap_msgfree(result);
  ldap_unbind(ldap_conn);
  return 0;
}
