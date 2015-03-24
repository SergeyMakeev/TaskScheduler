#pragma once

#include <alloca.h>

#define ALLOCATE_ON_STACK(BYTES_COUNT) alloca(BYTES_COUNT)
