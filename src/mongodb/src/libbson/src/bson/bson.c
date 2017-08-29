/*
 * Copyright 2013 MongoDB, Inc.
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


#include "bson.h"
#include "bson-config.h"
#include "b64_ntop.h"
#include "bson-private.h"
#include "bson-string.h"
#include "bson-iso8601-private.h"

#include <string.h>
#include <math.h>


#ifndef BSON_MAX_RECURSION
#define BSON_MAX_RECURSION 200
#endif


typedef enum {
   BSON_VALIDATE_PHASE_START,
   BSON_VALIDATE_PHASE_TOP,
   BSON_VALIDATE_PHASE_LF_REF_KEY,
   BSON_VALIDATE_PHASE_LF_REF_UTF8,
   BSON_VALIDATE_PHASE_LF_ID_KEY,
   BSON_VALIDATE_PHASE_LF_DB_KEY,
   BSON_VALIDATE_PHASE_LF_DB_UTF8,
   BSON_VALIDATE_PHASE_NOT_DBREF,
} bson_validate_phase_t;


typedef enum {
   BSON_JSON_MODE_LEGACY,
   BSON_JSON_MODE_CANONICAL,
   BSON_JSON_MODE_RELAXED,
} bson_json_mode_t;


/*
 * Structures.
 */
typedef struct {
   bson_validate_flags_t flags;
   ssize_t err_offset;
   bson_validate_phase_t phase;
   bson_error_t error;
} bson_validate_state_t;


typedef struct {
   uint32_t count;
   bool keys;
   ssize_t *err_offset;
   uint32_t depth;
   bson_string_t *str;
   bson_json_mode_t mode;
} bson_json_state_t;


/*
 * Forward declarations.
 */
static bool
_bson_as_json_visit_array (const bson_iter_t *iter,
                           const char *key,
                           const bson_t *v_array,
                           void *data);
static bool
_bson_as_json_visit_document (const bson_iter_t *iter,
                              const char *key,
                              const bson_t *v_document,
                              void *data);
static char *
_bson_as_json_visit_all (const bson_t *bson,
                         size_t *length,
                         bson_json_mode_t mode);

/*
 * Globals.
 */
static const uint8_t gZero;

/*
 *--------------------------------------------------------------------------
 *
 * _bson_impl_inline_grow --
 *
 *       Document growth implementation for documents that currently
 *       contain stack based buffers. The document may be switched to
 *       a malloc based buffer.
 *
 * Returns:
 *       true if successful; otherwise false indicating INT_MAX overflow.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_bson_impl_inline_grow (bson_impl_inline_t *impl, /* IN */
                        size_t size)              /* IN */
{
   bson_impl_alloc_t *alloc = (bson_impl_alloc_t *) impl;
   uint8_t *data;
   size_t req;

   if (((size_t) impl->len + size) <= sizeof impl->data) {
      return true;
   }

   req = bson_next_power_of_two (impl->len + size);

   if (req <= INT32_MAX) {
      data = bson_malloc (req);

      memcpy (data, impl->data, impl->len);
      alloc->flags &= ~BSON_FLAG_INLINE;
      alloc->parent = NULL;
      alloc->depth = 0;
      alloc->buf = &alloc->alloc;
      alloc->buflen = &alloc->alloclen;
      alloc->offset = 0;
      alloc->alloc = data;
      alloc->alloclen = req;
      alloc->realloc = bson_realloc_ctx;
      alloc->realloc_func_ctx = NULL;

      return true;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_impl_alloc_grow --
 *
 *       Document growth implementation for documents containing malloc
 *       based buffers.
 *
 * Returns:
 *       true if successful; otherwise false indicating INT_MAX overflow.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_bson_impl_alloc_grow (bson_impl_alloc_t *impl, /* IN */
                       size_t size)             /* IN */
{
   size_t req;

   /*
    * Determine how many bytes we need for this document in the buffer
    * including necessary trailing bytes for parent documents.
    */
   req = (impl->offset + impl->len + size + impl->depth);

   if (req <= *impl->buflen) {
      return true;
   }

   req = bson_next_power_of_two (req);

   if ((req <= INT32_MAX) && impl->realloc) {
      *impl->buf = impl->realloc (*impl->buf, req, impl->realloc_func_ctx);
      *impl->buflen = req;
      return true;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_grow --
 *
 *       Grows the bson_t structure to be large enough to contain @size
 *       bytes.
 *
 * Returns:
 *       true if successful, false if the size would overflow.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_bson_grow (bson_t *bson,  /* IN */
            uint32_t size) /* IN */
{
   if ((bson->flags & BSON_FLAG_INLINE)) {
      return _bson_impl_inline_grow ((bson_impl_inline_t *) bson, size);
   }

   return _bson_impl_alloc_grow ((bson_impl_alloc_t *) bson, size);
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_data --
 *
 *       A helper function to return the contents of the bson document
 *       taking into account the polymorphic nature of bson_t.
 *
 * Returns:
 *       A buffer which should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static BSON_INLINE uint8_t *
_bson_data (const bson_t *bson) /* IN */
{
   if ((bson->flags & BSON_FLAG_INLINE)) {
      return ((bson_impl_inline_t *) bson)->data;
   } else {
      bson_impl_alloc_t *impl = (bson_impl_alloc_t *) bson;
      return (*impl->buf) + impl->offset;
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_encode_length --
 *
 *       Helper to encode the length of the bson_t in the first 4 bytes
 *       of the bson document. Little endian format is used as specified
 *       by bsonspec.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static BSON_INLINE void
_bson_encode_length (bson_t *bson) /* IN */
{
#if BSON_BYTE_ORDER == BSON_LITTLE_ENDIAN
   memcpy (_bson_data (bson), &bson->len, sizeof (bson->len));
#else
   uint32_t length_le = BSON_UINT32_TO_LE (bson->len);
   memcpy (_bson_data (bson), &length_le, sizeof (length_le));
#endif
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_append_va --
 *
 *       Appends the length,buffer pairs to the bson_t. @n_bytes is an
 *       optimization to perform one array growth rather than many small
 *       growths.
 *
 *       @bson: A bson_t
 *       @n_bytes: The number of bytes to append to the document.
 *       @n_pairs: The number of length,buffer pairs.
 *       @first_len: Length of first buffer.
 *       @first_data: First buffer.
 *       @args: va_list of additional tuples.
 *
 * Returns:
 *       true if the bytes were appended successfully.
 *       false if it bson would overflow INT_MAX.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static BSON_INLINE bool
_bson_append_va (bson_t *bson,              /* IN */
                 uint32_t n_bytes,          /* IN */
                 uint32_t n_pairs,          /* IN */
                 uint32_t first_len,        /* IN */
                 const uint8_t *first_data, /* IN */
                 va_list args)              /* IN */
{
   const uint8_t *data;
   uint32_t data_len;
   uint8_t *buf;

   BSON_ASSERT (!(bson->flags & BSON_FLAG_IN_CHILD));
   BSON_ASSERT (!(bson->flags & BSON_FLAG_RDONLY));

   if (BSON_UNLIKELY (!_bson_grow (bson, n_bytes))) {
      return false;
   }

   data = first_data;
   data_len = first_len;

   buf = _bson_data (bson) + bson->len - 1;

   do {
      n_pairs--;
      memcpy (buf, data, data_len);
      bson->len += data_len;
      buf += data_len;

      if (n_pairs) {
         data_len = va_arg (args, uint32_t);
         data = va_arg (args, const uint8_t *);
      }
   } while (n_pairs);

   _bson_encode_length (bson);

   *buf = '\0';

   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_append --
 *
 *       Variadic function to append length,buffer pairs to a bson_t. If the
 *       append would cause the bson_t to overflow a 32-bit length, it will
 *       return false and no append will have occurred.
 *
 * Parameters:
 *       @bson: A bson_t.
 *       @n_pairs: Number of length,buffer pairs.
 *       @n_bytes: the total number of bytes being appended.
 *       @first_len: Length of first buffer.
 *       @first_data: First buffer.
 *
 * Returns:
 *       true if successful; otherwise false indicating INT_MAX overflow.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_bson_append (bson_t *bson,              /* IN */
              uint32_t n_pairs,          /* IN */
              uint32_t n_bytes,          /* IN */
              uint32_t first_len,        /* IN */
              const uint8_t *first_data, /* IN */
              ...)
{
   va_list args;
   bool ok;

   BSON_ASSERT (n_pairs);
   BSON_ASSERT (first_len);
   BSON_ASSERT (first_data);

   /*
    * Check to see if this append would overflow 32-bit signed integer. I know
    * what you're thinking. BSON uses a signed 32-bit length field? Yeah. It
    * does.
    */
   if (BSON_UNLIKELY (n_bytes > (BSON_MAX_SIZE - bson->len))) {
      return false;
   }

   va_start (args, first_data);
   ok = _bson_append_va (bson, n_bytes, n_pairs, first_len, first_data, args);
   va_end (args);

   return ok;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_append_bson_begin --
 *
 *       Begin appending a subdocument or subarray to the document using
 *       the key provided by @key.
 *
 *       If @key_length is < 0, then strlen() will be called on @key
 *       to determine the length.
 *
 *       @key_type MUST be either BSON_TYPE_DOCUMENT or BSON_TYPE_ARRAY.
 *
 * Returns:
 *       true if successful; otherwise false indicating INT_MAX overflow.
 *
 * Side effects:
 *       @child is initialized if true is returned.
 *
 *--------------------------------------------------------------------------
 */

static bool
_bson_append_bson_begin (bson_t *bson,           /* IN */
                         const char *key,        /* IN */
                         int key_length,         /* IN */
                         bson_type_t child_type, /* IN */
                         bson_t *child)          /* OUT */
{
   const uint8_t type = child_type;
   const uint8_t empty[5] = {5};
   bson_impl_alloc_t *aparent = (bson_impl_alloc_t *) bson;
   bson_impl_alloc_t *achild = (bson_impl_alloc_t *) child;

   BSON_ASSERT (!(bson->flags & BSON_FLAG_RDONLY));
   BSON_ASSERT (!(bson->flags & BSON_FLAG_IN_CHILD));
   BSON_ASSERT (key);
   BSON_ASSERT ((child_type == BSON_TYPE_DOCUMENT) ||
                (child_type == BSON_TYPE_ARRAY));
   BSON_ASSERT (child);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   /*
    * If the parent is an inline bson_t, then we need to convert
    * it to a heap allocated buffer. This makes extending buffers
    * of child bson documents much simpler logic, as they can just
    * realloc the *buf pointer.
    */
   if ((bson->flags & BSON_FLAG_INLINE)) {
      BSON_ASSERT (bson->len <= 120);
      if (!_bson_grow (bson, 128 - bson->len)) {
         return false;
      }
      BSON_ASSERT (!(bson->flags & BSON_FLAG_INLINE));
   }

   /*
    * Append the type and key for the field.
    */
   if (!_bson_append (bson,
                      4,
                      (1 + key_length + 1 + 5),
                      1,
                      &type,
                      key_length,
                      key,
                      1,
                      &gZero,
                      5,
                      empty)) {
      return false;
   }

   /*
    * Mark the document as working on a child document so that no
    * further modifications can happen until the caller has called
    * bson_append_{document,array}_end().
    */
   bson->flags |= BSON_FLAG_IN_CHILD;

   /*
    * Initialize the child bson_t structure and point it at the parents
    * buffers. This allows us to realloc directly from the child without
    * walking up to the parent bson_t.
    */
   achild->flags = (BSON_FLAG_CHILD | BSON_FLAG_NO_FREE | BSON_FLAG_STATIC);

   if ((bson->flags & BSON_FLAG_CHILD)) {
      achild->depth = ((bson_impl_alloc_t *) bson)->depth + 1;
   } else {
      achild->depth = 1;
   }

   achild->parent = bson;
   achild->buf = aparent->buf;
   achild->buflen = aparent->buflen;
   achild->offset = aparent->offset + aparent->len - 1 - 5;
   achild->len = 5;
   achild->alloc = NULL;
   achild->alloclen = 0;
   achild->realloc = aparent->realloc;
   achild->realloc_func_ctx = aparent->realloc_func_ctx;

   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_append_bson_end --
 *
 *       Complete a call to _bson_append_bson_begin.
 *
 * Returns:
 *       true if successful.
 *
 * Side effects:
 *       @child is destroyed and no longer valid after calling this
 *       function.
 *
 *--------------------------------------------------------------------------
 */

static bool
_bson_append_bson_end (bson_t *bson,  /* IN */
                       bson_t *child) /* IN */
{
   BSON_ASSERT (bson);
   BSON_ASSERT ((bson->flags & BSON_FLAG_IN_CHILD));
   BSON_ASSERT (!(child->flags & BSON_FLAG_IN_CHILD));

   /*
    * Unmark the IN_CHILD flag.
    */
   bson->flags &= ~BSON_FLAG_IN_CHILD;

   /*
    * Now that we are done building the sub-document, add the size to the
    * parent, not including the default 5 byte empty document already added.
    */
   bson->len = (bson->len + child->len - 5);

   /*
    * Ensure we have a \0 byte at the end and proper length encoded at
    * the beginning of the document.
    */
   _bson_data (bson)[bson->len - 1] = '\0';
   _bson_encode_length (bson);

   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_array_begin --
 *
 *       Start appending a new array.
 *
 *       Use @child to append to the data area for the given field.
 *
 *       It is a programming error to call any other bson function on
 *       @bson until bson_append_array_end() has been called. It is
 *       valid to call bson_append*() functions on @child.
 *
 *       This function is useful to allow building nested documents using
 *       a single buffer owned by the top-level bson document.
 *
 * Returns:
 *       true if successful; otherwise false and @child is invalid.
 *
 * Side effects:
 *       @child is initialized if true is returned.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_array_begin (bson_t *bson,    /* IN */
                         const char *key, /* IN */
                         int key_length,  /* IN */
                         bson_t *child)   /* IN */
{
   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (child);

   return _bson_append_bson_begin (
      bson, key, key_length, BSON_TYPE_ARRAY, child);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_array_end --
 *
 *       Complete a call to bson_append_array_begin().
 *
 *       It is safe to append other fields to @bson after calling this
 *       function.
 *
 * Returns:
 *       true if successful.
 *
 * Side effects:
 *       @child is invalid after calling this function.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_array_end (bson_t *bson,  /* IN */
                       bson_t *child) /* IN */
{
   BSON_ASSERT (bson);
   BSON_ASSERT (child);

   return _bson_append_bson_end (bson, child);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_document_begin --
 *
 *       Start appending a new document.
 *
 *       Use @child to append to the data area for the given field.
 *
 *       It is a programming error to call any other bson function on
 *       @bson until bson_append_document_end() has been called. It is
 *       valid to call bson_append*() functions on @child.
 *
 *       This function is useful to allow building nested documents using
 *       a single buffer owned by the top-level bson document.
 *
 * Returns:
 *       true if successful; otherwise false and @child is invalid.
 *
 * Side effects:
 *       @child is initialized if true is returned.
 *
 *--------------------------------------------------------------------------
 */
bool
bson_append_document_begin (bson_t *bson,    /* IN */
                            const char *key, /* IN */
                            int key_length,  /* IN */
                            bson_t *child)   /* IN */
{
   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (child);

   return _bson_append_bson_begin (
      bson, key, key_length, BSON_TYPE_DOCUMENT, child);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_document_end --
 *
 *       Complete a call to bson_append_document_begin().
 *
 *       It is safe to append new fields to @bson after calling this
 *       function, if true is returned.
 *
 * Returns:
 *       true if successful; otherwise false indicating INT_MAX overflow.
 *
 * Side effects:
 *       @child is destroyed and invalid after calling this function.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_document_end (bson_t *bson,  /* IN */
                          bson_t *child) /* IN */
{
   BSON_ASSERT (bson);
   BSON_ASSERT (child);

   return _bson_append_bson_end (bson, child);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_array --
 *
 *       Append an array to @bson.
 *
 *       Generally, bson_append_array_begin() will result in faster code
 *       since few buffers need to be malloced.
 *
 * Returns:
 *       true if successful; otherwise false indicating INT_MAX overflow.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_array (bson_t *bson,        /* IN */
                   const char *key,     /* IN */
                   int key_length,      /* IN */
                   const bson_t *array) /* IN */
{
   static const uint8_t type = BSON_TYPE_ARRAY;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (array);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   /*
    * Let's be a bit pedantic and ensure the array has properly formatted key
    * names.  We will verify this simply by checking the first element for "0"
    * if the array is non-empty.
    */
   if (array && !bson_empty (array)) {
      bson_iter_t iter;

      if (bson_iter_init (&iter, array) && bson_iter_next (&iter)) {
         if (0 != strcmp ("0", bson_iter_key (&iter))) {
            fprintf (stderr,
                     "%s(): invalid array detected. first element of array "
                     "parameter is not \"0\".\n",
                     BSON_FUNC);
         }
      }
   }

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + array->len),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        array->len,
                        _bson_data (array));
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_binary --
 *
 *       Append binary data to @bson. The field will have the
 *       BSON_TYPE_BINARY type.
 *
 * Parameters:
 *       @subtype: the BSON Binary Subtype. See bsonspec.org for more
 *                 information.
 *       @binary: a pointer to the raw binary data.
 *       @length: the size of @binary in bytes.
 *
 * Returns:
 *       true if successful; otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_binary (bson_t *bson,           /* IN */
                    const char *key,        /* IN */
                    int key_length,         /* IN */
                    bson_subtype_t subtype, /* IN */
                    const uint8_t *binary,  /* IN */
                    uint32_t length)        /* IN */
{
   static const uint8_t type = BSON_TYPE_BINARY;
   uint32_t length_le;
   uint32_t deprecated_length_le;
   uint8_t subtype8 = 0;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (binary);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   subtype8 = subtype;

   if (subtype == BSON_SUBTYPE_BINARY_DEPRECATED) {
      length_le = BSON_UINT32_TO_LE (length + 4);
      deprecated_length_le = BSON_UINT32_TO_LE (length);

      return _bson_append (bson,
                           7,
                           (1 + key_length + 1 + 4 + 1 + 4 + length),
                           1,
                           &type,
                           key_length,
                           key,
                           1,
                           &gZero,
                           4,
                           &length_le,
                           1,
                           &subtype8,
                           4,
                           &deprecated_length_le,
                           length,
                           binary);
   } else {
      length_le = BSON_UINT32_TO_LE (length);

      return _bson_append (bson,
                           6,
                           (1 + key_length + 1 + 4 + 1 + length),
                           1,
                           &type,
                           key_length,
                           key,
                           1,
                           &gZero,
                           4,
                           &length_le,
                           1,
                           &subtype8,
                           length,
                           binary);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_bool --
 *
 *       Append a new field to @bson with the name @key. The value is
 *       a boolean indicated by @value.
 *
 * Returns:
 *       true if succesful; otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_bool (bson_t *bson,    /* IN */
                  const char *key, /* IN */
                  int key_length,  /* IN */
                  bool value)      /* IN */
{
   static const uint8_t type = BSON_TYPE_BOOL;
   uint8_t abyte = !!value;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + 1),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        1,
                        &abyte);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_code --
 *
 *       Append a new field to @bson containing javascript code.
 *
 *       @javascript MUST be a zero terminated UTF-8 string. It MUST NOT
 *       containing embedded \0 characters.
 *
 * Returns:
 *       true if successful; otherwise false.
 *
 * Side effects:
 *       None.
 *
 * See also:
 *       bson_append_code_with_scope().
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_code (bson_t *bson,           /* IN */
                  const char *key,        /* IN */
                  int key_length,         /* IN */
                  const char *javascript) /* IN */
{
   static const uint8_t type = BSON_TYPE_CODE;
   uint32_t length;
   uint32_t length_le;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (javascript);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   length = (int) strlen (javascript) + 1;
   length_le = BSON_UINT32_TO_LE (length);

   return _bson_append (bson,
                        5,
                        (1 + key_length + 1 + 4 + length),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        4,
                        &length_le,
                        length,
                        javascript);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_code_with_scope --
 *
 *       Append a new field to @bson containing javascript code with
 *       supplied scope.
 *
 * Returns:
 *       true if successful; otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_code_with_scope (bson_t *bson,           /* IN */
                             const char *key,        /* IN */
                             int key_length,         /* IN */
                             const char *javascript, /* IN */
                             const bson_t *scope)    /* IN */
{
   static const uint8_t type = BSON_TYPE_CODEWSCOPE;
   uint32_t codews_length_le;
   uint32_t codews_length;
   uint32_t js_length_le;
   uint32_t js_length;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (javascript);

   if (scope == NULL) {
      return bson_append_code (bson, key, key_length, javascript);
   }

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   js_length = (int) strlen (javascript) + 1;
   js_length_le = BSON_UINT32_TO_LE (js_length);

   codews_length = 4 + 4 + js_length + scope->len;
   codews_length_le = BSON_UINT32_TO_LE (codews_length);

   return _bson_append (bson,
                        7,
                        (1 + key_length + 1 + 4 + 4 + js_length + scope->len),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        4,
                        &codews_length_le,
                        4,
                        &js_length_le,
                        js_length,
                        javascript,
                        scope->len,
                        _bson_data (scope));
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_dbpointer --
 *
 *       This BSON data type is DEPRECATED.
 *
 *       Append a BSON dbpointer field to @bson.
 *
 * Returns:
 *       true if successful; otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_dbpointer (bson_t *bson,           /* IN */
                       const char *key,        /* IN */
                       int key_length,         /* IN */
                       const char *collection, /* IN */
                       const bson_oid_t *oid)
{
   static const uint8_t type = BSON_TYPE_DBPOINTER;
   uint32_t length;
   uint32_t length_le;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (collection);
   BSON_ASSERT (oid);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   length = (int) strlen (collection) + 1;
   length_le = BSON_UINT32_TO_LE (length);

   return _bson_append (bson,
                        6,
                        (1 + key_length + 1 + 4 + length + 12),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        4,
                        &length_le,
                        length,
                        collection,
                        12,
                        oid);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_append_document --
 *
 *       Append a new field to @bson containing a BSON document.
 *
 *       In general, using bson_append_document_begin() results in faster
 *       code and less memory fragmentation.
 *
 * Returns:
 *       true if successful; otherwise false.
 *
 * Side effects:
 *       None.
 *
 * See also:
 *       bson_append_document_begin().
 *
 *--------------------------------------------------------------------------
 */

bool
bson_append_document (bson_t *bson,        /* IN */
                      const char *key,     /* IN */
                      int key_length,      /* IN */
                      const bson_t *value) /* IN */
{
   static const uint8_t type = BSON_TYPE_DOCUMENT;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (value);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + value->len),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        value->len,
                        _bson_data (value));
}


bool
bson_append_double (bson_t *bson, const char *key, int key_length, double value)
{
   static const uint8_t type = BSON_TYPE_DOUBLE;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

#if BSON_BYTE_ORDER == BSON_BIG_ENDIAN
   value = BSON_DOUBLE_TO_LE (value);
#endif

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + 8),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        8,
                        &value);
}


bool
bson_append_int32 (bson_t *bson, const char *key, int key_length, int32_t value)
{
   static const uint8_t type = BSON_TYPE_INT32;
   uint32_t value_le;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   value_le = BSON_UINT32_TO_LE (value);

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + 4),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        4,
                        &value_le);
}


bool
bson_append_int64 (bson_t *bson, const char *key, int key_length, int64_t value)
{
   static const uint8_t type = BSON_TYPE_INT64;
   uint64_t value_le;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   value_le = BSON_UINT64_TO_LE (value);

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + 8),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        8,
                        &value_le);
}


bool
bson_append_decimal128 (bson_t *bson,
                        const char *key,
                        int key_length,
                        const bson_decimal128_t *value)
{
   static const uint8_t type = BSON_TYPE_DECIMAL128;
   uint64_t value_le[2];

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (value);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   value_le[0] = BSON_UINT64_TO_LE (value->low);
   value_le[1] = BSON_UINT64_TO_LE (value->high);

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + 16),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        16,
                        value_le);
}


bool
bson_append_iter (bson_t *bson,
                  const char *key,
                  int key_length,
                  const bson_iter_t *iter)
{
   bool ret = false;

   BSON_ASSERT (bson);
   BSON_ASSERT (iter);

   if (!key) {
      key = bson_iter_key (iter);
      key_length = -1;
   }

   switch (bson_iter_type_unsafe (iter)) {
   case BSON_TYPE_EOD:
      return false;
   case BSON_TYPE_DOUBLE:
      ret = bson_append_double (bson, key, key_length, bson_iter_double (iter));
      break;
   case BSON_TYPE_UTF8: {
      uint32_t len = 0;
      const char *str;

      str = bson_iter_utf8 (iter, &len);
      ret = bson_append_utf8 (bson, key, key_length, str, len);
   } break;
   case BSON_TYPE_DOCUMENT: {
      const uint8_t *buf = NULL;
      uint32_t len = 0;
      bson_t doc;

      bson_iter_document (iter, &len, &buf);

      if (bson_init_static (&doc, buf, len)) {
         ret = bson_append_document (bson, key, key_length, &doc);
         bson_destroy (&doc);
      }
   } break;
   case BSON_TYPE_ARRAY: {
      const uint8_t *buf = NULL;
      uint32_t len = 0;
      bson_t doc;

      bson_iter_array (iter, &len, &buf);

      if (bson_init_static (&doc, buf, len)) {
         ret = bson_append_array (bson, key, key_length, &doc);
         bson_destroy (&doc);
      }
   } break;
   case BSON_TYPE_BINARY: {
      const uint8_t *binary = NULL;
      bson_subtype_t subtype = BSON_SUBTYPE_BINARY;
      uint32_t len = 0;

      bson_iter_binary (iter, &subtype, &len, &binary);
      ret = bson_append_binary (bson, key, key_length, subtype, binary, len);
   } break;
   case BSON_TYPE_UNDEFINED:
      ret = bson_append_undefined (bson, key, key_length);
      break;
   case BSON_TYPE_OID:
      ret = bson_append_oid (bson, key, key_length, bson_iter_oid (iter));
      break;
   case BSON_TYPE_BOOL:
      ret = bson_append_bool (bson, key, key_length, bson_iter_bool (iter));
      break;
   case BSON_TYPE_DATE_TIME:
      ret = bson_append_date_time (
         bson, key, key_length, bson_iter_date_time (iter));
      break;
   case BSON_TYPE_NULL:
      ret = bson_append_null (bson, key, key_length);
      break;
   case BSON_TYPE_REGEX: {
      const char *regex;
      const char *options;

      regex = bson_iter_regex (iter, &options);
      ret = bson_append_regex (bson, key, key_length, regex, options);
   } break;
   case BSON_TYPE_DBPOINTER: {
      const bson_oid_t *oid;
      uint32_t len;
      const char *collection;

      bson_iter_dbpointer (iter, &len, &collection, &oid);
      ret = bson_append_dbpointer (bson, key, key_length, collection, oid);
   } break;
   case BSON_TYPE_CODE: {
      uint32_t len;
      const char *code;

      code = bson_iter_code (iter, &len);
      ret = bson_append_code (bson, key, key_length, code);
   } break;
   case BSON_TYPE_SYMBOL: {
      uint32_t len;
      const char *symbol;

      symbol = bson_iter_symbol (iter, &len);
      ret = bson_append_symbol (bson, key, key_length, symbol, len);
   } break;
   case BSON_TYPE_CODEWSCOPE: {
      const uint8_t *scope = NULL;
      uint32_t scope_len = 0;
      uint32_t len = 0;
      const char *javascript = NULL;
      bson_t doc;

      javascript = bson_iter_codewscope (iter, &len, &scope_len, &scope);

      if (bson_init_static (&doc, scope, scope_len)) {
         ret = bson_append_code_with_scope (
            bson, key, key_length, javascript, &doc);
         bson_destroy (&doc);
      }
   } break;
   case BSON_TYPE_INT32:
      ret = bson_append_int32 (bson, key, key_length, bson_iter_int32 (iter));
      break;
   case BSON_TYPE_TIMESTAMP: {
      uint32_t ts;
      uint32_t inc;

      bson_iter_timestamp (iter, &ts, &inc);
      ret = bson_append_timestamp (bson, key, key_length, ts, inc);
   } break;
   case BSON_TYPE_INT64:
      ret = bson_append_int64 (bson, key, key_length, bson_iter_int64 (iter));
      break;
   case BSON_TYPE_DECIMAL128: {
      bson_decimal128_t dec;

      if (!bson_iter_decimal128 (iter, &dec)) {
         return false;
      }

      ret = bson_append_decimal128 (bson, key, key_length, &dec);
   } break;
   case BSON_TYPE_MAXKEY:
      ret = bson_append_maxkey (bson, key, key_length);
      break;
   case BSON_TYPE_MINKEY:
      ret = bson_append_minkey (bson, key, key_length);
      break;
   default:
      break;
   }

   return ret;
}


bool
bson_append_maxkey (bson_t *bson, const char *key, int key_length)
{
   static const uint8_t type = BSON_TYPE_MAXKEY;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   return _bson_append (
      bson, 3, (1 + key_length + 1), 1, &type, key_length, key, 1, &gZero);
}


bool
bson_append_minkey (bson_t *bson, const char *key, int key_length)
{
   static const uint8_t type = BSON_TYPE_MINKEY;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   return _bson_append (
      bson, 3, (1 + key_length + 1), 1, &type, key_length, key, 1, &gZero);
}


bool
bson_append_null (bson_t *bson, const char *key, int key_length)
{
   static const uint8_t type = BSON_TYPE_NULL;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   return _bson_append (
      bson, 3, (1 + key_length + 1), 1, &type, key_length, key, 1, &gZero);
}


bool
bson_append_oid (bson_t *bson,
                 const char *key,
                 int key_length,
                 const bson_oid_t *value)
{
   static const uint8_t type = BSON_TYPE_OID;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (value);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + 12),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        12,
                        value);
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_append_regex_options_sorted --
 *
 *       Helper to append regex options to a buffer in a sorted order.
 *       Any duplicate or unsupported options will be ignored.
 *
 * Parameters:
 *       @buffer: Buffer to which sorted options will be appended
 *       @options: Regex options
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static BSON_INLINE void
_bson_append_regex_options_sorted (bson_string_t *buffer, /* IN */
                                   const char *options)   /* IN */
{
   const char *c;

   for (c = BSON_REGEX_OPTIONS_SORTED; *c; c++) {
      if (strchr (options, *c)) {
         bson_string_append_c (buffer, *c);
      }
   }
}


bool
bson_append_regex (bson_t *bson,
                   const char *key,
                   int key_length,
                   const char *regex,
                   const char *options)
{
   static const uint8_t type = BSON_TYPE_REGEX;
   uint32_t regex_len;
   bson_string_t *options_sorted;
   bool r;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   if (!regex) {
      regex = "";
   }

   if (!options) {
      options = "";
   }

   regex_len = (int) strlen (regex) + 1;
   options_sorted = bson_string_new (NULL);

   _bson_append_regex_options_sorted (options_sorted, options);

   r =  _bson_append (bson,
                      5,
                      (1 + key_length + 1 + regex_len + options_sorted->len),
                      1,
                      &type,
                      key_length,
                      key,
                      1,
                      &gZero,
                      regex_len,
                      regex,
                      options_sorted->len + 1,
                      options_sorted->str);

   bson_string_free (options_sorted, true);

   return r;
}


bool
bson_append_utf8 (
   bson_t *bson, const char *key, int key_length, const char *value, int length)
{
   static const uint8_t type = BSON_TYPE_UTF8;
   uint32_t length_le;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (BSON_UNLIKELY (!value)) {
      return bson_append_null (bson, key, key_length);
   }

   if (BSON_UNLIKELY (key_length < 0)) {
      key_length = (int) strlen (key);
   }

   if (BSON_UNLIKELY (length < 0)) {
      length = (int) strlen (value);
   }

   length_le = BSON_UINT32_TO_LE (length + 1);

   return _bson_append (bson,
                        6,
                        (1 + key_length + 1 + 4 + length + 1),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        4,
                        &length_le,
                        length,
                        value,
                        1,
                        &gZero);
}


bool
bson_append_symbol (
   bson_t *bson, const char *key, int key_length, const char *value, int length)
{
   static const uint8_t type = BSON_TYPE_SYMBOL;
   uint32_t length_le;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (!value) {
      return bson_append_null (bson, key, key_length);
   }

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   if (length < 0) {
      length = (int) strlen (value);
   }

   length_le = BSON_UINT32_TO_LE (length + 1);

   return _bson_append (bson,
                        6,
                        (1 + key_length + 1 + 4 + length + 1),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        4,
                        &length_le,
                        length,
                        value,
                        1,
                        &gZero);
}


bool
bson_append_time_t (bson_t *bson, const char *key, int key_length, time_t value)
{
#ifdef BSON_OS_WIN32
   struct timeval tv = {(long) value, 0};
#else
   struct timeval tv = {value, 0};
#endif

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   return bson_append_timeval (bson, key, key_length, &tv);
}


bool
bson_append_timestamp (bson_t *bson,
                       const char *key,
                       int key_length,
                       uint32_t timestamp,
                       uint32_t increment)
{
   static const uint8_t type = BSON_TYPE_TIMESTAMP;
   uint64_t value;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   value = ((((uint64_t) timestamp) << 32) | ((uint64_t) increment));
   value = BSON_UINT64_TO_LE (value);

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + 8),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        8,
                        &value);
}


bool
bson_append_now_utc (bson_t *bson, const char *key, int key_length)
{
   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (key_length >= -1);

   return bson_append_time_t (bson, key, key_length, time (NULL));
}


bool
bson_append_date_time (bson_t *bson,
                       const char *key,
                       int key_length,
                       int64_t value)
{
   static const uint8_t type = BSON_TYPE_DATE_TIME;
   uint64_t value_le;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   value_le = BSON_UINT64_TO_LE (value);

   return _bson_append (bson,
                        4,
                        (1 + key_length + 1 + 8),
                        1,
                        &type,
                        key_length,
                        key,
                        1,
                        &gZero,
                        8,
                        &value_le);
}


bool
bson_append_timeval (bson_t *bson,
                     const char *key,
                     int key_length,
                     struct timeval *value)
{
   uint64_t unix_msec;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (value);

   unix_msec =
      (((uint64_t) value->tv_sec) * 1000UL) + (value->tv_usec / 1000UL);
   return bson_append_date_time (bson, key, key_length, unix_msec);
}


bool
bson_append_undefined (bson_t *bson, const char *key, int key_length)
{
   static const uint8_t type = BSON_TYPE_UNDEFINED;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (key_length < 0) {
      key_length = (int) strlen (key);
   }

   return _bson_append (
      bson, 3, (1 + key_length + 1), 1, &type, key_length, key, 1, &gZero);
}


bool
bson_append_value (bson_t *bson,
                   const char *key,
                   int key_length,
                   const bson_value_t *value)
{
   bson_t local;
   bool ret = false;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);
   BSON_ASSERT (value);

   switch (value->value_type) {
   case BSON_TYPE_DOUBLE:
      ret = bson_append_double (bson, key, key_length, value->value.v_double);
      break;
   case BSON_TYPE_UTF8:
      ret = bson_append_utf8 (bson,
                              key,
                              key_length,
                              value->value.v_utf8.str,
                              value->value.v_utf8.len);
      break;
   case BSON_TYPE_DOCUMENT:
      if (bson_init_static (
             &local, value->value.v_doc.data, value->value.v_doc.data_len)) {
         ret = bson_append_document (bson, key, key_length, &local);
         bson_destroy (&local);
      }
      break;
   case BSON_TYPE_ARRAY:
      if (bson_init_static (
             &local, value->value.v_doc.data, value->value.v_doc.data_len)) {
         ret = bson_append_array (bson, key, key_length, &local);
         bson_destroy (&local);
      }
      break;
   case BSON_TYPE_BINARY:
      ret = bson_append_binary (bson,
                                key,
                                key_length,
                                value->value.v_binary.subtype,
                                value->value.v_binary.data,
                                value->value.v_binary.data_len);
      break;
   case BSON_TYPE_UNDEFINED:
      ret = bson_append_undefined (bson, key, key_length);
      break;
   case BSON_TYPE_OID:
      ret = bson_append_oid (bson, key, key_length, &value->value.v_oid);
      break;
   case BSON_TYPE_BOOL:
      ret = bson_append_bool (bson, key, key_length, value->value.v_bool);
      break;
   case BSON_TYPE_DATE_TIME:
      ret =
         bson_append_date_time (bson, key, key_length, value->value.v_datetime);
      break;
   case BSON_TYPE_NULL:
      ret = bson_append_null (bson, key, key_length);
      break;
   case BSON_TYPE_REGEX:
      ret = bson_append_regex (bson,
                               key,
                               key_length,
                               value->value.v_regex.regex,
                               value->value.v_regex.options);
      break;
   case BSON_TYPE_DBPOINTER:
      ret = bson_append_dbpointer (bson,
                                   key,
                                   key_length,
                                   value->value.v_dbpointer.collection,
                                   &value->value.v_dbpointer.oid);
      break;
   case BSON_TYPE_CODE:
      ret = bson_append_code (bson, key, key_length, value->value.v_code.code);
      break;
   case BSON_TYPE_SYMBOL:
      ret = bson_append_symbol (bson,
                                key,
                                key_length,
                                value->value.v_symbol.symbol,
                                value->value.v_symbol.len);
      break;
   case BSON_TYPE_CODEWSCOPE:
      if (bson_init_static (&local,
                            value->value.v_codewscope.scope_data,
                            value->value.v_codewscope.scope_len)) {
         ret = bson_append_code_with_scope (
            bson, key, key_length, value->value.v_codewscope.code, &local);
         bson_destroy (&local);
      }
      break;
   case BSON_TYPE_INT32:
      ret = bson_append_int32 (bson, key, key_length, value->value.v_int32);
      break;
   case BSON_TYPE_TIMESTAMP:
      ret = bson_append_timestamp (bson,
                                   key,
                                   key_length,
                                   value->value.v_timestamp.timestamp,
                                   value->value.v_timestamp.increment);
      break;
   case BSON_TYPE_INT64:
      ret = bson_append_int64 (bson, key, key_length, value->value.v_int64);
      break;
   case BSON_TYPE_DECIMAL128:
      ret = bson_append_decimal128 (
         bson, key, key_length, &(value->value.v_decimal128));
      break;
   case BSON_TYPE_MAXKEY:
      ret = bson_append_maxkey (bson, key, key_length);
      break;
   case BSON_TYPE_MINKEY:
      ret = bson_append_minkey (bson, key, key_length);
      break;
   case BSON_TYPE_EOD:
   default:
      break;
   }

   return ret;
}


void
bson_init (bson_t *bson)
{
   bson_impl_inline_t *impl = (bson_impl_inline_t *) bson;

   BSON_ASSERT (bson);

   impl->flags = BSON_FLAG_INLINE | BSON_FLAG_STATIC;
   impl->len = 5;
   impl->data[0] = 5;
   impl->data[1] = 0;
   impl->data[2] = 0;
   impl->data[3] = 0;
   impl->data[4] = 0;
}


void
bson_reinit (bson_t *bson)
{
   uint8_t *data;

   BSON_ASSERT (bson);

   data = _bson_data (bson);

   bson->len = 5;

   data[0] = 5;
   data[1] = 0;
   data[2] = 0;
   data[3] = 0;
   data[4] = 0;
}


bool
bson_init_static (bson_t *bson, const uint8_t *data, size_t length)
{
   bson_impl_alloc_t *impl = (bson_impl_alloc_t *) bson;
   uint32_t len_le;

   BSON_ASSERT (bson);
   BSON_ASSERT (data);

   if ((length < 5) || (length > INT_MAX)) {
      return false;
   }

   memcpy (&len_le, data, sizeof (len_le));

   if ((size_t) BSON_UINT32_FROM_LE (len_le) != length) {
      return false;
   }

   if (data[length - 1]) {
      return false;
   }

   impl->flags = BSON_FLAG_STATIC | BSON_FLAG_RDONLY;
   impl->len = (uint32_t) length;
   impl->parent = NULL;
   impl->depth = 0;
   impl->buf = &impl->alloc;
   impl->buflen = &impl->alloclen;
   impl->offset = 0;
   impl->alloc = (uint8_t *) data;
   impl->alloclen = length;
   impl->realloc = NULL;
   impl->realloc_func_ctx = NULL;

   return true;
}


bson_t *
bson_new (void)
{
   bson_impl_inline_t *impl;
   bson_t *bson;

   bson = bson_malloc (sizeof *bson);

   impl = (bson_impl_inline_t *) bson;
   impl->flags = BSON_FLAG_INLINE;
   impl->len = 5;
   impl->data[0] = 5;
   impl->data[1] = 0;
   impl->data[2] = 0;
   impl->data[3] = 0;
   impl->data[4] = 0;

   return bson;
}


bson_t *
bson_sized_new (size_t size)
{
   bson_impl_alloc_t *impl_a;
   bson_t *b;

   BSON_ASSERT (size <= INT32_MAX);

   b = bson_malloc (sizeof *b);
   impl_a = (bson_impl_alloc_t *) b;

   if (size <= BSON_INLINE_DATA_SIZE) {
      bson_init (b);
      b->flags &= ~BSON_FLAG_STATIC;
   } else {
      impl_a->flags = BSON_FLAG_NONE;
      impl_a->len = 5;
      impl_a->parent = NULL;
      impl_a->depth = 0;
      impl_a->buf = &impl_a->alloc;
      impl_a->buflen = &impl_a->alloclen;
      impl_a->offset = 0;
      impl_a->alloclen = BSON_MAX (5, size);
      impl_a->alloc = bson_malloc (impl_a->alloclen);
      impl_a->alloc[0] = 5;
      impl_a->alloc[1] = 0;
      impl_a->alloc[2] = 0;
      impl_a->alloc[3] = 0;
      impl_a->alloc[4] = 0;
      impl_a->realloc = bson_realloc_ctx;
      impl_a->realloc_func_ctx = NULL;
   }

   return b;
}


bson_t *
bson_new_from_data (const uint8_t *data, size_t length)
{
   uint32_t len_le;
   bson_t *bson;

   BSON_ASSERT (data);

   if ((length < 5) || (length > INT_MAX) || data[length - 1]) {
      return NULL;
   }

   memcpy (&len_le, data, sizeof (len_le));

   if (length != (size_t) BSON_UINT32_FROM_LE (len_le)) {
      return NULL;
   }

   bson = bson_sized_new (length);
   memcpy (_bson_data (bson), data, length);
   bson->len = (uint32_t) length;

   return bson;
}


bson_t *
bson_new_from_buffer (uint8_t **buf,
                      size_t *buf_len,
                      bson_realloc_func realloc_func,
                      void *realloc_func_ctx)
{
   bson_impl_alloc_t *impl;
   uint32_t len_le;
   uint32_t length;
   bson_t *bson;

   BSON_ASSERT (buf);
   BSON_ASSERT (buf_len);

   if (!realloc_func) {
      realloc_func = bson_realloc_ctx;
   }

   bson = bson_malloc0 (sizeof *bson);
   impl = (bson_impl_alloc_t *) bson;

   if (!*buf) {
      length = 5;
      len_le = BSON_UINT32_TO_LE (length);
      *buf_len = 5;
      *buf = realloc_func (*buf, *buf_len, realloc_func_ctx);
      memcpy (*buf, &len_le, sizeof (len_le));
      (*buf)[4] = '\0';
   } else {
      if ((*buf_len < 5) || (*buf_len > INT_MAX)) {
         bson_free (bson);
         return NULL;
      }

      memcpy (&len_le, *buf, sizeof (len_le));
      length = BSON_UINT32_FROM_LE (len_le);
   }

   if ((*buf)[length - 1]) {
      bson_free (bson);
      return NULL;
   }

   impl->flags = BSON_FLAG_NO_FREE;
   impl->len = length;
   impl->buf = buf;
   impl->buflen = buf_len;
   impl->realloc = realloc_func;
   impl->realloc_func_ctx = realloc_func_ctx;

   return bson;
}


bson_t *
bson_copy (const bson_t *bson)
{
   const uint8_t *data;

   BSON_ASSERT (bson);

   data = _bson_data (bson);
   return bson_new_from_data (data, bson->len);
}


void
bson_copy_to (const bson_t *src, bson_t *dst)
{
   const uint8_t *data;
   bson_impl_alloc_t *adst;
   size_t len;

   BSON_ASSERT (src);
   BSON_ASSERT (dst);

   if ((src->flags & BSON_FLAG_INLINE)) {
      memcpy (dst, src, sizeof *dst);
      dst->flags = (BSON_FLAG_STATIC | BSON_FLAG_INLINE);
      return;
   }

   data = _bson_data (src);
   len = bson_next_power_of_two ((size_t) src->len);

   adst = (bson_impl_alloc_t *) dst;
   adst->flags = BSON_FLAG_STATIC;
   adst->len = src->len;
   adst->parent = NULL;
   adst->depth = 0;
   adst->buf = &adst->alloc;
   adst->buflen = &adst->alloclen;
   adst->offset = 0;
   adst->alloc = bson_malloc (len);
   adst->alloclen = len;
   adst->realloc = bson_realloc_ctx;
   adst->realloc_func_ctx = NULL;
   memcpy (adst->alloc, data, src->len);
}


static bool
should_ignore (const char *first_exclude, va_list args, const char *name)
{
   bool ret = false;
   const char *exclude = first_exclude;
   va_list args_copy;

   va_copy (args_copy, args);

   do {
      if (!strcmp (name, exclude)) {
         ret = true;
         break;
      }
   } while ((exclude = va_arg (args_copy, const char *)));

   va_end (args_copy);

   return ret;
}


static void
_bson_copy_to_excluding_va (const bson_t *src,
                            bson_t *dst,
                            const char *first_exclude,
                            va_list args)
{
   bson_iter_t iter;

   if (bson_iter_init (&iter, src)) {
      while (bson_iter_next (&iter)) {
         if (!should_ignore (first_exclude, args, bson_iter_key (&iter))) {
            if (!bson_append_iter (dst, NULL, 0, &iter)) {
               /*
                * This should not be able to happen since we are copying
                * from within a valid bson_t.
                */
               BSON_ASSERT (false);
               return;
            }
         }
      }
   }
}


void
bson_copy_to_excluding (const bson_t *src,
                        bson_t *dst,
                        const char *first_exclude,
                        ...)
{
   va_list args;

   BSON_ASSERT (src);
   BSON_ASSERT (dst);
   BSON_ASSERT (first_exclude);

   bson_init (dst);

   va_start (args, first_exclude);
   _bson_copy_to_excluding_va (src, dst, first_exclude, args);
   va_end (args);
}

void
bson_copy_to_excluding_noinit (const bson_t *src,
                               bson_t *dst,
                               const char *first_exclude,
                               ...)
{
   va_list args;

   BSON_ASSERT (src);
   BSON_ASSERT (dst);
   BSON_ASSERT (first_exclude);

   va_start (args, first_exclude);
   _bson_copy_to_excluding_va (src, dst, first_exclude, args);
   va_end (args);
}


void
bson_destroy (bson_t *bson)
{
   BSON_ASSERT (bson);

   if (!(bson->flags &
         (BSON_FLAG_RDONLY | BSON_FLAG_INLINE | BSON_FLAG_NO_FREE))) {
      bson_free (*((bson_impl_alloc_t *) bson)->buf);
   }

   if (!(bson->flags & BSON_FLAG_STATIC)) {
      bson_free (bson);
   }
}


uint8_t *
bson_reserve_buffer (bson_t *bson, uint32_t size)
{
   if (bson->flags &
       (BSON_FLAG_CHILD | BSON_FLAG_IN_CHILD | BSON_FLAG_RDONLY)) {
      return NULL;
   }

   if (!_bson_grow (bson, size)) {
      return NULL;
   }

   if (bson->flags & BSON_FLAG_INLINE) {
      /* bson_grow didn't spill over */
      ((bson_impl_inline_t *) bson)->len = size;
   } else {
      ((bson_impl_alloc_t *) bson)->len = size;
   }

   return _bson_data (bson);
}


bool
bson_steal (bson_t *dst, bson_t *src)
{
   bson_impl_inline_t *src_inline;
   bson_impl_inline_t *dst_inline;
   bson_impl_alloc_t *alloc;

   BSON_ASSERT (dst);
   BSON_ASSERT (src);

   bson_init (dst);

   if (src->flags & (BSON_FLAG_CHILD | BSON_FLAG_IN_CHILD | BSON_FLAG_RDONLY)) {
      return false;
   }

   if (src->flags & BSON_FLAG_INLINE) {
      src_inline = (bson_impl_inline_t *) src;
      dst_inline = (bson_impl_inline_t *) dst;
      dst_inline->len = src_inline->len;
      memcpy (dst_inline->data, src_inline->data, sizeof src_inline->data);

      /* for consistency, src is always invalid after steal, even if inline */
      src->len = 0;
   } else {
      memcpy (dst, src, sizeof (bson_t));
      alloc = (bson_impl_alloc_t *) dst;
      alloc->flags |= BSON_FLAG_STATIC;
      alloc->buf = &alloc->alloc;
      alloc->buflen = &alloc->alloclen;
   }

   if (!(src->flags & BSON_FLAG_STATIC)) {
      bson_free (src);
   } else {
      /* src is invalid after steal */
      src->len = 0;
   }

   return true;
}


uint8_t *
bson_destroy_with_steal (bson_t *bson, bool steal, uint32_t *length)
{
   uint8_t *ret = NULL;

   BSON_ASSERT (bson);

   if (length) {
      *length = bson->len;
   }

   if (!steal) {
      bson_destroy (bson);
      return NULL;
   }

   if ((bson->flags &
        (BSON_FLAG_CHILD | BSON_FLAG_IN_CHILD | BSON_FLAG_RDONLY))) {
      /* Do nothing */
   } else if ((bson->flags & BSON_FLAG_INLINE)) {
      bson_impl_inline_t *inl;

      inl = (bson_impl_inline_t *) bson;
      ret = bson_malloc (bson->len);
      memcpy (ret, inl->data, bson->len);
   } else {
      bson_impl_alloc_t *alloc;

      alloc = (bson_impl_alloc_t *) bson;
      ret = *alloc->buf;
      *alloc->buf = NULL;
   }

   bson_destroy (bson);

   return ret;
}


const uint8_t *
bson_get_data (const bson_t *bson)
{
   BSON_ASSERT (bson);

   return _bson_data (bson);
}


uint32_t
bson_count_keys (const bson_t *bson)
{
   uint32_t count = 0;
   bson_iter_t iter;

   BSON_ASSERT (bson);

   if (bson_iter_init (&iter, bson)) {
      while (bson_iter_next (&iter)) {
         count++;
      }
   }

   return count;
}


bool
bson_has_field (const bson_t *bson, const char *key)
{
   bson_iter_t iter;
   bson_iter_t child;

   BSON_ASSERT (bson);
   BSON_ASSERT (key);

   if (NULL != strchr (key, '.')) {
      return (bson_iter_init (&iter, bson) &&
              bson_iter_find_descendant (&iter, key, &child));
   }

   return bson_iter_init_find (&iter, bson, key);
}


int
bson_compare (const bson_t *bson, const bson_t *other)
{
   const uint8_t *data1;
   const uint8_t *data2;
   size_t len1;
   size_t len2;
   int64_t ret;

   data1 = _bson_data (bson) + 4;
   len1 = bson->len - 4;

   data2 = _bson_data (other) + 4;
   len2 = other->len - 4;

   if (len1 == len2) {
      return memcmp (data1, data2, len1);
   }

   ret = memcmp (data1, data2, BSON_MIN (len1, len2));

   if (ret == 0) {
      ret = (int64_t) (len1 - len2);
   }

   return (ret < 0) ? -1 : (ret > 0);
}


bool
bson_equal (const bson_t *bson, const bson_t *other)
{
   return !bson_compare (bson, other);
}


static bool
_bson_as_json_visit_utf8 (const bson_iter_t *iter,
                          const char *key,
                          size_t v_utf8_len,
                          const char *v_utf8,
                          void *data)
{
   bson_json_state_t *state = data;
   char *escaped;

   escaped = bson_utf8_escape_for_json (v_utf8, v_utf8_len);

   if (escaped) {
      bson_string_append (state->str, "\"");
      bson_string_append (state->str, escaped);
      bson_string_append (state->str, "\"");
      bson_free (escaped);
      return false;
   }

   return true;
}


static bool
_bson_as_json_visit_int32 (const bson_iter_t *iter,
                           const char *key,
                           int32_t v_int32,
                           void *data)
{
   bson_json_state_t *state = data;

   if (state->mode == BSON_JSON_MODE_CANONICAL) {
      bson_string_append_printf (
         state->str, "{ \"$numberInt\" : \"%" PRId32 "\" }", v_int32);
   } else {
      bson_string_append_printf (state->str, "%" PRId32, v_int32);
   }

   return false;
}


static bool
_bson_as_json_visit_int64 (const bson_iter_t *iter,
                           const char *key,
                           int64_t v_int64,
                           void *data)
{
   bson_json_state_t *state = data;

   if (state->mode == BSON_JSON_MODE_CANONICAL) {
      bson_string_append_printf (
         state->str, "{ \"$numberLong\" : \"%" PRId64 "\"}", v_int64);
   } else {
      bson_string_append_printf (state->str, "%" PRId64, v_int64);
   }

   return false;
}


static bool
_bson_as_json_visit_decimal128 (const bson_iter_t *iter,
                                const char *key,
                                const bson_decimal128_t *value,
                                void *data)
{
   bson_json_state_t *state = data;
   char decimal128_string[BSON_DECIMAL128_STRING];
   bson_decimal128_to_string (value, decimal128_string);

   bson_string_append (state->str, "{ \"$numberDecimal\" : \"");
   bson_string_append (state->str, decimal128_string);
   bson_string_append (state->str, "\" }");

   return false;
}


static bool
_bson_as_json_visit_double (const bson_iter_t *iter,
                            const char *key,
                            double v_double,
                            void *data)
{
   bson_json_state_t *state = data;
   bson_string_t *str = state->str;
   uint32_t start_len;
   bool legacy;

   /* Determine if legacy (i.e. unwrapped) output should be used. Relaxed mode
    * will use this for nan and inf values, which we check manually since old
    * platforms may not have isinf or isnan. */
   legacy = state->mode == BSON_JSON_MODE_LEGACY ||
            (state->mode == BSON_JSON_MODE_RELAXED &&
             !(v_double != v_double || v_double * 0 != 0));

   if (!legacy) {
      bson_string_append (state->str, "{ \"$numberDouble\" : \"");
   }

   if (!legacy && v_double != v_double) {
      bson_string_append (str, "NaN");
   } else if (!legacy && v_double * 0 != 0) {
      if (v_double > 0) {
         bson_string_append (str, "Infinity");
      } else {
         bson_string_append (str, "-Infinity");
      }
   } else {
      start_len = str->len;
      bson_string_append_printf (str, "%.20g", v_double);

      /* ensure trailing ".0" to distinguish "3" from "3.0" */
      if (strspn (&str->str[start_len], "0123456789-") ==
          str->len - start_len) {
         bson_string_append (str, ".0");
      }
   }

   if (!legacy) {
      bson_string_append (state->str, "\" }");
   }

   return false;
}


static bool
_bson_as_json_visit_undefined (const bson_iter_t *iter,
                               const char *key,
                               void *data)
{
   bson_json_state_t *state = data;

   bson_string_append (state->str, "{ \"$undefined\" : true }");

   return false;
}


static bool
_bson_as_json_visit_null (const bson_iter_t *iter, const char *key, void *data)
{
   bson_json_state_t *state = data;

   bson_string_append (state->str, "null");

   return false;
}


static bool
_bson_as_json_visit_oid (const bson_iter_t *iter,
                         const char *key,
                         const bson_oid_t *oid,
                         void *data)
{
   bson_json_state_t *state = data;
   char str[25];

   bson_oid_to_string (oid, str);
   bson_string_append (state->str, "{ \"$oid\" : \"");
   bson_string_append (state->str, str);
   bson_string_append (state->str, "\" }");

   return false;
}


static bool
_bson_as_json_visit_binary (const bson_iter_t *iter,
                            const char *key,
                            bson_subtype_t v_subtype,
                            size_t v_binary_len,
                            const uint8_t *v_binary,
                            void *data)
{
   bson_json_state_t *state = data;
   size_t b64_len;
   char *b64;

   b64_len = (v_binary_len / 3 + 1) * 4 + 1;
   b64 = bson_malloc0 (b64_len);
   b64_ntop (v_binary, v_binary_len, b64, b64_len);

   if (state->mode == BSON_JSON_MODE_CANONICAL ||
       state->mode == BSON_JSON_MODE_RELAXED) {
      bson_string_append (state->str, "{ \"$binary\" : { \"base64\": \"");
      bson_string_append (state->str, b64);
      bson_string_append (state->str, "\", \"subType\" : \"");
      bson_string_append_printf (state->str, "%02x", v_subtype);
      bson_string_append (state->str, "\" } }");
   } else {
      bson_string_append (state->str, "{ \"$binary\" : \"");
      bson_string_append (state->str, b64);
      bson_string_append (state->str, "\", \"$type\" : \"");
      bson_string_append_printf (state->str, "%02x", v_subtype);
      bson_string_append (state->str, "\" }");
   }

   bson_free (b64);

   return false;
}


static bool
_bson_as_json_visit_bool (const bson_iter_t *iter,
                          const char *key,
                          bool v_bool,
                          void *data)
{
   bson_json_state_t *state = data;

   bson_string_append (state->str, v_bool ? "true" : "false");

   return false;
}


static bool
_bson_as_json_visit_date_time (const bson_iter_t *iter,
                               const char *key,
                               int64_t msec_since_epoch,
                               void *data)
{
   bson_json_state_t *state = data;

   if (state->mode == BSON_JSON_MODE_CANONICAL ||
       (state->mode == BSON_JSON_MODE_RELAXED && msec_since_epoch < 0)) {
      bson_string_append (state->str, "{ \"$date\" : { \"$numberLong\" : \"");
      bson_string_append_printf (state->str, "%" PRId64, msec_since_epoch);
      bson_string_append (state->str, "\" } }");
   } else if (state->mode == BSON_JSON_MODE_RELAXED) {
      bson_string_append (state->str, "{ \"$date\" : \"");
      _bson_iso8601_date_format (msec_since_epoch, state->str);
      bson_string_append (state->str, "\" }");
   } else {
      bson_string_append (state->str, "{ \"$date\" : ");
      bson_string_append_printf (state->str, "%" PRId64, msec_since_epoch);
      bson_string_append (state->str, " }");
   }

   return false;
}


static bool
_bson_as_json_visit_regex (const bson_iter_t *iter,
                           const char *key,
                           const char *v_regex,
                           const char *v_options,
                           void *data)
{
   bson_json_state_t *state = data;
   char *escaped;

   escaped = bson_utf8_escape_for_json (v_regex, -1);
   if (!escaped) {
      return true;
   }

   if (state->mode == BSON_JSON_MODE_CANONICAL ||
       state->mode == BSON_JSON_MODE_RELAXED) {
      bson_string_append (state->str,
                          "{ \"$regularExpression\" : { \"pattern\" : \"");
      bson_string_append (state->str, escaped);
      bson_string_append (state->str, "\", \"options\" : \"");
      _bson_append_regex_options_sorted (state->str, v_options);
      bson_string_append (state->str, "\" } }");
   } else {
      bson_string_append (state->str, "{ \"$regex\" : \"");
      bson_string_append (state->str, escaped);
      bson_string_append (state->str, "\", \"$options\" : \"");
      _bson_append_regex_options_sorted (state->str, v_options);
      bson_string_append (state->str, "\" }");
   }

   bson_free (escaped);

   return false;
}


static bool
_bson_as_json_visit_timestamp (const bson_iter_t *iter,
                               const char *key,
                               uint32_t v_timestamp,
                               uint32_t v_increment,
                               void *data)
{
   bson_json_state_t *state = data;

   bson_string_append (state->str, "{ \"$timestamp\" : { \"t\" : ");
   bson_string_append_printf (state->str, "%u", v_timestamp);
   bson_string_append (state->str, ", \"i\" : ");
   bson_string_append_printf (state->str, "%u", v_increment);
   bson_string_append (state->str, " } }");

   return false;
}


static bool
_bson_as_json_visit_dbpointer (const bson_iter_t *iter,
                               const char *key,
                               size_t v_collection_len,
                               const char *v_collection,
                               const bson_oid_t *v_oid,
                               void *data)
{
   bson_json_state_t *state = data;
   char *escaped;
   char str[25];

   escaped = bson_utf8_escape_for_json (v_collection, -1);
   if (!escaped) {
      return true;
   }

   if (state->mode == BSON_JSON_MODE_CANONICAL ||
       state->mode == BSON_JSON_MODE_RELAXED) {
      bson_string_append (state->str, "{ \"$dbPointer\" : { \"$ref\" : \"");
      bson_string_append (state->str, escaped);
      bson_string_append (state->str, "\"");

      if (v_oid) {
         bson_oid_to_string (v_oid, str);
         bson_string_append (state->str, ", \"$id\" : { \"$oid\" : \"");
         bson_string_append (state->str, str);
         bson_string_append (state->str, "\" }");
      }

      bson_string_append (state->str, " } }");
   } else {
      bson_string_append (state->str, "{ \"$ref\" : \"");
      bson_string_append (state->str, escaped);
      bson_string_append (state->str, "\"");

      if (v_oid) {
         bson_oid_to_string (v_oid, str);
         bson_string_append (state->str, ", \"$id\" : \"");
         bson_string_append (state->str, str);
         bson_string_append (state->str, "\"");
      }

      bson_string_append (state->str, " }");
   }

   bson_free (escaped);

   return false;
}


static bool
_bson_as_json_visit_minkey (const bson_iter_t *iter,
                            const char *key,
                            void *data)
{
   bson_json_state_t *state = data;

   bson_string_append (state->str, "{ \"$minKey\" : 1 }");

   return false;
}


static bool
_bson_as_json_visit_maxkey (const bson_iter_t *iter,
                            const char *key,
                            void *data)
{
   bson_json_state_t *state = data;

   bson_string_append (state->str, "{ \"$maxKey\" : 1 }");

   return false;
}


static bool
_bson_as_json_visit_before (const bson_iter_t *iter,
                            const char *key,
                            void *data)
{
   bson_json_state_t *state = data;
   char *escaped;

   if (state->count) {
      bson_string_append (state->str, ", ");
   }

   if (state->keys) {
      escaped = bson_utf8_escape_for_json (key, -1);
      if (escaped) {
         bson_string_append (state->str, "\"");
         bson_string_append (state->str, escaped);
         bson_string_append (state->str, "\" : ");
         bson_free (escaped);
      } else {
         return true;
      }
   }

   state->count++;

   return false;
}


static void
_bson_as_json_visit_corrupt (const bson_iter_t *iter, void *data)
{
   *(((bson_json_state_t *) data)->err_offset) = iter->off;
}


static bool
_bson_as_json_visit_code (const bson_iter_t *iter,
                          const char *key,
                          size_t v_code_len,
                          const char *v_code,
                          void *data)
{
   bson_json_state_t *state = data;
   char *escaped;

   escaped = bson_utf8_escape_for_json (v_code, v_code_len);
   if (!escaped) {
      return true;
   }

   bson_string_append (state->str, "{ \"$code\" : \"");
   bson_string_append (state->str, escaped);
   bson_string_append (state->str, "\" }");
   bson_free (escaped);

   return false;
}


static bool
_bson_as_json_visit_symbol (const bson_iter_t *iter,
                                     const char *key,
                                     size_t v_symbol_len,
                                     const char *v_symbol,
                                     void *data)
{
   bson_json_state_t *state = data;
   char *escaped;

   escaped = bson_utf8_escape_for_json (v_symbol, v_symbol_len);
   if (!escaped) {
      return true;
   }

   if (state->mode == BSON_JSON_MODE_CANONICAL ||
       state->mode == BSON_JSON_MODE_RELAXED) {
      bson_string_append (state->str, "{ \"$symbol\" : \"");
      bson_string_append (state->str, escaped);
      bson_string_append (state->str, "\" }");
   } else {
      bson_string_append (state->str, "\"");
      bson_string_append (state->str, escaped);
      bson_string_append (state->str, "\"");
   }

   bson_free (escaped);

   return false;
}


static bool
_bson_as_json_visit_codewscope (const bson_iter_t *iter,
                                const char *key,
                                size_t v_code_len,
                                const char *v_code,
                                const bson_t *v_scope,
                                void *data)
{
   bson_json_state_t *state = data;
   char *code_escaped;
   char *scope;

   code_escaped = bson_utf8_escape_for_json (v_code, v_code_len);
   if (!code_escaped) {
      return true;
   }

   /* Encode scope with the same mode */
   scope = _bson_as_json_visit_all (v_scope, NULL, state->mode);

   if (!scope) {
      bson_free (code_escaped);
      return true;
   }

   bson_string_append (state->str, "{ \"$code\" : \"");
   bson_string_append (state->str, code_escaped);
   bson_string_append (state->str, "\", \"$scope\" : ");
   bson_string_append (state->str, scope);
   bson_string_append (state->str, " }");

   bson_free (code_escaped);
   bson_free (scope);

   return false;
}


static const bson_visitor_t bson_as_json_visitors = {
   _bson_as_json_visit_before,
   NULL, /* visit_after */
   _bson_as_json_visit_corrupt,
   _bson_as_json_visit_double,
   _bson_as_json_visit_utf8,
   _bson_as_json_visit_document,
   _bson_as_json_visit_array,
   _bson_as_json_visit_binary,
   _bson_as_json_visit_undefined,
   _bson_as_json_visit_oid,
   _bson_as_json_visit_bool,
   _bson_as_json_visit_date_time,
   _bson_as_json_visit_null,
   _bson_as_json_visit_regex,
   _bson_as_json_visit_dbpointer,
   _bson_as_json_visit_code,
   _bson_as_json_visit_symbol,
   _bson_as_json_visit_codewscope,
   _bson_as_json_visit_int32,
   _bson_as_json_visit_timestamp,
   _bson_as_json_visit_int64,
   _bson_as_json_visit_maxkey,
   _bson_as_json_visit_minkey,
   NULL, /* visit_unsupported_type */
   _bson_as_json_visit_decimal128,
};


static bool
_bson_as_json_visit_document (const bson_iter_t *iter,
                              const char *key,
                              const bson_t *v_document,
                              void *data)
{
   bson_json_state_t *state = data;
   bson_json_state_t child_state = {0, true, state->err_offset};
   bson_iter_t child;

   if (state->depth >= BSON_MAX_RECURSION) {
      bson_string_append (state->str, "{ ... }");
      return false;
   }

   if (bson_iter_init (&child, v_document)) {
      child_state.str = bson_string_new ("{ ");
      child_state.depth = state->depth + 1;
      child_state.mode = state->mode;
      if (bson_iter_visit_all (&child, &bson_as_json_visitors, &child_state)) {
         return true;
      }

      bson_string_append (child_state.str, " }");
      bson_string_append (state->str, child_state.str->str);
      bson_string_free (child_state.str, true);
   }

   return false;
}


static bool
_bson_as_json_visit_array (const bson_iter_t *iter,
                           const char *key,
                           const bson_t *v_array,
                           void *data)
{
   bson_json_state_t *state = data;
   bson_json_state_t child_state = {0, false, state->err_offset};
   bson_iter_t child;

   if (state->depth >= BSON_MAX_RECURSION) {
      bson_string_append (state->str, "{ ... }");
      return false;
   }

   if (bson_iter_init (&child, v_array)) {
      child_state.str = bson_string_new ("[ ");
      child_state.depth = state->depth + 1;
      child_state.mode = state->mode;
      if (bson_iter_visit_all (&child, &bson_as_json_visitors, &child_state)) {
         return true;
      }

      bson_string_append (child_state.str, " ]");
      bson_string_append (state->str, child_state.str->str);
      bson_string_free (child_state.str, true);
   }

   return false;
}


static char *
_bson_as_json_visit_all (const bson_t *bson,
                         size_t *length,
                         bson_json_mode_t mode)
{
   bson_json_state_t state;
   bson_iter_t iter;
   ssize_t err_offset = -1;

   BSON_ASSERT (bson);

   if (length) {
      *length = 0;
   }

   if (bson_empty0 (bson)) {
      if (length) {
         *length = 3;
      }

      return bson_strdup ("{ }");
   }

   if (!bson_iter_init (&iter, bson)) {
      return NULL;
   }

   state.count = 0;
   state.keys = true;
   state.str = bson_string_new ("{ ");
   state.depth = 0;
   state.err_offset = &err_offset;
   state.mode = mode;

   if (bson_iter_visit_all (&iter, &bson_as_json_visitors, &state) ||
       err_offset != -1) {
      /*
       * We were prematurely exited due to corruption or failed visitor.
       */
      bson_string_free (state.str, true);
      if (length) {
         *length = 0;
      }
      return NULL;
   }

   bson_string_append (state.str, " }");

   if (length) {
      *length = state.str->len;
   }

   return bson_string_free (state.str, false);
}


char *
bson_as_canonical_extended_json (const bson_t *bson, size_t *length)
{
   return _bson_as_json_visit_all (bson, length, BSON_JSON_MODE_CANONICAL);
}


char *
bson_as_json (const bson_t *bson, size_t *length)
{
   return _bson_as_json_visit_all (bson, length, BSON_JSON_MODE_LEGACY);
}


char *
bson_as_relaxed_extended_json (const bson_t *bson, size_t *length)
{
   return _bson_as_json_visit_all (bson, length, BSON_JSON_MODE_RELAXED);
}


char *
bson_array_as_json (const bson_t *bson, size_t *length)
{
   bson_json_state_t state;
   bson_iter_t iter;
   ssize_t err_offset = -1;

   BSON_ASSERT (bson);

   if (length) {
      *length = 0;
   }

   if (bson_empty0 (bson)) {
      if (length) {
         *length = 3;
      }

      return bson_strdup ("[ ]");
   }

   if (!bson_iter_init (&iter, bson)) {
      return NULL;
   }

   state.count = 0;
   state.keys = false;
   state.str = bson_string_new ("[ ");
   state.depth = 0;
   state.err_offset = &err_offset;
   state.mode = BSON_JSON_MODE_LEGACY;
   bson_iter_visit_all (&iter, &bson_as_json_visitors, &state);

   if (bson_iter_visit_all (&iter, &bson_as_json_visitors, &state) ||
       err_offset != -1) {
      /*
       * We were prematurely exited due to corruption or failed visitor.
       */
      bson_string_free (state.str, true);
      if (length) {
         *length = 0;
      }
      return NULL;
   }

   bson_string_append (state.str, " ]");

   if (length) {
      *length = state.str->len;
   }

   return bson_string_free (state.str, false);
}


#define VALIDATION_ERR(_flag, _msg, ...) \
   bson_set_error (&state->error, BSON_ERROR_INVALID, _flag, _msg, __VA_ARGS__)

static bool
_bson_iter_validate_utf8 (const bson_iter_t *iter,
                          const char *key,
                          size_t v_utf8_len,
                          const char *v_utf8,
                          void *data)
{
   bson_validate_state_t *state = data;
   bool allow_null;

   if ((state->flags & BSON_VALIDATE_UTF8)) {
      allow_null = !!(state->flags & BSON_VALIDATE_UTF8_ALLOW_NULL);

      if (!bson_utf8_validate (v_utf8, v_utf8_len, allow_null)) {
         state->err_offset = iter->off;
         VALIDATION_ERR (
            BSON_VALIDATE_UTF8, "invalid utf8 string for key \"%s\"", key);
         return true;
      }
   }

   if ((state->flags & BSON_VALIDATE_DOLLAR_KEYS)) {
      if (state->phase == BSON_VALIDATE_PHASE_LF_REF_UTF8) {
         state->phase = BSON_VALIDATE_PHASE_LF_ID_KEY;
      } else if (state->phase == BSON_VALIDATE_PHASE_LF_DB_UTF8) {
         state->phase = BSON_VALIDATE_PHASE_NOT_DBREF;
      }
   }

   return false;
}


static void
_bson_iter_validate_corrupt (const bson_iter_t *iter, void *data)
{
   bson_validate_state_t *state = data;

   state->err_offset = iter->err_off;
   VALIDATION_ERR (BSON_VALIDATE_NONE, "%s", "corrupt BSON");
}


static bool
_bson_iter_validate_before (const bson_iter_t *iter,
                            const char *key,
                            void *data)
{
   bson_validate_state_t *state = data;

   if ((state->flags & BSON_VALIDATE_EMPTY_KEYS)) {
      if (key[0] == '\0') {
         state->err_offset = iter->off;
         VALIDATION_ERR (BSON_VALIDATE_EMPTY_KEYS, "%s", "empty key");
         return true;
      }
   }

   if ((state->flags & BSON_VALIDATE_DOLLAR_KEYS)) {
      if (key[0] == '$') {
         if (state->phase == BSON_VALIDATE_PHASE_LF_REF_KEY &&
             strcmp (key, "$ref") == 0) {
            state->phase = BSON_VALIDATE_PHASE_LF_REF_UTF8;
         } else if (state->phase == BSON_VALIDATE_PHASE_LF_ID_KEY &&
                    strcmp (key, "$id") == 0) {
            state->phase = BSON_VALIDATE_PHASE_LF_DB_KEY;
         } else if (state->phase == BSON_VALIDATE_PHASE_LF_DB_KEY &&
                    strcmp (key, "$db") == 0) {
            state->phase = BSON_VALIDATE_PHASE_LF_DB_UTF8;
         } else {
            state->err_offset = iter->off;
            VALIDATION_ERR (BSON_VALIDATE_DOLLAR_KEYS,
                            "keys cannot begin with \"$\": \"%s\"",
                            key);
            return true;
         }
      } else if (state->phase == BSON_VALIDATE_PHASE_LF_ID_KEY ||
                 state->phase == BSON_VALIDATE_PHASE_LF_REF_UTF8 ||
                 state->phase == BSON_VALIDATE_PHASE_LF_DB_UTF8) {
         state->err_offset = iter->off;
         VALIDATION_ERR (BSON_VALIDATE_DOLLAR_KEYS,
                         "invalid key within DBRef subdocument: \"%s\"",
                         key);
         return true;
      } else {
         state->phase = BSON_VALIDATE_PHASE_NOT_DBREF;
      }
   }

   if ((state->flags & BSON_VALIDATE_DOT_KEYS)) {
      if (strstr (key, ".")) {
         state->err_offset = iter->off;
         VALIDATION_ERR (
            BSON_VALIDATE_DOT_KEYS, "keys cannot contain \".\": \"%s\"", key);
         return true;
      }
   }

   return false;
}


static bool
_bson_iter_validate_codewscope (const bson_iter_t *iter,
                                const char *key,
                                size_t v_code_len,
                                const char *v_code,
                                const bson_t *v_scope,
                                void *data)
{
   bson_validate_state_t *state = data;
   size_t offset = 0;

   if (!bson_validate (v_scope, state->flags, &offset)) {
      state->err_offset = iter->off + offset;
      VALIDATION_ERR (BSON_VALIDATE_NONE, "%s", "corrupt code-with-scope");
      return false;
   }

   return true;
}


static bool
_bson_iter_validate_document (const bson_iter_t *iter,
                              const char *key,
                              const bson_t *v_document,
                              void *data);


static const bson_visitor_t bson_validate_funcs = {
   _bson_iter_validate_before,
   NULL, /* visit_after */
   _bson_iter_validate_corrupt,
   NULL, /* visit_double */
   _bson_iter_validate_utf8,
   _bson_iter_validate_document,
   _bson_iter_validate_document, /* visit_array */
   NULL,                         /* visit_binary */
   NULL,                         /* visit_undefined */
   NULL,                         /* visit_oid */
   NULL,                         /* visit_bool */
   NULL,                         /* visit_date_time */
   NULL,                         /* visit_null */
   NULL,                         /* visit_regex */
   NULL,                         /* visit_dbpoint */
   NULL,                         /* visit_code */
   NULL,                         /* visit_symbol */
   _bson_iter_validate_codewscope,
};


static bool
_bson_iter_validate_document (const bson_iter_t *iter,
                              const char *key,
                              const bson_t *v_document,
                              void *data)
{
   bson_validate_state_t *state = data;
   bson_iter_t child;
   bson_validate_phase_t phase = state->phase;

   if (!bson_iter_init (&child, v_document)) {
      state->err_offset = iter->off;
      return true;
   }

   if (state->phase == BSON_VALIDATE_PHASE_START) {
      state->phase = BSON_VALIDATE_PHASE_TOP;
   } else {
      state->phase = BSON_VALIDATE_PHASE_LF_REF_KEY;
   }

   bson_iter_visit_all (&child, &bson_validate_funcs, state);

   if (state->phase == BSON_VALIDATE_PHASE_LF_ID_KEY ||
       state->phase == BSON_VALIDATE_PHASE_LF_REF_UTF8 ||
       state->phase == BSON_VALIDATE_PHASE_LF_DB_UTF8) {
      if (state->err_offset <= 0) {
         state->err_offset = iter->off;
      }

      return true;
   }

   state->phase = phase;

   return false;
}


static void
_bson_validate_internal (const bson_t *bson, bson_validate_state_t *state)
{
   bson_iter_t iter;

   state->err_offset = -1;
   state->phase = BSON_VALIDATE_PHASE_START;
   memset (&state->error, 0, sizeof state->error);

   if (!bson_iter_init (&iter, bson)) {
      state->err_offset = 0;
      VALIDATION_ERR (BSON_VALIDATE_NONE, "%s", "corrupt BSON");
   } else {
      _bson_iter_validate_document (&iter, NULL, bson, state);
   }
}


bool
bson_validate (const bson_t *bson, bson_validate_flags_t flags, size_t *offset)
{
   bson_validate_state_t state;

   state.flags = flags;
   _bson_validate_internal (bson, &state);

   if (state.err_offset > 0 && offset) {
      *offset = (size_t) state.err_offset;
   }

   return state.err_offset < 0;
}


bool
bson_validate_with_error (const bson_t *bson,
                          bson_validate_flags_t flags,
                          bson_error_t *error)
{
   bson_validate_state_t state;

   state.flags = flags;
   _bson_validate_internal (bson, &state);

   if (state.err_offset > 0 && error) {
      memcpy (error, &state.error, sizeof *error);
   }

   return state.err_offset < 0;
}


bool
bson_concat (bson_t *dst, const bson_t *src)
{
   BSON_ASSERT (dst);
   BSON_ASSERT (src);

   if (!bson_empty (src)) {
      return _bson_append (
         dst, 1, src->len - 5, src->len - 5, _bson_data (src) + 4);
   }

   return true;
}
