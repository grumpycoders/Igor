/**
* msf.cpp  -  MSF parser core.
* PDB parser uses this engine to parse a .pdb file.
*
* This is the part of the PDB parser.
* (C) Great, 2010.
* xgreatx@gmail.com
*/

#include <stdio.h>
#include "msf.h"
//#include "misc.h"

#include <Buffer.h>
#include <Input.h>

using namespace Balau;

//
// Caller supplies pointer to an array of page indices, and number of pages.
// This routine tries to load the corresponding pages from MSF file to the 
//  memory and returns pointer to it.
// This is an internal routine.
//
static PVOID MsfLoadPages(MSF* msf, ULONG *pdwPointers, SIZE_T nPages)
{
    PVOID Data = malloc(nPages * msf->hdr->dwPageSize);
    if (Data != NULL)
    {
        for (ULONG i = 0; i < nPages; i++)
        {
            PVOID Page = PAGE(msf, pdwPointers[i]);
            SIZE_T Offset = msf->hdr->dwPageSize * i;

            memcpy((PUCHAR)Data + Offset, Page, msf->hdr->dwPageSize);
        }
    }

    return Data;
}

// Open multi-stream file, parse its header and load root directory.
// Return Value: handle for successfully opened MSF or NULL on error.
//
MSF* MsfOpen(const char *szFileName)
{
    IO<Input> file(new Input(szFileName));
    file->open();

    size_t size = file->getSize();
    uint8_t * buffer = (uint8_t *)malloc(size);
    file->forceRead(buffer, size);

    MSF* msf = (MSF*)malloc(sizeof(MSF));

    msf->MapV = buffer;
    msf->MsfSize = size;

    msf->RootPages = STREAM_SPAN_PAGES(msf, msf->hdr->dwRootSize);
    msf->RootPointersPages = STREAM_SPAN_PAGES(msf, msf->RootPages*sizeof(ULONG));
    
    msf->pdwRootPointers = (PULONG)MsfLoadPages(msf, msf->hdr->dwRootPointers, msf->RootPointersPages);
    
    if (msf->pdwRootPointers != NULL)
    {
        msf->Root = (MSF_ROOT*)MsfLoadPages(msf, msf->pdwRootPointers, msf->RootPages);

        if (msf->Root != NULL)
        {
            msf->pLoadedStreams = (MSF_STREAM*)malloc(sizeof(MSF_STREAM)* msf->Root->dwStreamCount);
            if (msf->pLoadedStreams != NULL)
            {
                return msf;
            }

            free(msf->Root);
        }

        free(msf->pdwRootPointers);
    }

    free(msf);

    return NULL;
}

//
// Get the array of page indices for the specified stream.
// NB: this function does not have the return value. So, if
// the specified stream does not contain any pages, the pointer
// still will be returned. In this case, you should not try to
// dereference it - these pages are the pages of the NEXT stream.
//

void MsfGetStreamPointers(MSF* msf, ULONG iStreamNumber, PULONG* pdwPointers)
{
    PULONG StreamPointers = &msf->Root->dwStreamSizes[msf->Root->dwStreamCount];

    ULONG j = 0;
    for (ULONG i = 0; i < iStreamNumber; i++)
    {
        ULONG nPages = STREAM_SPAN_PAGES(msf, msf->Root->dwStreamSizes[i]);

        j += nPages;
    }

    *pdwPointers = &StreamPointers[j];
}

//
// This function tries to load stream from MSF with the specified number.
// On success, return value is a handle to loaded stream.
// On error, return value is 0.
// Therefore, you should not use this routine, 'cause all streams are loaded
//  in MsfOpen() and you should just reference them by MsfReferenceStream/MsfReferenceStreamByType call.
//

MSF_STREAM* MsfLoadStream(MSF* msf, ULONG iStreamNumber)
{
    MSF_STREAM* stream = &msf->pLoadedStreams[iStreamNumber];
    if (stream != NULL)
    {
        stream->msf = msf;
        stream->iStreamNumber = iStreamNumber;
        stream->ReferenceCount = 1;
        stream->uStreamSize = msf->Root->dwStreamSizes[iStreamNumber];

        if (stream->uStreamSize == spnNil)
        {
            stream->pStreamData = NULL;
            stream->nStreamPages = 0;
            stream->pdwStreamPointers = NULL;
            return stream;
        }

        stream->nStreamPages = STREAM_SPAN_PAGES(msf, stream->uStreamSize);

        MsfGetStreamPointers(msf, iStreamNumber, &stream->pdwStreamPointers);

        stream->pStreamData = MsfLoadPages(msf, stream->pdwStreamPointers, stream->nStreamPages);

        if (stream->pStreamData != NULL)
        {
            return stream;
        }

        memset(stream, 0, sizeof(MSF_STREAM));
    }

    return NULL;
}

//
// All streams are loaded when MSF is being opened.
// This routine just searches for the pointer of loaded stream.
// If there is no stream with the supplied number, the function returns NULL.
// Optionally, it returns stream size (in bytes) in the last parameter.
//

PVOID MsfReferenceStream(MSF* msf, ULONG iStreamNumber, SIZE_T *uStreamSize)
{
    MSF_STREAM *s = &msf->pLoadedStreams[iStreamNumber];
    PVOID pData = s->pStreamData;

    if (pData != NULL)
    {
        //InterlockedExchangeAdd(&s->ReferenceCount, (LONG)1);
        s->ReferenceCount++;

        if (uStreamSize)
            *uStreamSize = s->uStreamSize;
    }

    return pData;
}

//
// Free the memory occupied by stream related to MSF file.
// You can use MsfLoadStream() again later for the same stream.
//
void MsfUnloadStream(MSF_STREAM *s)
{
    if (s->pStreamData != NULL)
    {
        if (s->ReferenceCount > 0)
            printf("warning: trying to unload referenced stream #%d, refcount %d\n", s->iStreamNumber, s->ReferenceCount);

        free(s->pStreamData);
        memset(s, 0, sizeof(MSF_STREAM));
    }
}

//
// Dereference loaded stream. In theory, all referenced streams should be dereferenced when are no longer need.
// But, this functionality is not implemented yet.
// All loaded streams resides in the system memory until the whole PDB is unloaded.
// When stream reference count reaches zero, the stream is being unloaded.
//
VOID MsfDereferenceStream(MSF* msf, ULONG iStreamNumber)
{
    MSF_STREAM *s = &msf->pLoadedStreams[iStreamNumber];

    // TODO: add synchronization here.

    //LONG refCount = InterlockedExchangeAdd(&s->ReferenceCount, (LONG)-1);
    LONG refCount = s->ReferenceCount;
    s->ReferenceCount--;

    if (refCount == 1)
    {
        // reference count is zero.
        MsfUnloadStream(s);
    }
    else if (refCount == 0)
    {
        s->ReferenceCount = 0; // write back zero.

        //BUGBUG: dereferencing free stream! this is an error!
    }
}