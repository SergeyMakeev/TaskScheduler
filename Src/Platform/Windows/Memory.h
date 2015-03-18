#pragma once

#define ALLOCATE_ON_STACK(TYPE, COUNT) (TYPE*)_alloca(sizeof(TYPE) * COUNT)
