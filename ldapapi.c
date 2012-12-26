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

#include "ldapapi.h"
#include "utils.h"


static void init_pubkeys_from_ldap_values(char **vals, pubkeys_t *pubkey)
{
  int total_size_of_keys = 0;

  for(int i=0; vals[i]; i++) {
    total_size_of_keys += strlen(vals[i]);
  }

  char *keys = calloc(1, total_size_of_keys);
  strncpy(keys, vals[0], strlen(vals[0]));

  for(int i=1; vals[i]; i++) {
    strncat(keys, vals[i], strlen(vals[i]));
  }

  pubkey->keys = strdup(keys);
  pubkey->size = strlen(keys);
  free(keys);
}

static LDAP *get_ldap_connection(pkfs_config_t *config)
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

int ldap_user_check(const char *uid, pkfs_config_t *config)
{
  int count = 0;
  LDAP *ldap_conn = NULL;
  LDAPMessage *result = NULL;

  char *attrs[] = { "uid", NULL };

  char filter[MAX_FILTER];
  sprintf(filter, "(uid=%s)", uid);

  ldap_conn = get_ldap_connection(config);

  ldap_search_ext_s(ldap_conn, config->base, LDAP_SCOPE_SUBTREE,
    filter, attrs, 0, NULL, NULL, NULL, 1, &result);

  count = ldap_count_entries(ldap_conn, result);
  if (count == -1) {
    return -1;
  }

  int res = count > 0 ? 0 : 1;

  ldap_msgfree(result);
  ldap_unbind(ldap_conn);
  return res;
}

int get_public_keys(const char *uid, pubkeys_t *pubkeys, pkfs_config_t *config)
{
  LDAP *ldap_conn = NULL;
  LDAPMessage *result = NULL;
  LDAPMessage *entry  = NULL;
  BerElement* ber = NULL;

  int ldap_error;

  char **vals;
  char *attr;
  char *attrs[] = { config->key_attr, NULL };

  char filter[MAX_FILTER];
  sprintf(filter, "(uid=%s)", uid);

  ldap_conn = get_ldap_connection(config);
  ldap_error = ldap_search_ext_s(ldap_conn, config->base, LDAP_SCOPE_SUBTREE,
                 filter, attrs, 0, NULL, NULL, NULL, 1, &result);

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
