/***********************************************************************
*
* asmpass2.c - Pass 2 for the IBM 7090 assembler.
*
* Changes:
*   05/21/03   DGP   Original.
*   08/13/03   DGP   Added XREF support.
*   08/20/03   DGP   Changed to call the SLR(1) parser.
*   12/20/04   DGP   Add MAP functions and 2.0 additions.
*   02/02/05   DGP   Correct VFD processing and Indirect flag.
*   02/03/05   DGP   Added JOBSYM.
*   02/09/05   DGP   Added FAP mode.
*   03/01/05   DGP   Changed VFD expressions to DATAVALUE mode. And
*                    pad with leading spaces in Hollerith mode.
*   03/09/05   DGP   Fixed BCD and BCI processing.
*   03/11/05   DGP   Fixed chained IF[FT] statements.
*   03/15/05   DGP   Fixed "LOC  blank" in FAP mode.
*   03/24/05   DGP   Fixed UNPNCH mode to punch buffer first and allow 
*                    EOF at end.
*   05/24/05   DGP   Added count modifier field on type_chan ops.
*   05/25/05   DGP   Fixed disk operations.
*   06/07/05   DGP   Fixed FAP IFF.
*   06/08/05   DGP   Made ORG always relocatable in relocatable assembly.
*                    Added character delimiter to BCI.
*   06/13/05   DGP   Added VFD mode to literals.
*   12/15/05   RMS   MINGW changes.
*   02/20/06   DGP   Fix NARROW print.
*   03/22/06   DGP   Fixed channel code generation.
*   12/26/06   DGP   Fixed PUNCH/UNPNCH logic.
*   10/17/07   DGP   More VFD fixes.
*   05/14/09   DGP   Allow BCD to start with 0.
*   09/21/09   DGP   Support TITLE/DETAIL in FAP mode.
*                    Don't print first line if a comment in FAP mode.
*   09/30/09   DGP   Added FAP Linkage Director.
*   10/16/09   DGP   Fixup EVEN op.
*   11/03/09   DGP   Allow opcode terminated by '('.
*                    Allow EOS to terminate a VFD/ETC block.
*   03/31/10   DGP   Fixed "IFF N" processing.
*   04/01/10   DGP   Fixup short literal VFD.
*   04/30/10   DGP   Added sequence checking option.
*   05/10/10   DGP   Added opcore ACORE/BCORE support.
*   06/07/10   DGP   Added punch symbol table option.
*   06/08/10   DGP   Fix IFF skip count.
*   06/16/10   DGP   Don't list skipped lines unless PCC mode enabled.
*   07/07/10   DGP   Don't list RMT seq if macro generated and PMC OFF.
*   11/01/10   DGP   Added COMMON Common mode.
*   11/08/10   DGP   Allow relocatable operands in Type E instructions.
*   12/06/10   DGP   Changed to allow relocation of VFD address.
*   12/15/10   DGP   Added line number to temporary file.
*                    Added file sequence for errors.
*   12/16/10   DGP   Added a unified source read function readsource().
*   12/24/10   DGP   Correct page count to eject new page.
*   05/06/13   DGP   Fixed QUAL processing.
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
extern int symbolcount;		/* Number of symbols in symbol table */
extern int absolute;		/* In absolute section */
extern int radix;		/* Number scanner radix */
extern int linenum;		/* Source line number */
extern int errcount;		/* Number of errors in assembly */
extern int errnum;		/* Number of pass 2 errors for current line */
extern int warnnum;		/* Number of pass 2 warnings for current line */
extern int exprtype;		/* Expression type */
extern int p1errcnt;		/* Number of pass 0/1 errors */
extern int pgmlength;		/* Length of program */
extern int absmod;		/* In absolute module */
extern int monsym;		/* Include IBSYS Symbols (MONSYM) */
extern int jobsym;		/* Include IBJOB Symbols (JOBSYM) */
extern int termparn;		/* Parenthesis are terminals (NO()) */
extern int genxref;		/* Generate cross reference listing */
extern int addext;		/* Add extern for undef'd symbols (!absolute) */
extern int addrel;		/* ADDREL mode */
extern int qualindex;		/* QUALifier table index */
extern int begincount;		/* Number of BEGINs */
extern int litorigin;		/* Literal pool origin */
extern int litpool;		/* Literal pool size */
extern int entsymbolcount;	/* Number of ENTRY symbols */
extern int widemode;		/* Generate wide listing */
extern int sequcheck;		/* Check sequence numbers */
extern int punchsymtable;	/* Punch symbol table */
extern int lastsequ;		/* Last sequence number */
extern int rboolexpr;		/* RBOOL expression */
extern int lboolexpr;		/* LBOOL expression */
extern int fapmode;		/* FAP assembly mode */
extern int headcount;		/* Number of entries in headtable */
extern int cpumodel;		/* CPU model */
extern int genlnkdir;		/* FAP generate linkage director */
extern int bcore;		/* BCORE assembly */
extern int commonctr;		/* Common counter */
extern int commonused;		/* Common used */
extern int filenum;		/* Current file number */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char deckname[MAXLBLLEN+2];/* The assembly deckname */
extern char lbl[MAXLBLLEN+2];	/* The object label */
extern char ttlbuf[TTLSIZE+2];	/* The assembly TTL buffer */
extern char qualtable[MAXQUALS][MAXSYMLEN+2]; /* The QUALifier table */
extern char errline[10][120];	/* Pass 2 error lines for current statment */
extern char warnline[10][120];	/* Pass 2 warning lines for current statment */
extern char titlebuf[TTLSIZE+2];/* The assembly TITLE buffer */
extern char headtable[MAXHEADS];/* HEAD table */
extern char datebuf[48];	/* Date/Time buffer for listing */
extern char lnkdirsym[MAXSYMLEN+2];/* Linkage director symbol */

extern SymNode *addextsym;	/* Added symbol for externals */
extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern SymNode *entsymbols[MAXSYMBOLS];/* The Entry Symbol table */
extern ErrTable p1error[MAXERRORS];/* The pass 0/1 error table */
extern BeginTable begintable[MAXBEGINS];/* The USE/BEGIN table */
extern time_t curtime;		/* Assembly time */
extern struct tm *timeblk;	/* Assembly time */


static int linecnt;		/* Current page line count */
static int objrecnum;		/* Object record number */
static int objcnt;		/* Current count in object record */
static int pagenum;		/* Current listing page number */
static int pccmode;		/* PCC mode flag */
static int pmcmode;		/* PMC mode flag */
static int printed;		/* Line has been printed flag */
static int orgout;		/* Origin has been output flag */
static int inmode;		/* Input source line mode */
static int asmskip;		/* In skip line mode IFF/IFT */
static int gotoskip;		/* In GOTO mode */
static int gotoblank;		/* BLANK option on GOTO */
static int gbllistmode;		/* LIST/UNLIST flag */
static int listmode;		/* Listing requested flag */
static int punchmode;		/* PUNCH/UNPCH flag */
static int etccount;		/* Number of ETC records */
static int curruse;		/* Current USE index */
static int prevuse;		/* Previous USE index */
static int usebegin;		/* BEGIN in use */
static int ifcont;		/* IFF/IFT continued */
static int ifrel;		/* IFF/IFT continue relation OR/AND */
static int ifskip;		/* IFF/IFT prior skip result */
static int litloc;		/* Literal pool location */
static int dupin;		/* DUP input line count */
static int noreflst;		/* REF psop, no listing */
static int nooperands;		/* No operands on line */
static int firstindex;		/* First INDEX seen */
static int detailmode;		/* DETAIL/TITLE mode flag */
static int tcdemitted;		/* Emitted a TCD */
static int xline;		/* X line on listing, in goto or skip */
static int prevloc;		/* Previous LOC pc value */
static int locorg;		/* Origin of current LOC */
static int inloc;		/* In a LOC section */
static int idtlen;		/* IDT length */
static int idtemitted;		/* IDT emitted */

static char pcbuf[10];		/* PC print buffer */
static char objbuf[16];		/* Object field buffer */
static char objrec[82];		/* Object record buffer */
static char opbuf[32];		/* OP print buffer */
static char idtbuf[10];		/* IDT buffer */
static char lastmod[10];	/* module buffer */

static char cursym[MAXSYMLEN+2];/* Current label field symbol */
static char gotosym[MAXSYMLEN+2];/* GOTO target symbol */
static char lstbuf[MAXLINE];	/* Source line for listing */
static char etclines[MAXETCLINES][MAXLINE];/* ETC lines */
static char errtmp[256];	/* Error print string */

static void p2aop (OpCode *, int, char *);
static void p2bop (OpCode *, int, char *);
static void p2cop (OpCode *, int, char *);
static void p2dop (OpCode *, int, char *);
static void p2eop (OpCode *, int, char *);
static void p2chanop (OpCode *, int, char *);

typedef struct {
   void (*proc) (OpCode *, int, char *);
} Inst_Proc;

Inst_Proc inst_proc[MAX_INST_TYPES] = {
   p2aop, p2bop, p2cop, p2dop, p2eop, p2chanop
};


/***********************************************************************
* printctlheader - Print control card header on listing.
***********************************************************************/

static void
printctlheader (FILE *lstfd)
{
   if (!listmode) return;

   /*
   ** If top of page, print header
   */

   if (linecnt >= LINESPAGE)
   {
      linecnt = 0;
      if (pagenum) fputc ('\f', lstfd);
      if (widemode)
	 fprintf (lstfd, H1WIDEFORMAT, VERSION, titlebuf, datebuf, ++pagenum);
      else
	 fprintf (lstfd, H1FORMAT, VERSION, titlebuf, datebuf, ++pagenum);
      fprintf (lstfd, H2FORMAT, "IBSYS CONTROL CARDS");
   }
}

/***********************************************************************
* printctlcard - Print control card
***********************************************************************/

static void
printctlcard (FILE *lstfd)
{
   if (!listmode) return;

   printctlheader (lstfd);
   fprintf (lstfd, "%04d  %s", linenum, lstbuf);
}

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
      if (pagenum) fputc ('\f', lstfd);
      if (widemode)
	 fprintf (lstfd, H1WIDEFORMAT, VERSION, titlebuf, datebuf, ++pagenum);
      else
	 fprintf (lstfd, H1FORMAT, VERSION, titlebuf, datebuf, ++pagenum);
      fprintf (lstfd, H2FORMAT, ttlbuf);
   }
}

/***********************************************************************
* printline - Print line on listing.
*
*0        1         2         3         4
*1234567890123456789012345678901234567890
*
*ooooo  SOOOO FF T AAAAA   LLLLLG  TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
*ooooo  SO DDDDD T AAAAA   LLLLLG  TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
*
***********************************************************************/

static void
printline (FILE *lstfd)
{
   char mch;

   if (!listmode) return;

   /*
   ** Check if current modes enable the line to be listed.
   */

   if (!(inmode & RMTSEQ))
   {
      if (!detailmode && (inmode & (CALL | SAVE)))
	 return;
      if (!pmcmode && (inmode & (MACEXP | RETURN)))
	 return;
      if (!pccmode)
      {
	 if (inmode & SKPINST) 
	    return;
	 if (inmode & MACCALL && inmode & MACEXP)
	    return;
      }
   }

   /*
   ** If Remote Sequence line, mark with a 'R'.
   */

   if (inmode & RMTSEQ) {
      if ((inmode & SKPINST) || (inmode & MACCALL))
          return;
      if (!pmcmode) return;
      mch = 'R';
   }

   /*
   ** If in skip/goto mark with the X
   */

   else if (xline || (inmode & SKPINST))
   {
      mch = 'X';
      if (!pccmode)
      {
	 printed = TRUE;
	 return;
      }
   }

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
*ooooo  SDDDDDDDDDDDD
*
***********************************************************************/

static void
printdata (FILE *lstfd, int opc)
{
   if (!listmode) return;

   printheader (lstfd);
   sprintf (pcbuf, PCFORMAT, opc);
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

   j = 0;
   /*
   ** Print the definition line number
   */
   fprintf (lstfd, " %4d ", sym->line);

   /*
   ** Go through the list printing reference line numbers
   */

   while (xref)
   {
      if (j >= (widemode ? 14 : 9))
      {
	 fprintf (lstfd, "\n");
	 printheader (lstfd);
	 if (!linecnt)
	 {
	    fprintf (lstfd, " SYMBOL    ADDR    DEF            REFERENCES\n\n");
	    linecnt = 2;
	 }
	 fprintf (lstfd, "                        ");
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
   int qualcount;

   if (!listmode) return;
   if (noreflst) return;

   /*
   ** Get symbol qualifiers (heads)
   */

   qualcount = 1;
   for (i = 0; i < symbolcount; i++)
   {
      int lq;

      sym = symbols[i];
      if (sym->qualifier[0])
      {
	 if (qualcount > 1)
	 {
	    lq = qualcount;
	    for (j = 1; j < qualcount; j++)
	    {
	       if (strcmp (qualtable[j], sym->qualifier) > 0) lq = j;
	       if (!strcmp (qualtable[j], sym->qualifier))
	       {
	          break;
	       }
	    }
	    if (j == qualcount)
	    {
	       for (j = qualcount; j > lq; j--)
	       {
	          strcpy (qualtable[j], qualtable[j-1]);
	       }
	       strcpy (qualtable[lq], sym->qualifier);
	       qualcount++;
	    }
	 }
	 else
	    strcpy (qualtable[qualcount++], sym->qualifier);
      }
   }

#ifdef DEBUGSYMQUAL
   fprintf (stderr, "printsymbols: qualtable:\n");
   for (j = 0; j < qualcount; j++)
      fprintf (stderr, "   id%02d = '%s'\n", j, qualtable[j]);
#endif

   /*
   ** Print each qualifer section seperately
   */

   for (qualindex = 0; qualindex < qualcount; qualindex++)
   {
      linecnt = MAXLINE;
      j = 0;
      if (qualindex == 0)
	 sprintf (ttlbuf, "SYMBOL TABLE");
      else
	 sprintf (ttlbuf, "SYMBOL TABLE : %s %s",
		  fapmode ? "HEAD" : "QUALIFIER", qualtable[qualindex]);

      for (i = 0; i < symbolcount; i++)
      {

	 sym = symbols[i];
	 if ((sym->symbol[0] != LITERALSYM) && 
	     !strcmp (qualtable[qualindex], sym->qualifier))
	 {
	    char type;

	    printheader (lstfd);
	    if (genxref && !linecnt)
	    {
	       fprintf (lstfd,
			" SYMBOL    ADDR    DEF            REFERENCES\n\n");
	       linecnt = 2;
	    }

	    if (sym->flags & GLOBAL) type = 'G';
	    else if (sym->flags & EXTERNAL)
	    {
	       if (sym->flags & NOEXPORT)
	       {
		  if (sym->flags & RELOCATABLE)
		     type = 'R';
		  else
		     type = ' ';
	       }
	       else
		  type = 'E';
	    }
	    else if (sym->flags & COMMON) type = 'C';
	    else if (sym->flags & BOOL) type = 'B';
	    else if (sym->flags & SETVAR) type = 'S';
	    else if (sym->flags & TAPENO) type = 'T';
	    else if (sym->flags & RELOCATABLE) type = 'R';
	    else type = ' ';

	    if (sym->flags & BOOL)
	       fprintf (lstfd, BSYMFORMAT, sym->symbol, sym->value, type);
	    else
	       fprintf (lstfd, SYMFORMAT, sym->symbol, sym->value, type);

	    j++;
	    if (genxref)
	    {
	       j = printxref (sym, lstfd);
	    }

	    if (j >= (widemode ? 6 : 4))
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

   /*
   ** Print the ENTRY symbol table
   */

   for (qualindex = 0; qualindex < qualcount; qualindex++)
   {
      linecnt = MAXLINE;
      j = 0;
      if (qualindex == 0)
	 sprintf (ttlbuf, "ENTRY SYMBOL TABLE");
      else
	 sprintf (ttlbuf, "ENTRY SYMBOL TABLE : %s %s",
		  fapmode ? "HEAD" : "QUALIFIER", qualtable[qualindex]);

      for (i = 0; i < entsymbolcount; i++)
      {
	 sym = entsymbols[i];
	 if (!strcmp (qualtable[qualindex], sym->qualifier))
	 {
	    char type;

	    printheader (lstfd);
	    if (genxref && !linecnt)
	    {
	       fprintf (lstfd,
			" SYMBOL    ADDR    DEF            REFERENCES\n\n");
	       linecnt = 2;
	    }

	    type = 'G';

	    fprintf (lstfd, SYMFORMAT,
		     sym->symbol,
		     sym->value,
		     type);
	    j++;
	    if (genxref)
	    {
	       j = printxref (sym, lstfd);
	    }

	    if (j >= (widemode ? 6 : 4))
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
}

/***********************************************************************
* punchfinish - Punch a record with sequence numbers.
***********************************************************************/

static void
punchfinish (FILE *outfd)
{
   if (!punchmode) return;

   if (objcnt)
   {
      if (lbl[0])
	 strncpy (&objrec[LBLSTART], lbl, strlen(lbl));
      sprintf (&objrec[SEQUENCENUM], SEQFORMAT, ++objrecnum);
      fputs (objrec, outfd);
      memset (objrec, ' ', sizeof(objrec));
      objcnt = 0;
   }
}

/***********************************************************************
* punchidt - Punch an IDT record.
***********************************************************************/

static void
punchidt (FILE *outfd)
{
   char temp[20];

   if (!punchmode) return;

   if (!idtemitted)
   {
      sprintf (temp, OBJSYMFORMAT, IDT_TAG, idtbuf, idtlen);
      strncpy (&objrec[objcnt], temp, WORDTAGLEN);
      objcnt += WORDTAGLEN;
      idtemitted = TRUE;

      /* 
      ** If common was used, emit COMMON.
      */

      if (fapmode && commonused)
      {
	 sprintf (temp, WORDFORMAT, FAPCOMMON_TAG, commonctr);
	 strncpy (&objrec[objcnt], temp, WORDTAGLEN);
	 objcnt += WORDTAGLEN;
      }
      commonctr = FAPCOMMONSTART;
   }
}

/***********************************************************************
* punchrecord - Punch an object value into record.
***********************************************************************/

static void 
punchrecord (FILE *outfd)
{
   if (!punchmode) return;

   if (objcnt+WORDTAGLEN >= CHARSPERREC)
   {
      punchfinish (outfd);
   }
   punchidt (outfd);
   if (!orgout)
   {
      char temp[20];

      sprintf (temp, WORDFORMAT, absolute ? ABSORG_TAG : RELORG_TAG, 0);
      strncpy (&objrec[objcnt], temp, WORDTAGLEN);
      objcnt += WORDTAGLEN;
      orgout = TRUE;
   }
   strncpy (&objrec[objcnt], objbuf, WORDTAGLEN);
   objbuf[0] = '\0';
   objcnt += WORDTAGLEN;
}

/***********************************************************************
* punchsymbols - Punch EXTRN and ENTRY symbols.
***********************************************************************/

static void
punchsymbols (FILE *outfd)
{
   int i;

   if (!punchmode) return;

   /*
   ** Punch external/COMMON symbol
   */

   for (i = 0; i < symbolcount; i++)
   {
      SymNode *s;

      s = symbols[i];
      if ((s->flags & EXTERNAL) && !(s->flags & NOEXPORT))
      {
	 sprintf (objbuf, OBJSYMFORMAT,
		  s->flags & RELOCATABLE ? RELEXTRN_TAG : ABSEXTRN_TAG,
		  s->symbol, s->value);
         
         punchrecord (outfd);
      }
      else if (s->flags & GLOBAL)
      {
	 sprintf (objbuf, OBJSYMFORMAT,
		  s->flags & RELOCATABLE ? RELGLOBAL_TAG : ABSGLOBAL_TAG,
		  s->symbol, s->value);
         punchrecord (outfd);
      }
      else if (punchsymtable)
      {
	 if (s->flags & COMMON)
	    sprintf (objbuf, OBJSYMFORMAT, COMMON_TAG, s->symbol, s->value);
	 else if (s->flags & RELOCATABLE)
	    sprintf (objbuf, OBJSYMFORMAT, RELSYM_TAG, s->symbol, s->value);
	 else 
	    sprintf (objbuf, OBJSYMFORMAT, ABSSYM_TAG, s->symbol, s->value);
         
         punchrecord (outfd);
      }
   }

   /*
   ** Punch entry symbol
   */

   for (i = 0; i < entsymbolcount; i++)
   {
      SymNode *s;

      s = entsymbols[i];
      sprintf (objbuf, OBJSYMFORMAT,
	       s->flags & RELOCATABLE ? RELGLOBAL_TAG : ABSGLOBAL_TAG,
	       s->symbol, s->value);
      punchrecord (outfd);
   }
}

/***********************************************************************
* puncheof - Punch EOF mark.
***********************************************************************/

static void
puncheof (FILE *outfd)
{
   char temp[80];

   if (!punchmode) return;

   punchfinish (outfd);
   strncpy (objrec, "$EOF", 4);
   sprintf (temp, "%-8.8s  %02d/%02d/%02d  %02d:%02d:%02d    ASM7090 %s",
	    fapmode ? lbl : deckname,
	    timeblk->tm_mon+1, timeblk->tm_mday, timeblk->tm_year - 100,
	    timeblk->tm_hour, timeblk->tm_min, timeblk->tm_sec,
	    VERSION);
   strncpy (&objrec[7], temp, strlen(temp));
   objcnt = 1;
   punchfinish (outfd);
   idtemitted = FALSE;
}

/***********************************************************************
* checksequence - Check sequence numbers.
***********************************************************************/

static void
checksequence (char *srcbuf, int srcmode)
{
   if (!sequcheck) return;
   if (srcmode & (RMTSEQ | MACEXP | GENINST | DUPINST)) return;

   if ((strlen (srcbuf) >= 80) && isdigit (srcbuf[79]))
   {
      char *bp;
      int num;

      bp = &srcbuf[79];
      while (isdigit (*bp) && (bp > &srcbuf[71])) bp--;
      bp++;
      num = atoi (bp);

      if (!lastmod[0] || strncmp (lastmod, &srcbuf[72], 4))
      {
	 strncpy (lastmod, &srcbuf[72], 4);
	 lastmod[4] = '\0';
	 lastsequ = 0;
      }
      if (num <= lastsequ)
      {
	 char temp[10];

	 strncpy (temp, &srcbuf[72], 8);
	 temp[8] = '\0';
	 sprintf (errtmp, "Sequence number: %d, %s", lastsequ, temp);
	 logwarning (errtmp);
      }
      else
      {
	 lastsequ = num;
      }
   }
   else if (lastmod[0])
   {
	 sprintf (errtmp, "Blank sequence: %d, %s", lastsequ, lastmod);
	 logwarning (errtmp);
   }
   else if (!lastmod[0] && linenum > 1)
   {
      sequcheck = FALSE;
   }
}

/***********************************************************************
* printerrors - Print any error/warning message for this line.
***********************************************************************/

static int
printerrors (FILE *lstfd, int listmode)
{
   int i;
   int ret;

   ret = 0;
   if (warnnum)
   {
      for (i = 0; i < warnnum; i++)
      {
	 if (listmode)
	 {
	    fprintf (lstfd, "WARNING: %s\n", warnline[i]);
	    linecnt++;
	 }
	 else if (gbllistmode)
	 {
	    fprintf (lstfd, "WARNING: %d-%d: %s\n",
		     filenum + 1, linenum, warnline[i]);
	    linecnt++;
	 }
	 else
	 {
	    fprintf (stderr, "asm7090: %d-%d: %s\n",
		     filenum + 1, linenum, warnline[i]);
	 }
	 warnline[i][0] = '\0';
      }
   }

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
	    fprintf (lstfd, "ERROR: %d-%d: %s\n",
		     filenum + 1, linenum, errline[i]);
	    linecnt++;
	 }
	 else
	 {
	    fprintf (stderr, "asm7090: %d-%d: %s\n",
		     filenum + 1, linenum, errline[i]);
	 }
	 errline[i][0] = '\0';
      }
      ret = -1;
   }

   if (p1errcnt)
      for (i = 0; i < p1errcnt; i++)
	 if ((p1error[i].errorline == linenum) &&
	     (p1error[i].errorfile == filenum))
	 {
	    if (listmode)
	    {
	       fprintf (lstfd, "ERROR: %s\n", p1error[i].errortext);
	       linecnt++;
	    }
	    else if (gbllistmode)
	    {
	       fprintf (lstfd, "ERROR: %d-%d: %s\n",
			filenum + 1, linenum, p1error[i].errortext);
	       linecnt++;
	    }
	    else
	    {
	       fprintf (stderr, "asm7090: %d-%d: %s\n",
			filenum + 1, linenum, p1error[i].errortext);
	    }
	    ret = -1;
	 }

   return (ret);
}

/***********************************************************************
* processliterals - Process literals.
***********************************************************************/

static void
processliterals (FILE *outfd, FILE *lstfd, int listmode)
{
   t_uint64 dll;
   char *cp;
   char *token;
   int i, j;
   int literalcount = 0;
   int escapecount = 0;
   int val;
   int junk;
   int tokentype;
   int bitcount;
   int bitword;
   char term;
   char lcltoken[TOKENLEN];

   /*
   ** Get number of literals in symbol table
   */

   for (i = 0; i < symbolcount; i++)
   {
      if (symbols[i]->symbol[0] == LITERALSYM) literalcount++;
   }
   escapecount = literalcount * 2; /* just in case pc is out of wack */

   /*
   ** process found literals
   */

   while (literalcount)
   {
      if (--escapecount == 0) break;
      for (i = 0; i < symbolcount; i++)
      {
	 
	 /*
	 ** If this symbol is a literal for this pc, process it
	 */

	 if (symbols[i]->symbol[0] == LITERALSYM && symbols[i]->value == pc)
	 {
	    SymNode *s;
	    char sign;
	    char parsebuf[80];

	    sign = '+';
	    s = symbols[i];
	    strcpy (parsebuf,  &s->symbol[1]);
	    cp = parsebuf;

	    switch (*cp)
	    {

	    case 'H':		/* Hollerith */
	       cp++;
	       opbuf[0] = ' ';
	       for (j = 0; j < 6; j++)
	       {
		  sprintf (&opbuf[j*2+1], CHAR1FORMAT, tobcd[*cp]);
		  objbuf[0] = ABSDATA_TAG;
		  sprintf (&objbuf[j*2+1], CHAR1FORMAT, tobcd[*cp]);
		  if (*cp != '\0') cp++;
	       }
	       strcat (opbuf, "   ");
	       break;

	    case 'O':		/* Octal */
	       cp++;
	       if (*cp == '-')
	       {
	          sign = '-';
		  cp++;
	       }
#if defined(WIN32)
	       sscanf (cp, "%I64o", &dll);
#else
	       sscanf (cp, "%llo", &dll);
#endif
	       goto PUTINOBJ;

	    case 'V':		/* VFD */
	       cp++;
	       dll = 0;
	       bitcount = 0;
	       bitword = 0;
#ifdef DEBUGLVFD
               fprintf (stderr, "processliteral: VFD = %s\n", cp);
#endif
	       while (*cp)
	       {
		  t_uint64 vval;
		  t_uint64 mask;
		  int bits;
		  int chartype;
		  char ctype;

		  cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
		  
#ifdef DEBUGLVFD
		  fprintf (stderr, 
			"   token = %s, tokentype = %d, val = %o, term = %c\n",
			token, tokentype, val, term);
		  fprintf (stderr, "   bitcount = %d, bitword = %d\n",
			bitcount, bitword);
#endif
		  ctype = token[0];
		  vval = 0;
		  if (term == '/')
		  {
		     chartype=FALSE;
		     if (ctype == 'O')
		     {
			if (*(cp+1) == '/')
			   exprtype = BOOLEXPR | DATAVALUE;
			else
			   exprtype = ADDREXPR | DATAVALUE;
			bits = atoi (&token[1]);
			radix = 8;
		     }
		     else if (ctype == 'H')
		     {
			bits = atoi (&token[1]);
			chartype = TRUE;
			radix = 10;
		     }
		     else
		     {
			exprtype = ADDREXPR | DATAVALUE;
			bits = atoi (token);
			radix = 10;
		     }
		     cp++;
		     mask = 0;
		     for (i = 0; i < bits; i++) mask = (mask << 1) | 1;
#ifdef DEBUGLVFD
		     fprintf (stderr, "   mask = %llo, bits = %d\n",
			   mask, bits);
#endif
		     if (chartype)
		     {
			char *sp;
			int k;

			sp = cp;
			while (*cp && *cp != ',' && !isspace(*cp)) cp++;
			term = *cp;
			*cp++ = '\0';
			strcpy (lcltoken, sp);
			vval = 0;
			k = bits/6 - strlen(lcltoken);
			for (i = 0; i < k; i++)
			   vval = (vval << 6) | tobcd[' '];
			for (i = 0; i < strlen(lcltoken); i++)
			   vval = (vval << 6) | tobcd[lcltoken[i]];
#ifdef DEBUGLVFD
			fprintf (stderr, 
	     "   H%d: token = %s, tokentype = %d, vval = %llo, term = %c(%x)\n",
			      bits, lcltoken, tokentype, vval, term, term);
#endif
		     }
		     else
		     {
#ifdef DEBUGLVFD
                        fprintf (stderr, "   VFD LIT expr = %s\n", cp);
#endif
			cp = exprscan (cp, &val, &term, &junk, 1, FALSE, 0);
			vval = val;
#ifdef DEBUGLVFD
			fprintf (stderr,
			      "   %c%d: vval = %llo, term = %c(%x)\n",
			      radix == 8 ? 'O' : 'D', bits, vval, term, term);
#endif
		     }
		     radix = 10;
		     vval &= mask;
		     dll = dll << bits | vval;
		     bitcount += bits;
		     bitword += bits;
		  }
	       }
#ifdef DEBUGLVFD
               fprintf (stderr, "   bitcount = %d, bitword = %d\n",
		        bitcount, bitword);
#endif
	       if (36 - bitword)
	       {
	          dll <<= 36 - bitword;
	       }
	       if (dll & SIGNMASK)
	       {
	          sign = '-';
		  dll &= ~SIGNMASK;
	       }
	       goto PUTINOBJ;

	    default: 		/* Must be Decimal */
	       exprtype = DATAEXPR;
	       dll = 0;
	       cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
	       sign = '+';
	       if (tokentype == '-' || tokentype == '+')
	       {
		  sign = term;
		  cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
	       }

	       /*
	       ** If decimal integer, convert
	       */

	       if (tokentype == DECNUM)
	       {
#if defined(WIN32)
		  sscanf (token, "%I64d", &dll);
#else
		  sscanf (token, "%lld", &dll);
#endif
	       }

	       /*
	       ** Single presision Binary or Floating point
	       */

	       else if (tokentype == SNGLFNUM || tokentype == SNGLBNUM)
	       {
		  token[12] = '\0';
#if defined(WIN32)
		  sscanf (token, "%I64o", &dll);
#else
		  sscanf (token, "%llo", &dll);
#endif
	       }

	       /*
	       ** Double presision Binary or Floating point
	       */

	       else if (tokentype == DBLFNUM || tokentype == DBLBNUM)
	       {
		  int  sexp;
		  char p2[14];
		  char exp[4];

		  /* 201400000000000000000 */
		  strncpy (exp, token, 3);
		  exp[3] = '\0';
		  strcpy (p2, &token[12]);
		  token[12] = '\0';
#if defined(WIN32)
		  sscanf (token, "%I64o", &dll);
#else
		  sscanf (token, "%llo", &dll);
#endif

		  /*
		  ** Print and punch high order word now
		  */

		  exprtype = ADDREXPR;
		  sprintf (opbuf, OPLFORMAT, sign, dll);
		  sprintf (objbuf, OCTLFORMAT, ABSDATA_TAG,
			sign == '-' ? (dll | SIGNMASK) : dll);
		  if (listmode)
		  {
		     sprintf (pcbuf, PCFORMAT, pc);
		     printheader (lstfd);
		     fprintf (lstfd, LITFORMAT,
			      pcbuf, opbuf, s->symbol);
		     linecnt++;
		  }
		  punchrecord (outfd);
		  pc++;
		  
		  /*
		  ** Now format low order word
		  */

		  if (tokentype == DBLFNUM)
		  {
		     char lcltoken[TOKENLEN];

		     sscanf (exp, "%o", &sexp);
		     if (sexp < 27) sexp = 27;
		     sprintf (lcltoken, "%3.3o%s", sexp - 27, p2);
		  }
		  else
		  {
		     sprintf (lcltoken, "%s000", p2);
		  }
#if defined(WIN32)
		  sscanf (lcltoken, "%I64o", &dll);
#else
		  sscanf (lcltoken, "%llo", &dll);
#endif
	       }

	       /*
	       ** None of the above types, error
	       */

	       else
	       {
		  logerror("Invalid literal type");
	       }

PUTINOBJ:
	       exprtype = ADDREXPR;
	       sprintf (opbuf, OPLFORMAT, sign, dll);
	       sprintf (objbuf, OCTLFORMAT, ABSDATA_TAG,
			sign == '-' ? (dll | SIGNMASK) : dll);
	    }

	    if (listmode)
	    {
	       sprintf (pcbuf, PCFORMAT, pc);
	       printheader (lstfd);
	       if (tokentype == DBLFNUM || tokentype == DBLBNUM)
		  fprintf (lstfd, L2FORMAT,
			   pcbuf, opbuf);
	       else
		  fprintf (lstfd, LITFORMAT,
			   pcbuf, opbuf, s->symbol);
	       linecnt++;
	       printerrors (lstfd, listmode);
	    }
	    punchrecord (outfd);
	    symdelete (s);

	    pc++;
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
p2aop (OpCode *op, int flag, char *bp)
{
   int decr = 0;
   int tag = 0;
   int addr = 0;
   int addrrelo = FALSE;
   int decrrelo = FALSE;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   if (!nooperands)
   {
      bp = exprscan (bp, &addr, &term, &addrrelo, 1, FALSE, 0);
      if (term == ',')
      {
	 bp = exprscan (bp, &tag, &term, &junk, 1, FALSE, 0);
	 if (term == ',')
	 {
	    bp = exprscan (bp, &decr, &term, &decrrelo, 1, FALSE, 0);
	 }
      }
   }

   if (op->opmod) tag |= op->opmod;
   if (flag) tag |= 04;
   if (bcore && (op->opflags & BCORE_BITS)) tag |= 1;

   if (addrrelo && decrrelo) term = RELBOTH_TAG;
   else if (addrrelo) term = RELADDR_TAG;
   else if (decrrelo) term = RELDECR_TAG;
   else term = ABSDATA_TAG;

   sprintf (opbuf, OPAFORMAT,
	    (op->opvalue & 04000) ? '-' : ' ', (op->opvalue & 03777) >> 9,
	    decr & 077777, tag & 07, addr & 077777);
   sprintf (objbuf, OBJAFORMAT, term,
	    op->opvalue >> 9, decr & 077777, tag & 07, addr & 077777);
   pc++;
}

/***********************************************************************
* p2bop - Process B type operation code.
***********************************************************************/

static void
p2bop (OpCode *op, int flag, char *bp)
{
   int tag = 0;
   int addr = 0;
   int relocatable;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);

   bp = exprscan (bp, &addr, &term, &relocatable, 1, FALSE, 0);
   if (term == ',')
   {
      bp = exprscan (bp, &tag, &term, &junk, 1, FALSE, 0);
      if (term == ',')
      {
	 int mod;
	 bp = exprscan (bp, &mod, &term, &junk, 1, FALSE, 0);
	 flag |= mod;
      }
   }

   sprintf (opbuf, OPFORMAT,
	    (op->opvalue & 04000) ? '-' : ' ', op->opvalue & 03777,
	    flag & 077, tag & 07, addr & 077777);
   sprintf (objbuf, OBJFORMAT,
	    relocatable ? RELADDR_TAG : ABSDATA_TAG,
	    op->opvalue, flag & 077, tag & 07, addr & 077777);
   pc++;
}

/***********************************************************************
* p2cop - Process C type operation code.
***********************************************************************/

static void
p2cop (OpCode *op, int flag, char *bp)
{
   int count = 0;
   int tag = 0;
   int addr = 0;
   int relocatable;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   bp = exprscan (bp, &addr, &term, &relocatable, 1, FALSE, 0);
   if (term == ',')
   {
      bp = exprscan (bp, &tag, &term, &junk, 1, FALSE, 0);
      if (term == ',')
      {
	 bp = exprscan (bp, &count, &term, &junk, 1, FALSE, 0);
	 if (count > 255)
	 {
	    sprintf (errtmp, "Invalid count: %d", count);
	    logerror (errtmp);
	 }
      }
   }

   sprintf (opbuf, OPFORMAT,
	    (op->opvalue & 04000) ? '-' : ' ',
	    (op->opvalue & 03777) | ((count & 0300) >> 6),
	    count & 077, tag & 07, addr & 077777);
   sprintf (objbuf, OBJFORMAT,
	    relocatable ? RELADDR_TAG : ABSDATA_TAG,
	    op->opvalue | ((count & 0300) >> 6),
	    count & 077, tag & 07, addr & 077777);
   pc++;
}

/***********************************************************************
* p2dop - Process D type operation code.
***********************************************************************/

static void
p2dop (OpCode *op, int flag, char *bp)
{
   int addr = 0;
   int relocatable;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   radix = 8;
   exprtype = BOOLEXPR | BOOLVALUE ;
   bp = exprscan (bp, &addr, &term, &relocatable, 1, FALSE, 0);
   radix = 10;

   sprintf (opbuf, OPDFORMAT,
	    (op->opvalue & 04000) ? '-' : ' ',
	    op->opvalue & 03777,
	    flag & 077, addr & 0777777);
   sprintf (objbuf, OBJDFORMAT,
	    ABSDATA_TAG,
	    op->opvalue, flag & 077, addr & 0777777);
   pc++;
}

/***********************************************************************
* p2eop - Process E type operation code. 
***********************************************************************/

static void
p2eop (OpCode *op, int flag, char *bp)
{
   int mod = 0;
   int tag = 0;
   int relocatable;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   bp = exprscan (bp, &mod, &term, &relocatable, 1, FALSE, 0);
   if (term == ',')
   {
      bp = exprscan (bp, &tag, &term, &junk, 1, FALSE, 0);
   }

   sprintf (opbuf, OPFORMAT,
	    (op->opvalue & 04000) ? '-' : ' ', op->opvalue & 03777,
	    0, tag & 07, op->opmod | mod);
   sprintf (objbuf, OBJFORMAT,
	    relocatable ? RELADDR_TAG : ABSDATA_TAG,
	    op->opvalue, 0, tag & 07, op->opmod | mod);
   pc++;
}

/***********************************************************************
* p2chanop - Process channel type operation code.
***********************************************************************/

static void
p2chanop (OpCode *op, int flag, char *bp)
{
   int decr = 0;
   int tag = 0;
   int addr = 0;
   int count = 0;
   int addrrelo = FALSE;
   int decrrelo = FALSE;
   int junk;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   bp = exprscan (bp, &addr, &term, &addrrelo, 1, FALSE, 0);
   if (term == ',')
   {
      bp = exprscan (bp, &tag, &term, &junk, 1, FALSE, 0);
      if (term == ',')
      {
	 bp = exprscan (bp, &decr, &term, &decrrelo, 1, FALSE, 0);
	 if (term == ',')
	    bp = exprscan (bp, &count, &term, &junk, 1, FALSE, 0);
      }
   }

   if (op->opmod) tag |= op->opmod;
   if (flag) tag |= 04;
   if (bcore && (op->opflags & BCORE_BITS)) tag |= 1;

   if (addrrelo && decrrelo) term = RELBOTH_TAG;
   else if (addrrelo) term = RELADDR_TAG;
   else if (decrrelo) term = RELDECR_TAG;
   else term = ABSDATA_TAG;

   sprintf (opbuf, OPCHANFORMAT,
	    (op->opvalue & 04000) ? '-' : ' ',
	    ((op->opvalue & 03700) >> 6) | count,
	    decr & 007777, tag & 07, addr & 077777);
   sprintf (objbuf, OBJCHANFORMAT, term,
	    (op->opvalue >> 6) | count,
	    decr & 007777, tag & 07, addr & 077777);
   pc++;
}

/***********************************************************************
* p2diskop - Process DISK I/O type operation code.
***********************************************************************/

static void
p2diskop (OpCode *op, int flag, char *bp, FILE *lstfd, FILE *outfd)
{
   char *token;
   char *cp;
   int access = 0;
   int track = 0;
   int record = 0;
   int junk;
   int tokentype;
   char term;
   char cvtrec[4];

#ifdef DEBUGDISK
   fprintf (stderr, "p2diskop: op = %s, flag = %s\n",
	 op->opcode, flag ? "TRUE" : "FALSE");
#endif

   bp = tokscan (bp, &token, &tokentype, &junk, &term, TRUE);
#ifdef DEBUGDISK
   fprintf (stderr, "   token = %s, tokentype = %d, term = %c(%02x)\n",
	 token, tokentype, term, term);
#endif
   access = 01212;
   track = 012121212;
   cvtrec[0] = 0;
   cvtrec[1] = 0;

   if (tokentype == DECNUM)
   {
      if (strlen (token) > 2)
      {
	 sprintf (errtmp, "Invalid access/module field: %s", token);
	 logerror (errtmp);
      }
      else
      {
	 for (cp = token; *cp; cp++)
	    access = (access << 6) | (*cp == '0' ? 012 : tobcd[*cp]);
      }
   }
   if (term == ',')
   {
      bp = tokscan (bp, &token, &tokentype, &junk, &term, TRUE);
#ifdef DEBUGDISK
      fprintf (stderr, "   token = %s, tokentype = %d, term = %c(%02x)\n",
	    token, tokentype, term, term);
#endif
      if (tokentype == DECNUM)
      {
	 if (strlen (token) > 4)
	 {
	    sprintf (errtmp, "Invalid track/record field: %s", token);
	    logerror (errtmp);
	 }
	 else
	 {
	    for (cp = token; *cp; cp++)
	       track = (track << 6) | (*cp == '0' ? 012 : tobcd[*cp]);
	 }
      }
      if (term == ',')
      {
	 cvtrec[0] = *bp++;
	 if (isspace(*bp)) 
	    cvtrec[1] = ' ';
	 else
	    cvtrec[1] = *bp++; 
      }
   }

   if (cvtrec[0] == 0 || cvtrec[0] == '0')
      record = 012;
   else
      record = tobcd[cvtrec[0]];
   record <<= 6;
   if (cvtrec[1] == 0 || cvtrec[1] == '0')
      record |= 012;
   else
      record |= tobcd[cvtrec[1]];

   sprintf (pcbuf, PCFORMAT, pc);
   sprintf (opbuf, OPDISKFORMAT,
	 op->opvalue, access & 07777, (track >> 12) & 07777);
   sprintf (objbuf, OBJDISKFORMAT,
	 ABSDATA_TAG,
	 op->opvalue, access & 07777, (track >> 12) & 07777);
   printline (lstfd);
   punchrecord (outfd);
   pc++;
   sprintf (opbuf, OPDISKFORMAT,
	 track & 07777, record, op->opmod);
   sprintf (objbuf, OBJDISKFORMAT,
	 ABSDATA_TAG,
	 track & 07777, record, op->opmod);
   printdata (lstfd, pc);
   punchrecord (outfd);
   pc++;
   printed = TRUE;
}

/***********************************************************************
* p2lookup - lookup cursym in current context.
***********************************************************************/

static SymNode *
p2lookup (void)
{
   SymNode *s;
   char temp[32];

   s = NULL;
   if (fapmode)
   {
      if (headcount && (strlen(cursym) < MAXSYMLEN))
      {
	 sprintf (temp, "%c", headtable[0]);
	 s = symlookup (cursym, temp, FALSE, FALSE);
      }
      if (!s)
      {
	 s = symlookup (cursym, "", FALSE, FALSE);
      }
   }
   else
   {
      s = symlookup (cursym, qualtable[qualindex], FALSE, FALSE);
   }
   return (s);
}

/***********************************************************************
* wordchar - Prepare a word of characters for output.
***********************************************************************/

static char * 
wordchar (char *bp, int bit12mode, int chcnt, int fill)
{
   int j;
   int ndx;
   uint8 ch;

   if (bit12mode) ndx = 3;
   else           ndx = 6;

   opbuf[0] = ' ';
   for (j = 0; j < ndx; j++)
   {
      if (bit12mode)
      {

	 if (chcnt > 0)
	 {
	    if (*bp == '*')
	    {
	       bp++;
	       ch = bit12[tobcd[*bp]] ^ 0100;
	    }
	    else
	    {
	       ch = bit12[tobcd[*bp]];
	    }
	    chcnt--;
	 }
	 else
	    ch = 0057;
	 sprintf (&opbuf[j*4+1], BIT12FORMAT, ch);
	 objbuf[0] = ABSDATA_TAG;
	 sprintf (&objbuf[j*4+1], BIT12FORMAT, ch);
      }
      else
      {
	 if (fill)
	 {
	    if (chcnt > 0)
	    {
	       ch = tobcd[*bp];
	       chcnt--;
	    }
	    else
	       ch = 0057;
	 }
	 else
	 {
	    ch = tobcd[*bp];
	 }
	 sprintf (&opbuf[j*2+1], CHAR1FORMAT, ch);
	 objbuf[0] = ABSDATA_TAG;
	 sprintf (&objbuf[j*2+1], CHAR1FORMAT, ch);
      }
      if (*bp != '\n') bp++;
   }
   strcat (opbuf, "   ");
   return (bp);
}

/***********************************************************************
* p2pop - Process Pseudo operation code.
***********************************************************************/

static int
p2pop (OpCode *op, char *bp, FILE *lstfd, FILE *outfd)
{
   t_uint64 mask;
   t_uint64 vval;
   t_uint64 rval;
   SymNode *s;
   OpCode *addop;
   char *cp;
   char *token;
   int tmpnum0;
   int tmpnum1;
   int i, j;
   int etcndx;
   int spc;
   int val;
   int relocatable;
   int bitcount;
   int bitword;
   int tokentype;
   int bit12mode;
   int combssflag;
   int chcnt;
   char sign;
   char term;
   char emittag;
   char lcltoken[TOKENLEN];

   strcpy (pcbuf, PCBLANKS);
   strcpy (opbuf, OPBLANKS);
   bit12mode = FALSE;
   combssflag = FALSE;

   switch (op->opvalue)
   {

   case ACORE_T:		/* ACORE */
      bcore = FALSE;
      break;

   case BCORE_T:		/* BCORE */
      bcore = TRUE;
      break;

   case BCD_T:			/* BCD */
      sprintf (pcbuf, PCFORMAT, pc);
      cp = bp;
      j = FALSE;
      while (*bp && isspace (*bp)) bp++;
      if (isdigit (*bp))
      {
         tmpnum0 = *bp++ - '0';
	 if (tmpnum0 == 0) {
	     bp--;
	     tmpnum0 = 10;
	 }
	 cp = bp;
         goto PACKCHARS;
      }
      else 
      {
	 tmpnum0 = 10;
	 bp = &inbuf[VARSTART-3];
         goto PACKCHARS;
      }
      sprintf (errtmp, "Invalid BCD size: %s", cp);
      logerror (errtmp);
      break;

   case BIT12_T:		/* 12BIT */
      bit12mode = TRUE;

   case BCI_T:			/* BCI */
      sprintf (pcbuf, PCFORMAT, pc);
      while (*bp && isspace (*bp)) bp++;
      val = 0;
      chcnt = 0;
      j = FALSE;
      if (bit12mode) goto DELIM_ONLY;

      if (*bp == ',')
      {
	 bp++;
	 val = 10;
      }
      else
      {
	 if (!isdigit (*bp))
	 {
	    char cterm;

   DELIM_ONLY:
	    j = TRUE;
	    cterm = *bp++;

	    cp = bp;
	    while (*cp && *cp != cterm)
	    {
	       if (bit12mode && *cp == '*') ;
	       else chcnt++;
	       cp++;
	    }
	    *cp = '\n';
	    if (bit12mode)
	       val = (chcnt / 3) + ((chcnt % 3) ? 1 : 0);
	    else
	       val = (chcnt / 6) + ((chcnt % 6) ? 1 : 0);
	 }
	 else
	    bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      }
      tmpnum0 = val;
      if (tmpnum0 < 0 || tmpnum0 > 10)
      {
	 sprintf (errtmp, "Invalid BCI size: %d", val);
	 logerror (errtmp);
	 break;
      }
PACKCHARS:
      if (tmpnum0 == 1)
      {
	 bp = wordchar (bp, bit12mode, chcnt, j);
      }
      else
      {
	 printed = TRUE;
	 for (i = 0; i < tmpnum0; i++)
	 {
	    bp = wordchar (bp, bit12mode, chcnt, j);
	    chcnt -= bit12mode ? 3 : 6;

	    if (listmode)
	    {
	       if (i == 0)
		  printline (lstfd);
	       else if (detailmode)
		  printdata (lstfd, pc+i);
	    }
	    punchrecord (outfd);
	 }
      }
      pc += tmpnum0;
      break;

   case BEGIN_T:		/* BEGIN */
      if (begincount < MAXBEGINS)
      {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGUSE
	 fprintf (stderr, "p2: USE: token = %s, curruse = %d, pc = %05o\n",
		  token, curruse, pc);
#endif
	 if (term != ',' || tokentype == EOS)
	 {
	    logerror ("Invalid BEGIN statement");
	 }
	 else
	 {
	    for (i = 0; i < begincount; i++)
	    {
	       if (!strcmp (token, begintable[i].symbol))
	       {
		  val = begintable[i].bvalue;
		  break;
	       }
	    }
	 }
	 sprintf (pcbuf, PCFORMAT, val);
      }
      else
      {
	 logerror ("Too many BEGIN sections");
      }
      break;

   case BES_T:			/* BES */
      bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      if (val < 0 || val > 32767)
      {
	 sprintf (errtmp, "Invalid BES size: %d", val);
	 logerror (errtmp);
	 break;
      }
      pc += val;
      pc &= 077777;
      sprintf (pcbuf, PCFORMAT, pc);
      if (val)
      {
	 if (inloc)
	    sprintf (objbuf, WORDFORMAT, BSS_TAG, val);
	 else
	    sprintf (objbuf, WORDFORMAT, absolute ? ABSORG_TAG : RELORG_TAG,
	    	     pc);
      }
      break;

   case BFT_T:			/* BFT */
      radix = 8;
      exprtype = BOOLEXPR | BOOLVALUE ;
      cp = bp;
      cp = exprscan (cp, &val, &term, &relocatable, 1, FALSE, 0);
      radix = 10;
      if (rboolexpr && !lboolexpr)
	 p2dop (oplookup ("RFT"), 0, bp);
      else 
	 p2dop (oplookup ("LFT"), 0, bp);
      break;

   case BNT_T:			/* BNT */
      radix = 8;
      exprtype = BOOLEXPR | BOOLVALUE ;
      cp = bp;
      cp = exprscan (cp, &val, &term, &relocatable, 1, FALSE, 0);
      radix = 10;
      if (rboolexpr && !lboolexpr)
	 p2dop (oplookup ("RNT"), 0, bp);
      else 
	 p2dop (oplookup ("LNT"), 0, bp);
      break;
      
   case BOOL_T:			/* BOOL */
   case LBOOL_T:		/* LBOOL */
   case RBOOL_T:		/* RBOOL */
      if (cursym[0])
      {
	 if ((s = p2lookup()) != NULL)
	 {
	    sprintf (opbuf, BOOLFORMAT, s->value);
	    if (s->flags & RELOCATABLE)
	       logerror ("BOOL expressions must be absolute");
	 }
	 else
	    sprintf (opbuf, BOOLFORMAT, 0);
      }
      else
	 logerror ("BOOL requires a label");
      break;

   case BSS_T:			/* BSS */
      sprintf (pcbuf, PCFORMAT, pc);
      bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      if (val < 0 || val > 32767)
      {
	 sprintf (errtmp, "Invalid BSS size: %d", val);
	 logerror (errtmp);
	 break;
      }
      pc += val;
      pc &= 077777;
      if (val)
      {
	 if (inloc)
	    sprintf (objbuf, WORDFORMAT, BSS_TAG, val);
	 else
	    sprintf (objbuf, WORDFORMAT, absolute ? ABSORG_TAG : RELORG_TAG,
	    pc);
      }
      break;

   case COMBSS_T:		/* COMMON BSS */
      combssflag = TRUE;

   case COMBES_T:		/* COMMON BES */
   case COMMON_T:		/* COMMON storage */
      if (fapmode)
      {
	 commonused = TRUE;
	 bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	 if (val < 0)
	    commonctr += -val;
	 else
	    commonctr -= val;
	 commonctr &= 077777;
	 if (cursym[0])
	 {
	    if ((s = p2lookup()) != NULL)
	    {
	       sprintf (opbuf, ADDRFORMAT, s->value);
	    }
	 }
	 else
	    sprintf (opbuf, ADDRFORMAT, commonctr);
      }
      else
      {
	 i = begincount - 1;
	 if (!begintable[i].value)
	    begintable[i].value = begintable[i].bvalue;
	 val = begintable[i].value;
#ifdef DEBUGUSE
         fprintf (stderr, "asmpass2: COMMON: common = %d, val = %05o\n",
	       i, val);
#endif
	 sprintf (pcbuf, PCFORMAT, val);
	 bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	 sprintf (opbuf, OP2FORMAT, ' ', 0200000, val);
	 begintable[i].value += val;
#ifdef DEBUGUSE
         fprintf (stderr, "   new val = %05o\n", begintable[i].value);
#endif
      }
      break;

   case DEC_T:			/* DECimal */
      sprintf (pcbuf, PCFORMAT, pc);
      j = 0;
      spc = 0;
      rval = 0;
      exprtype = DATAEXPR;
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 spc++;
	 sign = '+';
	 if (tokentype == '-' || tokentype == '+')
	 {
	    sign = term;
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 }

	 if (token[0] == '\0')
	 {
	    rval = 0;
	 }

	 /*
	 ** If decimal number, convert
	 */

	 else if (tokentype == DECNUM)
	 {
#if defined(WIN32)
	    sscanf (token, "%I64d", &rval);
#else
	    sscanf (token, "%lld", &rval);
#endif
	 }

	 /*
	 ** Single precision floating or binary point
	 */

	 else if (tokentype == SNGLFNUM || tokentype == SNGLBNUM)
	 {
	    token[12] = '\0';
#if defined(WIN32)
	    sscanf (token, "%I64o", &rval);
#else
	    sscanf (token, "%llo", &rval);
#endif
#ifdef DEBUG_BINPOINT
	    fprintf (stderr, "DEC_T: toktype = %d, token = '%s', rval = %llo\n",
	       tokentype, token, rval);
#endif
	    
	 }

	 /*
	 ** Double precision floating or binary point
	 */

	 else if (tokentype == DBLFNUM || tokentype == DBLBNUM)
	 {
	    int sexp;
	    char p2[14];
	    char exp[4];

	    /* 201 400000000 000000000 */
	    strncpy (exp, token, 3);
	    exp[3] = '\0';
	    strcpy (p2, &token[12]);
	    token[12] = '\0';
#ifdef DEBUG_FLOAT
            fprintf (stderr, "   exp = %s, p2 = %s\n",
		     exp, p2);
            fprintf (stderr, "   token-1 = %s\n", token);
#endif
#if defined(WIN32)
	    sscanf (token, "%I64o", &rval);
#else
	    sscanf (token, "%llo", &rval);
#endif
	    exprtype = ADDREXPR;
	    sprintf (opbuf, OPLFORMAT, sign, rval);
	    sprintf (objbuf, OCTLFORMAT, ABSDATA_TAG,
		     sign == '-' ? (rval | SIGNMASK) : rval);
	    if (listmode)
	    {
	       if (j == 0)
		  printline (lstfd);
	       else if (detailmode)
		  printdata (lstfd, pc+j);
	    }
	    punchrecord (outfd);
	    j++;
	    spc++;
	    if (tokentype == DBLFNUM)
	    {
	       sscanf (exp, "%o", &sexp);
	       if (sexp < 27) sexp = 27;
	       sprintf (lcltoken, "%3.3o%s", sexp - 27, p2);
	    }
	    else
	    {
	       sprintf (lcltoken, "%s000", p2);
	    }
#ifdef DEBUG_FLOAT
            fprintf (stderr, "   token-2 = %s\n", lcltoken);
#endif
#if defined(WIN32)
	    sscanf (lcltoken, "%I64o", &rval);
#else
	    sscanf (lcltoken, "%llo", &rval);
#endif
	 }

	 sprintf (opbuf, OPLFORMAT, sign, rval);
	 sprintf (objbuf, OCTLFORMAT, ABSDATA_TAG,
		  sign == '-' ? (rval | SIGNMASK) : rval);
	 printed = TRUE;
	 if (listmode)
	 {
	    if (j == 0)
	       printline (lstfd);
	    else if (detailmode)
	       printdata (lstfd, pc+j);
	 }
	 punchrecord (outfd);
	 j++;
      } while (term == ',');
      exprtype = ADDREXPR;
      pc += spc;
      break;

   case DETAIL_T:		/* DETAIL in listing */
      detailmode = TRUE;
      if (!pccmode)
	 printed = TRUE;
      break;

   case DUP_T:			/* DUPlicate */
      /*
      ** Scan off DUP input line count
      */

      bp = exprscan (bp, &dupin, &term, &relocatable, 1, FALSE, 0);
      break;

   case END_T:			/* END of assembly */
      {
	 cp = bp;
	 cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
	 if (tokentype != EOS)
	 {
	    bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	    sprintf (opbuf, ADDRFORMAT, val);
	    sprintf (objbuf, WORDFORMAT,
		     (!absolute && relocatable) ? RELENTRY_TAG : ABSENTRY_TAG,
		     val);
	 }
      }
      return (TRUE);
      break;

   case ENDQ_T:			/* END of Qualified section */
      qualindex --;
      if (qualindex < 0) qualindex = 0;
      break;

   case ENT_T:			/* ENTRY */
      spc = FALSE;
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (fapmode && term == '-')
      {
	 spc = TRUE;
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      }
      if (tokentype != SYM)
      {
	 if (!(fapmode && tokentype == DECNUM && val == 0))
	 {
	    sprintf (errtmp, "ENTRY requires symbol: %s", token);
	    logerror (errtmp);
	 }
      }
      else if (strlen (token) > MAXSYMLEN)
      {
	 sprintf (errtmp, "Symbol too long: %s", token);
	 logerror (errtmp);
      }
      else
      {
	 SymNode *es;

	 if (!(s = symlookup (token, fapmode ? "" : qualtable[qualindex],
		     FALSE, TRUE)))
	 {
	    sprintf (errtmp, "ENTRY undefined: %s", token);
	    logerror (errtmp);
	 }
	 else
	 {
	    if (fapmode)
	    {
	       s->flags |= GLOBAL;
	       if (spc) s->flags |= SGLOBAL;
	    }
	    else
	    {
	       if (cursym[0])
	       {
		  if ((es = entsymlookup (cursym, qualtable[qualindex],
					  TRUE, TRUE)) != NULL)
		  {
		     es->flags |= GLOBAL;
		     es->value = s->value;
		  }
	       }
	       else
	       {
		  if ((es = entsymlookup (token, qualtable[qualindex],
					  TRUE, TRUE)) != NULL)
		  {
		     es->flags |= GLOBAL;
		     es->value = s->value;
		  }
	       }
	    }
	 }
      }
      break;

   case EJE_T:			/* EJECT */
      if (listmode)
      {
	 if (linecnt)
	 {
	    linecnt = MAXLINE;
	    printheader (lstfd);
	 }
	 if (!pccmode)
	    printed = TRUE;
      }
      break;

   case EQU_T:			/* EQU */
      if (cursym[0] == '\0')
      {
	 if (fapmode)
	 {
	    bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	    sprintf (opbuf, ADDRFORMAT, val);
	 }
	 else
	 {
	    logerror ("EQU requires a label");
	 }
      }
      else
      {
	 if ((s = p2lookup()) != NULL)
	 {
	    sprintf (opbuf, ADDRFORMAT, s->value);
	    if (s->flags & NOEXPORT)
	       s->vsym->value = s->value;
	 }
         else 
	    sprintf (opbuf, ADDRFORMAT, 0);
      }
      break;

   case EVEN_T:			/* EVEN */
      sprintf (pcbuf, PCFORMAT, pc);
      if (pc & 00001)
	 p2bop (oplookup ("AXT"), 0, "0,0\n");
      break;

   case GOTO_T:			/* GOTO */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (strlen(token) > MAXSYMLEN) token[MAXSYMLEN] = '\0';
      gotoblank = FALSE;
      strcpy (gotosym, token);
      if (term == ',')
      {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 if (!strcmp (token, "BLANK"))
	    gotoblank = TRUE;
      }
#ifdef DEBUGGOTO
      fprintf (stderr, "asmpass2: GOTO: gotosym = '%s', gotoblank = %s\n",
	    gotosym, gotoblank ? "TRUE" : "FALSE");
#endif
      gotoskip = TRUE;
      if (!pccmode)
	 printed = TRUE;
      break;

   case HEAD_T:			/* HEAD */
      headcount = 0;
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 headtable[headcount++] = token[0];
	 if (headcount >= MAXHEADS)
	 {
	    logerror ("Too many HEAD symbols");
	    break;
	 }
      } while (term == ',');
      if (headtable[0] == '0') headcount = 0;
      break;

   case HED_T:			/* HED */
      headcount = 0;
      if (cursym[0])
      {
	 headtable[headcount++] = cursym[0];
	 do {
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	    if (token[0])
	    {
	       headtable[headcount++] = token[0];
	       if (headcount >= MAXHEADS)
	       {
		  break;
	       }
	    }
	 } while (term == ',');
	 if (headtable[0] == '0') headcount = 0;
      }
      break;

   case IFF_T:			/* IF False */
      if (fapmode)
      {
	 char tok1[TOKENLEN];
	 char tok2[TOKENLEN];

	 tok1[0] = '\0';
	 tok2[0] = '\0';
	 asmskip = 0;
	 bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	 i = 0;
	 if (term == ',')
	 {
	    int paren = FALSE;

	    if (*bp == '(')
	    {
	       paren = TRUE;
	       bp++;
	    }
	    token = bp;
	    while (*bp && *bp != ',')
	    {
	       if (paren) bp++;
	       else if (!isspace(*bp)) bp++;
	       else break;
	    }
	    if (paren) *(bp-1) = '\0';
	    term = *bp;
	    *bp++ = '\0';
	    strcpy (tok1, token);
	    if (term == ',')
	    {
	       paren = FALSE;
	       if (*bp == '(')
	       {
		  paren = TRUE;
		  bp++;
	       }
	       token = bp;
	       while (*bp)
	       {
	          if (paren)
		  {
		     if (*bp == ')') break;
		  }
		  else
		  {
		     if (*bp == ',' || isspace (*bp)) break;
		  }
		  bp++;
	       }
	       term = *bp;
	       *bp++ = '\0';
	       strcpy (tok2, token);
	    }
	    i = strcmp (tok1, tok2);
	 }
	 if (val)
	 {
	    if (i) asmskip = 1;
	 }
	 else
	 {
	    if (!i) asmskip = 1;
	 }
	 if (!pccmode)
	    printed = TRUE;
#ifdef DEBUGP2IFF
	 fprintf (stderr,
	    "p2IFF: asmskip = %d, val = %d, i = %d, tok1 = '%s', tok2 = '%s'\n",
		  asmskip, val, i, tok1, tok2);
#endif
      }
      else
      {
	 /*
	 ** Scan the conditional expression and get result
	 */

	 bp = condexpr (bp, &val, &term);
	 if (val >= 0)
	 {
	    int skip;

#ifdef DEBUGIF
	    fprintf (stderr, "p2IFF: val = %d, ifcont = %s\n",
		  val, ifcont ? "TRUE" : "FALSE");
#endif
	    skip = val;
	    asmskip = 0;

	    /*
	    ** If continued, use last result here
	    */

	    if (ifcont)
	    {
#ifdef DEBUGIF
	       fprintf (stderr, "   ifrel = %s, ifskip = %s, skip = %s",
		  ifrel == IFOR ? "OR" : "AND",
		  ifskip ? "TRUE" : "FALSE",
		  skip ? "TRUE" : "FALSE");
#endif
	       if (term == ',')
	       {
		  if (ifrel == IFAND) skip = ifskip || skip;
		  else                skip = ifskip && skip;
		  goto NEXT_IFF_RELATION;
	       }
	       ifcont = FALSE;
	       if (ifrel == IFAND)
	       {
		  if (ifskip || skip)
		     asmskip = 2;
	       }
	       else if (ifrel == IFOR)
	       {
		  if (ifskip && skip)
		     asmskip = 2;
	       }
#ifdef DEBUGIF
	       fprintf (stderr, "   asmskip = %d\n", asmskip);
#endif
	    }

	    /*
	    ** If not continued, check for relation
	    */

	    else if (term == ',')
	    {
	    NEXT_IFF_RELATION:
	       ifcont = TRUE;
	       ifskip = skip;
	       if (!(strcmp (bp, "OR")))
		  ifrel = IFOR;
	       else if (!(strcmp (bp, "AND")))
		  ifrel = IFAND;
	       else
	       {
		  sprintf (errtmp, "Invalid relation: %s", bp);
		  logerror (errtmp);
	       }
#ifdef DEBUGIF
	       fprintf (stderr, "   ifskip = %s\n", ifskip ? "TRUE" : "FALSE");
#endif
	    }

	    /*
	    ** Neither, just do it
	    */

	    else if (skip)
	    {
	       asmskip = 2;
#ifdef DEBUGIF
	       fprintf (stderr, "   asmskip = %d\n", asmskip);
#endif
	    }
	 }
      }
      if (!pccmode)
	 printed = TRUE;
      break;

   case IFT_T:			/* IF True */
      /*
      ** Scan the conditional expression and get result
      */

      bp = condexpr (bp, &val, &term);
      if (val >= 0)
      {
	 int skip;

#ifdef DEBUGIF
	 fprintf (stderr, "p2IFT: val = %d, ifcont = %s\n",
	       val, ifcont ? "TRUE" : "FALSE");
#endif
	 skip = !val;
	 asmskip = 0;

	 /*
	 ** If continued, use last result here
	 */

	 if (ifcont)
	 {
#ifdef DEBUGIF
	       fprintf (stderr, "   ifrel = %s, ifskip = %s, skip = %s",
		  ifrel == IFOR ? "OR" : "AND",
		  ifskip ? "TRUE" : "FALSE",
		  skip ? "TRUE" : "FALSE");
#endif
	    if (term == ',')
	    {
	       if (ifrel == IFAND) skip = ifskip || skip;
	       else                skip = ifskip && skip;
	       goto NEXT_IFT_RELATION;
	    }
	    ifcont = FALSE;
	    if (ifrel == IFAND)
	    {
	       if (ifskip || skip)
		  asmskip = 2;
	    }
	    else if (ifrel == IFOR)
	    {
	       if (ifskip && skip)
		  asmskip = 2;
	    }
#ifdef DEBUGIF
	    fprintf (stderr, "   asmskip = %d\n", asmskip);
#endif
	 }

	 /*
	 ** If not continued, check for relation
	 */

	 else if (term == ',')
	 {
	 NEXT_IFT_RELATION:
	    ifcont = TRUE;
	    ifskip = skip;
	    if (!(strcmp (bp, "OR")))
	       ifrel = IFOR;
	    else if (!(strcmp (bp, "AND")))
	       ifrel = IFAND;
	    else
	    {
	       sprintf (errtmp, "Invalid relation: %s", bp);
	       logerror (errtmp);
	    }
#ifdef DEBUGIF
	    fprintf (stderr, "   ifskip = %s\n", ifskip ? "TRUE" : "FALSE");
#endif
	 }

	 /*
	 ** Neither, just do it
	 */

	 else if (skip)
	 {
	    asmskip = 2;
#ifdef DEBUGIF
	    fprintf (stderr, "   asmskip = %d\n", asmskip);
#endif
	 }
      }
      if (!pccmode)
	 printed = TRUE;
      break;

   case IIB_T:			/* IIB */
      radix = 8;
      exprtype = BOOLEXPR | BOOLVALUE ;
      cp = bp;
      cp = exprscan (cp, &val, &term, &relocatable, 1, FALSE, 0);
      radix = 10;
      if (rboolexpr && !lboolexpr)
	 p2dop (oplookup ("IIR"), 0, bp);
      else 
	 p2dop (oplookup ("IIL"), 0, bp);
      break;

   case INDEX_T:		/* INDEX */
#ifdef DEBUGINDEX
      fprintf (stderr, "asmpass2: INDEX: bp = %s", bp);
#endif
      if (pccmode)
      {
	 printline (lstfd);
      }
      if (!firstindex)
      {
	 firstindex = TRUE;
	 fprintf (lstfd, "\n%-40.40sTable of Contents\n", "");
	 linecnt += 2;
      }
      fprintf (lstfd, "\n");
      linecnt++;
      cp = bp;
      cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
      if (tokentype != EOS)
      {
	 do {
	    char sym[MAXSYMLEN+2];

	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGINDEX
	    fprintf (stderr, "   token = %s, tokentype = %d, term = %c\n", 
		     token, tokentype, term);
#endif
	    if (tokentype == SYM)
	    {
	       strcpy (sym, token);
	       s = symlookup (token, "", FALSE, FALSE);
	       if (s)
		  val = s->value;
	    }
	    else if (tokentype == PCSYMBOL)
	    {
	       strcpy (sym, "*  ");
	       val = pc;
	    }
	       
	    printheader (lstfd);
	    fprintf (lstfd, "%-40.40s%06o     %-8.8s\n", "", val, sym);
	    linecnt++;
	    if (term != ',')
	    {
	       fprintf (lstfd, "\n");
	       linecnt++;
	    }
	 } while (term == ',');
      }
      printed = TRUE;
      break;

   case LBL_T:			/* LBL */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (strlen(token) > MAXLBLLEN) token[MAXLBLLEN] = '\0';
      if (strcmp (lbl, token))
      {
	 objrecnum = 0;
	 strcpy (lbl, token);
	 strcpy (idtbuf, lbl);
	 idtlen = absmod ? 0 : pgmlength & 077777;
      }
      if (!pccmode)
	 printed = TRUE;
      break;

   case LIST_T:			/* LIST */
      listmode = gbllistmode;
      if (!pccmode)
	 printed = TRUE;
      break;

   case LIT_T:			/* LITeral to pool */
      while (*bp && isspace(*bp)) bp++;
      exprtype = DATAEXPR;
      do
      {
	 char eterm;
	 char litbuf[MAXFIELDSIZE];

	 cp = bp;
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 spc++;
	 sign = '+';
	 if (tokentype == '-' || tokentype == '+')
	 {
	    sign = term;
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 }
	 strcpy (litbuf, "=");
	 strncat (litbuf, cp, bp-cp);
	 if (litbuf[bp-cp] == ',') litbuf[bp-cp] = '\0';
	 litbuf[1+bp-cp] = '\0';
#ifdef DEBUGLIT
         fprintf (stderr, "LIT: litbuf = %s\n", litbuf);
#endif
	 cp = exprscan (litbuf, &val, &eterm, &relocatable, 1, FALSE, 0);

      } while (term == ',');
      exprtype = ADDREXPR;
      break;

   case LOC_T:			/* LOC */
      val = -1;
      inloc = TRUE;
      if (fapmode)
      {
         cp = tokscan (bp, &token, &tokentype, &j, &term, FALSE);
	 if (tokentype == EOS)
	 {
	    val = prevloc + (pc - locorg);
	    inloc = FALSE;
	 }
      }
      if (val < 0)
	 bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      if (val < 0 || val > 32767)
      {
	 sprintf (errtmp, "Invalid LOC value: %d", val);
	 logerror (errtmp);
	 break;
      }
      if (!absmod)
      {
	 logerror ("LOC must be in absolute assembly");
	 break;
      }
      absolute = TRUE;
      prevloc = pc;
      pc = val;
      locorg = val;
      sprintf (pcbuf, PCFORMAT, pc);
      break;

   case LORG_T:			/* Literal ORiGin */
      printline (lstfd);
      printed = TRUE;
      spc = pc;
      processliterals (outfd, lstfd, listmode);
#ifdef DEBUGLIT
      fprintf (stderr, "p2-LORG: old pc = %o, new pc = %o\n",
	       spc, pc);
#endif
      break;

   case MAX_T:			/* MAX */
      if (cursym[0] == '\0')
      {
	 logerror ("MAX requires a label");
      }
      else
      {
	 s = p2lookup();
	 if (s)
	    sprintf (opbuf, ADDRFORMAT, s->value);
      }
      break;

   case MIN_T:			/* MIN */
      if (cursym[0] == '\0')
      {
	 logerror ("MIN requires a label");
      }
      else
      {
	 s = p2lookup();
	 if (s)
	    sprintf (opbuf, ADDRFORMAT, s->value);
      }
      break;

   case LINK_T:
   case NOLNK_T:		/* NOLNK */
      if (!pccmode)
	 printed = TRUE;
      break;

   case NULL_T:			/* NULL */
      if (cursym[0])
      {
	 s = p2lookup();
	 if (s)
	    sprintf (opbuf, ADDRFORMAT, s->value);
      }
      break;

   case OCT_T:			/* OCTal */
      sprintf (pcbuf, PCFORMAT, pc);
      j = 0;
      spc = 0;
      radix = 8;
      exprtype = DATAEXPR;
      do {
	 cp = bp;
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 spc++;
	 sign = '+';
	 if (tokentype == '-' || tokentype == '+')
	 {
	    sign = term;
	    cp = bp;
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 }
#ifdef DEBUGOCT
         fprintf (stderr,
	       "%d: OCT: token = %s, tokentype = %d, val = %o, term = %c\n",
	       linenum, token, tokentype, val, term);
         fprintf (stderr, "   token[0] = %d\n", token[0]);
#endif
	 if (token[0] == '\0') /* Null argument */
	 {
	    tmpnum0 = 0;
	    tmpnum1 = 0;
	 }
	 else
	 {
	    if (strlen(token) > 12)
	    {
	       sprintf (errtmp, "Invalid octal number: %s", bp);
	       logerror (errtmp);
	       break;
	    }
	    if ((i = strlen (token)) > 6)
	    {
	       sscanf (&token[i-6], "%o", &tmpnum1);
	       token[i-6] = '\0';
	       sscanf (token, "%o", &tmpnum0);
	    }
	    else 
	    {
	       tmpnum0 = 0;
	       sscanf (token, "%o", &tmpnum1);
	    }
	 }
	 if (sign == '-') tmpnum0 |= 0400000;
	 sprintf (opbuf, OP2FORMAT,
		  tmpnum0 & 0400000 ? '-' : '+',
		  tmpnum0 & 0377777, tmpnum1);
	 sprintf (objbuf, OCT2FORMAT, ABSDATA_TAG, tmpnum0, tmpnum1);
	 printed = TRUE;
	 if (listmode)
	 {
	    if (j == 0)
	       printline (lstfd);
	    else if (detailmode)
	       printdata (lstfd, pc+j);
	 }
	 punchrecord (outfd);
	 j++;
      } while (term == ',');
      pc += spc;
      exprtype = ADDREXPR;
      radix = 10;
      break;

   case OPSYN_T:		/* OP SYNonym */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGOPSYN
      fprintf (stderr,
        "OPSYN: cursym = %s, token = %s, tokentype = %d, val = %o, term = %c\n",
	      cursym, token, tokentype, val, term);
#endif
      if ((addop = oplookup (token)) != NULL)
      {
	 opdel (cursym);
	 opadd (cursym, addop->opvalue, addop->opmod, addop->optype);
      }
      break;

   case ORG_T:			/* ORiGin */
      inloc = FALSE;
      bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      if (val < 0 || val > 32767)
      {
	 sprintf (errtmp, "Invalid ORG value: %d", val);
	 logerror (errtmp);
	 break;
      }
      if (absmod)
	 absolute = TRUE;
      else
	 absolute = FALSE;
      pc = val;
      sprintf (pcbuf, PCFORMAT, pc);
      sprintf (objbuf, WORDFORMAT, absolute ? ABSORG_TAG : RELORG_TAG, pc);
      orgout = TRUE;
      break;

   case PCC_T:			/* Print Control Cards */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (!strncmp (token, "ON", 2)) pccmode = TRUE;
      else if (!strncmp (token, "OFF", 3)) pccmode = FALSE;
      else pccmode = pccmode ? FALSE : TRUE;
      if (!pccmode)
	 printed = TRUE;
      break;

   case PMC_T:			/* Print Macro Cards */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (!strncmp (token, "ON", 2)) pmcmode = TRUE;
      else if (!strncmp (token, "OFF", 3)) pmcmode = FALSE;
      else pmcmode = pmcmode ? FALSE : TRUE;
      if (!pccmode)
	 printed = TRUE;
      break;

   case PUNCH_T:		/* PUNCH on */
      punchmode = TRUE;
      break;

   case QUAL_T:			/* QUALified section */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (strlen(token) > MAXSYMLEN) token[MAXSYMLEN] = '\0';
      qualindex ++;
      strcpy (qualtable[qualindex], token);
      break;

   case REF_T:			/* don't list REFerence table  */
      if (fapmode)
      {
	 noreflst = TRUE;
      }
      break;

   case REM_T:			/* REMark */
      cp = &lstbuf[OPCSTART];
      while (cp < (lstbuf + (bp-inbuf)))
         *cp++ = ' ';
      break;

   case RIB_T:			/* RIB */
      radix = 8;
      exprtype = BOOLEXPR | BOOLVALUE ;
      cp = bp;
      cp = exprscan (cp, &val, &term, &relocatable, 1, FALSE, 0);
      radix = 10;
      if (rboolexpr && !lboolexpr)
	 p2dop (oplookup ("RIR"), 0, bp);
      else 
	 p2dop (oplookup ("RIL"), 0, bp);
      break;

   case SET_T:			/* SET */
      bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      if (cursym[0])
      {
	 if (!(s = symlookup (cursym, "", FALSE, FALSE)))
	    s = symlookup (cursym, "", TRUE, FALSE);
	 if (s)
	 {
	    s->value = val;
	    s->flags &= ~RELOCATABLE;
	    sprintf (opbuf, ADDRFORMAT, s->value);
	 }
      }
      break;

   case SIB_T:			/* SIB */
      radix = 8;
      exprtype = BOOLEXPR | BOOLVALUE ;
      cp = bp;
      cp = exprscan (cp, &val, &term, &relocatable, 1, FALSE, 0);
      radix = 10;
      if (rboolexpr && !lboolexpr)
	 p2dop (oplookup ("SIR"), 0, bp);
      else 
	 p2dop (oplookup ("SIL"), 0, bp);
      break;

   case SPC_T:			/* SPACE */
      bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      if (val < 0 || val > LINESPAGE)
      {
	 sprintf (errtmp, "Invalid SPACE count: %d", val);
	 logerror (errtmp);
	 break;
      }
      if (listmode)
      {
	 if (val == 0) val = 1;
	 for (i = val; i > 0; i--)
	 {
	    printheader (lstfd);
	    fputs ("\n", lstfd);
	    linecnt++;
	 }
	 if (pccmode)
	    printline (lstfd);
	 printed = TRUE;
      }
      break;

   case TAPENO_T:		/* TAPE Number */
      if ((s = p2lookup()) != NULL)
	 sprintf (opbuf, ADDRFORMAT, s->value);
      else 
	 sprintf (opbuf, ADDRFORMAT, 0);
      break;

   case TCD_T:			/* Transfer Control Directive */
      if (inbuf[VARSTART])
      {
	 bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	 sprintf (opbuf, ADDRFORMAT, val);
	 sprintf (objbuf, WORDFORMAT,
		  (!absolute && relocatable) ? RELXFER_TAG : ABSXFER_TAG,
		  val);
	 punchrecord(outfd);
	 puncheof (outfd);
	 tcdemitted = TRUE;
      }
      else
      {
	 logerror ("TCD requires operand");
      }
      break;

   case TITLE_T:		/* no DETAIL in listing */
      detailmode = FALSE;
      if (!pccmode)
	 printed = TRUE;
      break;

   case TTL_T:			/* sub TiTLe */
      while (*bp && isspace (*bp)) bp++;
      cp = bp;
      while (*bp != '\n')
      {
	 if (bp - inbuf > RIGHTMARGIN) break;
	 if (bp - cp == TTLSIZE) break;
	 bp++;
      }
      *bp = '\0';
      strcpy (ttlbuf, cp);
      if (linecnt)
      {
	 linecnt = MAXLINE;
	 printheader (lstfd);
      }
      if (!pccmode)
	 printed = TRUE;
      break;

   case UNLIST_T:		/* UNLIST */
      listmode = FALSE;
      break;

   case UNPNCH_T:		/* UNPNCH */
      punchfinish (outfd);
      punchmode = FALSE;
      break;

   case USE_T:			/* USE */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (tokentype != EOS)
      {
#ifdef DEBUGUSE
	 fprintf (stderr, 
	    "p1: USE: token = %s, curruse = %d, prevuse = %d, pc = %05o\n",
		  token, curruse, prevuse, pc);
#endif
	 begintable[curruse].value = pc;
	 if (!strcmp (token, "PREVIOUS"))
	 {
	    curruse = prevuse;
	    prevuse = curruse - 1;
	 }
	 else
	 {
	    prevuse = curruse;
	    for (i = 0; i < begincount; i++)
	    {
	       if (!strcmp (token, begintable[i].symbol))
	       {
		  if (!begintable[i].value)
		  {
		     if (begintable[i].bvalue)
			begintable[i].value = begintable[i].bvalue;
		     else
			begintable[i].value = begintable[i-1].value;
		  }
		  curruse = i;
		  break;
	       }
	    }
	 }
	 pc = begintable[curruse].value;
#ifdef DEBUGUSE
	 fprintf (stderr, "   new curruse = %d, prevuse = %d, pc = %05o\n",
		  curruse, prevuse, pc);
#endif
	 sprintf (pcbuf, PCFORMAT, pc);
	 sprintf (objbuf, WORDFORMAT,
		  absolute ? ABSORG_TAG : RELORG_TAG, pc);
      }
      break;

   case VFD_T:			/* Variable Field Definition */
      sprintf (pcbuf, PCFORMAT, pc);
      tmpnum0 = 0;
      tmpnum1 = 0;
      bitcount = 0;
      bitword = 0;
      etcndx = 0;
      j = 0;
      rval = 0;
      spc = pc;
      emittag = ABSDATA_TAG;

      /*
      ** Skip leading blanks
      */

      i = FALSE;
      while (*bp && isspace(*bp))
      {
	 i = TRUE;
	 bp++;
      }

      /*
      ** If no operands, set zero value;
      */

      if (i && (bp - inbuf >= NOOPERAND))
      {
#ifdef DEBUGP2VFD
	 fprintf (stderr, "VFD-p2: NO Operand\n");
#endif
         bitcount = 36;
	 bitword = 1;
      }

      /*
      ** Operands present, process.
      */

      else
      {

	 cp = bp;
	 while (*bp && !isspace(*cp)) cp++;
	 *cp++ = '\n';
	 *cp++ = '\0';
	 rval = 0;
#ifdef DEBUGP2VFD
	 fprintf (stderr, "VFD-p2: bp = %s\n", bp);
	 fprintf (stderr, "   etccount = %d\n", etccount);
#endif

	 while (*bp)
	 {
	    int bits;
	    int resid;
	    int chartype;
	    int octtype;
	    char ctype;

	    /*
	    ** Scan off type/length field
	    */

	    exprtype = ADDREXPR;
	    relocatable = FALSE;
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	    
#ifdef DEBUGP2VFD
	    fprintf (stderr,
		  "   token = %s, tokentype = %d, vval = %llo, term = %c\n",
		  token, tokentype, vval, term);
	    fprintf (stderr, "   bitcount = %d, bitword = %d\n",
		  bitcount, bitword);
#endif
	    if (tokentype == EOS) break;

	    ctype = token[0];
	    if (term == '/')
	    {
	       octtype = FALSE;
	       chartype = FALSE;
	       if (ctype == 'O')
	       {
		  octtype = TRUE;
		  if (*(bp+1) == '/')
		     exprtype = BOOLEXPR | DATAVALUE;
		  else
		     exprtype = ADDREXPR | DATAVALUE;
		  bits = atoi (&token[1]);
		  radix = 8;
	       }
	       else if (ctype == 'H')
	       {
		  bits = atoi (&token[1]);
		  chartype = TRUE;
		  radix = 10;
	       }
	       else
	       {
		  exprtype = ADDREXPR | DATAVALUE;
		  bits = atoi (token);
		  radix = 10;
	       }

	       /*
	       ** Generate value mask
	       */

	       bp++;
	       mask = 0;
	       for (i = 0; i < bits; i++) mask = (mask << 1) | 1;
#ifdef DEBUGP2VFD
	       fprintf (stderr, "   mask = %llo, bits = %d\n", mask, bits);
#endif
	       /*
	       ** Process data field
	       */

	       vval = 0;
	       if (chartype)
	       {
		  int k;

		  cp = bp;
		  while (*bp && *bp != ',' && !isspace(*bp)) bp++;
		  term = *bp;
		  *bp++ = '\0';
		  strcpy (lcltoken, cp);
		  k = bits/6 - strlen(lcltoken);
		  for (i = 0; i < k; i++)
		     vval = (vval << 6) | tobcd[' '];
		  for (i = 0; i < strlen(lcltoken); i++)
		     vval = (vval << 6) | tobcd[lcltoken[i]];
#ifdef DEBUGP2VFD
		  fprintf (stderr, 
	     "   H%d: token = %s, tokentype = %d, vval = %llo, term = %c(%x)\n",
			bits, lcltoken, tokentype, vval, term, term);
#endif
	       }

	       else
	       {
		  int isalldigits = TRUE;

		  cp = bp;
		  while (*bp && *bp != ',' && !isspace(*bp))
		  {
		     if (!isdigit(*bp))
		     {
			isalldigits = FALSE;
		     }
		     bp++;
		  }
		  term = *bp;
		  *bp++ = '\0';
#ifdef DEBUGP2VFD
                  fprintf (stderr, "   exprtype = %o, iso expr = %s\n",
			   exprtype, cp);
#endif
		  if (isalldigits)
		  {
		     if (radix == 8)
		     {
#if defined(WIN32)
			sscanf (cp, "%I64o", &vval);
#else
			sscanf (cp, "%llo", &vval);
#endif
		     }
		     else
		     {
#if defined(WIN32)
			sscanf (cp, "%I64d", &vval);
#else
			sscanf (cp, "%lld", &vval);
#endif
		     }
		  }
		  else
		  {
		     if (*cp == '/')
		     {
			cp++;
#if defined(WIN32)
		        vval = 0777777777777I64;
#else
		        vval = 0777777777777ULL;
#endif
		        if (*cp)
			{
			   cp--;
#ifdef DEBUGP2VFD
			   fprintf (stderr, "   VFD expr0 = %s\n", cp);
#endif
			   cp = exprscan (cp, &val, &term, &relocatable, 1, FALSE, 0);
			   vval = val;
			}
		     }
		     else
		     {
#ifdef DEBUGP2VFD
                        fprintf (stderr, "   VFD expr1 = %s\n", cp);
#endif
			cp = exprscan (cp, &val, &term, &relocatable, 1, FALSE, 0);
			vval = val;
		     }
		     if (relocatable)
		     {
			if (octtype)
			{
			   logerror ("Invalid boolean expression");
			}
		        if ((bitcount + bits) < 19)
			   emittag = RELDECR_TAG;
			else
			{
			   if (emittag == RELDECR_TAG)
			      emittag = RELBOTH_TAG;
			   else if (emittag == ABSDATA_TAG)
			      emittag = RELADDR_TAG;
			}
		     }
		  }
#ifdef DEBUGP2VFD
		  fprintf (stderr, "   %c%02d: vval = %012llo, term = %c(%x)\n",
			  radix == 8 ? 'O' : 'D', bits, vval, term, term);
		  fprintf (stderr, "        mask = %012llo, emittag = %c\n",
			   mask, emittag);
#endif
	       }
	       radix = 10;
	       vval &= mask;

	       /*
	       ** Emit word when length is exceeded
	       */

	       if (bitword + bits >= 36)
	       {
		  resid = (bitword + bits) - 36;
#ifdef DEBUGP2VFD
		  fprintf (stderr, "   resid = %d\n", resid);
#endif
		  rval = (rval << (bits - resid)) | (vval >> resid);
		  tmpnum0 = (rval >> 18) & 0777777;
		  tmpnum1 = rval & 0777777;
		  sprintf (opbuf, OP2FORMAT,
			   tmpnum0 & 0400000 ? '-' : '+',
			   tmpnum0 & 0377777, tmpnum1);
		  sprintf (objbuf, OCT2FORMAT,
			   emittag,
			   tmpnum0, tmpnum1);
#ifdef DEBUGP2VFD
		  fprintf (stderr, "   tmpnum0 = %o, tmpnum1 = %o\n\n", 
			  tmpnum0, tmpnum1);
#endif
		  emittag = ABSDATA_TAG;
		  relocatable = FALSE;
		  printed = TRUE;
		  if (listmode)
		  {
		     if (j == 0)
		     {
			printline (lstfd);
			if (etcndx < etccount)
			{
			   strcpy (lstbuf, etclines[etcndx]);
			   if (!widemode)
			   {
			      int ii;

			      for (ii = 0; ii < strlen(lstbuf); ii++)
			      {
				 if (ii >= NARROWPRINTMARGIN-2)
				 {
				    lstbuf[ii] = '\n';
				    lstbuf[ii+1] = '\0';
				    break;
				 }
				 else if (lstbuf[ii] == '\n')
				    break;
			      }
			   }
			   etcndx++;
			}
			else lstbuf[0] = '\0';
		     }
		     else
		     {
			if (etcndx < etccount)
			{
			   sprintf (pcbuf, PCFORMAT, pc);
			   printline (lstfd);
			   if (etcndx < etccount)
			   {
			      strcpy (lstbuf, etclines[etcndx]);
			      if (!widemode)
			      {
				 int ii;

				 for (ii = 0; ii < strlen(lstbuf); ii++)
				 {
				    if (ii >= NARROWPRINTMARGIN-2)
				    {
				       lstbuf[ii] = '\n';
				       lstbuf[ii+1] = '\0';
				       break;
				    }
				    else if (lstbuf[ii] == '\n')
				       break;
				 }
			      }
			      etcndx++;
			   }
			   else lstbuf[0] = '\0';
			}
			else
			{
			   if (lstbuf[0])
			      printline (lstfd);
			   else if (detailmode)
			      printdata (lstfd, pc);
			}
		     }
		  }
		  punchrecord (outfd);
		  pc++;
		  j++;
		  mask = 0;
		  for (i = 0; i < resid; i++) mask = (mask << 1) | 1;
		  vval &= mask;
		  rval = vval;
		  bitword = 0;
		  bitcount += bits - resid;
		  bits = resid;
	       }
	       else
	       {
		  rval = (rval << bits) | vval;
	       }
	       bitcount += bits;
	       bitword += bits;
	    }
	    if (term == '\n') break;
	 }
      }

      /*
      ** Partial word, left justify and emit
      */

      if (bitword)
      {
	 rval <<= (36 - bitword);
	 tmpnum0 = (rval >> 18) & 0777777;
	 tmpnum1 = rval & 0777777;
	 sprintf (opbuf, OP2FORMAT,
		  tmpnum0 & 0400000 ? '-' : '+',
		  tmpnum0 & 0377777, tmpnum1);
	 sprintf (objbuf, OCT2FORMAT, emittag,
		  tmpnum0, tmpnum1);
#ifdef DEBUGP2VFD
	 fprintf (stderr, "   tmpnum0 = %o, tmpnum1 = %o\n", 
		 tmpnum0, tmpnum1);
#endif
	 if (j != 0)
	 {
	    if (detailmode)
	       printdata (lstfd, pc);
	    punchrecord (outfd);
	 }
	 pc++;
      }
#ifdef DEBUGP2VFD
      fprintf (stderr, "   bitcount = %d\n", bitcount);
#endif
      val = bitcount / 36;
      val = val + ((bitcount - (val * 36)) ? 1 : 0);
#ifdef DEBUGP2VFD
      fprintf (stderr, "   val = %d\n", val);
#endif
      radix = 10;
      spc += val;
      if (spc != pc)
      {
         printf ("ERROR: pc not set: pc = %o, spc = %o\n", pc, spc);
      }
      break;

   default: ;
   }
   return (FALSE);
}

/***********************************************************************
* p2lnkdir - Pass 2 FAP Linkage Director
***********************************************************************/

void
p2lnkdir (OpCode *op, FILE *lstfd, FILE *outfd)
{
   t_uint64 cvtsym = 0;
   t_uint64 lnkdata = 0;
   int i;

   if (op && op->optype == TYPE_P)
   {
       switch (op->opvalue)
       {
       case ABS_T:
       case COUNT_T:
       case DETAIL_T:
       case EJE_T:
       case ENT_T:
       case EXT_T:
       case FUL_T:
       case HEAD_T:
       case HED_T:
       case INSERT_T:
       case LABEL_T:
       case LBL_T:
       case LINK_T:
       case LIST_T:
       case MACRO_T:
       case MACRON_T:
       case OPSYN_T:
       case ORG_T:
       case PCC_T:
       case PCG_T:
       case PMC_T:
       case PUNCH_T:
       case REM_T:
       case SPC_T:
       case SST_T:
       case TITLE_T:
       case TTL_T:
       case UNLIST_T:
       case UNPNCH_T:
          return;
       default:
          break;
       }
   }

   genlnkdir = FALSE;
   for (i = 0; i < MAXSYMLEN; i++)
      cvtsym = (cvtsym << 6) | tobcd[lnkdirsym[i]];
   if (listmode)
   {
      fprintf (lstfd, "\n        LINKAGE DIRECTOR\n");
      linecnt += 2;
      sprintf (opbuf, OPLFORMAT, ' ', lnkdata);
      printdata (lstfd, pc);
      sprintf (opbuf, OPLFORMAT, ' ', cvtsym);
      printdata (lstfd, pc+1);
      fprintf (lstfd, "\n");
      linecnt++;
      strcpy (opbuf, OPBLANKS);
   }
   sprintf (objbuf, OCTLFORMAT, ABSDATA_TAG, lnkdata);
   punchrecord (outfd);
   sprintf (objbuf, OCTLFORMAT, ABSDATA_TAG, cvtsym);
   punchrecord (outfd);
   pc += 2;
}

/***********************************************************************
* asmpass2 - Pass 2 
***********************************************************************/

int
asmpass2 (FILE *tmpfd, FILE *outfd, int lstmode, FILE *lstfd)
{
   char *bp, *cp;
   char *token;
   int i;
   int status = 0;
   int done = 0;
   int srcmode;
   int flag;
   int val;
   int tokentype;
   int ctlcards;
   int begcomments;
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
      perror ("asm7090: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Initialize.
   */

   memset (objrec, ' ', sizeof(objrec));

   litloc = litorigin;

   gbllistmode = lstmode;
   listmode = lstmode;

   linecnt = MAXLINE;

   begcomments = TRUE;

   detailmode = FALSE;
   pccmode = FALSE;
   pmcmode = FALSE;
   printed = FALSE;
   orgout = FALSE;
   punchmode = TRUE;
   usebegin = FALSE;
   gotoskip = FALSE;
   gotoblank = FALSE;
   ctlcards = FALSE;
   noreflst = FALSE;
   firstindex = FALSE;
   xline = FALSE;
   tcdemitted = FALSE;
   idtemitted = FALSE;
   inloc = FALSE;

   objrecnum = 0;
   objcnt = 0;
   pagenum = 0;
   qualindex = 0;
   linenum = 0;
   asmskip = 0;
   headcount = 0;
   curruse = 0;
   prevuse = 0;
   prevloc = 0;
   locorg = 0;
   pc = 0;
   lastsequ = 0;
   lastmod[0] = '\0';

   for (i = 0; i < begincount; i++)
      begintable[i].value = 0;

   strcpy (idtbuf, deckname);
   idtlen = absmod ? 0 : pgmlength & 077777;

   /*
   ** Process the source.
   */

   while (!done)
   {
      if (readsource (tmpfd, &srcmode, &linenum, &filenum, srcbuf) < 0)
      {
         done = TRUE;
	 break;
      }
#ifdef DEBUGP2RDR
      fprintf (stderr, "   gotoskip = %s, asmskip = %s, pc = %o\n",
	       gotoskip ? "TRUE" : "FALSE",
	       asmskip ? "TRUE" : "FALSE", pc);
#endif

      strcpy (lstbuf, srcbuf);
      if (!widemode)
      {
	 int ii;

	 for (ii = 0; ii < strlen(lstbuf); ii++)
	 {
	    if (ii >= NARROWPRINTMARGIN-2)
	    {
	       lstbuf[ii] = '\n';
	       lstbuf[ii+1] = '\0';
	       break;
	    }
	    else if (lstbuf[ii] == '\n')
	       break;
	 }
      }

      /*
      ** If a IBSYS control card, print it.
      */

      if (srcmode & CTLCARD)
      {
	 ctlcards = TRUE;
         printctlcard (lstfd);
	 continue;
      }

      if (ctlcards)
      {
	 ctlcards = FALSE;
         linecnt = MAXLINE;
      }

      exprtype = ADDREXPR;
      radix = 10;
      pc &= 077777;
      printed = FALSE;
      nooperands = FALSE;
      errnum = 0;
      warnnum = 0;
      errline[0][0] = '\0';
      objbuf[0] = '\0';
      strcpy (pcbuf, PCBLANKS);

      checksequence (srcbuf, srcmode);
      strcpy (opbuf, OPBLANKS);

      inmode = srcmode;

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
	       if (readsource (tmpfd, &srcmode, &linenum, &filenum, srcbuf) < 0)
	       {
		  done = TRUE;
		  break;
	       }

	       checksequence (srcbuf, srcmode);
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
	 bp = inbuf;

	 if (!asmskip)
	 {
	    if (*bp != COMMENTSYM)
	    {
	       OpCode *op;

	       begcomments = FALSE;

	       /*
	       ** If label present, scan it off.
	       ** On MAP/FAP the symbol can start in any column up to 6.
	       ** And FAP can have embedded blanks, eg. "( 3.4)"
	       */

	       if (strncmp (bp, "      ", 6))
	       {
		  char *cp;
		  char *dp;
		  char temp[8];

		  strncpy (temp, bp, 6);
		  temp[6] = '\0';
		  for (cp = temp; *cp; cp++); 
		  *cp-- = '\0';
		  while (isspace(*cp)) *cp-- = '\0';
		  bp = bp + (cp - temp + 1);
		  cp = dp = temp;
		  while (*cp)
		  {
		     if (*cp == ' ') cp++;
		     else *dp++ = *cp++;
		  }
		  *dp = '\0';

		  strcpy (cursym, temp);
#ifdef DEBUGCURSYM
		  fprintf (stderr, "asmpass2: cursym = %s\n", cursym);
		  fprintf (stderr, "    rem = %s", bp);
#endif
		  while (*bp && isspace (*bp)) bp++;
	       }
	       else 
	       {
		  cursym[0] = '\0';
		  while (*bp && isspace (*bp)) bp++;
	       }

	       /*
	       ** Check if in GOTO skip
	       */

 	       if (gotoskip)
	       {
#ifdef DEBUGGOTO
		  fprintf (stderr, "   cursym = '%s', gotosym = '%s'\n",
			   cursym, gotosym);
#endif
		  if (cursym[0] && !strcmp (cursym, gotosym))
		  {
		     if (gotoblank) cursym[0] = '\0';
		     xline = FALSE;
		     gotoskip = FALSE;
		     gotoblank = FALSE;
		  }
		  else
		  {
		     xline = TRUE;
		     if (!pccmode)
			printed = TRUE;
		     goto PRINT_LINE;
		  }
	       }
	       else if (gotosym[0])
	       {
	          if (!strcmp (cursym, gotosym))
		  {
		     gotosym[0] = '\0';
		     if (gotoblank) cursym[0] = '\0';
		     gotoblank = FALSE;
		  }
	       }

	       /*
	       ** Scan off opcode.
	       */

	       if (*bp == '\0')
	       {
		  strcpy (opcode, "PZE"); /* Make a null opcode a PZE */
		  term = ' ';
	       }
	       else if (!strncmp (&inbuf[OPCSTART], "   ", 3))
	       {
		  strcpy (opcode, "PZE");
		  term = ' ';
		  bp = &inbuf[10];
	       }
	       else
	       {
		  int oldtermparn;

		  oldtermparn = termparn;
		  termparn = TRUE;
		  bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
		  termparn = oldtermparn;
		  strcpy (opcode, token);
	       }

	       /*
	       ** Check for indirect addressing.
	       ** Maybe either "TRA*  SYM" or "TRA  *SYM".
	       */

	       if (term == '*')
	       {
		  flag = 060;
		  bp++;
	       }
	       else
	       {
		  while (*bp && isspace(*bp))
		  {
		     if (*bp == '\n' || (bp - inbuf >= NOOPERAND))
		     {
			nooperands = TRUE;
		        break;
		     }
		     bp++;
		  }
		  if (!nooperands && *bp == INDIRECTSYM &&
		     ((isalnum(*(bp+1)) || (*(bp+1) == '.'))))
		  {
		     flag = 060;
		     bp++;
		  }
		  else
		     flag = 0;
	       }

	       /*
	       ** If not a macro call ref, then process
	       */

	       if (!(inmode & MACCALL))
	       {
		  /*
		  ** Process according to type.
		  */

		  if ((op = oplookup (opcode)) != NULL)
		  {
		     if (bcore && (op->opflags & BCORE_ILL))
		     {
			sprintf (errtmp, "Invalid BCORE operation: %s", opcode);
			logerror (errtmp);
		     }

		     if (!(inmode & MACDEF) && genlnkdir)
		     {
		        p2lnkdir (op, lstfd, outfd);
		     }

		     if (op->optype == TYPE_P)
		     {
			done = p2pop (op, bp, lstfd, outfd);
		     }
		     else if (op->optype == TYPE_DISK)
		     {
			p2diskop (op, flag, bp, lstfd, outfd);
		     }
		     else
		     {
			inst_proc[op->optype].proc (op, flag, bp);
		     }
		  }
		  else
		  {
		     if (!gotoskip)
		     {
			pc++;
			sprintf (errtmp, "Invalid opcode: %s", opcode);
			logerror (errtmp);
		     }
		  }
	       }

	       /*
	       ** MACRO Call, put out the PC
	       */

	       else 
	       {
		  if (genlnkdir)
		     p2lnkdir (NULL, lstfd, outfd);
		  if (!(inmode & MACEXP) || cursym[0])
		     sprintf (pcbuf, PCFORMAT, pc);
	          if (etccount)
		  {
		     int etcndx;

		     printline (lstfd);
		     printed = TRUE;
		     for (etcndx = 0; etcndx < etccount; etcndx++)
		     {
			strcpy (lstbuf, etclines[etcndx]);
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
	       if (begcomments && fapmode)
	       {
	          printed = TRUE;
		  begcomments = FALSE;
	       }
	       if (gotoskip)
	          xline = TRUE;
	    }
	 }
	 else
	 {
	    xline = TRUE;
	 }

	 if (asmskip)
	 {
	    asmskip--;   
	    if (!pccmode)
	       printed = TRUE;
	 }
      }

      /*
      ** Only list the input count lines in a DUP, unless DETAIL is set.
      */

      if (inmode == DUPINST && !detailmode)
      {
	 if (dupin) dupin--;
	 else printed = TRUE;
      }

      /*
      ** Write out a print line.
      */

   PRINT_LINE:
      if (listmode && !printed)
      {
         printline (lstfd);
	 printed = FALSE;
      }
      if (!asmskip)
	 xline = FALSE;

      /*
      ** Write an object buffer.
      */

      if (objbuf[0])
      {
         punchrecord (outfd);
      }

      /*
      ** Process errors for this line.
      */

      if ((val = printerrors (lstfd, listmode)) < 0)
         status = -1;

   }

   listmode = lstmode;
   if (!done)
   {
      errcount++;
      if (listmode)
      {
         fprintf (lstfd, "ERROR: No END record\n");
      }
      else
      {
	 fprintf (stderr, "asm7090: %d: No END record\n", linenum);
      }
      status = -1;
   }
   else
   {

      /*
      ** Process literals
      */

      processliterals (outfd, lstfd, listmode);

      if (!fapmode && begintable[begincount-1].value)
      {
	 i = begincount - 1;
	 sprintf (objbuf, WORDFORMAT, BSS_TAG,
		  begintable[i].value - begintable[i].bvalue);
	 punchrecord (outfd);
      }

      /*
      ** Punch EXTRN and ENTRY entries
      */

      punchsymbols (outfd);
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
      if (fapmode)  fprintf (lstfd, "FAP ");
      else          fprintf (lstfd, "MAP ");
      if (cpumodel == 7096) fprintf (lstfd, "CTSS");
      else                  fprintf (lstfd, "%d", cpumodel);
      fprintf (lstfd, " mode assembly\n");

      fprintf (lstfd, "Options in effect: ");
      if (absmod)   fprintf (lstfd, "ABSMOD ");
      else
        if (addrel) fprintf (lstfd, "ADDREL ");
        else        fprintf (lstfd, "RELMOD ");
      if (monsym)   fprintf (lstfd, "MONSYM ");
      if (jobsym)   fprintf (lstfd, "JOBSYM ");
      if (termparn) fprintf (lstfd, "NO() ");
      else          fprintf (lstfd, "()OK ");

      if (!fapmode && !absmod && (monsym || jobsym))
         fprintf (lstfd, "Symbol modes IGNORED for relocatable assembly.\n");

      if (deckname[0])
         fprintf (lstfd, "\nDeckname: %s", deckname);
      fprintf (lstfd, "\n");
   }

   return (status);
}
