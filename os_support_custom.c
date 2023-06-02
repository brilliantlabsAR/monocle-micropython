#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "py/runtime.h"
#include "py/misc.h"

#include "os_support_custom.h"
#include "nrfx_log.h"

void *speex_alloc (int size)
{
   NRFX_LOG("%s %d", __func__, size);
   return m_malloc0(size);
}

void *speex_alloc_scratch (int size)
{
   NRFX_LOG("%s %d", __func__, size);
   return m_malloc0(size);
}

void *speex_realloc (void *ptr, int size)
{
   NRFX_LOG("%s %p %d", __func__, ptr, size);
   return m_realloc(ptr, size);
}

void speex_free (void *ptr)
{
   NRFX_LOG("%s %p", __func__, ptr);
   m_free(ptr);
}

void speex_free_scratch (void *ptr)
{
   NRFX_LOG("%s %p", __func__, ptr);
   m_free(ptr);
}
