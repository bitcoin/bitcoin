/*
 * Copyright 2015 MongoDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mongoc-host-list-private.h"
/* strcasecmp on windows */
#include "mongoc-util-private.h"


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_host_list_equal --
 *
 *       Check two hosts have the same domain (case-insensitive), port,
 *       and address family.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_host_list_equal (const mongoc_host_list_t *host_a,
                         const mongoc_host_list_t *host_b)
{
   return (!strcasecmp (host_a->host_and_port, host_b->host_and_port) &&
           host_a->family == host_b->family);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_host_list_destroy_all --
 *
 *       Destroy whole linked list of hosts.
 *
 *--------------------------------------------------------------------------
 */
void
_mongoc_host_list_destroy_all (mongoc_host_list_t *host)
{
   mongoc_host_list_t *tmp;

   while (host) {
      tmp = host->next;
      bson_free (host);
      host = tmp;
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_host_list_from_string --
 *
 *       Populate a mongoc_host_list_t from a fully qualified address
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_host_list_from_string (mongoc_host_list_t *link_, const char *address)
{
   char *close_bracket;
   bool bracket_at_end;
   char *sport;
   uint16_t port;

   if (*address == '\0') {
      MONGOC_ERROR ("empty address in _mongoc_host_list_from_string");
      BSON_ASSERT (false);
   }

   close_bracket = strchr (address, ']');
   bracket_at_end = close_bracket && *(close_bracket + 1) == '\0';
   sport = strrchr (address, ':');

   if (sport < close_bracket || (close_bracket && sport != close_bracket + 1)) {
      /* ignore colons within IPv6 address like "[fe80::1]" */
      sport = NULL;
   }

   /* like "example.com:27019" or "[fe80::1]:27019", but not "[fe80::1]" */
   if (sport) {
      if (!mongoc_parse_port (&port, sport + 1)) {
         return false;
      }

      link_->port = port;
   } else {
      link_->port = MONGOC_DEFAULT_PORT;
   }

   /* like "[fe80::1]:27019" or ""[fe80::1]" */
   if (*address == '[' && (bracket_at_end || (close_bracket && sport))) {
      link_->family = AF_INET6;
      bson_strncpy (link_->host,
                    address + 1,
                    BSON_MIN (close_bracket - address, sizeof link_->host));
      mongoc_lowercase (link_->host, link_->host);
      bson_snprintf (link_->host_and_port,
                     sizeof link_->host_and_port,
                     "[%s]:%hu",
                     link_->host,
                     link_->port);

   } else if (strchr (address, '/') && strstr (address, ".sock")) {
      link_->family = AF_UNIX;

      if (sport) {
         /* weird: "/tmp/mongodb.sock:1234", ignore the port number */
         bson_strncpy (link_->host,
                       address,
                       BSON_MIN (sport - address + 1, sizeof link_->host));
      } else {
         bson_strncpy (link_->host, address, sizeof link_->host);
      }

      bson_strncpy (
         link_->host_and_port, link_->host, sizeof link_->host_and_port);
   } else if (sport == address) {
      /* bad address like ":27017" */
      return false;
   } else {
      link_->family = AF_INET;

      if (sport) {
         bson_strncpy (link_->host,
                       address,
                       BSON_MIN (sport - address + 1, sizeof link_->host));
      } else {
         bson_strncpy (link_->host, address, sizeof link_->host);
      }

      mongoc_lowercase (link_->host, link_->host);
      bson_snprintf (link_->host_and_port,
                     sizeof link_->host_and_port,
                     "%s:%hu",
                     link_->host,
                     link_->port);
   }

   link_->next = NULL;
   return true;
}
