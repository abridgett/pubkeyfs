/*
 * Public Key File System (PKFS)
 * Copyright (C) 2012 Kelsey Hightower <kelsey.hightower@gmail.com>
 *
 * This program can be distributed under the terms of the MIT license.
 *
 */
#include <ldap.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "ldapapi.h"
#include "utils.h"

int get_public_key(const char *uid, struct pkfs_pubkey *pubkey, struct pkfs_config *config)
{
  LDAPMessage *result = NULL;
  LDAPMessage *entry = NULL;
  LDAP *ldap_conn = NULL;
  BerElement* ber = NULL;
  
  int ldap_error;
  int desired_version = LDAP_VERSION3;

  char** vals;
  char* attr;
  char *attrs[] = {
    config->key_attribute,
    NULL
  };

  char filter[MAX_FILTER];
  sprintf(filter, "(uid=%s)", uid);

  if (ldap_initialize(&ldap_conn, config->uri) != LDAP_SUCCESS) {
    return 1; 
  } else {
    ldap_set_option(ldap_conn, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
    ldap_bind_s(ldap_conn, config->dn, config->pass, LDAP_AUTH_SIMPLE);
  }

  ldap_error = ldap_search_ext_s(ldap_conn, config->base, LDAP_SCOPE_SUBTREE,
                                 filter, attrs, 0, NULL, NULL, NULL, 1, &result);

  if (ldap_error != 0) {
    syslog(LOG_ERR, "%s", ldap_err2string(ldap_error));
    ldap_msgfree(result);
    return -1;
  } else {
    entry = ldap_first_entry(ldap_conn, result);
    attr  = ldap_first_attribute(ldap_conn, entry, &ber);
    vals  = (char **)ldap_get_values(ldap_conn, entry, attr);

    pubkey->key = strdup(vals[0]);
    pubkey->size = strlen(vals[0]);
  
    ldap_msgfree(result);
    return 0;
  }
}

int ldap_user_check(const char *uid, struct pkfs_config *config)
{
  LDAP *ldap_conn = NULL;
  LDAPMessage *result = NULL;

  int count;
  int desired_version = LDAP_VERSION3;

  char *attrs[] = {
    "uid",
    NULL
  };

  char filter[MAX_FILTER];
  sprintf(filter, "(uid=%s)", uid);

  if (ldap_initialize(&ldap_conn, config->uri) != LDAP_SUCCESS) {
    return -1; 
  } else {
    ldap_set_option(ldap_conn, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
    ldap_bind_s(ldap_conn, config->dn, config->pass, LDAP_AUTH_SIMPLE);
  }

  ldap_search_ext_s(ldap_conn, config->base, LDAP_SCOPE_SUBTREE,
                    filter, attrs, 0, NULL, NULL, NULL, 1, &result);

  count = ldap_count_entries(ldap_conn, result);
  if (count == -1) {
    return -1;
  }

  return count > 0 ? 0 : 1;
}
