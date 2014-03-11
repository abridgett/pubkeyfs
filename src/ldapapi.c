/*
 * Public Key File System (PKFS)
 *
 * Copyright (C) Kelsey Hightower, 2012
 */
#include <ldap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/time.h>

#include "ldapapi.h"
#include "utils.h"

/* Global configuration */
extern pkfs_config_t *config;

static int get_ldap_connection(LDAP **ldap_conn);
static int get_ldap_results(LDAP *ldap_conn, char *uid, char *attr,
              LDAPMessage **results);
static void extract_pubkeys_from_ldap_values(struct berval **vals,
              pubkeys_t *pubkey);
static size_t calculate_total_length_of_keys(struct berval **vals);
static char *format_public_keys(struct berval **vals);
static void get_ldap_values(LDAP *ldap_conn, LDAPMessage *results,
              struct berval ***vals);


int ldap_user_check(char *uid)
{
  LDAP *ldap_conn = NULL;
  LDAPMessage *results = NULL;
  int res = 0;
  int count = 0;

  if ((res = get_ldap_connection(&ldap_conn)) != 0) {
    return res;
  }
  res = get_ldap_results(ldap_conn, uid, (char *)config->user_attr, &results);
  if (res == 0) {
    count = ldap_count_entries(ldap_conn, results);
  }
  ldap_msgfree(results);
  ldap_unbind_ext_s(ldap_conn, NULL, NULL);
  return count > 0 ? 0 : 1;
}

int get_public_keys(char *uid, pubkeys_t *pubkeys)
{
  LDAP *ldap_conn = NULL;
  LDAPMessage *results = NULL;
  struct berval **vals = NULL;
  int res = 0;

  if ((res = get_ldap_connection(&ldap_conn)) != 0) {
    return res;
  }
  res = get_ldap_results(ldap_conn, uid, (char *)config->key_attr, &results);
  if (res == 0) {
    get_ldap_values(ldap_conn, results, &vals);
    extract_pubkeys_from_ldap_values(vals, pubkeys);
    ldap_value_free_len(vals);
  }
  ldap_msgfree(results);
  ldap_unbind_ext_s(ldap_conn, NULL, NULL);
  return res;
}

void initialize_public_keys(pubkeys_t **pubkeys)
{
  *pubkeys = calloc(1, sizeof(pubkeys_t));
  (*pubkeys)->keys = NULL;
  (*pubkeys)->size = 0;
}

void destroy_public_keys(pubkeys_t *pubkeys)
{
  if (pubkeys->keys != NULL)
    free(pubkeys->keys);
  if (pubkeys != NULL)
    free(pubkeys);
}


//==== Utility Functions ====================================================
static int get_ldap_connection(LDAP **ldap_conn)
{
  int ldap_error;
  int desired_version = LDAP_VERSION3;
  struct berval cred;

  cred.bv_val = (char *)config->pass;
  cred.bv_len = strlen(config->pass);

  ldap_initialize(ldap_conn, config->uri);
  ldap_set_option(*ldap_conn, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
  ldap_error = ldap_sasl_bind_s(*ldap_conn, config->dn, LDAP_SASL_SIMPLE,
                 &cred, NULL, NULL, NULL);

  if (ldap_error < 0) {
    syslog(LOG_ERR, "LDAP connection error");
  }
  return ldap_error;
}

static int get_ldap_results(LDAP *ldap_conn, char *uid, char *attr,
  LDAPMessage **results)
{
  struct timeval timeout;
  timeout.tv_sec = config->timeout;
  timeout.tv_usec = 0;

  int ldap_error;
  char *attrs[] = { attr, NULL };

  char filter[MAX_FILTER];
  sprintf(filter, "(%s=%s)", config->user_attr, uid);

  ldap_error = ldap_search_ext_s(ldap_conn, config->base, LDAP_SCOPE_SUBTREE,
                 filter, attrs, 0, NULL, NULL, &timeout, 1, results);

  if (ldap_error != 0) {
    syslog(LOG_ERR, "%s", ldap_err2string(ldap_error));
  }
  return ldap_error;
}

static void extract_pubkeys_from_ldap_values(struct berval **vals,
  pubkeys_t *pubkey)
{
  char *keys = format_public_keys(vals);
  pubkey->keys = strdup(keys);
  pubkey->size = strlen(keys);
  free(keys);
}

static char *format_public_keys(struct berval **vals)
{
  size_t length = 0;
  char *keys = calloc(1, calculate_total_length_of_keys(vals));
  keys[0] = '\0';

  for(int i = 0; vals[i]; i++) {
    strcat(keys, vals[i]->bv_val);
    length = strlen(keys);

    if (keys[length - 1] != '\n') {
       keys[length] = '\n';
       keys[length + 1] = '\0';
    }
  }
  return keys;
}

static size_t calculate_total_length_of_keys(struct berval **vals)
{
  size_t total = 0;

  for(int i = 0; vals[i]; i++) {
    char *key = vals[i]->bv_val;
    size_t length = vals[i]->bv_len;

    total += length;
    if (key[length - 1] != '\n') {
      ++total; // Add room for newline
    }
  }
  ++total; // Add room for final '\0'
  return total;
}

static void get_ldap_values(LDAP *ldap_conn, LDAPMessage *results,
  struct berval ***vals)
{
  char *attr = NULL;
  LDAPMessage *entry = NULL;
  BerElement* ber = NULL;

  entry = ldap_first_entry(ldap_conn, results);
  attr  = ldap_first_attribute(ldap_conn, entry, &ber);
  *vals = ldap_get_values_len(ldap_conn, entry, attr);

  ber_free(ber, 0);
  ldap_memfree(attr);
}
