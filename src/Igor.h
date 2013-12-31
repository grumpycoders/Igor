#pragma once

#include <Exceptions.h>
#include <Handle.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef u64 igorAddress;

enum : u64 {
    IGOR_MIN_ADDRESS = 0,
    IGOR_MAX_ADDRESS = (u64) -1,
    IGOR_INVALID_ADDRESS = (u64) -1,
};

typedef Balau::IO<Balau::Handle> BFile;

