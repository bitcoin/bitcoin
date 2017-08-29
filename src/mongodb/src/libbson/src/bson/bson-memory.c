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


#include <stdlib.h>
#include <string.h>

#include "bson-atomic.h"
#include "bson-config.h"
#include "bson-memory.h"


static bson_mem_vtable_t gMemVtable = {
   malloc,
   calloc,
#ifdef BSON_HAVE_REALLOCF
   reallocf,
#else
   realloc,
#endif
   free,
};


/*
 *--------------------------------------------------------------------------
 *
 * bson_malloc --
 *
 *       Allocates @num_bytes of memory and returns a pointer to it.  If
 *       malloc failed to allocate the memory, abort() is called.
 *
 *       Libbson does not try to handle OOM conditions as it is beyond the
 *       scope of this library to handle so appropriately.
 *
 * Parameters:
 *       @num_bytes: The number of bytes to allocate.
 *
 * Returns:
 *       A pointer if successful; otherwise abort() is called and this
 *       function will never return.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void *
bson_malloc (size_t num_bytes) /* IN */
{
   void *mem = NULL;

   if (BSON_LIKELY (num_bytes)) {
      if (BSON_UNLIKELY (!(mem = gMemVtable.malloc (num_bytes)))) {
         abort ();
      }
   }

   return mem;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_malloc0 --
 *
 *       Like bson_malloc() except the memory is zeroed first. This is
 *       similar to calloc() except that abort() is called in case of
 *       failure to allocate memory.
 *
 * Parameters:
 *       @num_bytes: The number of bytes to allocate.
 *
 * Returns:
 *       A pointer if successful; otherwise abort() is called and this
 *       function will never return.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void *
bson_malloc0 (size_t num_bytes) /* IN */
{
   void *mem = NULL;

   if (BSON_LIKELY (num_bytes)) {
      if (BSON_UNLIKELY (!(mem = gMemVtable.calloc (1, num_bytes)))) {
         abort ();
      }
   }

   return mem;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_realloc --
 *
 *       This function behaves similar to realloc() except that if there is
 *       a failure abort() is called.
 *
 * Parameters:
 *       @mem: The memory to realloc, or NULL.
 *       @num_bytes: The size of the new allocation or 0 to free.
 *
 * Returns:
 *       The new allocation if successful; otherwise abort() is called and
 *       this function never returns.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void *
bson_realloc (void *mem,        /* IN */
              size_t num_bytes) /* IN */
{
   /*
    * Not all platforms are guaranteed to free() the memory if a call to
    * realloc() with a size of zero occurs. Windows, Linux, and FreeBSD do,
    * however, OS X does not.
    */
   if (BSON_UNLIKELY (num_bytes == 0)) {
      gMemVtable.free (mem);
      return NULL;
   }

   mem = gMemVtable.realloc (mem, num_bytes);

   if (BSON_UNLIKELY (!mem)) {
      abort ();
   }

   return mem;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_realloc_ctx --
 *
 *       This wraps bson_realloc and provides a compatible api for similar
 *       functions with a context
 *
 * Parameters:
 *       @mem: The memory to realloc, or NULL.
 *       @num_bytes: The size of the new allocation or 0 to free.
 *       @ctx: Ignored
 *
 * Returns:
 *       The new allocation if successful; otherwise abort() is called and
 *       this function never returns.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */


void *
bson_realloc_ctx (void *mem,        /* IN */
                  size_t num_bytes, /* IN */
                  void *ctx)        /* IN */
{
   return bson_realloc (mem, num_bytes);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_free --
 *
 *       Frees @mem using the underlying allocator.
 *
 *       Currently, this only calls free() directly, but that is subject to
 *       change.
 *
 * Parameters:
 *       @mem: An allocation to free.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_free (void *mem) /* IN */
{
   gMemVtable.free (mem);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_zero_free --
 *
 *       Frees @mem using the underlying allocator. @size bytes of @mem will
 *       be zeroed before freeing the memory. This is useful in scenarios
 *       where @mem contains passwords or other sensitive information.
 *
 * Parameters:
 *       @mem: An allocation to free.
 *       @size: The number of bytes in @mem.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_zero_free (void *mem,   /* IN */
                size_t size) /* IN */
{
   if (BSON_LIKELY (mem)) {
      memset (mem, 0, size);
      gMemVtable.free (mem);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_mem_set_vtable --
 *
 *       This function will change our allocationt vtable.
 *
 *       It is imperitive that this is called at the beginning of the
 *       process before any memory has been allocated by the default
 *       allocator.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_mem_set_vtable (const bson_mem_vtable_t *vtable)
{
   BSON_ASSERT (vtable);

   if (!vtable->malloc || !vtable->calloc || !vtable->realloc ||
       !vtable->free) {
      fprintf (stderr,
               "Failure to install BSON vtable, "
               "missing functions.\n");
      return;
   }

   gMemVtable = *vtable;
}

void
bson_mem_restore_vtable (void)
{
   bson_mem_vtable_t vtable = {
      malloc,
      calloc,
#ifdef BSON_HAVE_REALLOCF
      reallocf,
#else
      realloc,
#endif
      free,
   };

   bson_mem_set_vtable (&vtable);
}
