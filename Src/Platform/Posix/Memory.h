#pragma once

#include <alloca.h>
#include <string.h> // for memset

#define ALLOCATE_ON_STACK(TYPE, COUNT) (TYPE*)alloca(sizeof(TYPE) * COUNT)
