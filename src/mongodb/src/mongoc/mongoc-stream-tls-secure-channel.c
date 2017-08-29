/*
 * Copyright 2016 MongoDB, Inc.
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

/**
 * Significant portion of this file, such as
 * _mongoc_stream_tls_secure_channel_write &
 *_mongoc_stream_tls_secure_channel_read
 * comes straight from one of my favorite projects, cURL!
 * Thank you so much for having gone through the Secure Channel pain for me.
 *
 *
 * Copyright (C) 2012 - 2015, Marc Hoersken, <info@marc-hoersken.de>
 * Copyright (C) 2012, Mark Salisbury, <mark.salisbury@hp.com>
 * Copyright (C) 2012 - 2015, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/*
 * Based upon the PolarSSL implementation in polarssl.c and polarssl.h:
 *   Copyright (C) 2010, 2011, Hoi-Ho Chan, <hoiho.chan@gmail.com>
 *
 * Based upon the CyaSSL implementation in cyassl.c and cyassl.h:
 *   Copyright (C) 1998 - 2012, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * Thanks for code and inspiration!
 */

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL

#include <bson.h>

#include "mongoc-trace-private.h"
#include "mongoc-log.h"
#include "mongoc-stream-tls.h"
#include "mongoc-stream-tls-private.h"
#include "mongoc-stream-private.h"
#include "mongoc-stream-tls-secure-channel-private.h"
#include "mongoc-secure-channel-private.h"
#include "mongoc-ssl.h"
#include "mongoc-error.h"
#include "mongoc-counters-private.h"
#include "mongoc-errno-private.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream-tls-secure-channel"


#define SECURITY_WIN32
#include <security.h>
#include <schnlsp.h>
#include <schannel.h>

/* mingw doesn't define these */
#ifndef SP_PROT_TLS1_1_CLIENT
#define SP_PROT_TLS1_1_CLIENT 0x00000200
#endif

#ifndef SP_PROT_TLS1_2_CLIENT
#define SP_PROT_TLS1_2_CLIENT 0x00000800
#endif

size_t
mongoc_secure_channel_write (mongoc_stream_tls_t *tls,
                             const void *data,
                             size_t data_length);


static void
_mongoc_stream_tls_secure_channel_destroy (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_channel);


   /* See https://msdn.microsoft.com/en-us/library/windows/desktop/aa380138.aspx
    * Shutting Down an Schannel Connection
    */

   TRACE ("shutting down SSL/TLS connection");

   if (secure_channel->cred && secure_channel->ctxt) {
      SecBufferDesc BuffDesc;
      SecBuffer Buffer;
      SECURITY_STATUS sspi_status;
      SecBuffer outbuf;
      SecBufferDesc outbuf_desc;
      DWORD dwshut = SCHANNEL_SHUTDOWN;

      _mongoc_secure_channel_init_sec_buffer (
         &Buffer, SECBUFFER_TOKEN, &dwshut, sizeof (dwshut));
      _mongoc_secure_channel_init_sec_buffer_desc (&BuffDesc, &Buffer, 1);

      sspi_status =
         ApplyControlToken (&secure_channel->ctxt->ctxt_handle, &BuffDesc);

      if (sspi_status != SEC_E_OK) {
         MONGOC_ERROR ("ApplyControlToken failure: %d", sspi_status);
      }

      /* setup output buffer */
      _mongoc_secure_channel_init_sec_buffer (
         &outbuf, SECBUFFER_EMPTY, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer_desc (&outbuf_desc, &outbuf, 1);

      sspi_status =
         InitializeSecurityContext (&secure_channel->cred->cred_handle,
                                    &secure_channel->ctxt->ctxt_handle,
                                    /*tls->hostname*/ NULL,
                                    secure_channel->req_flags,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    &secure_channel->ctxt->ctxt_handle,
                                    &outbuf_desc,
                                    &secure_channel->ret_flags,
                                    &secure_channel->ctxt->time_stamp);

      if ((sspi_status == SEC_E_OK) || (sspi_status == SEC_I_CONTEXT_EXPIRED)) {
         /* send close message which is in output buffer */
         ssize_t written =
            mongoc_secure_channel_write (tls, outbuf.pvBuffer, outbuf.cbBuffer);

         FreeContextBuffer (outbuf.pvBuffer);

         if (outbuf.cbBuffer != (size_t) written) {
            TRACE ("failed to send close msg (wrote %zd out of %zd)",
                   written,
                   outbuf.cbBuffer);
         }
      }
   }

   /* free SSPI Schannel API security context handle */
   if (secure_channel->ctxt) {
      TRACE ("clear security context handle");
      DeleteSecurityContext (&secure_channel->ctxt->ctxt_handle);
      bson_free (secure_channel->ctxt);
   }

   /* free SSPI Schannel API credential handle */
   if (secure_channel->cred) {
      /* decrement the reference counter of the credential/session handle */
      /* if the handle was not cached and the refcount is zero */
      TRACE ("clear credential handle");
      FreeCredentialsHandle (&secure_channel->cred->cred_handle);
      bson_free (secure_channel->cred);
   }

   /* free internal buffer for received encrypted data */
   if (secure_channel->encdata_buffer != NULL) {
      bson_free (secure_channel->encdata_buffer);
      secure_channel->encdata_length = 0;
      secure_channel->encdata_offset = 0;
   }

   /* free internal buffer for received decrypted data */
   if (secure_channel->decdata_buffer != NULL) {
      bson_free (secure_channel->decdata_buffer);
      secure_channel->decdata_length = 0;
      secure_channel->decdata_offset = 0;
   }

   mongoc_stream_destroy (tls->base_stream);

   bson_free (secure_channel);
   bson_free (stream);

   mongoc_counter_streams_active_dec ();
   mongoc_counter_streams_disposed_inc ();
   EXIT;
}

static void
_mongoc_stream_tls_secure_channel_failed (mongoc_stream_t *stream)
{
   ENTRY;
   _mongoc_stream_tls_secure_channel_destroy (stream);
   EXIT;
}

static int
_mongoc_stream_tls_secure_channel_close (mongoc_stream_t *stream)
{
   int ret = 0;
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_channel);

   ret = mongoc_stream_close (tls->base_stream);
   RETURN (ret);
}

static int
_mongoc_stream_tls_secure_channel_flush (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_channel);
   RETURN (0);
}

static ssize_t
_mongoc_stream_tls_secure_channel_write (mongoc_stream_t *stream,
                                         char *buf,
                                         size_t buf_len)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;
   ssize_t written = -1;
   size_t data_len = 0;
   unsigned char *data = NULL;
   SecBuffer outbuf[4];
   SecBufferDesc outbuf_desc;
   SECURITY_STATUS sspi_status = SEC_E_OK;

   ENTRY;

   BSON_ASSERT (secure_channel);
   TRACE ("The entire buffer is: %d", buf_len);

   /* check if the maximum stream sizes were queried */
   if (secure_channel->stream_sizes.cbMaximumMessage == 0) {
      sspi_status = QueryContextAttributes (&secure_channel->ctxt->ctxt_handle,
                                            SECPKG_ATTR_STREAM_SIZES,
                                            &secure_channel->stream_sizes);

      if (sspi_status != SEC_E_OK) {
         TRACE ("failing here: %d", __LINE__);
         return -1;
      }
   }

   /* check if the buffer is longer than the maximum message length */
   if (buf_len > secure_channel->stream_sizes.cbMaximumMessage) {
      TRACE ("SHRINKING buf_len from %lu to %lu",
             buf_len,
             secure_channel->stream_sizes.cbMaximumMessage);
      buf_len = secure_channel->stream_sizes.cbMaximumMessage;
   }

   /* calculate the complete message length and allocate a buffer for it */
   data_len = secure_channel->stream_sizes.cbHeader + buf_len +
              secure_channel->stream_sizes.cbTrailer;
   data = (unsigned char *) bson_malloc (data_len);

   /* setup output buffers (header, data, trailer, empty) */
   _mongoc_secure_channel_init_sec_buffer (
      &outbuf[0],
      SECBUFFER_STREAM_HEADER,
      data,
      secure_channel->stream_sizes.cbHeader);
   _mongoc_secure_channel_init_sec_buffer (
      &outbuf[1],
      SECBUFFER_DATA,
      data + secure_channel->stream_sizes.cbHeader,
      (unsigned long) (buf_len & (size_t) 0xFFFFFFFFUL));
   _mongoc_secure_channel_init_sec_buffer (
      &outbuf[2],
      SECBUFFER_STREAM_TRAILER,
      data + secure_channel->stream_sizes.cbHeader + buf_len,
      secure_channel->stream_sizes.cbTrailer);
   _mongoc_secure_channel_init_sec_buffer (
      &outbuf[3], SECBUFFER_EMPTY, NULL, 0);
   _mongoc_secure_channel_init_sec_buffer_desc (&outbuf_desc, outbuf, 4);

   /* copy data into output buffer */
   memcpy (outbuf[1].pvBuffer, buf, buf_len);

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa375390.aspx */
   sspi_status =
      EncryptMessage (&secure_channel->ctxt->ctxt_handle, 0, &outbuf_desc, 0);

   /* check if the message was encrypted */
   if (sspi_status == SEC_E_OK) {
      written = 0;

      /* send the encrypted message including header, data and trailer */
      buf_len = outbuf[0].cbBuffer + outbuf[1].cbBuffer + outbuf[2].cbBuffer;
      written = mongoc_secure_channel_write (tls, data, buf_len);
   } else {
      written = -1;
   }

   bson_free (data);

   if (buf_len == (size_t) written) {
      /* Encrypted message including header, data and trailer entirely sent.
      *  The return value is the number of unencrypted bytes that were sent. */
      written = outbuf[1].cbBuffer;
   }

   return written;
}

/* This is copypasta from _mongoc_stream_tls_openssl_writev */
#define MONGOC_STREAM_TLS_BUFFER_SIZE 4096
static ssize_t
_mongoc_stream_tls_secure_channel_writev (mongoc_stream_t *stream,
                                          mongoc_iovec_t *iov,
                                          size_t iovcnt,
                                          int32_t timeout_msec)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;
   char buf[MONGOC_STREAM_TLS_BUFFER_SIZE];
   ssize_t ret = 0;
   ssize_t child_ret;
   size_t i;
   size_t iov_pos = 0;

   /* There's a bit of a dance to coalesce vectorized writes into
    * MONGOC_STREAM_TLS_BUFFER_SIZE'd writes to avoid lots of small tls
    * packets.
    *
    * The basic idea is that we want to combine writes in the buffer if they're
    * smaller than the buffer, flushing as it gets full.  For larger writes, or
    * the last write in the iovec array, we want to ignore the buffer and just
    * write immediately.  We take care of doing buffer writes by re-invoking
    * ourself with a single iovec_t, pointing at our stack buffer.
    */
   char *buf_head = buf;
   char *buf_tail = buf;
   char *buf_end = buf + MONGOC_STREAM_TLS_BUFFER_SIZE;
   size_t bytes;

   char *to_write = NULL;
   size_t to_write_len;

   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);
   BSON_ASSERT (secure_channel);
   ENTRY;

   TRACE ("Trying to write to the server");
   tls->timeout_msec = timeout_msec;

   TRACE ("count: %d, 0th: %lu", iovcnt, iov[0].iov_len);

   for (i = 0; i < iovcnt; i++) {
      iov_pos = 0;

      TRACE ("iov %d size: %lu", i, iov[i].iov_len);
      while (iov_pos < iov[i].iov_len) {
         if (buf_head != buf_tail ||
             ((i + 1 < iovcnt) &&
              ((buf_end - buf_tail) > (iov[i].iov_len - iov_pos)))) {
            /* If we have either of:
             *   - buffered bytes already
             *   - another iovec to send after this one and we don't have more
             *     bytes to send than the size of the buffer.
             *
             * copy into the buffer */

            bytes = BSON_MIN (iov[i].iov_len - iov_pos, buf_end - buf_tail);

            memcpy (buf_tail, (char *) iov[i].iov_base + iov_pos, bytes);
            buf_tail += bytes;
            iov_pos += bytes;

            if (buf_tail == buf_end) {
               /* If we're full, request send */

               to_write = buf_head;
               to_write_len = buf_tail - buf_head;

               buf_tail = buf_head = buf;
            }
         } else {
            /* Didn't buffer, so just write it through */

            to_write = (char *) iov[i].iov_base + iov_pos;
            to_write_len = iov[i].iov_len - iov_pos;

            iov_pos += to_write_len;
         }

         if (to_write) {
            /* We get here if we buffered some bytes and filled the buffer, or
             * if we didn't buffer and have to send out of the iovec */

            child_ret = _mongoc_stream_tls_secure_channel_write (
               stream, to_write, to_write_len);
            TRACE ("Child0wrote: %d, was supposed to write: %d",
                   child_ret,
                   to_write_len);

            if (child_ret < 0) {
               RETURN (ret);
            }

            ret += child_ret;

            iov_pos -= to_write_len - child_ret;

            to_write = NULL;
         }
      }
   }

   if (buf_head != buf_tail) {
      /* If we have any bytes buffered, send */

      child_ret = _mongoc_stream_tls_secure_channel_write (
         stream, buf_head, buf_tail - buf_head);
      TRACE ("Child1wrote: %d, was supposed to write: %d",
             child_ret,
             buf_tail - buf_head);

      if (child_ret < 0) {
         RETURN (child_ret);
      }

      ret += child_ret;
   }

   if (ret >= 0) {
      mongoc_counter_streams_egress_add (ret);
   }

   TRACE ("Returning %zu", ret);
   RETURN (ret);
}


static ssize_t
_mongoc_stream_tls_secure_channel_read (mongoc_stream_t *stream,
                                        char *buf,
                                        size_t len)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;
   ssize_t error = 0;
   size_t size = 0;
   ssize_t nread = -1;
   unsigned char *reallocated_buffer;
   size_t reallocated_length;
   bool done = false;
   SecBuffer inbuf[4];
   SecBufferDesc inbuf_desc;
   SECURITY_STATUS sspi_status = SEC_E_OK;
   /* we want the length of the encrypted buffer to be at least large enough
   *  that it can hold all the bytes requested and some TLS record overhead. */
   size_t min_encdata_length = len + MONGOC_SCHANNEL_BUFFER_FREE_SIZE;

   /****************************************************************************
    * Don't return or set secure_channel->recv_unrecoverable_err unless in the
    * cleanup.
    * The pattern for return error is set error, optional infof, goto cleanup.
    *
    * Our priority is to always return as much decrypted data to the caller as
    * possible, even if an error occurs. The state of the decrypted buffer must
    * always be valid. Transfer of decrypted data to the caller's buffer is
    * handled in the cleanup.
    */

   TRACE ("client wants to read %zu bytes", len);

   if (len && len <= secure_channel->decdata_offset) {
      TRACE ("enough decrypted data is already available");
      goto cleanup;
   } else if (secure_channel->recv_unrecoverable_err) {
      error = secure_channel->recv_unrecoverable_err;
      TRACE ("an unrecoverable error occurred in a prior call");
      goto cleanup;
   } else if (secure_channel->recv_sspi_close_notify) {
      /* once a server has indicated shutdown there is no more encrypted data */
      TRACE ("server indicated shutdown in a prior call");
      goto cleanup;
   } else if (!len) {
      /* It's debatable what to return when !len. Regardless we can't return
       * immediately because there may be data to decrypt (in the case we want
       * to
       * decrypt all encrypted cached data) so handle !len later in cleanup.
       */
      /* do nothing */
   } else if (!secure_channel->recv_connection_closed) {
      /* increase enc buffer in order to fit the requested amount of data */
      size = secure_channel->encdata_length - secure_channel->encdata_offset;

      if (size < MONGOC_SCHANNEL_BUFFER_FREE_SIZE ||
          secure_channel->encdata_length < min_encdata_length) {
         reallocated_length =
            secure_channel->encdata_offset + MONGOC_SCHANNEL_BUFFER_FREE_SIZE;

         if (reallocated_length < min_encdata_length) {
            reallocated_length = min_encdata_length;
         }

         reallocated_buffer =
            bson_realloc (secure_channel->encdata_buffer, reallocated_length);

         secure_channel->encdata_buffer = reallocated_buffer;
         secure_channel->encdata_length = reallocated_length;
         size = secure_channel->encdata_length - secure_channel->encdata_offset;
         TRACE ("encdata_buffer resized %zu", secure_channel->encdata_length);
      }

      TRACE ("encrypted data buffer: offset %zu length %zu",
             secure_channel->encdata_offset,
             secure_channel->encdata_length);

      /* read encrypted data from socket */
      nread =
         mongoc_secure_channel_read (tls,
                                     (char *) (secure_channel->encdata_buffer +
                                               secure_channel->encdata_offset),
                                     size);

      if (!nread) {
         nread = -1;

         if (MONGOC_ERRNO_IS_AGAIN (errno)) {
            TRACE ("Try again");
            error = EAGAIN;
         } else {
            secure_channel->recv_connection_closed = true;
            TRACE ("reading failed: %d", errno);
         }
      } else {
         secure_channel->encdata_offset += (size_t) nread;
         TRACE ("encrypted data got %zd", nread);
      }
   }

   TRACE ("encrypted data buffer: offset %zu length %zu",
          secure_channel->encdata_offset,
          secure_channel->encdata_length);

   /* decrypt loop */
   while (secure_channel->encdata_offset > 0 && sspi_status == SEC_E_OK &&
          (!len || secure_channel->decdata_offset < len ||
           secure_channel->recv_connection_closed)) {
      /* prepare data buffer for DecryptMessage call */
      _mongoc_secure_channel_init_sec_buffer (
         &inbuf[0],
         SECBUFFER_DATA,
         secure_channel->encdata_buffer,
         (unsigned long) (secure_channel->encdata_offset &
                          (size_t) 0xFFFFFFFFUL));

      /* we need 3 more empty input buffers for possible output */
      _mongoc_secure_channel_init_sec_buffer (
         &inbuf[1], SECBUFFER_EMPTY, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer (
         &inbuf[2], SECBUFFER_EMPTY, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer (
         &inbuf[3], SECBUFFER_EMPTY, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer_desc (&inbuf_desc, inbuf, 4);

      /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa375348.aspx
       */
      sspi_status = DecryptMessage (
         &secure_channel->ctxt->ctxt_handle, &inbuf_desc, 0, NULL);

      /* check if everything went fine (server may want to renegotiate
       * or shutdown the connection context) */
      if (sspi_status == SEC_E_OK || sspi_status == SEC_I_RENEGOTIATE ||
          sspi_status == SEC_I_CONTEXT_EXPIRED) {
         /* check for successfully decrypted data, even before actual
          * renegotiation or shutdown of the connection context */
         if (inbuf[1].BufferType == SECBUFFER_DATA) {
            TRACE ("decrypted data length: %lu", inbuf[1].cbBuffer);

            /* increase buffer in order to fit the received amount of data */
            size = inbuf[1].cbBuffer > MONGOC_SCHANNEL_BUFFER_FREE_SIZE
                      ? inbuf[1].cbBuffer
                      : MONGOC_SCHANNEL_BUFFER_FREE_SIZE;

            if (secure_channel->decdata_length -
                      secure_channel->decdata_offset <
                   size ||
                secure_channel->decdata_length < len) {
               /* increase internal decrypted data buffer */
               reallocated_length = secure_channel->decdata_offset + size;

               /* make sure that the requested amount of data fits */
               if (reallocated_length < len) {
                  reallocated_length = len;
               }

               reallocated_buffer = bson_realloc (
                  secure_channel->decdata_buffer, reallocated_length);
               secure_channel->decdata_buffer = reallocated_buffer;
               secure_channel->decdata_length = reallocated_length;
            }

            /* copy decrypted data to internal buffer */
            size = inbuf[1].cbBuffer;

            if (size) {
               memcpy (secure_channel->decdata_buffer +
                          secure_channel->decdata_offset,
                       inbuf[1].pvBuffer,
                       size);
               secure_channel->decdata_offset += size;
            }

            TRACE ("decrypted data added: %zu", size);
            TRACE ("decrypted data cached: offset %zu length %zu",
                   secure_channel->decdata_offset,
                   secure_channel->decdata_length);
         }

         /* check for remaining encrypted data */
         if (inbuf[3].BufferType == SECBUFFER_EXTRA && inbuf[3].cbBuffer > 0) {
            TRACE ("encrypted data length: %lu", inbuf[3].cbBuffer);

            /* check if the remaining data is less than the total amount
             * and therefore begins after the already processed data
             */
            if (secure_channel->encdata_offset > inbuf[3].cbBuffer) {
               /* move remaining encrypted data forward to the beginning of
                * buffer */
               memmove (secure_channel->encdata_buffer,
                        (secure_channel->encdata_buffer +
                         secure_channel->encdata_offset) -
                           inbuf[3].cbBuffer,
                        inbuf[3].cbBuffer);
               secure_channel->encdata_offset = inbuf[3].cbBuffer;
            }

            TRACE ("encrypted data cached: offset %zu length %zu",
                   secure_channel->encdata_offset,
                   secure_channel->encdata_length);
         } else {
            /* reset encrypted buffer offset, because there is no data remaining
             */
            secure_channel->encdata_offset = 0;
         }

         /* check if server wants to renegotiate the connection context */
         if (sspi_status == SEC_I_RENEGOTIATE) {
            TRACE ("remote party requests renegotiation");
            goto cleanup;
         }
         /* check if the server closed the connection */
         else if (sspi_status == SEC_I_CONTEXT_EXPIRED) {
            /* In Windows 2000 SEC_I_CONTEXT_EXPIRED (close_notify) is not
             * returned so we have to work around that in cleanup. */
            secure_channel->recv_sspi_close_notify = true;

            if (!secure_channel->recv_connection_closed) {
               secure_channel->recv_connection_closed = true;
               TRACE ("server closed the connection");
            }

            goto cleanup;
         }
      } else if (sspi_status == SEC_E_INCOMPLETE_MESSAGE) {
         if (!error) {
            error = EAGAIN;
         }

         TRACE ("failed to decrypt data, need more data");
         goto cleanup;
      } else {
         error = 1;
         TRACE ("failed to read data from server: %d", sspi_status);
         goto cleanup;
      }
   }

   TRACE ("encrypted data buffer: offset %zu length %zu",
          secure_channel->encdata_offset,
          secure_channel->encdata_length);

   TRACE ("decrypted data buffer: offset %zu length %zu",
          secure_channel->decdata_offset,
          secure_channel->decdata_length);

cleanup:
   /* Warning- there is no guarantee the encdata state is valid at this point */
   TRACE ("cleanup");

   /* Error if the connection has closed without a close_notify.
    * Behavior here is a matter of debate. We don't want to be vulnerable to a
    * truncation attack however there's some browser precedent for ignoring the
    * close_notify for compatibility reasons.
    * Additionally, Windows 2000 (v5.0) is a special case since it seems it
    * doesn't
    * return close_notify. In that case if the connection was closed we assume
    * it
    * was graceful (close_notify) since there doesn't seem to be a way to tell.
    */
   if (len && !secure_channel->decdata_offset &&
       secure_channel->recv_connection_closed &&
       !secure_channel->recv_sspi_close_notify) {
      error = 1;
      TRACE ("server closed abruptly (missing close_notify)");
   }

   /* Any error other than EAGAIN is an unrecoverable error. */
   if (error && error != EAGAIN) {
      secure_channel->recv_unrecoverable_err = error;
      TRACE ("fatal error");
   }

   size = len < secure_channel->decdata_offset ? len
                                               : secure_channel->decdata_offset;

   if (size) {
      memcpy (buf, secure_channel->decdata_buffer, size);
      memmove (secure_channel->decdata_buffer,
               secure_channel->decdata_buffer + size,
               secure_channel->decdata_offset - size);
      secure_channel->decdata_offset -= size;

      TRACE ("decrypted data returned %zu", size);
      TRACE ("decrypted data buffer: offset %zu length %zu",
             secure_channel->decdata_offset,
             secure_channel->decdata_length);
      return (ssize_t) size;
   }

   if (!error && !secure_channel->recv_connection_closed) {
      error = EAGAIN;
   }

   /* It's debatable what to return when !len. We could return whatever error we
    * got from decryption but instead we override here so the return is
    * consistent.
    */
   if (!len) {
      error = 0;
   }

   if (error == EAGAIN) {
      errno = EAGAIN;
      return 0;
   }

   TRACE ("error is: %d and errno is: %d", error, errno);
   return error ? -1 : 0;
}

/* This function is copypasta of _mongoc_stream_tls_openssl_readv */
static ssize_t
_mongoc_stream_tls_secure_channel_readv (mongoc_stream_t *stream,
                                         mongoc_iovec_t *iov,
                                         size_t iovcnt,
                                         size_t min_bytes,
                                         int32_t timeout_msec)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;
   ssize_t ret = 0;
   size_t i;
   size_t iov_pos = 0;
   int64_t now;
   int64_t expire = 0;

   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);
   BSON_ASSERT (secure_channel);
   ENTRY;

   tls->timeout_msec = timeout_msec;

   if (timeout_msec >= 0) {
      expire = bson_get_monotonic_time () + (timeout_msec * 1000UL);
   }

   for (i = 0; i < iovcnt; i++) {
      iov_pos = 0;

      while (iov_pos < iov[i].iov_len) {
         ssize_t read_ret = _mongoc_stream_tls_secure_channel_read (
            stream,
            (char *) iov[i].iov_base + iov_pos,
            (int) (iov[i].iov_len - iov_pos));

         if (read_ret < 0) {
            RETURN (-1);
         }

         if (expire) {
            now = bson_get_monotonic_time ();

            if ((expire - now) < 0) {
               if (read_ret == 0) {
                  mongoc_counter_streams_timeout_inc ();
                  errno = ETIMEDOUT;
                  RETURN (-1);
               }

               tls->timeout_msec = 0;
            } else {
               tls->timeout_msec = (expire - now) / 1000L;
            }
         }

         ret += read_ret;

         if ((size_t) ret >= min_bytes) {
            mongoc_counter_streams_ingress_add (ret);
            RETURN (ret);
         }

         iov_pos += read_ret;
      }
   }

   if (ret >= 0) {
      mongoc_counter_streams_ingress_add (ret);
   }

   RETURN (ret);
}

static int
_mongoc_stream_tls_secure_channel_setsockopt (mongoc_stream_t *stream,
                                              int level,
                                              int optname,
                                              void *optval,
                                              mongoc_socklen_t optlen)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_channel);
   RETURN (mongoc_stream_setsockopt (
      tls->base_stream, level, optname, optval, optlen));
}

static mongoc_stream_t *
_mongoc_stream_tls_secure_channel_get_base_stream (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_channel);
   RETURN (tls->base_stream);
}


static bool
_mongoc_stream_tls_secure_channel_check_closed (
   mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_channel);
   RETURN (mongoc_stream_check_closed (tls->base_stream));
}

bool
mongoc_stream_tls_secure_channel_handshake (mongoc_stream_t *stream,
                                            const char *host,
                                            int *events,
                                            bson_error_t *error)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_channel);

   TRACE ("Getting ready for state: %d", secure_channel->connecting_state + 1);

   switch (secure_channel->connecting_state) {
   case ssl_connect_1:

      if (mongoc_secure_channel_handshake_step_1 (tls, (char *) host)) {
         TRACE ("Step#1 Worked!\n\n");
         *events = POLLIN;
         RETURN (false);
      } else {
         TRACE ("Step#1 FAILED!");
      }

      break;

   case ssl_connect_2:
   case ssl_connect_2_reading:
   case ssl_connect_2_writing:

      if (mongoc_secure_channel_handshake_step_2 (tls, (char *) host)) {
         TRACE ("Step#2 Worked!\n\n");
         *events = POLLIN | POLLOUT;
         RETURN (false);
      } else {
         TRACE ("Step#2 FAILED!");
      }

      break;

   case ssl_connect_3:

      if (mongoc_secure_channel_handshake_step_3 (tls, (char *) host)) {
         TRACE ("Step#3 Worked!\n\n");
         *events = POLLIN | POLLOUT;
         RETURN (false);
      } else {
         TRACE ("Step#3 FAILED!");
      }

      break;

   case ssl_connect_done:
      TRACE ("Connect DONE!");
      /* reset our connection state machine */
      secure_channel->connecting_state = ssl_connect_1;
      RETURN (true);
      break;
   }


   *events = 0;
   bson_set_error (error,
                   MONGOC_ERROR_STREAM,
                   MONGOC_ERROR_STREAM_SOCKET,
                   "TLS handshake failed");

   RETURN (false);
}


mongoc_stream_t *
mongoc_stream_tls_secure_channel_new (mongoc_stream_t *base_stream,
                                      const char *host,
                                      mongoc_ssl_opt_t *opt,
                                      int client)
{
   SECURITY_STATUS sspi_status = SEC_E_OK;
   SCHANNEL_CRED schannel_cred;
   mongoc_stream_tls_t *tls;
   mongoc_stream_tls_secure_channel_t *secure_channel;

   ENTRY;
   BSON_ASSERT (base_stream);
   BSON_ASSERT (opt);


   secure_channel = (mongoc_stream_tls_secure_channel_t *) bson_malloc0 (
      sizeof *secure_channel);

   tls = (mongoc_stream_tls_t *) bson_malloc0 (sizeof *tls);
   tls->parent.type = MONGOC_STREAM_TLS;
   tls->parent.destroy = _mongoc_stream_tls_secure_channel_destroy;
   tls->parent.failed = _mongoc_stream_tls_secure_channel_failed;
   tls->parent.close = _mongoc_stream_tls_secure_channel_close;
   tls->parent.flush = _mongoc_stream_tls_secure_channel_flush;
   tls->parent.writev = _mongoc_stream_tls_secure_channel_writev;
   tls->parent.readv = _mongoc_stream_tls_secure_channel_readv;
   tls->parent.setsockopt = _mongoc_stream_tls_secure_channel_setsockopt;
   tls->parent.get_base_stream =
      _mongoc_stream_tls_secure_channel_get_base_stream;
   tls->parent.check_closed = _mongoc_stream_tls_secure_channel_check_closed;
   memcpy (&tls->ssl_opts, opt, sizeof tls->ssl_opts);
   tls->handshake = mongoc_stream_tls_secure_channel_handshake;
   tls->ctx = (void *) secure_channel;
   tls->timeout_msec = -1;
   tls->base_stream = base_stream;

   TRACE ("SSL/TLS connection with endpoint AcquireCredentialsHandle");

   /* setup Schannel API options */
   memset (&schannel_cred, 0, sizeof (schannel_cred));
   schannel_cred.dwVersion = SCHANNEL_CRED_VERSION;

/* SCHANNEL_CRED:
 * SCH_USE_STRONG_CRYPTO is not available in VS2010
 *   https://msdn.microsoft.com/en-us/library/windows/desktop/aa379810.aspx */
#ifdef SCH_USE_STRONG_CRYPTO
   schannel_cred.dwFlags = SCH_USE_STRONG_CRYPTO;
#endif
   if (opt->weak_cert_validation) {
      schannel_cred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION |
                               SCH_CRED_IGNORE_NO_REVOCATION_CHECK |
                               SCH_CRED_IGNORE_REVOCATION_OFFLINE;
      TRACE ("disabled server certificate checks");
   } else {
      schannel_cred.dwFlags |=
         SCH_CRED_AUTO_CRED_VALIDATION | SCH_CRED_REVOCATION_CHECK_CHAIN;
      TRACE ("enabled server certificate checks");
   }

   if (opt->allow_invalid_hostname) {
      schannel_cred.dwFlags |=
         SCH_CRED_NO_SERVERNAME_CHECK | SCH_CRED_IGNORE_NO_REVOCATION_CHECK;
   }

   if (opt->ca_file) {
      mongoc_secure_channel_setup_ca (secure_channel, opt);
   }

   if (opt->crl_file) {
      mongoc_secure_channel_setup_crl (secure_channel, opt);
   }

   if (opt->pem_file) {
      PCCERT_CONTEXT cert =
         mongoc_secure_channel_setup_certificate (secure_channel, opt);

      if (cert) {
         schannel_cred.cCreds = 1;
         schannel_cred.paCred = &cert;
      }
   }


   schannel_cred.grbitEnabledProtocols =
      SP_PROT_TLS1_1_CLIENT | SP_PROT_TLS1_2_CLIENT;

   secure_channel->cred = (mongoc_secure_channel_cred *) bson_malloc0 (
      sizeof (mongoc_secure_channel_cred));

   /* Example:
    *   https://msdn.microsoft.com/en-us/library/windows/desktop/aa375454%28v=vs.85%29.aspx
    * AcquireCredentialsHandle:
    *   https://msdn.microsoft.com/en-us/library/windows/desktop/aa374716.aspx
    */
   sspi_status = AcquireCredentialsHandle (
      NULL,                 /* principal */
      UNISP_NAME,           /* security package */
      SECPKG_CRED_OUTBOUND, /* we are preparing outbound connection */
      NULL,                 /*  Optional logon */
      &schannel_cred,       /* TLS "configuration", "auth data" */
      NULL,                 /* unused */
      NULL,                 /* unused */
      &secure_channel->cred->cred_handle, /* credential OUT param */
      &secure_channel->cred->time_stamp); /* certificate expiration time */

   if (sspi_status != SEC_E_OK) {
      LPTSTR msg = NULL;
      FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                     NULL,
                     GetLastError (),
                     LANG_NEUTRAL,
                     (LPTSTR) &msg,
                     0,
                     NULL);
      MONGOC_ERROR (
         "Failed to initialize security context, error code: 0x%04X%04X: '%s'",
         (sspi_status >> 16) & 0xffff,
         sspi_status & 0xffff,
         msg);
      LocalFree (msg);
      RETURN (NULL);
   }

   if (opt->ca_dir) {
      MONGOC_ERROR ("Setting mongoc_ssl_opt_t.ca_dir has no effect when built "
                    "against Secure Channel");
   }


   mongoc_counter_streams_active_inc ();
   RETURN ((mongoc_stream_t *) tls);
}
#endif /* MONGOC_ENABLE_SSL_SECURE_CHANNEL */
