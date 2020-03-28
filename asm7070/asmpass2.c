/***********************************************************************
*
* asmpass2.c - Pass 2 for the IBM 7070 assembler.
*
* Changes:
*   03/01/07   DGP   Original.
*   05/29/13   DGP   Added DRDW and RDW support.
*   06/03/13   DGP   Improved packed values to include numeric.
*                    Added floating point support.
*   06/11/13   DGP   Added DA fixes.
*   06/12/13   DGP   Added DSW support.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "asmdef.h"
#include "asmbcd.h"

extern int pc;			/* the assembler pc */
extern int curpcindex;		/* Current PC counter index */
extern int symbolcount;		/* Number of symbols in symbol table */
extern int radix;		/* Number scanner radix */
extern int linenum;		/* Source line number */
extern int errcount;		/* Number of errors in assembly */
extern int errnum;		/* Number of pass 2 errors for current line */
extern int exprtype;		/* Expression type */
extern int p1errcnt;		/* Number of pass 0/1 errors */
extern int pgmlength;		/* Length of program */
extern int genxref;		/* Generate cross reference listing */
extern int litorigin;		/* Literal pool origin */
extern int litpool;		/* Literal pool size */
extern int widemode;		/* Generate wide listing */
extern int stdlist;		/* Generate standard listing */
extern int cpumodel;		/* CPU model */
extern int rdwcount;		/* Number of RDWs */
extern int dacount;		/* Number of DAs */

extern int pccounter[MAXPCCOUNTERS];/* PC counters */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char deckname[MAXDECKNAMELEN+2];/* The assembly deckname */
extern char ttlbuf[TTLSIZE+2];	/* The assembly TTL buffer */
extern char errline[10][120];	/* Pass 2 error lines for current statment */
extern char titlebuf[TTLSIZE+2];/* The assembly TITLE buffer */
extern char datebuf[48];	/* Date/Time buffer for listing */

extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern ErrTable p1error[MAXERRORS];/* The pass 0/1 error table */
extern DA_t das[MAXDAS];	/* DAs */
extern RDW_t rdws[MAXRDWS];	/* RDWs */
extern time_t curtime;		/* Assembly time */
extern struct tm *timeblk;	/* Assembly time */

static t_uint64 packednum;	/* Buffer for DC char data */

static OpCode *lastop;		/* Last opcode, NULL if specified */
static OpCode *prevop;		/* Previous opcode */

static int objectpc;		/* The object pc */
static int linecnt;		/* Current page line count */
static int objrecnum;		/* Object record number */
static int objcnt;		/* Current count in object record */
static int pagenum;		/* Current listing page number */
static int printed;		/* Line has been printed flag */
static int orgout;		/* Origin has been output flag */
static int inmode;		/* Input source line mode */
static int gbllistmode;		/* LIST/UNLIST flag */
static int listmode;		/* Listing requested flag */
static int etccount;		/* Number of ETC records */
static int intcdmode;		/* In a TCD section */
static int xline;		/* X line on listing, in goto or skip */
static int idtemitted;		/* IDT emitted */
static int pglinenum;		/* Line number on current page */
static int packedpc;		/* Current packed data pc for DC */
static int packedlen;		/* Current packed data len for DC */
static int inpacked;		/* Packed data in process for DC */
static int indcrdw;		/* In RDW block */
static int rdwlen;		/* RDW length */
static int rdwndx;		/* RDW index */
static int inda;		/* In DA block */
static int dalen;		/* DA length */
static int dandx;		/* DA index */

static char pcbuf[10];		/* PC print buffer */
static char objrec[OBJRECORDLENGTH+2];/* Object record buffer */
static char opbuf[32];		/* OP print buffer */
static char cdfd[4];		/* FD for object */
static char objbuf[MAXDATALEN];	/* Object field buffer */
static char packedsign;		/* Current packed data sign */

static char cursym[MAXSYMLEN+2];/* Current label field symbol */
static char lstbuf[MAXLINE];	/* Source line for listing */
static char etclines[MAXETCLINES][MAXLINE];/* ETC lines */
static char errtmp[256];	/* Error print string */

static void p2aop (OpCode *, char *, FILE *, FILE *);
static void p2bop (OpCode *, char *, FILE *, FILE *);
static void p2cop (OpCode *, char *, FILE *, FILE *);
static void p2dop (OpCode *, char *, FILE *, FILE *);
static void p2eop (OpCode *, char *, FILE *, FILE *);
static void p2iop (OpCode *, char *, FILE *, FILE *);
static void p2lop (OpCode *, char *, FILE *, FILE *);
static void p2qop (OpCode *, char *, FILE *, FILE *);
static void p2sop (OpCode *, char *, FILE *, FILE *);
static void p2top (OpCode *, char *, FILE *, FILE *);
static void p2uop (OpCode *, char *, FILE *, FILE *);

typedef struct {
   void (*proc) (OpCode *, char *, FILE *, FILE *);
} Inst_Proc;

Inst_Proc inst_proc[MAX_INST_TYPES] = {
   p2aop, p2bop, p2cop, p2dop, p2eop, p2iop, p2lop,
   p2qop, p2sop, p2top, p2uop
};

/***********************************************************************
* printheader - Print header on listing.
***********************************************************************/

static void
printheader (FILE *lstfd)
{
   if (!listmode) return;

   /*
   ** If top of page, print header
   */

   if (linecnt > LINESPAGE)
   {
      linecnt = 0;
      pglinenum = 0;
      if (pagenum) fputc ('\f', lstfd);
      if (widemode)
	 fprintf (lstfd, H1WIDEFORMAT,
		  VERSION,
		  titlebuf,
		  datebuf,
		  ++pagenum);
      else
	 fprintf (lstfd, H1FORMAT,
		  VERSION,
		  titlebuf,
		  datebuf,
		  ++pagenum);
      fprintf (lstfd, H2FORMAT, ttlbuf);
   }
}

/***********************************************************************
* printline - Print line on listing.
*
*0        1         2         3         4
*1234567890123456789012345678901234567890
*
*oooo SOOIIPPAAAA   LLLLG  TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
*
***********************************************************************/

static void
printline (FILE *lstfd)
{
   char mch;

   if (!listmode) return;

   /*
   ** If in skip/goto mark with the X
   */

   if (xline || (inmode & SKPINST))
      mch = 'X';

   /*
   ** If macro call line, mark with an M.
   */

   else if (inmode & MACCALL)
      mch = 'M';

   /*
   ** If generated line, mark with plus.
   */

   else if (inmode & (MACEXP | GENINST | DUPINST))
      mch = '+';

   /*
   ** Otherwise a blank
   */

   else
      mch = ' ';

   /*
   ** Print line
   */

   printheader (lstfd);
   if (stdlist)
   {
      char lstindex[MAXSYMLEN+2];
      char lsttag[MAXSYMLEN+2];
      char lstopcode[MAXSYMLEN+2];
      char lstoperand[RIGHTMARGIN-VARSTART+2];
      char cdno[6];

      strncpy (lstindex, lstbuf, SYMSTART);
      lstindex[SYMSTART] = '\0';

      if (lstbuf[SYMSTART] == COMMENTSYM)
      {
	 strcpy (cdno, "     ");
	 fprintf (lstfd, SLCFORMAT,
		  ++pglinenum, lstindex, mch, &lstbuf[SYMSTART],
		  cdno, cdfd, pcbuf, opbuf);
      }
      else
      {
	 strncpy (lsttag, &lstbuf[SYMSTART], OPCSTART-SYMSTART);
	 lsttag[OPCSTART-SYMSTART] = '\0';

	 strncpy (lstopcode, &lstbuf[OPCSTART], VARSTART-OPCSTART);
	 lstopcode[VARSTART-OPCSTART] = '\0';

	 memset (lstoperand, ' ', sizeof(lstoperand));
	 strncpy (lstoperand, &lstbuf[VARSTART], RIGHTMARGIN-VARSTART);
	 lstoperand[RIGHTMARGIN-VARSTART] = '\0';

	 strcpy (cdno, "     ");

	 fprintf (lstfd, SL1FORMAT,
		  ++pglinenum, lstindex, mch, lsttag, lstopcode, lstoperand,
		  cdno, cdfd, pcbuf, opbuf);
      }
   }
   else
      fprintf (lstfd, L1FORMAT, pcbuf, opbuf, linenum, mch, lstbuf);

   lstbuf[0] = '\0';
   linecnt++;
}

/***********************************************************************
* printdata - Print the data line.
*
*0        1         2         3         4
*1234567890123456789012345678901234567890
*
*oooo  SDDDDDDDDDD
*
***********************************************************************/

static void
printdata (FILE *lstfd, int opc)
{
   if (!listmode) return;

   printheader (lstfd);
   sprintf (pcbuf, PCFORMAT, opc);
   if (stdlist)
   {
      char cdno[6];

      strcpy (cdno, "     ");

      fprintf (lstfd, SL2FORMAT,
	       ++pglinenum, "      X",
	       cdno, cdfd, pcbuf, opbuf);
   }
   else
      fprintf (lstfd, L2FORMAT, pcbuf, opbuf);
   linecnt++;
}

/***********************************************************************
* printxref - Print the cross references.
***********************************************************************/

static int
printxref (SymNode *sym, FILE *lstfd)
{
   XrefNode *xref = sym->xref_head;
   int j;

   /*
   ** Print the definition line number
   */

   fprintf (lstfd, " %4d ", sym->line);

   /*
   ** Go through the list printing reference line numbers
   */

   j = 0;
   while (xref)
   {
      if (j >= (widemode ? 14 : 7))
      {
	 fprintf (lstfd, "\n");
	 printheader (lstfd);
	 fprintf (lstfd, "                                 ");
	 linecnt++;
	 j = 0;
      }
      fprintf (lstfd, " %4d ", xref->line);
      xref = xref->next;
      j++;
   }
   j = 16;
   return (j);
}

/***********************************************************************
* printsymbols - Print the symbol table.
***********************************************************************/

static void
printsymbols (FILE *lstfd)
{
   SymNode *sym;
   int i, j;

   if (!listmode) return;

   linecnt = MAXLINE;
   j = 0;
   if (genxref)
      sprintf (ttlbuf,
	       " SYMBOL     ADDR TYPE   LEN   DEF            REFERENCES");
   else
      sprintf (ttlbuf, "SYMBOL TABLE");

   for (i = 0; i < symbolcount; i++)
   {

      sym = symbols[i];
      if ((sym->flags & LITERALMASK) == 0)
      {
	 char fdpos[4];
	 char type[4];
	 char lenfield[10];

	 printheader (lstfd);
	 strcpy (lenfield, "    ");
	 strcpy (fdpos, "  ");
	 strcpy (type, "  ");

	 if (sym->flags & CONVAR)
	 {
	    strcpy (type, "DC");
	    sprintf (lenfield, "%4d", sym->length);
	    sprintf (fdpos, "%02d", sym->fdpos);
	 }
	 else if (sym->flags & AREADEF)
	 {
	    strcpy (type, "DA");
	    sprintf (lenfield, "%4d", sym->length);
	 }
	 else if (sym->flags & AREAVAR)
	 {
	    strcpy (type, "DF");
	    sprintf (lenfield, "%4d", sym->length);
	    sprintf (fdpos, "%02d", sym->fdpos);
	 }
	 else if (sym->flags & RDWVAR)
	 {
	    strcpy (type, "RW");
	    sprintf (lenfield, "%4d", sym->length);
	 }
	 else if (sym->flags & DSWVAR)
	 {
	    strcpy (type, "DS");
	 }
	 else if (sym->flags & DSWSVAR)
	 {
	    strcpy (type, "ES");
	    sprintf (fdpos, "%02d", sym->fdpos);
	 }
	 else if (sym->flags & EQUVAR)
	 {
	    if (sym->flags & AFVAR)
	    {
	       strcpy (type, "AF");
	    }
	    else if (sym->flags & TCUVAR)
	    {
	       strcpy (type, "CU");
	    }
	    else if (sym->flags & TCVAR)
	    {
	       strcpy (type, "C");
	    }
	    else if (sym->flags & TUVAR)
	    {
	       strcpy (type, "U");
	    }
	    else if (sym->flags & IQVAR)
	    {
	       strcpy (type, "Q");
	    }
	    else if (sym->flags & IWVAR)
	    {
	       strcpy (type, "X");
	       sprintf (fdpos, "%02d", sym->fdpos);
	    }
	    else if (sym->flags & SWVAR)
	    {
	       strcpy (type, "S");
	    }
	    else if (sym->flags & SNVAR)
	    {
	       strcpy (type, "SN");
	    }
	    else if (sym->flags & TYVAR)
	    {
	       strcpy (type, "T");
	    }
	    else if (sym->flags & ULVAR)
	    {
	       strcpy (type, "I");
	    }
	    else if (sym->flags & UPVAR)
	    {
	       strcpy (type, "P");
	    }
	    else if (sym->flags & URVAR)
	    {
	       strcpy (type, "R");
	    }
	    else if (sym->flags & UWVAR)
	    {
	       strcpy (type, "W");
	    }
	    else
	    {
	       sprintf (fdpos, "%02d", sym->fdpos);
	    }
	 }

	 fprintf (lstfd, SYMFORMAT,
		  sym->symbol, sym->value, type, fdpos, lenfield);
	 j++;
	 if (genxref)
	 {
	    j = printxref (sym, lstfd);
	 }

	 if (j >= (widemode ? 4 : 2))
	 {
	    fprintf (lstfd, "\n");
	    linecnt++;
	    j = 0;
	 }
      }
   }
   if (j)
      fprintf (lstfd, "\n");

}

/***********************************************************************
* punchfinish - Punch a record with sequence numbers.
***********************************************************************/

static void
punchfinish (FILE *outfd)
{
   int length;

   length = objcnt - INSTFIELD;
#ifdef DEBUGOBJECT
   fprintf (stderr, "punchfinish: objectpc = %d, objcnt = %d, length = %d\n",
	    objectpc, objcnt, length);
   fprintf (stderr, "objrec = %s\n", objrec);
#endif
   if (length)
   {
      sprintf (&objrec[SEQUENCENUM], OBJSEQUENCEFORMAT, ++objrecnum);
#ifdef DEBUGOBJECT
      fprintf (stderr, "objrec = %s\n", objrec);
#endif
      objrec[OBJRECORDLENGTH] = '\n';
      objrec[OBJRECORDLENGTH+1] = '\0';
      fputs (objrec, outfd);
      memset (objrec, ' ', sizeof(objrec));
      objectpc += length;
      objcnt = INSTFIELD;
   }
}

/***********************************************************************
* punchnewrecord - Punch a record and start a new one.
***********************************************************************/

static void
punchnewrecord (FILE *outfd, int curpc)
{

   punchfinish (outfd);
}

/***********************************************************************
* punchrecord - Punch an object value into record.
***********************************************************************/

static void 
punchrecord (FILE *outfd, int length)
{

#ifdef DEBUGOBJECT
   fprintf (stderr, "punchrecord: length = %d, objbuf = %s\n", length, objbuf);
#endif

   if (objcnt+length > OBJLENGTH)
   {
#ifdef DEBUGOBJECT
      fprintf (stderr, "   objcnt = %d, length = %d\n",
	       objcnt, length);
#endif
      punchfinish (outfd);
   }

   strncpy (&objrec[objcnt], objbuf, length);
   objbuf[0] = '\0';
   objcnt += length;
}

/***********************************************************************
* puncheof - Punch EOF sequence.
***********************************************************************/

static void 
puncheof (FILE *outfd)
{
   char temp[80];

   punchfinish (outfd);
   strncpy (objrec, "$EOF", 4);
   sprintf (temp, "%-8.8s  %02d/%02d/%02d  %02d:%02d:%02d    ASM7070 %s",
	    deckname,
	    timeblk->tm_mon+1, timeblk->tm_mday, timeblk->tm_year - 100,
	    timeblk->tm_hour, timeblk->tm_min, timeblk->tm_sec,
	    VERSION);
   strncpy (&objrec[7], temp, strlen(temp));
   objcnt = 1;
   punchfinish (outfd);
}

/***********************************************************************
* printerrors - Print any error message for this line.
***********************************************************************/

static int
printerrors (FILE *lstfd, int listmode)
{
   int i;
   int ret;

#ifdef DEBUGERROR
   if (errnum || p1errcnt)
   {
      fprintf (stderr, "printerrors: linenum = %d, errcount = %d\n",
	      linenum, errcount);
      fprintf (stderr, "   p1errcnt = %d, errnum = %d\n",
	      p1errcnt, errnum);
   }
#endif

   ret = 0;
   if (errnum)
   {

      for (i = 0; i < errnum; i++)
      {
	 if (listmode)
	 {
	    fprintf (lstfd, "ERROR: %s\n", errline[i]);
	    linecnt++;
	 }
	 else if (gbllistmode)
	 {
	    fprintf (lstfd, "ERROR: %d: %s\n", linenum, errline[i]);
	    linecnt++;
	 }
	 else
	 {
	    fprintf (stderr, "asm7070: %d: %s\n", linenum, errline[i]);
	 }
	 errline[i][0] = '\0';
      }
      errnum = 0;
      ret = -1;
   }

   if (p1errcnt)
      for (i = 0; i < p1errcnt; i++)
	 if (p1error[i].errorline == linenum)
	 {
	    if (listmode)
	    {
	       fprintf (lstfd, "ERROR: %s\n", p1error[i].errortext);
	       linecnt++;
	    }
	    else if (gbllistmode)
	    {
	       fprintf (lstfd, "ERROR: %d: %s\n",
			linenum, p1error[i].errortext);
	       linecnt++;
	    }
	    else
	    {
	       fprintf (stderr, "asm7070: %d: %s\n",
			linenum, p1error[i].errortext);
	    }
	    ret = -1;
	 }

   return (ret);
}

/***********************************************************************
* chkinda - Check to see if we're in a DA.
***********************************************************************/

static void
chkinda (FILE *outfd, FILE *lstfd)
{
   if (inda)
   {
      int words;

      words = (dalen / 10) + ((dalen % 10) ? 1 : 0);
#ifdef DEBUGDA
      fprintf (stderr,
               "p2-chkinda: pc = %d, count = %d, words = %d, dalen = %d : %d\n",
	       pc, das[dandx].count, words, dalen, dalen / 10);
#endif
      inda = FALSE;
      pc = das[dandx].origin + (words * das[dandx].count);
      sprintf (objbuf, OBJDATAFORMAT, ADDR_TAG, '+', (t_uint64)pc);
      punchrecord (outfd, INSTLENGTH+1);
#ifdef DEBUGDA
      fprintf (stderr, "   origin = %d, new pc = %d\n\n",
	       das[dandx].origin, pc);
#endif
      dandx++;
   }
}

/***********************************************************************
* chkindcrdw - Check to see if we're in a DC RDW.
***********************************************************************/

static void
chkindcrdw (void)
{
   if (indcrdw)
   {
#ifdef DEBUGDCRDW
      fprintf (stderr, "p2-chkindcrdw: pc = %d, rdwlen = %d\n",
	       pc, rdwlen);
#endif
      indcrdw = FALSE;
   }
}

/***********************************************************************
* chkpacked - Check to see if we're in a DC Packed.
***********************************************************************/

static void
chkpacked (FILE * outfd, FILE *lstfd)
{
   if (inpacked)
   {
#ifdef DEBUGDCPACKED
      fprintf (stderr, "p2-chkpacked: pc = %d, packedpc = %d, packedlen = %d\n",
	       pc, packedpc, packedlen);
#endif
      inpacked = FALSE;
      strcpy (cdfd, FDBLANKS);
      if (packedlen)
      {
	 if (packedsign == '@')
	 {
	    for (;packedlen < 5; packedlen++)
	       packednum = (packednum * 100) + tobcd[' '];
	    sprintf (opbuf, CHARFORMAT, packedsign, packednum);
	 }
	 else
	 {
	    sprintf (opbuf, DATAFORMAT, packedsign, packednum);
	 }
	 sprintf (cdfd, FDFORMAT, packedlen*2-1);
	 if (listmode)
	 {
	    printdata (lstfd, pc);
	 }
	 sprintf (objbuf, OBJCHARFORMAT, DATA_TAG, packedsign, packednum);
	 punchrecord (outfd, INSTLENGTH+1);
	 pc++;
      }
      packednum = 0;
      packedlen = 0;
      packedpc = 0;
      packedsign = 0;
      strcpy (pcbuf, PCBLANKS);
      strcpy (opbuf, OPBLANKS);
   }
}

/***********************************************************************
* processchardata - Process char data (or literal).
***********************************************************************/

static char *
processchardata (FILE *outfd, FILE *lstfd, char *bp, int *len, int listmode)
{
   char *ep;
   int i, j;
   int pcinc;
   char condata[MAXDATALEN];

   if (inpacked && (packedsign != '@'))
   {
      chkpacked (outfd, lstfd);
      sprintf (pcbuf, PCFORMAT, pc);
   }

   inpacked = TRUE;
   packedsign = '@';

   pcinc = 0;
   if (*bp == STRINGSYM)
   {
      int len;

#ifdef DEBUGDCPACKED
      fprintf (stderr, "processchardata: pc = %d, packedlen= %d, str = '",
	       pc, packedlen);
#endif

      bp++;
      len = 0;
      if ((ep = strrchr (bp, STRINGSYM)) != NULL)
      {
         for (i = 0; bp < ep; i++, bp++)
	 {
	    condata[i] = *bp;
#ifdef DEBUGDCPACKED
	    fprintf (stderr, "%c", *bp);
#endif
	    len++;
	 }
         for (j = 0; j < i; j++)
	 {
	    unsigned char ch;
	    if (packedlen && ((packedlen % 5) == 0))
	    {
	       sprintf (opbuf, CHARFORMAT, STRINGSYM, packednum);
	       sprintf (cdfd, FDFORMAT, 9);
	       if (listmode)
	       {
		  if (!printed)
		  {
		     printline (lstfd);
		     printed = TRUE;
		  }
		  else
		  {
		     printdata (lstfd, pc+pcinc);
		  }
	       }
	       pcinc++;
	       packedpc++;
	       sprintf (objbuf, OBJCHARFORMAT, DATA_TAG, STRINGSYM, packednum);
	       punchrecord (outfd, INSTLENGTH+1);
	       packednum = 0;
	       packedlen = 0;
	    }
	    if ((ch = tobcd[condata[j]]) == 255)
	    {
	       logerror ("Invalid character");
	       ch = 0;
	    }
	    packednum = (packednum * 100) + ch;
	    packedlen++;
	 }
#ifdef DEBUGDCPACKED
	 fprintf (stderr, "'\n  len = %d, val = %d\n", len, pcinc);
#endif
      }
      else
	 logerror ("Non-terminated character constant");
   }
   else
      logerror ("Invalid character constant");

   *len = pcinc;
   return (bp);
}

/***********************************************************************
* processcondata - Process constant data (or literal).
***********************************************************************/

static char *
processcondata (FILE *outfd, FILE *lstfd, char *bp, int *len, int listmode)
{
   t_int64 num;
   int j;
   int digitcnt;
   int emitted;
   int pcinc;
   int dpcount;
   int zerocnt;
   int lclpacked;
   char sign;

#if defined(DEBUGDCNUMS) || defined(DEBUGDCPACKED)
   fprintf (stderr,
            "processconddata: pc = %d, packedlen= %d, bp = '%-15.15s'\n",
	    pc, packedlen, bp);
#endif

   num = 0;
   pcinc = 0;
   digitcnt = 0;
   zerocnt = 0;
   lclpacked = FALSE;
   dpcount = -1;

   sign = '+';
   if (*bp == '-' || *bp == '+')
      sign = *bp++;

   while (*bp && *bp != ' ')
   {
      if (inpacked && (packedsign != sign))
      {
	 chkpacked (outfd, lstfd);
	 sprintf (pcbuf, PCFORMAT, pc);
      }

      if (isdigit (*bp))
      {
	 num = (num * 10) + (*bp++ - '0');
	 if (num > 0)
	    digitcnt++;
	 if (dpcount >= 0 && num == 0)
	    zerocnt++;
	 if (digitcnt > 10)
	 {
	    logerror ("Numeric overflow");
	    num = 0;
	    break;
	 }
      }
      else if (*bp == '.')
      {
	 bp++;
	 if (dpcount >= 0)
	 {
	    logerror ("Invalid numeric");
	    num = 0;
	    break;
	 }
         dpcount = digitcnt;
      }
      else if (*bp == ',')
      {
         bp++;
	 continue;
      }
      else if (*bp == 'F')
      {
	 int exp, eexp;
	 char expsign;

#ifdef DEBUGDCNUMS
         fprintf (stderr,
	       "p2-DCfloat: pc = %d, sign = %c, num = %010lld, dpcount = %d\n",
		  pc+pcinc, sign, num, dpcount);
#endif
	 if (digitcnt > 8)
	 {
	    logerror ("Numeric overflow");
	    num = 0;
	    break;
	 }

	 if (dpcount < 0)
	    dpcount = digitcnt;

	 /* Normalize */
	 while (num < 10000000) num *= 10;

         bp++;
	 expsign = '+';
	 if (*bp == '+' || *bp == '-')
	    expsign = *bp++;

	 exp = 0;
	 for (; *bp && isdigit(*bp); bp++)
	    exp = (exp * 10) + (*bp - '0');
	 if (exp > 99)
	 {
	    logerror ("Exponent overflow");
	    num = 0;
	    break;
	 }

	 if (expsign == '-') exp = - exp;
         eexp = 50 + (exp + dpcount) - zerocnt;
	 num = ((t_uint64)eexp * 100000000) + num;

#ifdef DEBUGDCNUMS
         fprintf (stderr,
	          "   expsign = %c, exp = %d, eexp = %d, num = %01lld\n",
		  expsign, exp, eexp, num);
#endif
         if (*bp == '(')
	 {
	    logerror ("Position expression not allowed");
	    num = 0;
	    break;
	 }
      }
      else if (*bp == '(')
      {
	 char *ep;
	 int sv, ev;

	 inpacked = TRUE;
	 lclpacked = TRUE;
	 packedsign = sign;
	 sv = ev = 0;
	 bp++;
	 if ((ep = strchr (bp, ')')) != NULL)
	 {
	    int pos, ix;
	    char term;

	    *ep++ = 0;
	    bp = exprscan (bp, &sv, &pos, &ix, &term, FALSE);
	    if (term == ',')
	       bp = exprscan (bp, &ev, &pos, &ix, &term, FALSE);
	    if ((sv > ev) || (sv > 9) || (ev > 9))
	    {
	       logerror ("Invalid position expression");
	       num = 0;
	       break;
	    }

	    for (j = 0; j < (9 - ev); j++)
	       num *= 10;
	    packednum += num;
	    packedlen = ev;
#if defined(DEBUGDCNUMS) || defined(DEBUGDCPACKED)
	    fprintf (stderr,
     "  packedlen = %d, sv = %d, ev = %d, num = %010lld, packednum = %010lld\n",
		     packedlen, sv, ev, num, packednum);
#endif
	    sprintf (cdfd, FDFORMAT, sv*10+ev);
	    bp = ep;
	 }
	 else
	 {
	    logerror ("Invalid position expression");
	    num = 0;
	    break;
	 }
      }
      else if (*bp == '+' || *bp == '-')
      {
#if defined(DEBUGDCNUMS) || defined(DEBUGDCPACKED)
         fprintf (stderr, "NEW Num: pc = %d, bp = %20.20s\n", pc+pcinc, bp);
#endif
	 if (!inpacked)
	 {
	    chkpacked (outfd, lstfd);
	    sprintf (opbuf, DATAFORMAT, sign, num);
	    sprintf (cdfd, FDFORMAT, 9);
	    if (listmode)
	    {
	       if (pcinc)
	       {
		  printdata (lstfd, pc+pcinc);
	       }
	       else
	       {
		  printline (lstfd);
		  printed = TRUE;
	       }
	       sprintf (objbuf, OBJDATAFORMAT, DATA_TAG, sign, num);
	    }
	    punchrecord (outfd, INSTLENGTH+1);
	    pcinc++;
	 }

	 num = 0;
	 digitcnt = 0;
	 zerocnt = 0;
	 dpcount = -1;

	 sign = '+';
	 if (*bp == '-' || *bp == '+')
	    sign = *bp++;
      }
   }

   if (!inpacked || !lclpacked)
   {
      chkpacked (outfd, lstfd);
#if defined(DEBUGDCNUMS) || defined(DEBUGDCPACKED)
      fprintf (stderr, "  sign = %c, num = %010lld\n", sign, num);
#endif
      sprintf (opbuf, DATAFORMAT, sign, num);
      sprintf (cdfd, FDFORMAT, 9);
      if (listmode)
      {
	 if (pcinc)
	 {
	    printdata (lstfd, pc+pcinc);
	 }
	 else
	 {
	    sprintf (pcbuf, PCFORMAT, pc);
	    printline (lstfd);
	    printed = TRUE;
	 }
      }
      sprintf (objbuf, OBJDATAFORMAT, DATA_TAG, sign, num);
      punchrecord (outfd, INSTLENGTH+1);
      pcinc++;
   }

#if defined(DEBUGDCNUMS) || defined(DEBUGDCPACKED)
   fprintf (stderr, "  val = %d\n", pcinc);
#endif
   *len = pcinc;

   return (bp);
}

/***********************************************************************
* processliterals - Process literals.
***********************************************************************/

static void
processliterals (FILE *outfd, FILE *lstfd, int listmode)
{
   char *cp;
   int i;
   int len;
   int literalcount = 0;
   char lcltoken[TOKENLEN];

#ifdef DEBUGLIT
   fprintf (stderr, "p2-processliterals: symbolcount = %d\n", symbolcount);
#endif
   /*
   ** Get number of literals in symbol table
   */

   chkpacked (outfd, lstfd);
   for (i = 0; i < symbolcount; i++)
   {
      if (symbols[i]->flags & LITERALMASK) literalcount++;
   }
#ifdef DEBUGLIT
   fprintf (stderr, "   pc = %d, litorigin = %d, literalcount = %d\n",
	    pc, litorigin, literalcount);
#endif

   /*
   ** process found literals
   */

   if (pc != litorigin)
      punchnewrecord (outfd, litorigin);
   pc = litorigin;

   while (literalcount)
   {
      for (i = 0; i < symbolcount; i++)
      {
	 
	 /*
	 ** If this symbol is a literal for this pc, process it
	 */

	 if ((symbols[i]->flags & LITERALMASK) && (symbols[i]->value == pc))
	 {
	    SymNode *s;

	    s = symbols[i];
	    strcpy (lcltoken, s->symbol);
	    cp = lcltoken;
#ifdef DEBUGLIT
	    fprintf (stderr, "   i = %d, cp = '%s'\n", i, cp);
#endif

	    if (s->flags & MIXEDLITERAL)
	    {
               cp = processchardata (outfd, lstfd, cp, &len, FALSE);
	       chkpacked (outfd, lstfd);
	    }
	    else if (s->flags & SIGNEDLITERAL)
	    {
	       cp = processcondata (outfd, lstfd, cp, &len, FALSE);
	       chkpacked (outfd, lstfd);
	    }

	    sprintf (pcbuf, PCFORMAT, pc);
	    printheader (lstfd);
	    if (stdlist)
	       fprintf (lstfd, SLITFORMAT, s->symbol, pcbuf, opbuf);
	    else
	       fprintf (lstfd, LITFORMAT, pcbuf, opbuf, s->symbol);
	    linecnt++;
	    printerrors (lstfd, listmode);

#ifdef DEBUGLIT
	    fprintf (stderr, "   %s = %s\n", pcbuf, s->symbol);
#endif
	    pc += s->length;
	    literalcount--;
	    break;
	 }
      }
   }
}

/***********************************************************************
* p2aop - Process A type operation code.
***********************************************************************/

static void
p2aop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int addr = 0;
   int index = 0;
   int position = 0;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ' && !op->opmod2)
   {
      logerror ("Missing Operand");
   }
   else
   {
      bp = exprscan (bp, &addr, &position, &index, &term, FALSE);
      if (position < 0) position = 9;
   }

   if (index > MAXINDEX)
   {
      logerror ("Invalid index value");
      index = 0;
   }

   if (op->opvalue == 0 && op->opmod < 0) /* -00 opcode */
   {
      term = '-';
      opcode = op->opvalue;
   }
   else if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, position, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, position, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2bop - Process B type operation code.
***********************************************************************/

static void
p2bop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int position = 0;
   int index = 0;
   int addr = 0;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      bp = exprscan (bp, &addr, &position, &index, &term, FALSE);
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   if (index > MAXINDEX)
   {
      logerror ("Invalid index value");
      index = 0;
   }

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, op->opmod, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, op->opmod, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2cop - Process Channel type operation code.
***********************************************************************/

static void
p2cop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int chan = 0;
   int position = 0;
   int index = 0;
   int addr = 0;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      bp = exprscan (bp, &chan, &junk, &junk, &term, FALSE);
      if (term == ',')
	 bp = exprscan (bp, &addr, &position, &index, &term, FALSE);

      if (chan > MAXCHANNELS)
      {
	 logerror ("Invalid channel value");
	 chan = 0;
      }
      if (index > MAXINDEX)
      {
	 logerror ("Invalid index value");
	 index = 0;
      }
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, op->opmod + (chan * 10), addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, op->opmod + (chan * 10), addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2dop - Process D (coupled shift) type operation code.
***********************************************************************/

static void
p2dop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   char *sp;
   int opcode;
   int position = 0;
   int sc = 0;
   int so = 0;
   int index = 0;
   int addr = 0;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      if ((sp = strchr (bp, '(')) != NULL) *sp = '\0';
      bp = exprscan (bp, &sc, &junk, &junk, &term, FALSE);
      if (sp)
      {
	 bp = sp+1;
	 if ((sp = strchr (bp, ')')) != NULL) *sp = '\0';
	 bp = exprscan (bp, &so, &junk, &junk, &term, FALSE);
      }

      if (index > MAXINDEX)
      {
	 logerror ("Invalid index value");
	 index = 0;
      }
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   switch (op->opmod)
   {
   case 0: /* SR */
   case 1: /* SRR */
   case 2: /* SL */
      position = 0;
      addr = op->opmod * 100 + sc;
      break;
   case 3: /* SLC */
      position = sc;
      addr = op->opmod * 100;
      break;
   default: /* SRS, SLS */
      position = 0;
      addr = op->opmod;
      if (so > 10)
      {
	 addr += 2;
	 so -= 10;
      }
      addr = (so * 1000) + (addr * 100) + sc;
   }

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, position, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, position, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2eop - Process E (Switch) type operation code.
***********************************************************************/

static void
p2eop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int position = 0;
   int swnum = 0;
   int index = 0;
   int addr = 0;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      bp = exprscan (bp, &swnum, &junk, &junk, &term, FALSE);
      if (term == ',')
	 bp = exprscan (bp, &addr, &position, &index, &term, FALSE);
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   if (index > MAXINDEX)
   {
      logerror ("Invalid index value");
      index = 0;
   }

   if (swnum > MAXSWITCHES)
   {
      logerror ("Invalid switch value");
      swnum = 0;
   }

   while (swnum > 10)
   {
      opcode++;
      swnum -= 10;
   }

   position = op->opmod + (swnum - 1);

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, position, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, position, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2iop - Process Index type operation code.
***********************************************************************/

static void
p2iop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int index = 0;
   int ix = 0;
   int addr = 0;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      bp = exprscan (bp, &ix, &junk, &junk, &term, FALSE);
      if (term == ',')
      {
	 bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);
      }

      if (ix > MAXINDEX || index > MAXINDEX)
      {
	 logerror ("Invalid index value");
	 index = 0;
	 ix = 0;
      }
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, ix, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, ix, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2lop - Process Latch type operation code.
***********************************************************************/

static void
p2lop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int position = 0;
   int index = 0;
   int addr = 0;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }

   switch (op->opmod2)
   {
   case 0: /* BAL */
      bp = exprscan (bp, &addr, &position, &index, &term, FALSE);
      position = 0;
      break;

   case 1: /* BUL, ULF, ULN */
      if (*bp == 'A' || *bp == 'B')
      {
         position = op->opmod + (*bp - 'A');
	 bp += 2;
      }
      else
	 logerror ("Invalid unit latch");
      bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);
      break;

   case 2: /* BQL, QLF, QLN */
      if (*bp == '1' || *bp == '2')
      {
         position = op->opmod + (*bp - '1');
	 bp += 2;
      }
      else
	 logerror ("Invalid inquiry control");
      bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);
      break;

   case 3: /* BTL, TLF, TLN */
      bp = exprscan (bp, &position, &junk, &index, &term, FALSE);
      if (position < 10 || position > 49)
      {
	 logerror ("Invalid tape drive");
	 position = 0;
      }
      bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);
      break;

   default:
      if (*bp >= '1' && *bp <= '4')
      {
         position = op->opmod + (*bp - '0');
	 bp += 2;
      }
      else
	 logerror ("Invalid channel");
      bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   if (index > MAXINDEX)
   {
      logerror ("Invalid index value");
      index = 0;
   }

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, position, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, position, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2qop - Process inQuiry type operation code.
***********************************************************************/

static void
p2qop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int index = 0;
   int ix = 0;
   int addr = 0;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      bp = exprscan (bp, &ix, &junk, &junk, &term, FALSE);
      if (term == ',')
	 bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);
      if (ix < 1 || ix > 2)
      {
	 logerror ("Invalid inquiry control group");
	 ix = 0;
      }
      if (index > MAXINDEX)
      {
	 logerror ("Invalid index value");
	 index = 0;
      }
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, op->opmod + (ix * 10), addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, op->opmod + (ix * 10), addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2sop - Process Shift control type operation code.
***********************************************************************/

static void
p2sop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int position = 0;
   int index = 0;
   int addr = 0;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      bp = exprscan (bp, &addr, &position, &index, &term, FALSE);
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   switch (op->opmod)
   {
   case 13: /* SLC1 */
   case 23: /* SLC2 */
   case 33: /* SLC3 */
      position = addr;
      addr = op->opmod * 100;
      break;
   default:
      position = 0;
      addr += op->opmod * 100;
      break;
   }

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, position, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, position, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2top - Process Tape type operation code.
***********************************************************************/

static void
p2top (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int chan = 0;
   int tape;
   int position = 0;
   int index = 0;
   int addr = 0;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      bp = exprscan (bp, &chan, &junk, &junk, &term, FALSE);
      tape = chan - (chan / 10 * 10);
      chan /= 10;
      if (op->opmod == 0)
	 addr = op->opmod2;
      else if (term == ',')
	 bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);

      if (index > MAXINDEX)
      {
	 logerror ("Invalid index value");
	 index = 0;
      }
      if ( chan < 1 || chan > MAXCHANNELS)
      {
	 logerror ("Invalid channel value");
	 chan = 0;
      }
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
      opcode += chan;
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
      opcode += chan;
   }

   position = (tape * 10) + op->opmod;

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, position, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, position, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2uop - Process Unit record type operation code.
***********************************************************************/

static void
p2uop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int opcode;
   int position = 0;
   int index = 0;
   int syncro = 0;
   int addr = 0;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (*bp == ' ')
   {
      logerror ("Missing Operand");
   }
   else
   {
      if (op->opmod == 4) /* TYP doesn't need a syncro */
	 bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);
      else
      {
	 bp = exprscan (bp, &syncro, &junk, &junk, &term, FALSE);
	 if (syncro < 1 || syncro > MAXSYNCRO)
	 {
	    logerror ("Invalid syncronizer value");
	    syncro = 0;
	 }
	 if (term == ',')
	    bp = exprscan (bp, &addr, &junk, &index, &term, FALSE);
      }
   }

   if (op->opvalue < 0)
   {
      term = '-';
      opcode = -(op->opvalue);
   }
   else
   {
      term = '+';
      opcode = op->opvalue;
   }

   if (index > MAXINDEX)
   {
      logerror ("Invalid index value");
      index = 0;
   }

   position = op->opmod + (syncro * 10);

   sprintf (opbuf, OPFORMAT,
	    term, opcode, index, position, addr);

   sprintf (objbuf, OBJINSTFORMAT, INST_TAG,
	    term, opcode, index, position, addr);
   punchrecord (outfd, INSTLENGTH+1);

   pc += 1;
}

/***********************************************************************
* p2pop - Process Pseudo operation code.
***********************************************************************/

static int
p2pop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   SymNode *s;
   int index = 0;
   int position = 0;
   int val = 0;
   char term;
   char sign;

   strcpy (pcbuf, PCBLANKS);
   strcpy (opbuf, OPBLANKS);

   if (op->opvalue != DC_T)
   {
      chkpacked (outfd, lstfd);
      chkindcrdw();
   }
   if (op->opvalue != DA_T)
   {
      chkinda (outfd, lstfd);
   }

   switch (op->opvalue)
   {

   case CNTRL_T:		/* CoNTRoL directive */
      if (cursym[0])
      {
	 if (!strcmp (cursym, "ORIGIN"))
	 {
	    if (*bp != ' ')
	    {
	       bp = exprscan (bp, &val, &position, &index, &term, FALSE);
	       pc = val;
	       sprintf (pcbuf, PCFORMAT, pc);
	       sprintf (objbuf, OBJDATAFORMAT, ADDR_TAG, '+', (t_uint64)pc);
	       punchrecord (outfd, INSTLENGTH+1);
	    }
	 }
	 else if (!strcmp (cursym, "LITORIGIN"))
	 {
	    if (*bp != ' ')
	    {
	       bp = exprscan (bp, &val, &position, &index, &term, FALSE);
#ifdef DEBUGLIT
	       fprintf (stderr, "p2-LORG: litorigin = %d\n", val);
#endif
	       sprintf (opbuf, ADDRFORMAT, val);
	    }
	 }
	 else if (!strcmp (cursym, "BRANCH"))
	 {
	    OpCode *op;

	    val = DEFAULTPCLOC;
	    if (*bp != ' ')
	    {
	       bp = exprscan (bp, &val, &position, &index, &term, FALSE);
	    }
	    strcpy (pcbuf, PCBLANKS);
	    sprintf (opbuf, ADDRFORMAT, val);

	    op = oplookup ("B");
	    term = '+';

	    sprintf (objbuf, OBJINSTFORMAT, BRANCH_TAG,
		     term, op->opvalue, index, position, val);
	    punchrecord (outfd, INSTLENGTH+1);
	 }
	 else if (!strcmp (cursym, "END"))
	 {
	    OpCode *op;

	    val = DEFAULTPCLOC;
	    if (*bp != ' ')
	    {
	       bp = exprscan (bp, &val, &position, &index, &term, FALSE);
	    }
	    strcpy (pcbuf, PCBLANKS);
	    sprintf (opbuf, ADDRFORMAT, val);
	    printline (lstfd);
	    printed = TRUE;

	    processliterals (outfd, lstfd, listmode);

	    op = oplookup ("B");
	    term = '+';

	    sprintf (objbuf, OBJINSTFORMAT, BRANCH_TAG,
		     term, op->opvalue, index, position, val);
	    punchrecord (outfd, INSTLENGTH+1);
	    return (TRUE);
	 }
      }
      else
         logerror ("Label required");
      break;

   case DA_T:			/* Define Area */
      sprintf (pcbuf, PCFORMAT, pc);
      if (*bp != ' ')
      {
#ifdef DEBUGDA
	 fprintf (stderr, "p2-DA: pc = %d, inda = %s, dandx = %d, dalen = %d\n",
		  pc, inda ? "TRUE" : "FALSE", dandx, dalen);
	 if (inda)
	    fprintf (stderr,
	             "   origin = %d, count = %d, length = %d, symbol = %s\n",
	             das[dandx].origin, das[dandx].count,
		     das[dandx].length, das[dandx].symbol);
#endif
	 if (!inda)
	 {
	    inda = TRUE;

	    if (das[dandx].genrdw)
	    {
	       t_uint64 num;
	       int i;
	       int startaddr, endaddr;
	       int count, length;

	       count = das[dandx].count;
	       length = das[dandx].length;
	       startaddr = das[dandx].origin;
	       endaddr = das[dandx].origin + length - 1;

	       for (i = 0; i < count; i++)
	       {
		  sign = das[dandx].sign;
		  num = ((t_uint64)startaddr * 10000) + (t_uint64)endaddr;
		  if (sign == ' ')
		  {
		     sign = (i == count - 1) ? '-' : '+';
		  }
		  sprintf (opbuf, DATAFORMAT, sign, num);
		  sprintf (objbuf, OBJDATAFORMAT, DATA_TAG, sign, num);
		  punchrecord (outfd, INSTLENGTH+1);
		  if (count > 1)
		  {
		     if (i == 0)
		     {
			printline (lstfd);
			printed = TRUE;
		     }
		     else
			printdata (lstfd, pc);
		  }
		  startaddr += length;
		  endaddr += length;
		  pc++;
	       }
	    }
	    dalen = 0;
	 }
	 else
	 {
	    int fdpos;
	    int sv, ev;

	    sv = ev = 0;
	    while (*bp && isdigit(*bp))
	       sv = (sv * 10) + (*bp++ - '0');
	    if (*bp == ',')
	    {
	       bp++;
	       while (*bp && isdigit(*bp))
		  ev = (ev * 10) + (*bp++ - '0');
	    }
	    sprintf (pcbuf, PCFORMAT, das[dandx].origin + (sv / 10));
	    if ((sv / 10) != (ev / 10))
	       fdpos = ((sv % 10) * 10) + 9;
	    else
	       fdpos = ((sv % 10) * 10) + (ev % 10);
	    sprintf (cdfd, FDFORMAT, fdpos);
#ifdef DEBUGDA
            fprintf (stderr, "   sv = %d, ev = %d\n", sv, ev);
#endif
	    if (ev > dalen)
	       dalen = ev + 1;
	    pc = das[dandx].origin + (ev / 10);
	 }
      }
      else
         logerror ("Missing count");
      break;

   case DC_T:			/* Define Constant */
      sprintf (pcbuf, PCFORMAT, pc);

      sign = 0;
      if (*bp == '-' || *bp == '+')
      {
         sign = *bp++;
      }

      if (!strncmp (bp, "RDW", 3))
      {
	 t_uint64 num;
	 int startaddr, endaddr;

	 chkpacked (outfd, lstfd);
	 chkindcrdw();

#ifdef DEBUGDCRDW
	 fprintf (stderr, "p2-DC_RDW: pc = %d, sign = %c, rdwndx = %d\n",
		  pc, sign, rdwndx);
#endif
         indcrdw = TRUE;
	 if (cursym[0])
	 {
	    if ((s = symlookup (cursym, FALSE, FALSE)))
	       s->length = rdws[rdwndx].length;
	 }
	 startaddr = rdws[rdwndx].origin;
	 endaddr = rdws[rdwndx].origin + rdws[rdwndx].length - 1;
	 sprintf (pcbuf, PCFORMAT, pc);
	 num = ((t_uint64)startaddr * 10000) + (t_uint64)endaddr;
	 sprintf (opbuf, DATAFORMAT, sign, num);
	 sprintf (objbuf, OBJDATAFORMAT, DATA_TAG, sign, num);
	 punchrecord (outfd, INSTLENGTH+1);

	 rdwlen = 0;
	 rdwndx++;
	 val = 1;
      }
      else if (*bp == STRINGSYM)
      {
#ifdef DEBUGDCPACKED
	 fprintf (stderr, "p2-DC_CHAR: pc= %d, packedpc = %d, packedlen = %d\n",
	      pc, packedpc, packedlen);
#endif
	 if (cursym[0])
	 {
	    if ((s = symlookup (cursym, FALSE, FALSE)))
	       sprintf (cdfd, FDFORMAT, s->fdpos);
	 }
         bp = processchardata (outfd, lstfd, bp, &val, TRUE);
      }
      else
      {
	 if (cursym[0])
	 {
	    if ((s = symlookup (cursym, FALSE, FALSE)))
	       sprintf (cdfd, FDFORMAT, s->fdpos);
	 }
	 if (sign) bp--;
	 bp = processcondata (outfd, lstfd, bp, &val, TRUE);
      }
      rdwlen += val;
      pc += val;
      break;

   case DRDW_T:			/* Define Record Definition Word */
      {
	 t_uint64 num;
	 int startaddr, endaddr;
	 sign = '+';

	 sprintf (pcbuf, PCFORMAT, pc);
	 if (*bp == '-' || *bp == '+')
	    sign = *bp++;
	 bp = exprscan (bp, &startaddr, &position, &index, &term, FALSE);
	 if (term == ',')
	 {
	    bp = exprscan (bp, &endaddr, &position, &index, &term, FALSE);
	    num = ((t_uint64)startaddr * 10000) + (t_uint64)endaddr;
	    sprintf (opbuf, DATAFORMAT, sign, num);
	    sprintf (objbuf, OBJDATAFORMAT, DATA_TAG, sign, num);
	    punchrecord (outfd, INSTLENGTH+1);
	 }
	 if (cursym[0])
	 {
	    if ((s = symlookup (cursym, FALSE, FALSE)))
	       s->length = (endaddr - startaddr) + 1;
	 }
	 pc ++;
	 break;
      }

   case DSW_T:			/* DSW */
      sprintf (pcbuf, PCFORMAT, pc);
      if (*bp != ' ')
      {
	 t_uint64 num;
	 int pos;

	 num = 0;
	 for (pos = 0; *bp && *bp != ' ' && pos < 10; pos++)
	 {
	    char *token;
	    int tokentype;

	    if (*bp == '-' || *bp == '+') bp++;
	    bp = tokscan (bp, &token, &tokentype, &val, &term);
	    if (tokentype == SYM)
	    {
	       if ((s = symlookup (token, FALSE, FALSE)) != NULL)
	       {
		  num = (num * 10) + s->length;
	       }
	    }
	    else
	       logerror ("Invalid symbol");
	 }
	 if (*bp != ' ')
	    logerror ("Too many switches");
	 for (; pos < 10; pos++) num *= 10;
	 sprintf (opbuf, DATAFORMAT, '+', num);
	 sprintf (objbuf, OBJDATAFORMAT, DATA_TAG, '+', num);
	 punchrecord (outfd, INSTLENGTH+1);
      }
      else
	 logerror ("Missing Operand");
      pc++;
      break;

   case EJECT_T:		/* EJECT a page */
      if (linecnt)
      {
	 linecnt = MAXLINE;
	 printheader (lstfd);
      }
      printed = TRUE;
      break;

   case EQU_T:			/* EQU */
      if (*bp == '@')
      {
	 logerror ("Invalid expression");
	 break;
      }
      if (cursym[0] == '\0')
      {
	 logerror ("Label required");
      }
      else
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	    sprintf (opbuf, ADDRFORMAT, s->value);
         else 
	    sprintf (opbuf, ADDRFORMAT, 0);
      }
      break;

   default: ;
   }
   return (FALSE);
}

/***********************************************************************
* asmpass2 - Pass 2 
***********************************************************************/

int
asmpass2 (FILE *tmpfd, FILE *outfd, int lstmode, FILE *lstfd)
{
   OpCode *op;
   char *bp, *cp;
   char *token;
   int status = 0;
   int done;
   int srcmode;
   int val;
   int tokentype;
   int endcard;
   char term;
   char opcode[MAXSYMLEN+2];
   char srcbuf[MAXLINE];

#ifdef DEBUGP2RDR
   fprintf (stderr, "asmpass2: Entered\n");
#endif

   /*
   ** Rewind input file.
   */

   if (fseek (tmpfd, 0, SEEK_SET) < 0)
   {
      perror ("asm7070: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Initialize.
   */

   memset (objrec, ' ', sizeof(objrec));

   gbllistmode = lstmode;
   listmode = lstmode;

   linecnt = MAXLINE;
   pc = DEFAULTPCLOC;

   printed = FALSE;
   orgout = FALSE;
   xline = FALSE;
   intcdmode = FALSE;
   idtemitted = FALSE;
   endcard = FALSE;
   inpacked = FALSE;
   indcrdw = FALSE;

   objrecnum = 0;
   objectpc = 0;
   pagenum = 0;
   linenum = 0;
   packedpc = 0;
   packedlen = 0;
   rdwndx = 0;
   packedsign = 0;

   objcnt = INSTFIELD;
   op = lastop = NULL;

   if (stdlist)
      strcpy (ttlbuf, 
	      "LN CDREF   LABEL      OP      OPERAND                 COMMENTS  "
	      "                   CDNO FD  LOC  INSTRUCTION  RI REF");
   else
      strcpy (ttlbuf,
	      "LOC  INSTRUCTION   STMT         SOURCE STATEMENT");

   strcpy (opcode, "   ");

   /*
   ** Process the source.
   */

   done = FALSE;
   while (!done)
   {
      if (fread (&srcmode, 1, 4, tmpfd) != 4)
      {
         done = TRUE;
	 break;
      }
      if (fgets(srcbuf, MAXLINE, tmpfd) == NULL)
      {
         done = TRUE;
	 break;
      }

      linenum++;
#ifdef DEBUGP2RDR
      fprintf (stderr, "P2in = %s", srcbuf);
#endif
      if (stdlist)
	 strcpy (lstbuf, srcbuf);
      else
	 strcpy (lstbuf, &srcbuf[SYMSTART]);
      if ((cp = strchr (lstbuf, '\n')) != NULL) *cp = '\0';

      if (!widemode)
      {
	 int ii;

	 for (ii = 0; lstbuf[ii] && ii < strlen(lstbuf); ii++)
	 {
	    if (ii >= NARROWPRINTMARGIN-2)
	    {
	       lstbuf[ii+1] = '\0';
	       break;
	    }
	 }
      }

      exprtype = ADDREXPR;
      radix = 10;
      printed = FALSE;
      errnum = 0;
      errline[0][0] = '\0';
      objbuf[0] = '\0';
      strcpy (pcbuf, PCBLANKS);
      strcpy (opbuf, OPBLANKS);
      strcpy (cdfd, FDBLANKS);

      inmode = srcmode;

#ifdef DEBUGP2RDRM
      if (inmode & MACDEF)
	 fprintf (stderr, "min = %s", inbuf);
#endif
      if (!(srcmode & (MACDEF | SKPINST)))
      {
	 if (strlen (srcbuf) > RIGHTMARGIN)
	    srcbuf[RIGHTMARGIN+1] = '\0';
	 strcpy (inbuf, srcbuf);

	 /*
	 ** If a continued line get the next one and append to inbuf
	 */

	 if (srcmode & CONTINU)
	 {
	    bp = &inbuf[VARSTART];
	    while (*bp && !isspace(*bp)) bp++;
	    *bp = '\0';
	    etccount = 0;
	    while (srcmode & CONTINU)
	    {
	       if (fread (&srcmode, 1, 4, tmpfd) != 4)
	       {
		  done = TRUE;
		  break;
	       }
	       if (fgets(srcbuf, MAXLINE, tmpfd) == NULL)
	       {
		  done = TRUE;
		  break;
	       }
#ifdef DEBUGP2RDR
	       fprintf (stderr, "P2in = %s", srcbuf);
#endif
	       strcpy (etclines[etccount++], srcbuf);
	       if (strlen (srcbuf) > RIGHTMARGIN)
		  srcbuf[RIGHTMARGIN+1] = '\0';
	       cp = &srcbuf[VARSTART];
	       while (*cp && !isspace(*cp)) cp++;
	       *cp = '\0';
	       strcat (inbuf, &srcbuf[VARSTART]);
	    }
	 }
	 strcat (inbuf, "\n");
#ifdef DEBUGP2RDRC
	 if (inmode & CONTINU)
	    fprintf (stderr, "cin = %s", inbuf);
#endif
	 bp = &inbuf[SYMSTART];

	 if (*bp != COMMENTSYM)
	 {

	    /*
	    ** If label present, scan it off.
	    */

	    if (strncmp (bp, "      ", 6))
	    {
	       char *cp;

	       strncpy (cursym, bp, MAXSYMLEN);
	       cursym[MAXSYMLEN] = '\0';
	       for (cp = cursym; *cp; cp++); 
	       *cp-- = '\0';
	       while (isspace(*cp)) *cp-- = '\0';
#ifdef DEBUGCURSYM
	       fprintf (stderr, "asmpass2: cursym = %s\n", cursym);
	       fprintf (stderr, "    rem = %s", bp);
#endif
	       while (*bp && isspace (*bp)) bp++;
	    }
	    else 
	       cursym[0] = '\0';

	    /*
	    ** Scan off opcode.
	    */

	    bp = &inbuf[OPCSTART];
	    prevop = op;
	    if (*bp != ' ')
	    {
	       bp = tokscan (bp, &token, &tokentype, &val, &term);
	       strcpy (opcode, token);
	       op = NULL;
	    }

	    /*
	    ** If not a macro call ref, then process
	    */

	    if (!(inmode & MACCALL))
	    {

	       /*
	       ** Process according to type.
	       */

	       bp = &inbuf[VARSTART];
	       if ((op = oplookup (opcode)) != NULL)
	       {
		  if (op->optype == TYPE_P)
		  {
		     if ((endcard = p2pop (op, bp, outfd, lstfd)) == TRUE)
		        done = TRUE;
		  }
		  else
		  {
		     chkpacked (outfd, lstfd);
		     chkindcrdw();
		     chkinda (outfd, lstfd);
		     inst_proc[op->optype].proc (op, bp, outfd, lstfd);
		  }
	       }
	       else
	       {
		  pc += 1;
		  sprintf (errtmp, "Invalid opcode: %s", opcode);
		  logerror (errtmp);
	       }
	    }

	    /*
	    ** MACRO Call, put out the PC
	    */

	    else 
	    {
	       if (etccount)
	       {
		  int etcndx;

		  printline (lstfd);
		  printed = TRUE;
		  for (etcndx = 0; etcndx < etccount; etcndx++)
		  {
		     linenum++;
		     strcpy (lstbuf, &etclines[etcndx][SYMSTART]);
		     etclines[etcndx][0] = '\0';
		     if (!widemode)
		     {
			lstbuf[NARROWPRINTMARGIN-1] = '\n';
			lstbuf[NARROWPRINTMARGIN] = '\0';
		     }
		     printline (lstfd);
		  }
		  etccount = 0;
	       }
	    }
	 }
	 else
	 {
	    chkpacked (outfd, lstfd);
	    chkindcrdw();
	    chkinda (outfd, lstfd);
	 }
      }

      /*
      ** Write out a print line.
      */

      if (listmode && !printed)
      {
         printline (lstfd);
	 printed = FALSE;
      }
      xline = FALSE;

      /*
      ** Process errors for this line.
      */

      if ((val = printerrors (lstfd, listmode)) < 0)
         status = -1;

   }

   listmode = lstmode;
   if (!endcard)
   {
      errcount++;
      if (listmode)
      {
         fprintf (lstfd, "ERROR: No END record\n");
      }
      else
      {
	 fprintf (stderr, "asm7070: %d: No END record\n", linenum);
      }
      status = -1;
   }
   else
   {

      /*
      ** Finish the object.
      */

      punchfinish (outfd);
      puncheof (outfd);

      /*
      ** Print symbol table
      */

      if (listmode)
      {
         printsymbols (lstfd);
      }
   }

   /*
   ** Print options
   */

   if (listmode)
   {
      fprintf (lstfd, "\n");
      fprintf (lstfd, "Assembly mode: %d\n", cpumodel);

      if (deckname[0])
         fprintf (lstfd, "Deckname: %s\n", deckname);

      fprintf (lstfd, "Options: \n");
      fprintf (lstfd, "   ");
      if (!stdlist)
         fprintf (lstfd, "No ");
      fprintf (lstfd, "Standard listing\n");
      fprintf (lstfd, "   ");
      if (!genxref)
         fprintf (lstfd, "No ");
      fprintf (lstfd, "Cross reference\n");
   }

   return (status);
}
