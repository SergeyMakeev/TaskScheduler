#pragma once

template<class T>
T Min(T a, T b)
{
    return a < b ? a : b;
}

template<class T>
T Max(T a, T b)
{
    return a < b ? b : a;
}

template<class T>
T Clamp(T val, T min, T max)
{
    return Min(max, Max(min, val));
}

#define UNUSED(T)

