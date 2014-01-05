#pragma once

#pragma warning(disable:4200)

typedef unsigned long DWORD;

#include "pdb_info.h"

//
// Microsoft MSF (Multi-Stream File) file format, version 7.0
//

//
// File starts with version-dependent headers. This structures apply only to the MSF 7.0
//

// 32-byte signature
#define MSF_SIGNATURE_700 "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0"

// MSF File Header
typedef struct MSF_HDR
{
    char szMagic[32];          // 0x00  Signature
    DWORD dwPageSize;          // 0x20  Number of bytes in the pages (i.e. 0x400)
    DWORD dwFpmPage;           // 0x24  FPM (free page map) page (i.e. 0x2)
    DWORD dwPageCount;         // 0x28  Page count (i.e. 0x1973)
    DWORD dwRootSize;          // 0x2c  Size of stream directory (in bytes; i.e. 0x6540)
    DWORD dwReserved;          // 0x30  Always zero.
    DWORD dwRootPointers[0x49];// 0x34  Array of pointers to root pointers stream. 
} *PMSF_HDR;

typedef struct MSF_ROOT
{
    DWORD dwStreamCount;
    DWORD dwStreamSizes[];
} *PMSF_ROOT;

struct MSF_STREAM;

typedef struct MSF
{
    union {
        MSF_HDR* hdr;
        PVOID MapV;
        PUCHAR MapB;
    };
    ULONG MsfSize;
    ULONG RootPages;
    ULONG RootPointersPages;
    ULONG *pdwRootPointers;
    MSF_ROOT *Root;
    MSF_STREAM *pLoadedStreams;

    MSF_HDR HdrBackStorage;
} *PMSF;

typedef struct MSF_STREAM
{
    MSF* msf;
    ULONG iStreamNumber;
    LONG ReferenceCount;
    PVOID pStreamData;
    ULONG uStreamSize;
    ULONG nStreamPages;
    PULONG pdwStreamPointers;
} *PMSF_STREAM;

#define spnNil ((DWORD)-1)

#define ALIGN_DOWN(x, align) ((x) & ~(align-1))
#define ALIGN_UP(x, align) (((x) & (align-1))?ALIGN_DOWN(x,align)+align:(x))

#define PAGE(msf,x) (msf->MapB + msf->hdr->dwPageSize * (x))
#define STREAM_SPAN_PAGES(msf,size) (ALIGN_UP(size,msf->hdr->dwPageSize)/msf->hdr->dwPageSize)

template <class T>
struct MSF_STREAM_REF
{
    T* Data;
    ULONG Size;
};

PVOID MsfReferenceStream(MSF* msf, ULONG iStreamNumber, SIZE_T *uStreamSize=NULL);
VOID MsfDereferenceStream(MSF* msf, ULONG iStreamNumber);

template <class T>
BOOL MsfReferenceStreamType(MSF* msf, ULONG iStreamNumber, MSF_STREAM_REF<T> *streamRef)
{
    return ((streamRef->Data = (T*)MsfReferenceStream(msf, iStreamNumber, &streamRef->Size)) != NULL);
}

MSF* MsfOpen(const char *szFileName);
MSF_STREAM* MsfLoadStream(MSF* msf, ULONG iStreamNumber);