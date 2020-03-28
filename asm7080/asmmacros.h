/***********************************************************************
*
* asmmacros.h - System macros for the IBM 7080 computer.
*
* Changes:
*   04/05/07   DGP   Original.
*	
***********************************************************************/

/* I/O macros */

static char *ALTP[] = {
"     ALTP      MACRO  T2,X1 ",
"               LOD  14T2    ",
"               RAD  15(1    ",
"               UNL  15T2    ",
"               ST   14(1    ",
"     (1        ADCON  X1    ",
"               MEND         "
};

static char *BSTP[] = {
"     BSTP      MACRO NX1    ",
"               SEL    02'N' ",
"               RAD  15(2    ",
"     (1        BSP          ",
"               SUB  15#+1#  ",
"               TRZ  15(2+5  ",
"               TR     (1    ",
"     (2        ADCON  X1    ",
"               MEND         "
};

static char *DPDR[] = {
"     DPDR      MACRO  X1,X2,X3    ",
"               SEL    X1    ",
"               WR    1X2    ",
"               DOA    DRERR ",
"               LOD  14*     ",
"               TRA    DRERR ",
"               INCL   DRERR ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *DPPRA[] = {
"     DPPRA     MACRO NX2,X3    ",
"               SEL    02'N' ",
"               WR    1X2    ",
"               DOA    PRERB ",
"               LOD  14*     ",
"               TRA    PRERB ",
"               INCL   PRERB ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *DPPRT[] = {
"     DPPRT     MACRO NX2,X3,X4    ",
"               SEL    02'N' ",
"               WR    1X2    ",
"               DOA    PRERA ",
"               LOD  14*     ",
"               TRA    PRERA ",
"               INCL   PRERA ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *DPPCH[] = {
"     DPPCH     MACRO NX2,X3    ",
"               SEL    03'N' ",
"               WR    1X2    ",
"               DOA    PNERR ",
"               LOD  14*     ",
"               TRA    PNERR ",
"               INCL   PNERR ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *DPTP[] = {
"     DPTP      MACRO NX2,X3,X4    ",
"               SEL    02'N' ",
"               WR    1X2    ",
"               DOA    TPERR ",
"               LOD  14*     ",
"               TRA    TPERR ",
"               INCL   TPERR ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *DPTYP[] = {
"     DPTYP     MACRO NX2    ",
"               SEL    05'N' ",
"               WR    1X2    ",
"               DOA    XOFF  ",
"               LOD  14*     ",
"               TRA    XOFF  ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *FSTP[] = {
"     FSTP      MACRO NX1    ",
"               SEL    02'N' ",		// +00
"               RAD  15(2    ",		// +05
"     (1        RD    1@0    ",		// +10
"               TRS    (3    ",		// +15
"               SUB  15#+1#  ",		// +20
"               TRZ  15(4    ",		// +25
"               TR     (1    ",		// +30
"     (2        ADCON  X1    ",		// +35
"     (3        BSP          ",		// +40
"     (4        TRA    *+5   ",		// +45
"               SEL    902   ",		// +50
"               TRS    *+5   ",		// +55
"               SEL    02'N' ",		// +60
"               MEND         "
};

static char *FWDTP[] = {
"     FWDTP     MACRO N      ",
"               SEL    02'N' ",		// +00
"     (1        RD    1@0    ",		// +05
"               TRS    (2    ",		// +10
"               TR     (1    ",		// +15
"     (2        TRA    *+5   ",		// +20
"               SEL    902   ",		// +25
"               TRS    *+5   ",		// +30
"               SEL    02'N' ",		// +35
"               IOF          ",		// +40
"               MEND         "
};

static char *RWWLG[] = {
"     RWWLG     MACRO NX2,X3,X4 ",
"               NOP    (1    ",		// +00
"               SEL    02'N' ",		// +05
"               RD     X2    ",		// +10
"               DOA    TPERR ",		// +15
"               LOD  14*     ",		// +20
"               TRA    TPERR ",		// +25
"               INCL   TPERR ",		// +30
"               ADCON  X3    ",		// +35
"               ADCON  X4    ",		// +40
"     (1        SGN  15*-49  ",		// +45
"               TR     *+45  ",		// +50
"               SEL    02'N' ",		// +55
"               RWW    X2    ",		// +60
"               ADCON  X3    ",		// +65
"               ADCON  X4    ",		// +70
"               INCL   XOFF  ",
"               MEND         "
};

static char *WWRTP[] = {
"     WWRTP     MACRO NX6,X7,X8 ",
"               SEL    02'N' ",
"               WR     X6    ",
"               DOA    RWWER ",
"               LOD  14*     ",
"               TRA    RWWER ",
"               INCL   RWWER ",
"               ADCON  X7    ",
"               ADCON  X8    ",
"               INCL   TPERR ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *PRNTA[] = {
"     PRNTA     MACRO NX2,X3    ",
"               SEL    04'N' ",
"               WR     X2    ",
"               DOA    PRERB ",
"               LOD  14*     ",
"               TRA    PRERB ",
"               INCL   PRERB ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *PRINT[] = {
"     PRINT     MACRO NX2,X3,X4 ",
"               SEL    04'N' ",
"               WR     X2    ",
"               DOA    PRERA ",
"               LOD  14*     ",
"               TRA    PRERA ",
"               INCL   PRERA ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *PUNCH[] = {
"     PUNCH     MACRO NX2,X3 ",
"               SEL    03'N' ",
"               WR     X2    ",
"               DOA    PNERR ",
"               LOD  14*     ",
"               TRA    PNERR ",
"               INCL   PNERR ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *RDCD[] = {
"     RDCD      MACRO NX2,X3 ",
"               SEL    01'N' ",
"               RD     X2    ",
"               DOA    CDERR ",
"               LOD  14*     ",
"               TRA    CDERR ",
"               INCL   CDERR ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *RDDR[] = {
"     RDDR      MACRO  X1,X2,X3 ",
"               SEL    X1    ",
"               RD     X2    ",
"               DOA    DRERR ",
"               LOD  14*     ",
"               TRA    DRERR ",
"               INCL   DRERR ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *RDTP[] = {
"     RDTP      MACRO NX2,X3,X4 ",
"               SEL    02'N' ",
"               RD     X2    ",
"               DOA    TPERR ",
"               LOD  14*     ",
"               TRA    TPERR ",
"               INCL   TPERR ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *RWWTP[] = {
"     RWWTP     MACRO NX2,X3,X4 ",
"               SEL    02'N' ",
"               RWW    X2    ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               MEND         "
};

static char *RWDTP[] = {
"     RWDTP     MACRO N      ",
"               SEL    02'N' ",
"               RWD          ",
"               IOF          ",
"               MEND         "
};

static char *TYPE[] = {
"     TYPE      MACRO NX2    ",
"               SEL    05'N' ",
"               WR     X2    ",
"               DOA    XOFF  ",
"               LOD  14*     ",
"               TRA    XOFF  ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *TYPCK[] = {
"     TYPCK     MACRO NX2,X3 ",
"               SEL    05'N' ",
"               WR     X2    ",
"               DOA    TYPER ",
"               LOD  14*     ",
"               TRA    TYPER ",
"               INCL   TYPER ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WRCTP[] = {
"     WRCTP     MACRO NX2,X3,X4 ",
"               SEL    02'N' ",
"               WR     X2    ",
"               BSP          ",
"               RD    1@0    ",
"               DOA    CWRER ",
"               LOD  14*     ",
"               TRA    CWRER ",
"               INCL   CWRER ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               INCL   TPERR ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WRDR[] = {
"     WRDR      MACRO  X1,X2,X3 ",
"               SEL    X1    ",
"               WR     X2    ",
"               DOA    DRERR ",
"               LOD  14*     ",
"               TRA    DRERR ",
"               INCL   DRERR ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WREPA[] = {
"     WREPA     MACRO NX2,X3 ",
"               SEL    04'N' ",
"               WRE    X2    ",
"               DOA    WRERR ",
"               LOD  14*     ",
"               TRA    WRERR ",
"               INCL   WRERR ",
"               ADCON  *     ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WREPR[] = {
"     WREPR     MACRO NX2,X3,X4 ",
"               SEL    04'N' ",
"               WRE    X2    ",
"               DOA    WRERR ",
"               LOD  14*     ",
"               TRA    WRERR ",
"               INCL   WRERR ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WREPN[] = {
"     WREPN     MACRO NX2,X3 ",
"               SEL    03'N' ",
"               WRE    X2    ",
"               DOA    WRERR ",
"               LOD  14*     ",
"               TRA    WRERR ",
"               INCL   WRERR ",
"               ADCON  *     ",
"               ADCON  X3    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WRETP[] = {
"     WRETP     MACRO NX2,X3,X4 ",
"               SEL    02'N' ",
"               WRE    X2    ",
"               DOA    WRERR ",
"               LOD  14*     ",
"               TRA    WRERR ",
"               INCL   WRERR ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WRETY[] = {
"     WRETY     MACRO NX2    ",
"               SEL    05'N' ",
"               WRE    X2    ",
"               DOA    XOFF  ",
"               LOD  14*     ",
"               TRA    XOFF  ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WRTP[] = {
"     WRTP      MACRO NX2,X3,X4 ",
"               SEL    02'N' ",
"               WR     X2    ",
"               DOA    TPERR ",
"               LOD  14*     ",
"               TRA    TPERR ",
"               INCL   TPERR ",
"               ADCON  X3    ",
"               ADCON  X4    ",
"               INCL   XOFF  ",
"               MEND         "
};

static char *WRTM[] = {
"     WRTM      MACRO N      ",
"               SEL    02'N' ",
"               WTM          ",
"               DOA    TPERR ",
"               LOD  14*     ",
"               TRA    TPERR ",
"               INCL   TPERR ",
"               ADCON  *     ",
"               ADCON  0     ",
"               INCL   XOFF  ",
"               MEND         "
};

/* Logical macros */

static char *ALTTR[] = {
"     ALTTR     MACRO  AD    ",
"               TR     (2    ",		// 00
"     (1        SGN  15(1-9  ",		// 05
"               TR     (3+5  ",		// 10
"     (2        SGN  15(1-9  ",		// 15
"               ADM  15(1-9  ",		// 20
"     (3        TR     AD    ",		// 25
"               MEND         "
};

static char *CHKT[] = {
"     CHKT      MACRO NAD,TAG ",
"               SET    00'N' ",
"               LOD    AD    ",
"               SUB    #+0#  ",
"               TR     (1    ",
"     TAG       CON          ",
"                     500000 ",
"                     N+000000000000000000000000000000000000000000000000000 ",
"     (1        ADM    TAG   ",
"               MEND         "
};

static char *END[] = {
"     END       MACRO NAD    ",
"               SUB   N#+1   ",
"               TRZ    *+5   ",
"               TR     AD+5  ",
"               MEND         "
};

static char *FTNOP[] = {
"     FTNOP     MACRO  AD    ",
"               NOP    AD    ",
"               SGN  15*-9   ",
"               MEND         "
};

static char *FTTR[] = {
"     FTTR      MACRO  AD    ",
"               NOP    *+15  ",
"               SGN  15*-9   ",
"               TR     AD    ",
"               MEND         "
};

static char *HLTOF[] = {
"     HLTOF     MACRO N      ",
"               SEL    00'N' ",
"               TRS    (1+5  ",
"     (1        HLT    09'N' ",
"               MEND         "
};

static char *HLTON[] = {
"     HLTON     MACRO N      ",
"               SEL    09'N' ",
"               TRS    (1    ",
"               TR     (1+5  ",
"     (1        HLT    09'N' ",
"               MEND         "
};

static char *HLTTR[] = {
"     HLTTR     MACRO  A1,A2 ",
"               HLT    A2    ",
"               TR     A1    ",
"               MEND         "
};

static char *LLL[] = {
"     LLL       MACRO NFIELD ",
"               SET   N0004  ",
"               LOD   N(1    ",
"     (1        ADCON  FIELD ",
"               MEND         "
};

static char *LLL14[] = {
"     LLL14     MACRO NFIELD ",
"               LOD  14(1    ",
"     (1        ADCON NFIELD ",
"               MEND         "
};

static char *LRL[] = {
"     LRL       MACRO NFIELD ",
"               SET   N0004  ",
"               LOD   N(1    ",
"     (1        ADCON  FIELD ",
"               MEND         "
};

static char *LRL14[] = {
"     LRL14     MACRO NFIELD ",
"               LOD  14(1    ",
"     (1        ADCON NFIELD ",
"               MEND         "
};

static char *LOOP[] = {
"     LOOP      MACRO NCNT   ",
"               RAD   NCNT   ",
"               MEND         "
};

static char *MOVE[] = {
"     MOVE      MACRO NFROM,TO ",
"               RCV   NTO    ",
"               TMT   NFROM  ",
"               MEND         "
};

static char *MOVEC[] = {
"     MOVEC     MACRO NFROM,TO ",
"               SET  1500'N' ",
"               RCV  15TO    ",
"               TMT  15FROM  ",
"               MEND         "
};

static char *MOVEI[] = {
"     MOVEI     MACRO  FROM,TO ",
"               RCV  14TO+1   ",
"               TMT  14FROM+1 ",
"               MEND         "
};

static char *ORDCH[] = {
"     ORDCH     MACRO NVAL,HIGH,EQUAL,ERROR ",
"               SET  1500'N' ",
"               LOD  15VAL   ",
"               CMP  15(1    ",
"               UNL  15(1    ",
"               TRH    HIGH  ",
"               TRE    EQUAL ",
"               TR     ERROR ",
"     (1        CON   N      ",
"               MEND         "
};

static char *RPTA[] = {
"     RPTA      MACRO NA1,A2 ",
"               NOP    (3    ",		// 00
"     (1        SGN  15(1-9  ",		// 05
"               RAD   N(2    ",		// 10
"               TR     (3    ",		// 15
"     (2        NOP    A2    ",         // 20
"     (3        SUB   N#+1#  ",		// 25
"               TRZ   N(4    ",		// 30
"               TR     A1    ",		// 35
"     (4        SGN  15(1-9  ",		// 40
"               ADM  15(1-9  ",		// 45
"               MEND         "
};

static char *RPTM[] = {
"     RPTM      MACRO  A1,A2 ",
"               NOP    (4    ",		// 00
"     (1        SGN  15(1-9  ",		// 05
"               RAD  15(2    ",		// 10
"               TR     (5    ",		// 15
"     (2        ADCON  A2    ",         // 20
"     (3        ADCON        ",		// 25
"     (4        RAD  15(3    ",		// 30
"     (5        SUB  15#+1#  ",		// 35
"               TRZ  15(6    ",		// 40
"               ST   15(3    ",		// 45
"               TR     A1    ",		// 50
"     (6        SGN  15(1-9  ",		// 55
"               ADM  15(1-9  ",		// 60
"               MEND         "
};

static char *RCMP[] = {
"     RCMP      MACRO N      ",
"               RAD  15#+2#  ",
"               CMP  15#-N#-1 ",
"               MEND         "
};

static char *RSGN[] = {
"     RSGN      MACRO N      ",
"               RAD  15#+N#  ",
"               MEND         "
};

static char *SCMP[] = {
"     SCMP      MACRO N      ",
"               TRE    (2    ",		// 00
"               TRH    (1    ",		// 05
"               RAD  15#+3#  ",		// 10
"               TR     (3    ",		// 15
"     (1        RAD  15#+1#  ",		// 20
"               TR     (3    ",		// 25
"     (2        RAD  15#+2#  ",		// 30
"     (3        UNL  15#-N#-1 ",	// 35
"               MEND         "
};

static char *SSGN[] = {
"     SSGN      MACRO N      ",
"               TRZ  15(2    ",		// 00
"               TRP  15(1    ",		// 05
"               RSU  15#+1#  ",		// 10
"               TR     (3    ",		// 15
"     (1        RAD  15#+1#  ",		// 20
"               TR     (3    ",		// 25
"     (2        RAD  15#+0#  ",		// 30
"     (3        ST   15#+N#  ",		// 35
"               MEND         "
};

static char *SEQCH[] = {
"     SEQCH     MACRO NA1,A2 ",
"               SET  1500'N' ",
"               LOD  15A1    ",
"               CMP  15(1-5  ",
"               TRH    (1+5  ",
"               CON   N      ",
"     (1        HLT    A2    ",
"               UNL    (1-5  ",
"               MEND         "
};

static char *SWNOP[] = {
"     SWNOP     MACRO  S1    ",
"               SGN  15S1-4  ",
"               ADM  15S1-4  ",
"               MEND         "
};

static char *SWTR[] = {
"     SWTR      MACRO  S1    ",
"               SGN  15S1-4  ",
"               MEND         "
};

static char *SETUP[] = {
"     SETUP     MACRO        ",
"               SET   11     ",
"               SET   22     ",
"               SET   33     ",
"               SET   44     ",
"               SET   55     ",
"               SET  144     ",
"               SET  1510    ",
"               MEND         "
};

static char *TREH[] = {
"     TREH      MACRO  A1    ",
"               TRH    A1    ",
"               TRE    A1    ",
"               MEND         "
};

static char *TREL[] = {
"     TREL      MACRO  A1    ",
"               TRH    (1+5  ",
"               TRE    A1    ",
"     (1        TR     A1    ",
"               MEND         "
};

static char *TRLOW[] = {
"     TRLOW     MACRO  A1    ",
"               TRH    (1+5  ",
"               TRE    (1+5  ",
"     (1        TR     A1    ",
"               MEND         "
};

static char *TRMIN[] = {
"     TRMIN     MACRO  A1    ",
"               TRP    (1+5  ",
"     (1        TR     A1    ",
"               MEND         "
};

static char *TRNZ[] = {
"     TRNZ      MACRO NA1    ",
"               TRZ   N(1+5  ",
"     (1        TR     A1    ",
"               MEND         "
};

static char *TRNE[] = {
"     TRNZ      MACRO NA1    ",
"               TRE   N(1+5  ",
"     (1        TR     A1    ",
"               MEND         "
};

static char *DOA[] = {
"     DOA       MACRO NSUB   ",
"               TRA   N(1    ",		// 00
"               TR     (2+5  ",		// 05
"     (1        LOD  14*     ",		// 10
"     (2        TR     SUB   ",		// 15
"               INCL   SUB   ",		// 20
"               MEND         "
};

static char *DOP[] = {
"     DOP       MACRO NSUB   ",
"               TRP   N(1    ",		// 00
"               TR     (2+5  ",		// 05
"     (1        LOD  14*     ",		// 10
"     (2        TR     SUB   ",		// 15
"               INCL   SUB   ",		// 20
"               MEND         "
};

static char *DOZ[] = {
"     DOZ       MACRO NSUB   ",
"               TRZ   N(1    ",		// 00
"               TR     (2+5  ",		// 05
"     (1        LOD  14*     ",		// 10
"     (2        TR     SUB   ",		// 15
"               INCL   SUB   ",		// 20
"               MEND         "
};

static char *IFE[] = {
"     IFE       MACRO NI1,I2,AD ",
"               SET   NI1    ",
"               LOD   NI1    ",
"               CMP   NI2    ",
"               TRE   NAD    ",
"               MEND         "
};

static char *IFLOW[] = {
"     IFLOW     MACRO NI1,I2,AD ",
"               SET   NI1    ",
"               LOD   NI1    ",
"               CMP   NI2    ",
"               TRLOW NAD    ",
"               MEND         "
};

static char *IFH[] = {
"     IFH       MACRO NI1,I2,AD ",
"               SET   NI1    ",
"               LOD   NI1    ",
"               CMP   NI2    ",
"               TRH   NAD    ",
"               MEND         "
};

static char *IFEH[] = {
"     IFEH      MACRO NI1,I2,AD ",
"               SET   NI1    ",
"               LOD   NI1    ",
"               CMP   NI2    ",
"               TREH  NAD    ",
"               MEND         "
};

static char *IFEL[] = {
"     IFEL      MACRO NI1,I2,AD ",
"               SET   NI1    ",
"               LOD   NI1    ",
"               CMP   NI2    ",
"               TREL  NAD    ",
"               MEND         "
};

static char *IFNE[] = {
"     IFNE      MACRO NI1,I2,AD ",
"               SET   NI1    ",
"               LOD   NI1    ",
"               CMP   NI2    ",
"               TRNE  NAD    ",
"               MEND         "
};

static char *IFZ[] = {
"     IFZ       MACRO NI1,I2,AD ",
"               SET   NI1    ",
"               LOD   NI1    ",
"               CMP   NI2    ",
"               TRZ   NAD    ",
"               MEND         "
};

static char *IFNZ[] = {
"     IFNZ      MACRO NI1,I2,AD ",
"               SET   NI1    ",
"               LOD   NI1    ",
"               CMP   NI2    ",
"               TRNZ  NAD    ",
"               MEND         "
};

static char **sysmacros[] =
{
/* I/O macros */
   ALTP,  BSTP,  DPDR,  DPPCH, DPPRA, DPPRT, DPTP,  DPTYP, FSTP,  FWDTP, PRINT,
   PRNTA, PUNCH, RDCD,  RDDR,  RDTP,  RWDTP, RWWLG, RWWTP, TYPCK, TYPE,  WRCTP,
   WRDR,  WREPA, WREPN, WREPR, WRETP, WRETY, WRTM,  WRTP,  WWRTP,
/* Logical macros */
   ALTTR, CHKT,  FTNOP, FTTR,  HLTOF, HLTON, HLTTR, LLL,   LLL14, LRL,   LRL14,
   LOOP,  MOVE,  MOVEC, MOVEI, ORDCH, RPTA,  RPTM,  RCMP,  RSGN,  SCMP,  SSGN,
   SEQCH, SWNOP, SWTR,  SETUP, TREH,  TREL,  TRLOW, TRMIN, TRNZ,  TRNE,  
   DOA,   DOP,   DOZ,   IFE,   IFLOW, IFH,   IFEH,  IFEL,  IFNE,  IFZ,   IFNZ,
/* Floating Decimal macros */
   NULL
};

