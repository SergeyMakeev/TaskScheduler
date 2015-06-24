#pragma once

#include <alloca.h>

#define MT_ALLOCATE_ON_STACK(BYTES_COUNT) alloca(BYTES_COUNT)
