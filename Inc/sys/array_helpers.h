#pragma once
#include <stdlib.h>

template <typename T, size_t N>
constexpr size_t countof(T const (&)[N]) noexcept
{
return N;
}