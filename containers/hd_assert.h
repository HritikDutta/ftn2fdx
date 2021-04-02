#ifndef HD_ASSERT_H
#define HD_ASSERT_H

#include <stdio.h>

#ifndef CONTAINER_NO_ASSERT
#define hd_assert(x) if (!(x)) { printf("Assertion Failed: %s \nLocation %s:%d", #x, __FILE__, __LINE__); __debugbreak(); }
#else
#define hd_assert(x)
#endif

#endif // HD_ASSERT_H