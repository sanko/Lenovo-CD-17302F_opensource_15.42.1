/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2013 Christian Grothoff

     libmicrohttpd is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     libmicrohttpd is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libmicrohttpd; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

/**
 * @file test_process_arguments.c
 * @brief  Testcase for HTTP URI arguments
 * @author Christian Grothoff
 */

#include "MHD_config.h"
#include "platform.h"
#include <curl/curl.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WINDOWS
#include <unistd.h>
#endif

static int oneone;

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};


static size_t
copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx)
{
  struct CBC *cbc = ctx;

  if (cbc->pos + size * nmemb > cbc->size)
    return 0;                   /* overflow */
  memcpy (&cbc->buf[cbc->pos], ptr, size * nmemb);
  cbc->pos += size * nmemb;
  return size * nmemb;
}


static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size,
          void **unused)
{
  static int ptr;
  const char *me = cls;
  struct MHD_Response *response;
  int ret;
  const char *hdr;

  if (0 != strcmp (me, method))
    return MHD_NO;              /* unexpected method */
  if (&ptr != *unused)
    {
      *unused = &ptr;
      return MHD_YES;
    }
  *unused = NULL;
  hdr = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "k");
  if ((hdr == NULL) || (0 != strcmp (hdr, "v x")))
    abort ();
  hdr = MHD_lookup_connection_value (connection,
                                     MHD_GET_ARGUMENT_KIND, "hash");
  if ((hdr == NULL) || (0 != strcmp (hdr, "#foo")))
    abort ();
  hdr = MHD_lookup_connection_value (connection,
                                     MHD_GET_ARGUMENT_KIND, "space");
  if ((hdr == NULL) || (0 != strcmp (hdr, "\240bar")))
    abort ();
  if (3 != MHD_get_connection_values (connection,
				      MHD_GET_ARGUMENT_KIND,
				      NULL, NULL))
    abort ();
  response = MHD_create_response_from_buffer (strlen (url),
					      (void *) url,
					      MHD_RESPMEM_MUST_COPY);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  if (ret == MHD_NO)
    abort ();
  return ret;
}


static int
testExternalGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLM *multi;
  CURLMcode mret;
  fd_set rs;
  fd_set ws;
  fd_set es;
  MHD_socket max;
  int running;
  struct CURLMsg *msg;
  time_t start;
  struct timeval tv;

  multi = NULL;
  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_DEBUG,
                        21080, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 256;
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL,
                    "http://127.0.0.1:21080/hello+world?k=v+x&hash=%23foo&space=%A0bar");
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
  if (oneone)
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);


  multi = curl_multi_init ();
  if (multi == NULL)
    {
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 512;
    }
  mret = curl_multi_add_handle (multi, c);
  if (mret != CURLM_OK)
    {
      curl_multi_cleanup (multi);
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 1024;
    }
  start = time (NULL);
  while ((time (NULL) - start < 5) && (multi != NULL))
    {
      max = 0;
      FD_ZERO (&rs);
      FD_ZERO (&ws);
      FD_ZERO (&es);
      curl_multi_perform (multi, &running);
      mret = curl_multi_fdset (multi, &rs, &ws, &es, &max);
      if (mret != CURLM_OK)
        {
          curl_multi_remove_handle (multi, c);
          curl_multi_cleanup (multi);
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 2048;
        }
      if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &max))
        {
          curl_multi_remove_handle (multi, c);
          curl_multi_cleanup (multi);
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 4096;
        }
      tv.tv_sec = 0;
      tv.tv_usec = 1000;
      select (max + 1, &rs, &ws, &es, &tv);
      curl_multi_perform (multi, &running);
      if (running == 0)
        {
          msg = curl_multi_info_read (multi, &running);
          if (msg == NULL)
            break;
          if (msg->msg == CURLMSG_DONE)
            {
              if (msg->data.result != CURLE_OK)
                printf ("%s failed at %s:%d: `%s'\n",
                        "curl_multi_perform",
                        __FILE__,
                        __LINE__, curl_easy_strerror (msg->data.result));
              curl_multi_remove_handle (multi, c);
              curl_multi_cleanup (multi);
              curl_easy_cleanup (c);
              c = NULL;
              multi = NULL;
            }
        }
      MHD_run (d);
    }
  if (multi != NULL)
    {
      curl_multi_remove_handle (multi, c);
      curl_easy_cleanup (c);
      curl_multi_cleanup (multi);
    }
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello+world"))
    return 8192;
  if (0 != strncmp ("/hello+world", cbc.buf, strlen ("/hello+world")))
    return 16384;
  return 0;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;

  oneone = (NULL != strrchr (argv[0], (int) '/')) ?
    (NULL != strstr (strrchr (argv[0], (int) '/'), "11")) : 0;
  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  errorCount += testExternalGet ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
