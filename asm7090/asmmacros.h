/***********************************************************************
*
* asmmacros.h - System macros for the IBM 7090 computer.
*
* Changes:
*   12/29/10   DGP   Copy from asmpass0.c
*	
***********************************************************************/

/*
** 7090 macros for 7094 instructions.
*/

static char *PPC[] =
{
   "(PC    MACRO   O,Y,T ",
   "       PX'O    Y,T ",
   "       P'O'C   Y,T ",
   "       PX'O    Y,T ",
   "       P'O'C   Y,T ",
   "       ENDM "
};

static char *PCA[] =
{
   "PCA    MACRO   Y,T ",
   "       (PC     A,Y,T ",
   "       ENDM "
};

static char *PCD[] =
{
   "PCD    MACRO   Y,T ",
   "       (PC     D,Y,T ",
   "       ENDM "
};

static char *PSC[] =
{
   "(SC    MACRO   O,Y,T ",
   "       SXA     *+1,T ",
   "       AXC     **,T ",
   "       SX'O'   Y,T ",
   "       LXA     *-2,T ",
   "       ENDM "
};

static char *SCA[] =
{
   "SCA    MACRO   Y,T ",
   "       (SC     A,Y,T ",
   "       ENDM "
};

static char *SCD[] =
{
   "SCD    MACRO   Y,T ",
   "       (SC     D,Y,T ",
   "       ENDM "
};

static char *PDAS[] =
{
   "(DAS   MACRO   OP,I,Y,T ",
   "       (IFFF   I,Y    ",
   "       (DFAS   Y,T,OP ",
   "       (IFTF   I,Y,T ",
   "       (XFAS   T,OP ",
   "       (IFTT   I,Y,T ",
   "       (YFAS   OP   ",
   "       IFT     /I/=/*/ ",
   "       (ZFAS   OP,Y,T,  ",
   "       ENDM "
};

static char *DFAD[] =
{
   "DFAD   MACRO*  I,Y,T ",
   "       (DAS    AD,I,Y,T ",
   "       ENDM "
};

static char *DFSB[] =
{
   "DFSB   MACRO*  I,Y,T ",
   "       (DAS    SB,I,Y,T ",
   "       ENDM "
};

static char *DFAM[] =
{
   "DFAM   MACRO*  I,Y,T ",
   "       (DAS    AM,I,Y,T ",
   "       ENDM "
};

static char *DFSM[] =
{
   "DFSM   MACRO*  I,Y,T ",
   "       (DAS    SM,I,Y,T ",
   "       ENDM "
};

static char *PDFAS[] =
{
   "(DFAS  MACRO   Y,T,OP ",
   "       STQ     E.1 ",
   "       F'OP    Y,T ",
   "       STO     E.2 ",
   "       XCA         ",
   "       FAD     E.1 ",
   "       F'OP    Y+1,T ",
   "       FAD     E.2 ",
   "       ENDM "
};

static char *PXFAS[] =
{
   "(XFAS  MACRO   T,OP ",
   "       NOP     ,T         ",
   "       STQ     E.1        ",
   "       F'OP'*  *-2        ",
   "       STO     E.2        ",
   "       TXI     *+1,T,-1   ",
   "       XCA                ",
   "       FAD     E.1        ",
   "       F'OP'*  *-7        ",
   "       FAD     E.2        ",
   "       TXI     *+1,T,1    ",
   "       ENDM "
};

static char *PYFAS[] =
{
   "(YFAS  MACRO   OP,S ",
   "       (SAV4   S       ",
   "       (DFAS   0,4,OP  ",
   "S      AXT     ,4      ",
   "       ENDM "
};

static char *PZFAS[] =
{
   "(ZFAS  MACRO   OP,Y,T,S ",
   "       (FIND   Y,T,S     ",
   "       CLA     E.1       ",
   "       (DFAS   0,4,OP    ",
   "S      AXT     ,4        ",
   "       ENDM "
};

static char *PIFFF[] =
{
   "(IFFF  MACRO   I,Y ",
   "       IFF     /Y/=/**/,AND ",
   "       IFF     /I/=/*/      ",
   "       ENDM "
};

static char *PIFTF[] =
{
   "(IFTF  MACRO   I,Y,T ",
   "       IFF     /T/=//,AND    ",
   "       IFF     /T/=/0/,AND   ",
   "       IFT     /Y/=/**/,AND  ",
   "       IFF     /I/=/*/       ",
   "       ENDM "
};

static char *PIFTT[] =
{
   "(IFTT  MACRO   I,Y,T ",
   "       IFT     /T/=//,OR    ",
   "       IFT     /T/=/0/,AND  ",
   "       IFT     /Y/=/**/,AND ",
   "       IFF     /I/=/*/      ",
   "       ENDM "
};

static char *PSAV4[] =
{
   "(SAV4  MACRO   S ",
   "       AXT     ,0     ",
   "       SXA     S,4    ",
   "       LAC     *-2,4  ",
   "       ENDM "
};

static char *PFIND[] =
{
   "(FIND  MACRO   Y,T,S ",
   "       NOP     Y,T    ",
   "       STO     E.1    ",
   "       CLA*    *-2    ",
   "       STA     *+2    ",
   "       STT     *+1    ",
   "       PXA     ,0     ",
   "       SUB     *-1    ",
   "       SXA     S,4    ",
   "       PAC     0,4    ",
   "       ENDM "
};

static char *PDL[] =
{
   "(DL    MACRO   OP1,OP2,I,Y,T ",
   "       (IFFF   I,Y           ",
   "       (DLST   OP1,OP2,Y,T   ",
   "       (IFTT   I,Y,T         ",
   "       (XLST   OP1,OP2       ",
   "       (IFTF   I,Y,T         ",
   "       (YLST   OP1,OP2,T     ",
   "       IFT     /I/=/*/       ",
   "       (ZLST   OP1,OP2,Y,T,  ",
   "       ENDM "
};

static char *DLD[] =
{
   "DLD    MACRO*  I,Y,T ",
   "       (DL     CLA,LDQ,I,Y,T ",
   "       ENDM "
};

static char *DST[] =
{
   "DST    MACRO*  I,Y,T ",
   "       (DL     STO,STQ,I,Y,T ",
   "       ENDM "
};

static char *PDLST[] =
{
   "(DLST  MACRO   OP1,OP2,Y,T ",
   "       OP1     Y,T         ",
   "       OP2     Y+1,T       ",
   "       ENDM "
};

static char *PXLST[] =
{
   "(XLST  MACRO   OP1,OP2 ",
   "       OP1     **      ",
   "       SXA     *+3,4   ",
   "       LAC     *-2,4   ",
   "       OP2     1,4     ",
   "       AXT     **,4    ",
   "       ENDM "
};

static char *PYLST[] =
{
   "(YLST  MACRO   OP1,OP2,T ",
   "       OP1     **,T      ",
   "       TXI     *+1,T,-1  ",
   "       OP2*    *-2       ",
   "       TXI     *+1,T,1   ",
   "       ENDM "
};

static char *PZLST[] =
{
   "(ZLST  MACRO   OP1,OP2,Y,T,S ",
   "       (FIND   Y,T,S         ",
   "       IFT     /OP1/=/STO/   ",
   "       CLA     E.1           ",
   "       (DLST   OP1,OP2,0,4   ",
   "S      AXT     **,4          ",
   "       ENDM "
};

static char *DFMP[] =
{
   "DFMP   MACRO*  I,Y,T  ",
   "       (IFFF   I,Y     ",
   "       (DFMP   Y,T     ",
   "       (IFTF   I,Y,T   ",
   "       (XFMP   T       ",
   "       (IFTT   I,Y,T   ",
   "       (YFMP           ",
   "       IFT     /I/=/*/ ",
   "       (ZFMP   Y,T,    ",
   "       ENDM "
};

static char *PDFMP[] =
{
   "(DFMP  MACRO   Y,T ",
   "       STO     E.1   ",
   "       FMP     Y,T   ",
   "       STO     E.2   ",
   "       LDQ     Y,T   ",
   "       FMP     E.1   ",
   "       STQ     E.3   ",
   "       STO     E.4   ",
   "       LDQ     Y+1,T ",
   "       FMP     E.1   ",
   "       FAD     E.2   ",
   "       FAD     E.3   ",
   "       FAD     E.4   ",
   "       ENDM "
};

static char *PXFMP[] =
{
   "(XFMP  MACRO   T ",
   "       NOP     ,T       ",
   "       STO     E.1      ",
   "       FMP*    *-2      ",
   "       STO     E.2      ",
   "       LDQ*    *-4      ",
   "       FMP     E.1      ",
   "       STQ     E.3      ",
   "       STO     E.4      ",
   "       TXI     *+1,T,-1 ",
   "       LDQ*    *-9      ",
   "       TXI     *+1,T,1  ",
   "       FMP     E.1      ",
   "       FAD     E.2      ",
   "       FAD     E.3      ",
   "       FAD     E.4      ",
   "       ENDM "
};

static char *PYFMP[] =
{
   "(YFMP  MACRO   S ",
   "       (SAV4   S    ",
   "       (DFMP   0,4  ",
   "S      AXT     **,4 ",
   "       ENDM "
};

static char *PZFMP[] =
{
   "(ZFMP  MACRO   Y,T,S ",
   "       (FIND   Y,T,S ",
   "       CLA     E.1   ",
   "       (DFMP   0,4   ",
   "S      AXT     ,4    ",
   "       ENDM "
};

static char *PDD[] =
{
   "(DD    MACRO   OP,I,Y,T ",
   "       (IFFF   I,Y      ",
   "       (DFDX   OP,Y,T   ",
   "       (IFTF   I,Y,T    ",
   "       (XFDX   T,OP     ",
   "       (IFTT   I,Y,T    ",
   "       (YFDX   OP       ",
   "       IFT     /I/=/*/  ",
   "       (ZFDX   OP,Y,T,  ",
   "       ENDM "
};

static char *DFDP[] =
{
   "DFDP   MACRO*  I,Y,T ",
   "       (DD     P,I,Y,T ",
   "       ENDM "
};

static char *DFDH[] =
{
   "DFDH   MACRO*  I,Y,T ",
   "       (DD     H,I,Y,T ",
   "       ENDM "
};

static char *PDFDX[] =
{
   "(DFDX  MACRO   OP,Y,T ",
   "       STQ     E.1    ",
   "       FD'OP   Y,T    ",
   "       STO     E.2    ",
   "       STQ     E.3    ",
   "       FMP     Y+1,T  ",
   "       CHS            ",
   "       FAD     E.2    ",
   "       FAD     E.1    ",
   "       FD'OP   Y,T    ",
   "       XCA            ",
   "       FAD     E.3    ",
   "       ENDM "
};

static char *PXFDX[] =
{
   "(XFDX  MACRO   T,OP ",
   "       NOP     ,T       ",
   "       STQ     E.1      ",
   "       FD'OP'* *-2      ",
   "       STO     E.2      ",
   "       STQ     E.3      ",
   "       TXI     *+1,T,-1 ",
   "       FMP*    *-6      ",
   "       CHS              ",
   "       FAD     E.2      ",
   "       FAD     E.1      ",
   "       TXI     *+1,T,1  ",
   "       FD'OP'* *-11     ",
   "       XCA              ",
   "       FAD     E.3      ",
   "       ENDM "
};

static char *PYFDX[] =
{
   "(YFDX  MACRO   OP,S ",
   "       (SAV4   S      ",
   "       (DFDX   OP,0,4 ",
   "S      AXT     ,4     ",
   "       ENDM "
};

static char *PZFDX[] =
{
   "(ZFDX  MACRO   OP,Y,T,S ",
   "       (FIND   Y,T,S  ",
   "       CLA     E.1    ",
   "       (DFDX   OP,0,4 ",
   "S      AXT     **,4   ",
   "       ENDM "
};

static char *FSCA[] =
{
   "FSCA   MACRO   Y,T ",
   "       PXA     Y,T ",
   "       PAC     0,4 ",
   "       SXA     Y,4 ",
   "       ENDM "
};

static char **macros7094[] =
{
   PPC,   PCA,   PCD,   PSC,   SCA,   SCD,   PDAS,  DFAD,  DFSB,  DFAM,
   DFSM,  PDFAS, PXFAS, PYFAS, PZFAS, PIFFF, PIFTF, PIFTT, PSAV4, PFIND,
   PDL,   DLD,   DST,   PDLST, PXLST, PYLST, PZLST, DFMP,  PDFMP, PXFMP,
   PYFMP, PZFMP, PDD,   DFDP,  DFDH,  PDFDX, PXFDX, PYFDX, PZFDX, FSCA,
   NULL
};


