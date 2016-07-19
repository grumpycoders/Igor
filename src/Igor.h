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

typedef int igor_result;

#define IGOR_FAILURE 0
#define IGOR_SUCCESS 1

typedef uint16_t igor_segment_handle;

/*
class igor_section_handle
{
public:
    uint16_t m_value;

    bool isValid()
    {
        if (m_value != 0xFFFF)
        {
            return true;
        }
        return false;
    }
};
*/
typedef u64 igorLinearAddress;

struct igorAddress {
    friend class IgorSession;
    friend struct s_igorDatabase;
    igorAddress & operator++();
    igorAddress & operator--();

    igorAddress & operator+=(igorLinearAddress d);
    igorAddress & operator-=(igorLinearAddress d);

    igorLinearAddress operator-(const igorAddress & d) const {
        IAssert(m_sessionId == d.m_sessionId, "Can't compare pointers from two different sessions");
        return this->offset - d.offset;
    }

    igorAddress operator++(int) { igorAddress t(*this); operator++(); return t; }
    igorAddress operator--(int) { igorAddress t(*this); operator--(); return t; }
    igorAddress operator+(igorLinearAddress d) const;
    igorAddress operator-(igorLinearAddress d) const;

    bool operator< (const igorAddress & b) const { return compareTo(b) == LT; }
    bool operator<=(const igorAddress & b) const { return compareTo(b) != GT; }
    bool operator> (const igorAddress & b) const { return compareTo(b) == GT; }
    bool operator>=(const igorAddress & b) const { return compareTo(b) != LT; }
    bool operator==(const igorAddress & b) const { return compareTo(b) == EQ; }
    bool operator!=(const igorAddress & b) const { return compareTo(b) != EQ; }

    explicit igorAddress() : igorAddress((uint16_t) 0, 0, -1) { }
    explicit igorAddress(struct s_igorDatabase * session, igorLinearAddress offset, igor_segment_handle sectionId);
    explicit igorAddress(class IgorSession * session, igorLinearAddress offset, igor_segment_handle sectionId);
    explicit igorAddress(uint16_t sessionId, igorLinearAddress offset, igor_segment_handle sectionId);

    igorAddress(const igorAddress & a)
        : m_offset(a.m_offset)
        , m_segmentId(a.m_segmentId)
        , m_sessionId(a.m_sessionId)
    { }
    igorAddress & operator=(const igorAddress & a)
    {
        m_offset = a.m_offset;
        m_segmentId = a.m_segmentId;
        m_sessionId = a.m_sessionId;

        return *this;
    }

    // may need more than that, like going to the session and check section limits.
    void makeInvalid()
    {
        m_sessionId = 0;
        m_offset = 0;
        m_segmentId = -1;
    }
    bool isValid() { return m_sessionId != 0; }
    bool isNotValid() { return !isValid(); }

    const igorLinearAddress & offset = m_offset;
    const igor_segment_handle & segmentId = m_segmentId;
    const uint16_t & sessionId = m_sessionId;

private:
    igorLinearAddress m_offset;
    igor_segment_handle m_segmentId;
    uint16_t m_sessionId;

    enum compareType {
        LT = -1,
        EQ = 0,
        GT = 1,
    };
    compareType compareTo(const igorAddress & b) const {
        IAssert(m_sessionId == b.m_sessionId, "Can't compare pointers from two different sessions");
        if (m_offset < b.m_offset)
            return LT;
        else if (m_offset > b.m_offset)
            return GT;
        else if (m_segmentId < b.m_segmentId)
            return LT;
        else if (m_segmentId > b.m_segmentId)
            return GT;
        return EQ;
    }

    // Be really careful, this grabs a reference. Don't do anything stupid like getSession()->blah().
    // this should change to a shared_ptr<IgorSession>
    class IgorSession * getSession() const;
};

typedef Balau::IO<Balau::Handle> BFile;

