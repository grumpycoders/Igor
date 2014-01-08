#include <stdlib.h>
#include "pdb.h"

#include "IgorMemory.h"

PPDB PdbOpen(const char *szPdbFileName, GUID* guidSig, DWORD age)
{
    PDB* pdb = (PDB*)malloc(sizeof(PDB));

    pdb->msf = MsfOpen(szPdbFileName);
    if (pdb->msf == NULL)
    {
        free(pdb);
        return NULL;
    }

    // Load all streams.
    for (ULONG i = 0; i < pdb->msf->Root->dwStreamCount; i++)
    {
        MsfLoadStream(pdb->msf, i);
    }

    MsfReferenceStreamType(pdb->msf, PDB_STREAM_PDB, &pdb->pdb);
    MsfReferenceStreamType(pdb->msf, PDB_STREAM_DBI, &pdb->dbi);

    // here check GUID & age

    // Load symbol information from PDB
    pdb->Symd = SYMLoadSymbols(pdb->msf);

    GSIInit(pdb->Symd);

    return pdb;
}