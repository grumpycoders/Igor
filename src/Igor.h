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

typedef u64 igorLinearAddress;

struct igorAddress {
    igorAddress & operator++() { ++offset; return *this; }
    igorAddress & operator--() { --offset; return *this; }
    igorAddress operator++(int) { igorAddress t(*this); operator++(); return t; }
    igorAddress operator--(int) { igorAddress t(*this); operator--(); return t; }

    igorAddress & operator+=(igorAddress d) { offset += d.offset; return *this; }
    igorAddress & operator-=(igorAddress d) { offset -= d.offset; return *this; }

    igorAddress & operator+=(igorLinearAddress d) { offset += d; return *this; }
    igorAddress & operator-=(igorLinearAddress d) { offset -= d; return *this; }

    igorAddress operator+(igorLinearAddress d) const { igorAddress t(*this); t += d; return t; }
    igorAddress operator-(igorLinearAddress d) const { igorAddress t(*this); t -= d; return t; }

    igorLinearAddress operator-(const igorAddress & d) const { return this->offset - d.offset; }

    bool operator<(const igorAddress & b) const { return this->offset < b.offset; }
    bool operator<=(const igorAddress & b) const { return this->offset <= b.offset; }
    bool operator>(const igorAddress & b) const { return this->offset > b.offset; }
    bool operator>=(const igorAddress & b) const { return this->offset >= b.offset; }
    bool operator==(const igorAddress & b) const { return this->offset == b.offset; }
    bool operator!=(const igorAddress & b) const { return this->offset != b.offset; }

    igorAddress() { } // keep it this way, otherwise it's not trivial
    explicit igorAddress(igorLinearAddress offset) : offset(offset) { }

    igorLinearAddress offset;
};

extern const igorAddress IGOR_MIN_ADDRESS;
extern const igorAddress IGOR_MAX_ADDRESS;
extern const igorAddress IGOR_INVALID_ADDRESS;

typedef Balau::IO<Balau::Handle> BFile;

