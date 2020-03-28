/***********************************************************************
*
* asmfps.h - Floating Point Support routines for the IBM 7090 assembler.
*
* Changes:
*   12/30/04   DGP   Hacked from S709 arith. routines.
*   03/15/05   DGP   Added dblprecision parm to ibm_strtod.
*	
***********************************************************************/

/*
** Misc constants
*/

#if defined(WIN32) && !defined(MINGW)

#define EXPMASK		0377000000000I64

#define DFRACMASK	00777777777777777777I64
#define DNRMMASK	00400000000000000000I64
#define DCRYMASK	01000000000000000000I64

#define ACSIGN		002000000000000I64
#define Q		001000000000000I64
#define P		000400000000000I64
#define BIT1		000200000000000I64
#define MAGMASK		000377777777777I64
#define DATAMASK	000777777777777I64

#define SIGN		000400000000000I64

#else

#define EXPMASK		0377000000000ULL

#define DFRACMASK	00777777777777777777ULL
#define DNRMMASK	00400000000000000000ULL
#define DCRYMASK	01000000000000000000ULL

#define ACSIGN		002000000000000ULL
#define Q		001000000000000ULL
#define P		000400000000000ULL
#define BIT1		000200000000000ULL
#define MAGMASK		000377777777777ULL
#define DATAMASK	000777777777777ULL

#define SIGN		000400000000000ULL
#endif

#define EXPSHIFT 	27
#define FRACSHIFT	27
#define FRACMASK	0000777777777
#define CRYMASK		0001000000000
#define NRMMASK		0000400000000
#define HIADD		0001000000000

#define SIGNSHIFT 35
#define SEXPMASK 0377


int ibm_strtod (char *, t_uint64 *, int);
