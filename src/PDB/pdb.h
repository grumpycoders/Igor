#pragma once

#include "msf.h"
#include "gsi.h"

typedef struct _PDB
{
    PSTR szPdbFileName;
    PMSF msf;  // MSF handle for loaded multi-stream file
    MSF_STREAM_REF<PDBStream70> pdb;
    MSF_STREAM_REF<NewDBIHdr> dbi;
    PSYMD Symd;
} PDB, *PPDB;

#define PDB_TI_MIN        0x00001000  // type info base
#define PDB_TI_MAX        0x00FFFFFF  // type info limit

// PDB Stream IDs
#define PDB_STREAM_ROOT   0
#define PDB_STREAM_PDB    1
#define PDB_STREAM_TPI    2
#define PDB_STREAM_DBI    3
#define PDB_STREAM_FPO    5

PPDB PdbOpen(const char *szPdbFileName, GUID* guidSig=NULL, DWORD age=0);