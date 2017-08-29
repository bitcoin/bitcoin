#include <mongoc.h>
#include <mongoc-list-private.h>

#include "TestSuite.h"


static void
test_mongoc_list_basic (void)
{
   mongoc_list_t *l;

   l = _mongoc_list_append (NULL, (void *) 1ULL);
   l = _mongoc_list_append (l, (void *) 2ULL);
   l = _mongoc_list_append (l, (void *) 3ULL);
   l = _mongoc_list_prepend (l, (void *) 4ULL);

   ASSERT (l);
   ASSERT (l->next);
   ASSERT (l->next->next);
   ASSERT (l->next->next->next);
   ASSERT (!l->next->next->next->next);

   ASSERT (l->data == (void *) 4ULL);
   ASSERT (l->next->data == (void *) 1ULL);
   ASSERT (l->next->next->data == (void *) 2ULL);
   ASSERT (l->next->next->next->data == (void *) 3ULL);

   l = _mongoc_list_remove (l, (void *) 4ULL);
   ASSERT (l->data == (void *) 1ULL);
   ASSERT (l->next->data == (void *) 2ULL);
   ASSERT (l->next->next->data == (void *) 3ULL);

   l = _mongoc_list_remove (l, (void *) 2ULL);
   ASSERT (l->data == (void *) 1ULL);
   ASSERT (l->next->data == (void *) 3ULL);
   ASSERT (!l->next->next);

   l = _mongoc_list_remove (l, (void *) 1ULL);
   ASSERT (l->data == (void *) 3ULL);
   ASSERT (!l->next);

   l = _mongoc_list_remove (l, (void *) 3ULL);
   ASSERT (!l);

   _mongoc_list_destroy (l);
}


void
test_list_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/List/Basic", test_mongoc_list_basic);
}
