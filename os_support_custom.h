#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void *speex_alloc (int size);
void *speex_alloc_scratch (int size);
void *speex_realloc (void *ptr, int size);
void speex_free (void *ptr);
void speex_free_scratch (void *ptr);
