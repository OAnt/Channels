#ifndef CHANNEL_COMMON_H
#define CHANNEL_COMMON_H

#include <stdlib.h>
#include <errno.h>

#define calloc_(ptr, n, size) if(!((ptr) = calloc((n), (size)))) return ENOMEM

#endif
