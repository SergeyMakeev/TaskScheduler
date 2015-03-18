#pragma once

#define ALLOCATE_ON_STACK(TYPE, COUNT) (TYPE*)_malloca(sizeof(TYPE) * COUNT)
