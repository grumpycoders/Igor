#pragma once

#include "msf.h"


//
// This is a common symbol union.
// We can cast any pointer from symbol table to PSYM and retrieve the appropriate information.
//

typedef union _SYM
{
    ALIGNSYM Sym;
    ANNOTATIONSYM Annotation;
    ATTRMANYREGSYM AttrManyReg;
    ATTRMANYREGSYM2 AttrManyReg2;
    ATTRREGREL AttrRegRel;
    ATTRREGSYM AttrReg;
    ATTRSLOTSYM AttrSlot;
    BLOCKSYM Block;
    BLOCKSYM16 Block16;
    BLOCKSYM32 Block32;
    BPRELSYM16 BpRel16;
    BPRELSYM32 BpRel32;
    BPRELSYM32_16t BpRel32_16t;
    CEXMSYM16 Cexm16;
    CEXMSYM32 Cexm32;
    CFLAGSYM CFlag;
    COMPILESYM Compile;
    CONSTSYM Const;
    CONSTSYM_16t Const_16t;
    DATASYM16 Data16;
    DATASYM32 Data32;
    ENTRYTHISSYM EntryThis;
    FRAMEPROCSYM FrameProc;
    FRAMERELSYM FrameRel;
    LABELSYM16 Label16;
    LABELSYM32 Label32;
    MANPROCSYM ManProc;
    MANPROCSYMMIPS ManProcMips;
    MANTYPREF ManTypRef;
    MANYREGSYM_16t ManyReg_16t;
    MANYREGSYM ManyReg;
    MANYREGSYM2 ManyReg2;
    OBJNAMESYM ObjName;
    OEMSYMBOL Oem;
    PROCSYM16 Proc16;
    PROCSYM32 Proc32;
    PROCSYM32_16t Proc32_16t;
    PROCSYMIA64 ProcIA64;
    PROCSYMMIPS ProcMips;
    PROCSYMMIPS_16t ProcMips_16t;
    PUBSYM32 Pub32;
    REFSYM Ref;
    REFSYM2 Ref2;
    REGREL16 RegRel16;
    REGREL32_16t RegRel32_16t;
    REGREL32 RegRel32;
    REGSYM Reg;
    REGSYM_16t Reg_16t;
    RETURNSYM Return;
    SEARCHSYM Search;
    SLINK32 Slink32;
    SLOTSYM32 Slot32;
    SYMTYPE SymType;
    THREADSYM32_16t Thread_16t;
    THUNKSYM Thunk;
    THUNKSYM16 Thunk16;
    THUNKSYM32 Thunk32;
    TRAMPOLINESYM Trampoline;
    UDTSYM Udt;
    UDTSYM_16t Udt_16t;
    UNAMESPACE UNameSpace;
    VPATHSYM16 VPath16;
    VPATHSYM32 VPath32;
    VPATHSYM32_16t VPath32_16t;
    WITHSYM16 With16;
    WITHSYM32 With32;
} SYM, *PSYM;

// Go to the next symbol.
#define NextSym(S) ((PSYM)((PUCHAR)(S) + (S)->Sym.reclen + sizeof(WORD)))

typedef union _SYM SYM, *PSYM;

typedef struct _SYMD
{
    MSF *msf;

    PNewDBIHdr dbi;
    ULONG DbiSize;
    PSYM SymRecs;
    PSYM SymMac;
    ULONG SymSize;
    PHDR TpiHdr;

    MSF_STREAM_REF<GSIHashHdr> gsi;
    PVOID pGSHr;
    PVOID pGSBuckets;
    int nGSHrs;
    int nGSBuckets;
    MSF_STREAM_REF<PSGSIHDR> psi;
    PGSIHashHdr pgsiPSHash;
    PVOID pPSHr;
    PVOID pPSBuckets;
    PVOID pPSAddrMap;
    int nPSHrs;
    int nPSBuckets;
    int nPSAddrMap;

} SYMD, *PSYMD;

PSYMD SYMLoadSymbols(MSF* msf);
VOID SYMUnloadSymbols(PSYMD Symd);

#define S_ALL ((WORD)-1)

VOID SYMDumpSymbols(PSYMD Symd, WORD SymbolMask);

enum SYMBOL_TYPE
{
    ST_PUB,
    ST_CONST,
    ST_TYPEDEF,
    ST_PROC,
    ST_DATA
};

typedef struct _SYMBOL
{
    ULONG Type;
    union {
        ULONG VirtualAddress;
        ULONG Value;
    };
    PVOID EngSymPtr;
    ULONG Displacement;
    PSTR SymbolName;
    PlfRecord TypeInfo;
    PSTR FullSymbolDeclaration;
} SYMBOL, *PSYMBOL;

PSYMBOL SYMGetSymFromAddr(PSYMD Symd, USHORT Segment, ULONG Rva);
VOID SYMFreeSymbol(PSYMBOL Symbol);

PSYMBOL SYMNextSym(PSYMD Symd, PSYMBOL Symbol);

// VOID GSIInit (PSYMD Symd);
// VOID GSIClose (PSYMD Symd);

//PSYM GSINearestSym(PSYMD Symd, PVOID Eip, ULONG *displacement);
PSYM GSIFindExact(PSYMD Symd, ULONG Offset, USHORT Segment);
VOID GSIEnumAll(PSYMD Symd, USHORT Segment = 0);
PSYM GSIByName(PSYMD Symd, char *name, BOOL CaseInSensitive);
PSYM SYMByName(PSYMD Symd, char *name, BOOL CaseInSensitive);
VOID GSIInit(PSYMD Symd);