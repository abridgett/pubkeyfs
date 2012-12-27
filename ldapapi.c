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

static void get_ldap_connection(LDAP **ldap_conn);
static void get_ldap_results(LDAP *ldap_conn, const char *uid, const char *attr,
              LDAPMessage **results);
static void init_pubkeys_from_ldap_values(char *vals[], pubkeys_t *pubkey);
static size_t calculate_total_length_of_keys(char *vals[]);
static char *format_public_keys(char *vals[]);


int ldap_user_check(const char *uid)
{
  LDAP *ldap_conn = NULL;
  LDAPMessage *results = NULL;

  get_ldap_connection(&ldap_conn);
  get_ldap_results(ldap_conn, uid, "uid", &results);
  int count = ldap_count_entries(ldap_conn, results);

  ldap_msgfree(results);
  ldap_unbind(ldap_conn);
  return count > 0 ? 0 : 1;
}

int get_public_keys(const char *uid, pubkeys_t *pubkeys)
{
  LDAP *ldap_conn = NULL;
  LDAPMessage *results = NULL;
  LDAPMessage *entry  = NULL;
  BerElement* ber = NULL;

  get_ldap_connection(&ldap_conn);
  get_ldap_results(ldap_conn, uid, config->key_attr, &results);
  entry = ldap_first_entry(ldap_conn, results);
  char *attr = ldap_first_attribute(ldap_conn, entry, &ber);
  char **vals = (char **)ldap_get_values(ldap_conn, entry, attr);

  init_pubkeys_from_ldap_values(vals, pubkeys);

  ber_free(ber, 0);
  ldap_memfree(attr);
  ldap_value_free(vals);
  ldap_msgfree(results);
  ldap_unbind(ldap_conn);
  return 0;
}


//==== Utility Functions ====================================================

static void get_ldap_connection(LDAP **ldap_conn)
{
  int ldap_error;
  int desired_version = LDAP_VERSION3;

  ldap_initialize(ldap_conn, config->uri);
  ldap_set_option(*ldap_conn, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
  ldap_error = ldap_bind_s(*ldap_conn, config->dn, config->pass,
                 LDAP_AUTH_SIMPLE);

  if (ldap_error < 0) {
    syslog(LOG_ERR, "LDAP connection error");
  }
}

static void get_ldap_results(LDAP *ldap_conn, const char *uid, const char *attr,
                            LDAPMessage **results)
{
  struct timeval timeout;
  timeout.tv_sec = config->timeout;
  timeout.tv_usec = 0;

  int ldap_error;
  char *attrs[] = { attr, NULL };

  char filter[MAX_FILTER];
  sprintf(filter, "(uid=%s)", uid);

  ldap_error = ldap_search_ext_s(ldap_conn, config->base, LDAP_SCOPE_SUBTREE,
                 filter, attrs, 0, NULL, NULL, &timeout, 1, results);

  if (ldap_error != 0) {
    syslog(LOG_ERR, "%s", ldap_err2string(ldap_error));
  }
}

static void init_pubkeys_from_ldap_values(char *vals[], pubkeys_t *pubkey)
{
  char *keys = format_public_keys(vals);
  pubkey->keys = strdup(keys);
  pubkey->size = strlen(keys);
  free(keys);
}

static char *format_public_keys(char *vals[])
{
  size_t length = 0;
  char *keys = calloc(1, calculate_total_length_of_keys(vals));
  keys[0] = '\0';

  for(int i = 0; vals[i]; i++) {
    strcat(keys, vals[i]);
    length = strlen(keys);

    if (keys[length - 1] != '\n') {
       keys[length] = '\n';
       keys[length + 1] = '\0';
    }
  }
  return keys;
}

static size_t calculate_total_length_of_keys(char *vals[])
{
  size_t total = 0;

  for(int i = 0; vals[i]; i++) {
    total += strlen(vals[i]);
    if (vals[i][strlen(vals[i]) - 1] != '\n') {
      ++total; // Add room for newline
    }
  }
  ++total; // Add room for final '\0'
  return total;
}