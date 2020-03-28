/***********************************************************************
*
* asmpass2.c - Pass 2 for the IBM 7080 assembler.
*
* Changes:
*   02/12/07   DGP   Original.
*   02/20/07   DGP   Changed object for 60 characters.
*   03/07/07   DGP   Added comment format for standard list mode.
*                    Made loader data one 80 char record.
*   05/22/13   DGP   Set default starting PC to 160.
*                    Ensure instruction PC ends in 4 or 9.
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
extern int noloader;		/* Don't punch loader option flag */
extern int bootobject;		/* "boot format" object */

extern int pccounter[MAXPCCOUNTERS];/* PC counters */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char deckname[MAXDECKNAMELEN+2];/* The assembly deckname */
extern char ttlbuf[TTLSIZE+2];	/* The assembly TTL buffer */
extern char errline[10][120];	/* Pass 2 error lines for current statment */
extern char titlebuf[TTLSIZE+2];/* The assembly TITLE buffer */
extern char datebuf[48];	/* Date/Time buffer for listing */

extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern ErrTable p1error[MAXERRORS];/* The pass 0/1 error table */
extern time_t curtime;		/* Assembly time */
extern struct tm *timeblk;	/* Assembly time */


static OpCode *lastop;		/* Last opcode, NULL if specified */
static OpCode *prevop;		/* Previous opcode */
static int switchpc;		/* PC value for CHRCD, BITCD and switch */
static int switchsize;		/* Size value for CHRCD, BITCD and switch */
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
static int firstpc;		/* PC value of the first inst. */

static char pcbuf[10];		/* PC print buffer */
static char objrec[OBJRECORDLENGTH+2];/* Object record buffer */
static char opbuf[32];		/* OP print buffer */
static char objbuf[MAXDATALEN];	/* Object field buffer */

static char cursym[MAXSYMLEN+2];/* Current label field symbol */
static char lstbuf[MAXLINE];	/* Source line for listing */
static char etclines[MAXETCLINES][MAXLINE];/* ETC lines */
static char errtmp[256];	/* Error print string */

static int memzones[16] = 
{
   000000000, 020000000, 040000000, 060000000, 
   000000040, 020000040, 040000040, 060000040, 
   000000020, 020000020, 040000020, 060000020, 
   000000060, 020000060, 040000060, 060000060 
};

static int regzones[16] = 
{
   000000000, 000002000, 000004000, 000006000, 
   000200000, 000202000, 000204000, 000206000, 
   000400000, 000402000, 000404000, 000406000, 
   000600000, 000602000, 000604000, 000606000 
};

static char *loader =
   "20100A0018B0#!2B0#?4Y0080I007980#R4N0#R970#N9U006190#H9B0!00"
   "U000090!9510004J1000";
   //"20100A0018B0#!2B0#?4Y0080I007984#R4N0#R970#N9U006190089B0!00"
   //"U00009009510004J1000";

/***********************************************************************
* cvtbcdnum - Convert decimal number to bcd characters.
***********************************************************************/
static int 
cvtbcdnum (int num, int len)
{
   int retval;
   int dig;
   int i;

   retval = 0;

   for (i = 0; i < len; i++)
   {
      dig = num % 10;
      num /= 10;
      if (dig == 0) dig = tobcd['0'];
      retval = retval | (dig << (6*i));
   }
   return (retval);
}

/***********************************************************************
* cvtbcdmem - Convert decimal memory address to bcd characters with zones.
***********************************************************************/
static int 
cvtbcdmem (int addr)
{
   int retval;
   int dig;
   int i;
   int zone, base;

   retval = 0;

   base = addr % 10000;
   zone = addr / 10000;
   if (zone > MAXZONES) zone = 0;

   for (i = 0; i < 4; i++)
   {
      dig = base % 10;
      base /= 10;
      if (dig == 0) dig = tobcd['0'];
      retval = retval | (dig << (6*i));
   }
   retval |= memzones[zone];
   return (retval);
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
*ooooo  O NN AAAAAA   LLLLG  TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
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
      char modifier, term;
      char lstindex[MAXSYMLEN+2];
      char lsttag[MAXSYMLEN+2];
      char lstopcode[MAXSYMLEN+2];
      char lstnumfield[4];
      char lstatfield[4];
      char lstoperand[RIGHTMARGIN-VARSTART+2];

      strncpy (lstindex, lstbuf, SYMSTART);
      lstindex[SYMSTART] = '\0';

      if (lstbuf[SYMSTART] == COMMENTSYM)
      {
	 fprintf (lstfd, SLCFORMAT,
		  lstindex, &lstbuf[SYMSTART], pcbuf, opbuf);
      }
      else
      {
	 strncpy (lsttag, &lstbuf[SYMSTART], OPCSTART-SYMSTART);
	 lsttag[OPCSTART-SYMSTART] = '\0';

	 strncpy (lstopcode, &lstbuf[OPCSTART], NUMSTART-OPCSTART);
	 lstopcode[NUMSTART-OPCSTART] = '\0';

	 strncpy (lstnumfield, &lstbuf[NUMSTART], VARSTART-NUMSTART);
	 lstnumfield[VARSTART-NUMSTART] = '\0';

	 memset (lstoperand, ' ', sizeof(lstoperand));
	 modifier = lstbuf[VARSTART];
	 term = lstbuf[VARSTART+1];
	 if (term == ',' && (modifier == 'L' || modifier == 'R' ||
		  modifier == 'H' || modifier == 'S' || modifier == 'I'))
	 {
	    strncpy (lstatfield, &lstbuf[VARSTART], 2);
	    strncpy (lstoperand, &lstbuf[VARSTART+2], RIGHTMARGIN-VARSTART-2);
	 }
	 else
	 {
	    strncpy (lstatfield, "  ", 2);
	    strncpy (lstoperand, &lstbuf[VARSTART], RIGHTMARGIN-VARSTART);
	 }
	 lstatfield[2] = '\0';
	 lstoperand[RIGHTMARGIN-VARSTART] = '\0';

	 fprintf (lstfd, SL1FORMAT,
		  lstindex, lsttag, lstopcode, lstnumfield, lstatfield,
		  lstoperand, pcbuf, opbuf);
      }
   }
   else
      fprintf (lstfd, L1FORMAT, pcbuf, opbuf, linenum, mch, lstbuf);

   lstbuf[0] = '\0';
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
      sprintf (ttlbuf, " SYMBOL     ADDRESS LENGTH  DEF            REFERENCES");
   else
      sprintf (ttlbuf, "SYMBOL TABLE");

   for (i = 0; i < symbolcount; i++)
   {

      sym = symbols[i];
      if (sym->symbol[0] != LITERALSYM)
      {
	 char type;
	 char lenfield[10];

	 printheader (lstfd);
	 strcpy (lenfield, "    ");

	 if (sym->flags & CONVAR)
	 {
	    type = 'C';
	    sprintf (lenfield, "%4d", sym->length);
	 }
	 else if (sym->flags & FPNVAR)
	 {
	    type = 'F';
	    sprintf (lenfield, "%4d", sym->length);
	 }
	 else if (sym->flags & RCDVAR)
	 {
	    type = 'R';
	    sprintf (lenfield, "%4d", sym->length);
	 }
	 else if (sym->flags & RPTVAR)
	 {
	    type = 'T';
	    sprintf (lenfield, "%4d", sym->length);
	 }
	 else if (sym->flags & BITCDVAR)
	 {
	    type = 'B';
	    sprintf (lenfield, "%4d", sym->length);
	 }
	 else if (sym->flags & CHRCDVAR)
	 {
	    type = 'H';
	    sprintf (lenfield, "%4d", sym->length);
	 }
	 else if (sym->flags & EXTERNAL)
	 {
	    type = 'I';
	 }
	 else type = ' ';

	 fprintf (lstfd, SYMFORMAT,
		  sym->symbol,
		  sym->value,
		  type,
		  lenfield);
	 j++;
	 if (genxref)
	 {
	    j = printxref (sym, lstfd);
	 }

	 if (j >= (widemode ? 4 : 3))
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
   int addrval;
   int length;
   char temp[10];

   length = objcnt - (bootobject ? 0 : INSTFIELD);
#ifdef DEBUGOBJECT
   printf ("punchfinish: objectpc = %d, objcnt = %d, length = %d\n",
	    objectpc, objcnt, length);
   printf ("objrec = %s\n", objrec);
#endif

   if (length)
   {
      if (!bootobject)
      {
      sprintf (&objrec[IDENTFIELD], OBJIDENTFORMAT, deckname);
      sprintf (&objrec[SERIALFIELD], OBJSERIALFORMAT, ++objrecnum);
      addrval = cvtbcdmem (objectpc);
      sprintf (&objrec[ADDRESSFIELD], OBJADDRESSFORMAT,
	       tonative[(addrval >> 18) & 077], 
	       tonative[(addrval >> 12) & 077], 
	       tonative[(addrval >>  6) & 077], 
	       tonative[addrval & 077]);
      if (intcdmode)
	 sprintf (temp, OBJCOLUMNFORMAT, 0);
      else
	 sprintf (temp, OBJCOLUMNFORMAT, length);
      strncpy (&objrec[COLUMNFIELD], temp, COLUMNLENGTH);
      }
#ifdef DEBUGOBJECTaa
      printf ("objrec = %s\n", objrec);
#endif
      objrec[OBJRECORDLENGTH] = '\n';
      objrec[OBJRECORDLENGTH+1] = '\0';
      fputs (objrec, outfd);
      memset (objrec, ' ', sizeof(objrec));
      objectpc = -1;
      objcnt = bootobject ? 0 : INSTFIELD;
   }
}

/***********************************************************************
* punchnewrecord - Punch a record and start a new one.
***********************************************************************/

static void
punchnewrecord (FILE *outfd, int curpc)
{
   int addrval;

#ifdef DEBUGOBJECT
   printf ("punchnewrecord: curpc = %d\n", curpc);
#endif

   punchfinish (outfd);
   if (bootobject) return;

   objectpc = curpc;
   addrval = cvtbcdmem (objectpc);
   sprintf (&objrec[ADDRESSFIELD], OBJADDRESSFORMAT,
	    tonative[(addrval >> 18) & 077], 
	    tonative[(addrval >> 12) & 077], 
	    tonative[(addrval >>  6) & 077], 
	    tonative[addrval & 077]);
}

/***********************************************************************
* punchrecord - Punch an object value into record.
***********************************************************************/

static void 
punchrecord (FILE *outfd, int length)
{

#ifdef DEBUGOBJECT
   printf ("punchrecord: length = %d, objbuf = %s\n", length, objbuf);
#endif

   if (objectpc < 0) objectpc = pc;
   if (objcnt+length > (bootobject ? 80 : OBJLENGTH))
   {
      int remainder = (bootobject ? 80 : OBJLENGTH) - objcnt;

#ifdef DEBUGOBJECT
      printf ("   objcnt = %d, length = %d, remainder = %d\n",
	       objcnt, length, remainder);
#endif
      strncpy (&objrec[objcnt], objbuf, remainder);
      objcnt += remainder;
      punchfinish (outfd);
      length = length - remainder;
      objectpc = pc + remainder;
      strcpy (objbuf, &objbuf[remainder]);
#ifdef DEBUGOBJECT
      printf (" length = %d\n", length);
      printf (" objbuf = %s\n", objbuf);
#endif
   }

   strncpy (&objrec[objcnt], objbuf, length);
   objbuf[0] = '\0';
   objcnt += length;
}

/***********************************************************************
* punchloader - Punch out the loader records.
***********************************************************************/

static void 
punchloader (FILE *outfd)
{
#ifdef DEBUGOBJECT
   printf ("punchloader: \n");
#endif
   fprintf (outfd, "%s\n", loader);
}

/***********************************************************************
* checkpc - Check if pc is alligned
***********************************************************************/

static void
checkpc (FILE *outfd)
{
   int finish = FALSE;
   while ((pc % 5) != 0)
   {
      finish = TRUE;
      pc++;
   }
   if (finish)
      punchfinish (outfd);
   if (objectpc < 0) objectpc = pc;
   if (firstpc < 0) firstpc = pc;
}

/***********************************************************************
* puncheof - Punch EOF sequence.
***********************************************************************/

static void 
puncheof (FILE *outfd)
{
   OpCode *op;
   int addrval;

#ifdef DEBUGOBJECT
   printf ("puncheof: \n");
#endif
   if (firstpc < 0) return;

   punchfinish (outfd);
   if (bootobject) return;

   intcdmode = TRUE;
   if ((op = oplookup ("TR")) != NULL)
   {
      checkpc (outfd);
      addrval = cvtbcdmem (firstpc + 4);
      sprintf (objbuf, OBJINSTFORMAT, op->opvalue,
	       tonative[(addrval >> 18) & 077], 
	       tonative[(addrval >> 12) & 077], 
	       tonative[(addrval >>  6) & 077], 
	       tonative[addrval & 077]);
      punchrecord (outfd, INSTLENGTH);
      punchfinish (outfd);
   }
}

/***********************************************************************
* printerrors - Print any error message for this line.
***********************************************************************/

static int
printerrors (FILE *lstfd, int listmode)
{
   int i;
   int ret;

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
	    fprintf (stderr, "asm7080: %d: %s\n", linenum, errline[i]);
	 }
	 errline[i][0] = '\0';
      }
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
	       fprintf (stderr, "asm7080: %d: %s\n",
			linenum, p1error[i].errortext);
	    }
	    ret = -1;
	 }

   return (ret);
}

/***********************************************************************
* processcondata - Process CON data (or literal).
***********************************************************************/

static char *
processcondata (char *bp, int numfield)
{
   char *cp;
   int i;
   char sign;
   char condata[MAXDATALEN];

   memset (condata, 0, numfield);
   sign = 0;
   if (*bp == '-' || *bp == '+')
      sign = *bp++;
   if (isdigit (*bp))
   {
      for (i = 0; i < numfield; i++)
      {
	 if (sign && *bp == ' ') condata[i] = tobcd['0'];
	 else if (*bp == ' ') condata[i] = tobcd[' '];
	 else if (*bp == '.') ;
	 else if (!isdigit (*bp))
	 {
	    logerror ("Invalid numeric constant");
	    break;
	 }
	 else
	 {
	    char term = *bp - '0';
	    if (term == 0) term = tobcd['0'];
	    condata[i] = term;
	 }
	 bp++;
      }
      if (sign) condata[i-1] |= tobcd[sign];
   }
   else
   {
      if (sign)
      {
	 logerror ("Invalid character constant");
      }
      else for (i = 0; i < numfield; i++)
      {
	 unsigned char ch;
	 if ((ch = tobcd[*bp++]) == 255)
	 {
	    logerror ("Invalid character");
	    ch = tobcd[' '];
	 }
	 condata[i] = ch;
      }
   }

   cp = objbuf;
   for (i = 0; i < numfield; i++)
      *cp++ = tonative[condata[i]];
   *cp = '\0';

   return (bp);
}

/***********************************************************************
* processfpndata - Process FPN data (or literal).
***********************************************************************/

static char *
processfpndata (char *bp)
{
   char *cp;
   int i;
   char term;
   char sign;
   char condata[FLOATPOINTLENGTH+2];

   /*
   ** Get exponent, must be signed and two digits.
   */

   memset (condata, tobcd['0'], FLOATPOINTLENGTH+2);
   if (*bp == '-' || *bp == '+')
   {
      sign = *bp++;
      if (!isdigit (*bp)) goto INVALID_FLOAT;
      term = *bp++ - '0';
      if (term == 0) term = tobcd['0'];
      condata[0] = term;
      if (isdigit(*bp))
      {
	 term = *bp++ - '0';
	 if (term == 0) term = tobcd['0'];
	 condata[1] = term;
      }
      else
	 goto INVALID_FLOAT;
      if (sign) condata[1] |= tobcd[sign];

      /*
      ** Get mantissa, must be signed and at least 1 digit.
      */

      if (!(*bp == '-' || *bp == '+')) goto INVALID_FLOAT;
      sign = *bp++;
      if (!isdigit (*bp)) goto INVALID_FLOAT;
      for (i = 2; *bp && i < FLOATPOINTLENGTH; i++)
      {
	 if (*bp == ' ') break;
	 else if (!isdigit(*bp)) goto INVALID_FLOAT;
	 term = *bp++ - '0';
	 if (term == 0) term = tobcd['0'];
	 condata[i] = term;
      }
      if (sign) condata[FLOATPOINTLENGTH-1] |= tobcd[sign];

      cp = objbuf;
      for (i = 0; i < FLOATPOINTLENGTH; i++)
	 *cp++ = tonative[condata[i]];
      *cp = '\0';
   }
   else
   {
   INVALID_FLOAT:
      logerror ("Invalid floating point constant");
   }
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
   int literalcount = 0;
   char lcltoken[TOKENLEN];

   /*
   ** Get number of literals in symbol table
   */

   for (i = 0; i < symbolcount; i++)
   {
      if (symbols[i]->symbol[0] == LITERALSYM) literalcount++;
   }
#ifdef DEBUGLIT
   fprintf (stderr, "p2-processliterals: literalcount = %d\n", literalcount);
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

	 if (symbols[i]->symbol[0] == LITERALSYM && symbols[i]->value == pc)
	 {
	    SymNode *s;

	    s = symbols[i];
	    strcpy (lcltoken, &s->symbol[1]);
	    if ((cp = strrchr(lcltoken, LITERALSYM)) != NULL)
	       *cp = '\0';
	    cp = lcltoken;

	    if (s->flags & MIXEDLITERAL)
	    {
               cp = processcondata (cp, s->length);
	       punchrecord (outfd, s->length);
	    }
	    else if (s->flags & SIGNEDLITERAL)
	    {
	       if (s->flags & FLOATLITERAL)
	       {
		  cp = processfpndata (cp);
		  punchrecord (outfd, FLOATPOINTLENGTH);
	       }
	       else
	       {
		  cp = processcondata (cp, s->length);
		  punchrecord (outfd, s->length);
	       }
	    }

	    {
	       sprintf (pcbuf, PCFORMAT, pc);
	       printheader (lstfd);
	       if (stdlist)
		  fprintf (lstfd, SLITFORMAT, s->symbol, pcbuf);
	       else
		  fprintf (lstfd, LITFORMAT, pcbuf, s->symbol);
	       linecnt++;
	       printerrors (lstfd, listmode);
	    }

#ifdef DEBUGLIT
	    fprintf (stderr, "   %s = %s\n", pcbuf, s->symbol);
#endif
	    if (listmode)
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
p2aop (OpCode *op, int numfield, char *bp, FILE *outfd)
{
   int addrval;
   int addr = 0;
   char term;

   checkpc (outfd);
   sprintf (pcbuf, PCFORMAT, pc+4);

   if (*bp != ' ')
      bp = exprscan (bp, &addr, &term, FALSE);

   addrval = cvtbcdmem (addr);

   addrval |= regzones[numfield & MAXREGISTERS];

   sprintf (opbuf, OPFORMAT,
	    op->opvalue, numfield, addr,
	    tonative[(addrval >> 18) & 077], 
	    tonative[(addrval >> 12) & 077], 
	    tonative[(addrval >>  6) & 077], 
	    tonative[addrval & 077]);

   sprintf (objbuf, OBJINSTFORMAT, op->opvalue,
	    tonative[(addrval >> 18) & 077], 
	    tonative[(addrval >> 12) & 077], 
	    tonative[(addrval >>  6) & 077], 
	    tonative[addrval & 077]);
#ifdef DEBUGADDR
   fprintf (stderr, "op = %c(%02o), addr = %08o, objbuf = %s\n",
            op->opvalue, tobcd[op->opvalue], addrval, objbuf);
#endif
   punchrecord (outfd, INSTLENGTH);

   pc += 5;
}

/***********************************************************************
* p2bop - Process B type operation code.
***********************************************************************/

static void
p2bop (OpCode *op, int numfield, char *bp, FILE *outfd)
{
   int addrval;
   int addr = 0;
   char term;

   checkpc (outfd);
   sprintf (pcbuf, PCFORMAT, pc+4);

   if (*bp != ' ')
      bp = exprscan (bp, &addr, &term, FALSE);

   addrval = cvtbcdmem (addr);

   if (op->opmod)
      addrval |= regzones[op->opmod & MAXREGISTERS];

   sprintf (opbuf, OPFORMAT,
	    op->opvalue, numfield, addr,
	    tonative[(addrval >> 18) & 077], 
	    tonative[(addrval >> 12) & 077], 
	    tonative[(addrval >>  6) & 077], 
	    tonative[addrval & 077]);

   sprintf (objbuf, OBJINSTFORMAT, op->opvalue,
	    tonative[(addrval >> 18) & 077], 
	    tonative[(addrval >> 12) & 077], 
	    tonative[(addrval >>  6) & 077], 
	    tonative[addrval & 077]);
#ifdef DEBUGADDR
   fprintf (stderr, "op = %c(%02o), addr = %08o, objbuf = %s\n",
            op->opvalue, tobcd[op->opvalue], addrval, objbuf);
#endif
   punchrecord (outfd, INSTLENGTH);

   pc += 5;
}

/***********************************************************************
* p2cop - Process C type operation code.
***********************************************************************/

static void
p2cop (OpCode *op, int numfield, char *bp, FILE *outfd)
{
   int addrval;
   int addr;

   checkpc (outfd);
   sprintf (pcbuf, PCFORMAT, pc+4);

   addr = op->opmod >> 6;
   addrval = regzones[op->opmod & MAXREGISTERS] | cvtbcdnum(addr, 4);

   sprintf (opbuf, OPFORMAT,
	    op->opvalue, numfield, addr,
	    tonative[(addrval >> 18) & 077], 
	    tonative[(addrval >> 12) & 077], 
	    tonative[(addrval >>  6) & 077], 
	    tonative[addrval & 077]);

   sprintf (objbuf, OBJINSTFORMAT, op->opvalue,
	    tonative[(addrval >> 18) & 077], 
	    tonative[(addrval >> 12) & 077], 
	    tonative[(addrval >>  6) & 077], 
	    tonative[addrval & 077]);
#ifdef DEBUGADDR
   fprintf (stderr, "op = %c(%02o), addr = %08o, objbuf = %s\n",
            op->opvalue, tobcd[op->opvalue], addrval, objbuf);
#endif
   punchrecord (outfd, INSTLENGTH);

   pc += 5;
}

/***********************************************************************
* p2pop - Process Pseudo operation code.
***********************************************************************/

static int
p2pop (OpCode *op, int numfield, char *bp, FILE *lstfd, FILE *outfd)
{
   OpCode *nop;
   SymNode *s;
   int addrval;
   int addr = 0;
   int val;
   int junk;
   char term;

   strcpy (pcbuf, PCBLANKS);
   strcpy (opbuf, OPBLANKS);

   switch (op->opvalue)
   {

   case ACON6_T:		/* Address CONstant 6 characters */
      sprintf (pcbuf, PCFORMAT, pc);
      if (*bp != ' ')
	 bp = exprscan (bp, &addr, &term, FALSE);

      val = addr / 100000;
      junk = addr - (val * 100000);
      if (val == 0) val = tobcd['0'];
      addrval = cvtbcdnum (junk, 5);

      sprintf (opbuf, ADC6FORMAT,
	       addr,
	       tonative[val & 077], 
	       tonative[(addrval >> 24) & 077], 
	       tonative[(addrval >> 18) & 077], 
	       tonative[(addrval >> 12) & 077], 
	       tonative[(addrval >>  6) & 077], 
	       tonative[addrval & 077]);


      sprintf (objbuf, OBJACON6FORMAT,
	       tonative[val & 077], 
	       tonative[(addrval >> 24) & 077], 
	       tonative[(addrval >> 18) & 077], 
	       tonative[(addrval >> 12) & 077], 
	       tonative[(addrval >>  6) & 077], 
	       tonative[addrval & 077]);
      punchrecord (outfd, ACON6LENGTH);
      pc += 6;
      break;

   case ACON5_T:		/* Address CONstant 5 characters */
      sprintf (pcbuf, PCFORMAT, pc);
      val = *(bp-1);
      if (*bp != ' ')
	 bp = exprscan (bp, &addr, &term, FALSE);

      if (addr > 79999)
      {
         logerror ("Address too large for ADCON");
	 addr = 79999;
      }

      addrval = cvtbcdnum (addr, 5);
      if (val == '-' || val == '+') addrval |= tobcd[val];

      sprintf (opbuf, ADC5FORMAT,
	       addr,
	       tonative[(addrval >> 24) & 077], 
	       tonative[(addrval >> 18) & 077], 
	       tonative[(addrval >> 12) & 077], 
	       tonative[(addrval >>  6) & 077], 
	       tonative[addrval & 077]);

      sprintf (objbuf, OBJACON5FORMAT,
	       tonative[(addrval >> 24) & 077], 
	       tonative[(addrval >> 18) & 077], 
	       tonative[(addrval >> 12) & 077], 
	       tonative[(addrval >>  6) & 077], 
	       tonative[addrval & 077]);
      punchrecord (outfd, ACON5LENGTH);
      pc += 5;
      break;

   case ACON4_T:		/* Address CONstant 4 characters */
      sprintf (pcbuf, PCFORMAT, pc);
      if (*bp != ' ')
	 bp = exprscan (bp, &addr, &term, FALSE);

      addrval = cvtbcdmem (addr);
      addrval |= regzones[numfield & MAXREGISTERS];

      sprintf (opbuf, ADC4FORMAT,
	       numfield, addr,
	       tonative[(addrval >> 18) & 077], 
	       tonative[(addrval >> 12) & 077], 
	       tonative[(addrval >>  6) & 077], 
	       tonative[addrval & 077]);

      sprintf (objbuf, OBJACON4FORMAT,
	       tonative[(addrval >> 18) & 077], 
	       tonative[(addrval >> 12) & 077], 
	       tonative[(addrval >>  6) & 077], 
	       tonative[addrval & 077]);
      punchrecord (outfd, ACON4LENGTH);
      pc += 4;
      break;

   case ADCON_T:		/* ADdress CONstant */
      if ((nop = oplookup ("NOP")) != NULL)
	 p2aop (nop, numfield, bp, outfd);
      break;

   case BITCD_T:		/* BIT CoDe */
      if (lastop)
      {
	 if (cursym[0])
	 {
	    if ((s = symlookup (cursym, FALSE, FALSE)))
	    {
	       sprintf (pcbuf, PCFORMAT, s->value);
	       if (!(s->swvalue == '1' || s->swvalue == '2' || s->swvalue == '4'
		  || s->swvalue == 'A'))
	       {
		  logerror ("Invalid bit value");
	       }
	    }
	 }
	 else
	    logerror ("Symbol required");
      }
      else
      {
	 sprintf (pcbuf, PCFORMAT, pc);
         if (cursym[0])
	    logerror ("Symbol not allowed");

	 switchpc = pc;
	 if (prevop && prevop->optype == TYPE_P && prevop->opvalue == RCD_T);
	 else
	 {
	    objbuf[0] =  *bp;
	    objbuf[1] = '\0';
	    punchrecord (outfd, 1);
	 }
	 pc += 1;
	 switchsize = 1;
      }
      break;

   case CHRCD_T:		/* ChaRacter CoDe */
      if (lastop)
      {
	 if (cursym[0])
	 {
	    if ((s = symlookup (cursym, FALSE, FALSE)))
	    {
	       sprintf (pcbuf, PCFORMAT, s->value);
	    }
	 }
	 else
	    logerror ("Symbol required");
      }
      else
      {
	 sprintf (pcbuf, PCFORMAT, pc);
         if (cursym[0])
	    logerror ("Symbol not allowed");

	 switchpc = pc;
	 if (numfield == 2)
	 {
	    if (prevop && prevop->optype == TYPE_P && prevop->opvalue == RCD_T);
	    else
	    {
	       objbuf[0] =  *bp++;
	       objbuf[1] =  *bp;
	       objbuf[2] = '\0';
	       punchrecord (outfd, 2);
	    }
	    pc += 2;
	    switchsize = 2;
	 }
         else if (numfield == 0)
	 {
	    if (prevop && prevop->optype == TYPE_P && prevop->opvalue == RCD_T);
	    else
	    {
	       objbuf[0] =  *bp;
	       objbuf[1] = '\0';
	       punchrecord (outfd, 1);
	    }
	    pc += 1;
	    switchsize = 1;
	 }
	 else
	 {
	    logerror ("Invalid size specified");
	    pc += 1;
	    switchsize = 1;
	 }
      }
      break;

   case CON_T:			/* CONstant */
      sprintf (pcbuf, PCFORMAT, pc);
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
            if (s->flags & NUMSACCESS)
	       sprintf (pcbuf, PCFORMAT, s->value);
	 }
      }
      bp = processcondata (bp, numfield);
      punchrecord (outfd, numfield);
      pc += numfield;
      break;

   case EJECT_T:		/* EJECT a page */
      if (linecnt)
      {
	 linecnt = MAXLINE;
	 printheader (lstfd);
      }
      break;

   case END_T:			/* END of assembly */
      return (TRUE);

   case FPN_T:			/* Floating Point Number */
      sprintf (pcbuf, PCFORMAT, pc);
      bp = processfpndata (bp);
      punchrecord (outfd, FLOATPOINTLENGTH);
      pc += FLOATPOINTLENGTH;
      break;

   case LASN_T:			/* Location ASsigNment */
      if (intcdmode)
      {
	 punchfinish (outfd);
	 intcdmode = FALSE;
      }
      if (*bp != ' ')
      {
	 bp = exprscan (bp, &val, &term, FALSE);
	 if (val < 0 || val >= 160000)
	 {
	    sprintf (errtmp, "Invalid LASN value: %d", val);
	    logerror (errtmp);
	    break;
	 }
	 if (curpcindex != numfield)
	 {
	    pccounter[curpcindex] = pc;
	 }
	 curpcindex = numfield;
	 pc = val;
      }
      else
      {
	 curpcindex = numfield;
	 pc = pccounter[numfield];
      }
      sprintf (pcbuf, PCFORMAT, pc);
      punchnewrecord (outfd, pc);
      break;

   case LITOR_T:			/* LITeral ORigin */
      if (*bp != ' ')
      {
	 bp = exprscan (bp, &val, &term, FALSE);
	 sprintf (opbuf, ADDRFORMAT, val);
      }
      else
         logerror ("LITOR requires operand");
      printline (lstfd);
      printed = TRUE;
      break;

   case RCD_T:			/* ReCorD definition */
      sprintf (pcbuf, PCFORMAT, pc);
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
            if (s->flags & NUMSACCESS)
	       sprintf (pcbuf, PCFORMAT, s->value);
	 }
      }
      pc += numfield;
      if (numfield)
	 punchnewrecord (outfd, pc);
      break;

   case RPT_T:			/* RePortT definition */
      sprintf (pcbuf, PCFORMAT, pc);
      if (*bp != ' ')
      {
	 char *cp;
	 int i;
	 char condata[MAXDATALEN];

	 for (i = 0; i < numfield; i++)
	 {
	    unsigned char ch;
	    if ((ch = tobcd[*bp++]) == 255)
	    {
	       logerror ("Invalid character");
	       ch = tobcd[' '];
	    }
	    condata[i] = ch;
	 }

	 cp = objbuf;
	 for (i = 0; i < numfield; i++)
	    *cp++ = tonative[condata[i]];
	 *cp = '\0';
      }

      punchrecord (outfd, numfield);
      pc += numfield;
      break;

   case SASN_T:			/* Special location ASsigNment */
      if (intcdmode)
      {
	 punchfinish (outfd);
	 intcdmode = FALSE;
      }
      if (*bp != ' ')
      {
	 bp = exprscan (bp, &val, &term, FALSE);
	 if (val < 0 || val >= 160000)
	 {
	    sprintf (errtmp, "Invalid SASN value: %d", val);
	    logerror (errtmp);
	    break;
	 }
	 pccounter[0] = pc;
	 pc = val;
	 sprintf (pcbuf, PCFORMAT, pc);
	 punchnewrecord (outfd, pc);
      }
      else
         logerror ("SASN requires operand");
      break;

   case TCD_T:			/* Transfer Control Directive */
      punchfinish (outfd);
      intcdmode = TRUE;
      break;

   case TITLE_T:		/* TITLE block in listing */
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
   int done = 0;
   int i;
   int srcmode;
   int val;
   int tokentype;
   int numfield;
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
      perror ("asm7080: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Initialize.
   */

   memset (objrec, ' ', sizeof(objrec));

   gbllistmode = lstmode;
   listmode = lstmode;

   linecnt = MAXLINE;

   printed = FALSE;
   orgout = FALSE;
   xline = FALSE;
   intcdmode = FALSE;
   idtemitted = FALSE;

   objrecnum = 0;
   pagenum = 0;
   pc = STARTPC;
   linenum = 0;
   objcnt = bootobject ? 0 : INSTFIELD;
   objectpc = -1;
   firstpc = -1;
   op = lastop = NULL;

   if (stdlist)
      strcpy (ttlbuf, 
	      "INDEX   TAG       OP   NU AT    OPERAND           COMMENTS      "              "               LOC  OP SU   ADDRESS   PGLIN SER REF");
   else
      strcpy (ttlbuf,
	      " LOC  OP SU   ADDRESS     STMT         SOURCE STATEMENT");

   strcpy (opcode, "   ");

   /*
   ** Punch loader into beginning of the object deck.
   */

   if (!noloader)
      punchloader (outfd);

   /*
   ** Process the source.
   */

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
	       i = NUMSTART - OPCSTART;
	       strncpy (opcode, bp, i);
	       opcode[i] = '\0';
	       for (i--; i; i--)
	       {
		  if (isspace (opcode[i])) opcode[i] = '\0';
		  else break;
	       }
	       op = NULL;
	    }

	    /*
	    ** If not a macro call ref, then process
	    */

	    if (!(inmode & MACCALL))
	    {
	       /*
	       ** Scan off numeric field
	       */

	       lastop = op;
	       bp = &inbuf[NUMSTART];
	       if (inbuf[OPCSTART] == ' ' && lastop)
	          bp -= 3;
	       numfield = 0;

	       for (; bp < &inbuf[VARSTART]; bp++)
	       {
		  if (*bp != ' ' && isdigit(*bp))
		     numfield = numfield * 10 + (*bp - '0');
	       }

	       /*
	       ** Process according to type.
	       */

	       bp = &inbuf[VARSTART];
	       if ((op = oplookup (opcode)) != NULL)
	       {
		  switch (op->optype)
		  {
		     case TYPE_A:
			p2aop (op, numfield, bp, outfd);
			break;

		     case TYPE_B:
			p2bop (op, numfield, bp, outfd);
			break;

		     case TYPE_C:
		     case TYPE_D:
			p2cop (op, numfield, bp, outfd);
			break;

		     case TYPE_P:
			done = p2pop (op, numfield, bp, lstfd, outfd);
		  }
	       }
	       else
	       {
		  pc += 5;
		  sprintf (errtmp, "Invalid opcode: %s", opcode);
		  logerror (errtmp);
	       }
	    }

	    /*
	    ** MACRO Call, put out the PC
	    */

	    else 
	    {
	       if (!(inmode & MACEXP))
		  sprintf (pcbuf, PCFORMAT, pc);
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
   if (!done)
   {
      errcount++;
      if (listmode)
      {
         fprintf (lstfd, "ERROR: No END record\n");
      }
      else
      {
	 fprintf (stderr, "asm7080: %d: No END record\n", linenum);
      }
      status = -1;
   }
   else
   {

      /*
      ** Process literals
      */

      processliterals (outfd, lstfd, listmode);

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
      fprintf (lstfd, "Assembly mode: ");
      if (cpumodel == 7053) fprintf (lstfd, "705-III\n");
      else                  fprintf (lstfd, "%d\n", cpumodel);

      if (deckname[0])
         fprintf (lstfd, "Deckname: %s\n", deckname);

      fprintf (lstfd, "Options: \n");
      fprintf (lstfd, "   ");
      if (bootobject)
         fprintf (lstfd, "Boot ");
      else if (noloader)
         fprintf (lstfd, "No ");
      fprintf (lstfd, "Loader\n");
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
