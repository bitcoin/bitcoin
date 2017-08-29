/*
 * Copyright 2015 MongoDB, Inc.
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

#include <bson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <getopt.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif


#define DEFAULT_PORT "5000"
#define DEFAULT_HOST "localhost"


/*
 * bson-streaming-remote-open --
 *
 *       Makes a connection to the specified host and port over TCP. This
 *       abstracts away most of the socket details required to make a
 *       connection.
 *
 * Parameters:
 *       @hostname: The name of the host to connect to, or NULL.
 *       @port: The port number of the server on the host, or NULL.
 *
 * Returns:
 *       A valid file descriptor ready for reading on success.
 *       -1 on failure.
 */
int
bson_streaming_remote_open (const char *hostname, const char *port)
{
   int error, sock;
   struct addrinfo hints, *ptr, *server_list;

   /*
    * Look up the host's address information, hinting that we'd like to use a
    * TCP/IP connection.
    */
   memset (&hints, 0, sizeof hints);
   hints.ai_family = PF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;
   error =
      getaddrinfo ((!hostname || !strlen (hostname)) ? DEFAULT_HOST : hostname,
                   (!port || !strlen (port)) ? DEFAULT_PORT : port,
                   &hints,
                   &server_list);

   if (error) {
      fprintf (stderr,
               "bson-streaming-remote-open: Failed to get server info: %s\n",
               gai_strerror (error));
      return -1;
   }

   /*
    * Iterate through the results of getaddrinfo, attempting to connect to the
    * first possible result.
    */
   for (ptr = server_list; ptr != NULL; ptr = ptr->ai_next) {
      sock = socket (ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

      if (sock < 0) {
         perror ("bson-streaming-remote-open: socket creation failed");
         continue;
      }

      if (connect (sock, ptr->ai_addr, ptr->ai_addrlen) < 0) {
         close (sock);
         perror ("bson-streaming-remote-open: connect failure");
         continue;
      }

      /*
       * Connection success.
       */
      break;
   }

   freeaddrinfo (server_list);
   if (ptr == NULL) {
      fprintf (stderr,
               "bson-streaming-remote-open: failed to connect to server\n");
      return -1;
   }

   return sock;
}


/*
 * main --
 *
 *       Connects to a server and reads BSON from it. This program takes the
 *       following command line options:
 *
 *          -h              Print this help and exit.
 *          -s SERVER_NAME  Specify the host name of the server.
 *          -p PORT_NUM     Specify the port number to connect to on the server.
 */
int
main (int argc, char *argv[])
{
   bson_reader_t *reader;
   char *hostname = NULL;
   char *json;
   char *port = NULL;
   const bson_t *document;
   int fd;
   int opt;

   opterr = 1;

   /*
    * Parse command line arguments.
    */
   while ((opt = getopt (argc, argv, "hs:p:")) != -1) {
      switch (opt) {
      case 'h':
         fprintf (
            stdout, "Usage: %s [-s SERVER_NAME] [-p PORT_NUM]\n", argv[0]);
         free (hostname);
         free (port);
         return EXIT_SUCCESS;
      case 'p':
         free (port);
         port = (char *) malloc (strlen (optarg) + 1);
         strcpy (port, optarg);
         break;
      case 's':
         free (hostname);
         hostname = (char *) malloc (strlen (optarg) + 1);
         strcpy (hostname, optarg);
         break;
      default:
         fprintf (stderr, "Unknown option: %s\n", optarg);
      }
   }

   /*
    * Open a file descriptor on the remote and read in BSON documents, one by
    * one. As an example of processing, this prints the incoming documents as
    * JSON onto STDOUT.
    */
   fd = bson_streaming_remote_open (hostname, port);
   if (fd == -1) {
      free (hostname);
      free (port);
      return EXIT_FAILURE;
   }

   reader = bson_reader_new_from_fd (fd, true);
   while ((document = bson_reader_read (reader, NULL))) {
      json = bson_as_canonical_extended_json (document, NULL);
      fprintf (stdout, "%s\n", json);
      bson_free (json);
   }

   /*
    * Destroy the reader, which performs cleanup. The ``true'' argument passed
    * to bson_reader_new_from_fd tells libbson to close the file descriptor on
    * destruction.
    */
   bson_reader_destroy (reader);
   free (hostname);
   free (port);
   return EXIT_SUCCESS;
}
