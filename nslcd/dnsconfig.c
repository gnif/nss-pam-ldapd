/*
   dnsconfig.c - lookup code for DNS SRV records
   This file was part of the nss_ldap library which has been
   forked into the nss-ldapd library.

   Copyright (C) 1997-2005 Luke Howard

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

/*
 * Support DNS SRV records. I look up the SRV record for
 * _ldap._tcp.gnu.org.
 * and build the DN DC=gnu,DC=org.
 * Thanks to Assar & co for resolve.[ch].
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <string.h>
#ifdef HAVE_LBER_H
#include <lber.h>
#endif
#ifdef HAVE_LDAP_H
#include <ldap.h>
#endif

#include "ldap-nss.h"
#include "util.h"
#include "resolve.h"
#include "dnsconfig.h"

#define DC_ATTR                 "DC"
#define DC_ATTR_AVA             DC_ATTR"="
#define DC_ATTR_AVA_LEN         (sizeof(DC_ATTR_AVA) - 1)

/* map gnu.org into DC=gnu,DC=org */
static enum nss_status
_nss_ldap_getdnsdn (char *src_domain,
                    char **rval, char **buffer, size_t * buflen)
{
  char *p;
  int len = 0;
#ifdef HAVE_STRTOK_R
  char *st = NULL;
#endif
  char *bptr;
  char *domain, *domain_copy;

  /* we need to take a copy of domain, because strtok() modifies
   * it in place. Bad.
   */
  domain_copy = strdup (src_domain);
  if (domain_copy == NULL)
    {
      return NSS_STATUS_TRYAGAIN;
    }

  domain = domain_copy;

  bptr = *rval = *buffer;
  **rval = '\0';

#ifndef HAVE_STRTOK_R
  while ((p = strtok (domain, ".")))
#else
  while ((p = strtok_r (domain, ".", &st)))
#endif
    {
      len = strlen (p);

      if (*buflen < (size_t) (len + DC_ATTR_AVA_LEN + 1 /* D C = [,|\0] */ ))
        {
          free (domain_copy);
          return NSS_STATUS_TRYAGAIN;
        }

      if (domain == NULL)
        {
          strcpy (bptr, ",");
          bptr++;
        }
      else
        {
          domain = NULL;
        }

      strcpy (bptr, DC_ATTR_AVA);
      bptr += DC_ATTR_AVA_LEN;

      strcpy (bptr, p);
      bptr += len;              /* don't include comma */
      *buffer += len + DC_ATTR_AVA_LEN + 1;
      *buflen -= len + DC_ATTR_AVA_LEN + 1;
    }

  if (bptr != NULL)
    {
      (*rval)[bptr - *rval] = '\0';
    }

  free (domain_copy);

  return NSS_STATUS_SUCCESS;
}

enum nss_status
_nss_ldap_mergeconfigfromdns (struct ldap_config * result,
                              char **buffer, size_t *buflen)
{
  enum nss_status stat = NSS_STATUS_SUCCESS;
  struct dns_reply *r;
  struct resource_record *rr;
  char domain[MAXHOSTNAMELEN + 1];
  char *pDomain;
  char uribuf[1024];

  if ((_res.options & RES_INIT) == 0 && res_init () == -1)
    {
      return NSS_STATUS_UNAVAIL;
    }

  if (result->ldc_srv_domain != NULL)
    pDomain = result->ldc_srv_domain;
  else
    {
      snprintf (domain, sizeof (domain), "_ldap._tcp.%s.", _res.defdname);
      pDomain = domain;
    }

  r = dns_lookup (pDomain, "srv");
  if (r == NULL)
    {
      return NSS_STATUS_NOTFOUND;
    }

  /* XXX sort by priority */
  for (rr = r->head; rr != NULL; rr = rr->next)
    {
      if (rr->type == T_SRV)
        {
          snprintf (uribuf, sizeof(uribuf), "ldap%s:%s:%d",
            (rr->u.srv->port == LDAPS_PORT) ? "s" : "",
            rr->u.srv->target,
            rr->u.srv->port);

          stat = _nss_ldap_add_uri (result, uribuf, buffer, buflen);
          if (stat != NSS_STATUS_SUCCESS)
            {
              break;
            }
        }
    }

  dns_free_data (r);
  stat = NSS_STATUS_SUCCESS;

  if (result->ldc_base == NULL)
    {
      stat = _nss_ldap_getdnsdn (_res.defdname, &result->ldc_base,
                                 buffer, buflen);
    }

  return stat;
}
