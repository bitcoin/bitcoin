//
// This file is a part of minialloc, released under the MIT
// License (see COPYING.MIT)
//
// Copyright (c)      2022 The Dash Core developers
//

#ifndef MINIALLOC_MINIALLOC_H
#define MINIALLOC_MINIALLOC_H

#include <minialloc/export.h>

extern "C" {
LIBRARY_EXPORT
void* msecure_malloc(const size_t size);

LIBRARY_EXPORT
void msecure_free(void *ptr);
}

#endif // MINIALLOC_MINIALLOC_H
