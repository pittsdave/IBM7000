/***********************************************************************
*
* asmpass1 - Pass 1 for the IBM 7070 assembler.
*
* Changes:
*   03/01/07   DGP   Original.
*   05/29/13   DGP   Added DRDW and RDW support.
*   06/03/13   DGP   Improved packed values to include numeric.
*                    Added floating point support.
*   06/11/13   DGP   Fixed Duplicate symbol bug.
*                    Added DA fixes.
*   06/12/13   DGP   Added DSW support.
*   06/26/13   DGP   Set FDPOS for DRDW.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "asmdef.h"


extern int pc;			/* the assembler pc */
extern int curpcindex;		/* Current PC counter index */
extern int symbolcount;		/* Number of symbols in symbol table */
extern int radix;		/* Number scanner radix */
extern int linenum;		/* Source line number */
extern int exprtype;		/* Expression type */
extern int p1errcnt;		/* Number of pass 0/1 errors */
extern int pgmlength;		/* Length of program */
extern int genxref;		/* Generate cross reference listing */
extern int litorigin;		/* Literal pool origin */
extern int litpoolsize;		/* Literal pool size */
extern int inpass;		/* Which pass are we in */
extern int errcount;		/* Number of errors in assembly */
extern int rdwcount;		/* Number of RDWs */
extern int dacount;		/* Number of DAs */

extern int pccounter[MAXPCCOUNTERS];/* PC counters */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char deckname[MAXDECKNAMELEN+2];/* The assembly deckname */
extern char ttlbuf[TTLSIZE+2];	/* The assembly TTL buffer */
extern char titlebuf[TTLSIZE+2];/* The assembly TITLE buffer */

extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern ErrTable p1error[MAXERRORS];/* The pass 0/1 error table */
extern DA_t das[MAXDAS];	/* DAs */
extern RDW_t rdws[MAXRDWS];	/* RDWs */

static OpCode *lastop;		/* Last opcode, NULL if specified */

static int chksyms;		/* Check symbols in this pass */
static int inmode;		/* Source line input mode */
static int packedpc;		/* Current packed data pc for DC */
static int packedlen;		/* Current packed data len for DC */
static int inpacked;		/* Packed data in process for DC */
static int indcrdw;		/* In RDW block */
static int rdwlen;		/* RDW length */
static int rdwndx;		/* RDW index */
static int inda;		/* In DA block */
static int dalen;		/* DA length */
static int dandx;		/* DA index */

static char packedsign;		/* Current packed data sign */

static char cursym[MAXSYMLEN+2];/* Current label field symbol */
static char errtmp[256];	/* Error print string */

/***********************************************************************
* chkerror - Check to see if the Pass 1 error has been recorded.
***********************************************************************/

int
chkerror (int line)
{
   int i;

   for (i = 0; i < p1errcnt; i++)
      if (p1error[i].errorline == line)
         return (TRUE);
   return (FALSE);
}

/***********************************************************************
* chkinda - Check to see if we're in a DA.
***********************************************************************/

static void
chkinda (void)
{
   if (inda)
   {
      SymNode *s;
      int words;

      words = (dalen / 10) + ((dalen % 10) ? 1 : 0);
#ifdef DEBUGDA
      fprintf (stderr,
               "p1-chkinda: pc = %d, count = %d, words = %d, dalen = %d : %d\n",
	       pc, das[dandx].count, words, dalen, dalen / 10);
#endif
      inda = FALSE;
      if ((dandx + 1) >= MAXDAS)
      {
         logerror ("Too many DA definitions");
	 return;
      }

      if ((s = symlookup (das[dandx].symbol, FALSE, FALSE)))
      {
         s->length = words;
      }
      das[dandx].length = words;
      pc = das[dandx].origin + (words * das[dandx].count);
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
      fprintf (stderr, "p1-chkindcrdw: pc = %d, rdwlen = %d\n",
	       pc, rdwlen);
#endif
      indcrdw = FALSE;
      if ((dandx + 1) >= MAXRDWS)
      {
         logerror ("Too many RDW definitions");
	 return;
      }

      rdws[rdwndx].length = rdwlen;
      rdwndx++;
   }
}

/***********************************************************************
* chkpacked - Check to see if we're in a DC Packed.
***********************************************************************/

static void
chkpacked (void)
{
   if (inpacked)
   {
#ifdef DEBUGDCPACKED
      fprintf (stderr, "p1-chkpacked: pc = %d, packedpc = %d, packedlen = %d\n",
	       pc, packedpc, packedlen);
#endif
      inpacked = FALSE;
      packedlen = 0;
      packedpc = 0;
      packedsign = 0;
      pc++;
   }
}

/***********************************************************************
* p1allop - Operation processors (all we do here is advance the pc).
***********************************************************************/

static void
p1allop (OpCode *op, char *bp)
{
   int val;
   int pos, ix;
   char term;

   chkpacked();
   chkindcrdw();
   chkinda();

   if (*bp == STRINGSYM || *bp == '-' || *bp == '+')
      bp = exprscan (bp, &val, &pos, &ix, &term, chksyms);

   pc += 1;
}

/***********************************************************************
* processliterals - Process literals.
***********************************************************************/

static void
processliterals ()
{
   SymNode *s;
   int i, j;
   int litpc = litorigin;
   int literalcount = 0;

   /*
   ** Get number of literals in symbol table per type
   */

   for (i = 0; i < symbolcount; i++)
   {
      s = symbols[i];
      if (s->flags & LITERALMASK)
      {
	 literalcount++;
	 s->value = litpc;
	 litpc += s->length;
	 if ((s->symbol[0] == '-' || s->symbol[0] == '+') &&
	    isdigit (s->symbol[1]))
	 {
	    s->fdpos = (10 - (strlen (&s->symbol[1]))) * 10 + 9;
	 }
      }
   }

#ifdef DEBUGLIT
   fprintf (stderr,
	    "p1-processliterals: pc = %d, litorigin = %d, litpoolsize = %d\n",
	    pc, litorigin, litpoolsize);
#endif

}

/***********************************************************************
* p1pop - Process Pseudo operation codes. Most ops just advance the pc.
* Most errors are ignored here and reported in pass 2.
***********************************************************************/

static int
p1pop (OpCode *op, char *bp)
{
   SymNode *s;
   char *token;
   int tokentype;
   int pos, ix;
   int val;
   int fdpos;
   int modes;
   char term;
   char sign;

   if (op->opvalue != DC_T)
   {
      chkpacked();
      chkindcrdw();
   }
   if (op->opvalue != DA_T)
   {
      chkinda();
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
	       bp = exprscan (bp, &val, &pos, &ix, &term, chksyms);
	       pc = val;
	    }
	 }
	 else if (!strcmp (cursym, "LITORIGIN"))
	 {
	    if (*bp != ' ')
	    {
	       bp = exprscan (bp, &val, &pos, &ix, &term, chksyms);
#ifdef DEBUGLIT
	       fprintf (stderr, "p1-LORG: litorigin = %d\n", val);
#endif
	       litorigin = val;
	    }
	 }
	 else if (!strcmp (cursym, "END"))
	 {
	    val = pc;
	    if (!litorigin) litorigin = pc;
	    processliterals();
	    return (TRUE);
	 }
      }
      break;

   case DA_T:			/* Define Area */
      if (*bp != ' ')
      {
#ifdef DEBUGDA
	 fprintf (stderr, "p1-DA: pc = %d, inda = %s, dandx = %d, dalen = %d\n",
		  pc, inda ? "TRUE" : "FALSE", dandx, dalen);
	 if (inda)
	    fprintf (stderr, "   origin = %d, symbol = %s\n",
	             das[dandx].origin, das[dandx].symbol);
#endif
	 if (!inda)
	 {
	    inda = TRUE;
	    dacount++;
	    memset (&das[dandx], 0, sizeof(DA_t));
	    das[dandx].address = -1;

	    bp = exprscan (bp, &val, &pos, &ix, &term, chksyms);
	    if (term == ',')
	    {
	       char sign = ' ';

	       if (*bp == '-' || *bp == '+')
		  sign = *bp++;
	       if (!strncmp (bp, "RDW", 3))
	       {
		  bp += 3;
	          das[dandx].sign = sign;
		  das[dandx].genrdw = TRUE;
	       }
	       if (*bp == ',')
	       {
		  int lval = 0;

		  bp++;
		  bp = exprscan (bp, &lval, &pos, &ix, &term, chksyms);
		  das[dandx].address = lval;
		  das[dandx].index = ix;
	       }
	    }
	    if (cursym[0])
	    {
	       strcpy (das[dandx].symbol, cursym);
	       if ((s = symlookup (cursym, FALSE, FALSE)))
	       {
		  s->flags |= AREADEF;
		  s->value = pc;
		  s->fdpos = 9;
	       }
	    }
	    pc += val;
	    das[dandx].origin = pc;
	    das[dandx].count = val;
	    dalen = 0;
	 }
	 else
	 {
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
	    if (cursym[0])
	    {
#ifdef DEBUGDA
	       fprintf (stderr, "   cursym = %s, pc = %d\n", cursym, pc);
#endif
	       if ((s = symlookup (cursym, FALSE, FALSE)))
	       {
		  s->flags |= AREAVAR;
		  s->index = das[dandx].index;
		  if (das[dandx].address >= 0)
		     s->value = das[dandx].address + (sv / 10);
		  else
		     s->value = das[dandx].origin + (sv / 10);
		  s->length = (ev - sv) + 1;
		  if ((sv / 10) != (ev / 10))
		     s->fdpos = ((sv % 10) * 10) + 9;
		  else
		     s->fdpos = ((sv % 10) * 10) + (ev % 10);
	       }
	    }
#ifdef DEBUGDA
            fprintf (stderr, "   sv = %d, ev = %d\n", sv, ev);
#endif
	    if (ev > dalen)
	       dalen = ev + 1;
	    pc = das[dandx].origin + (ev / 10);
	 }
      }
      break;

   case DC_T:			/* Define Constant */
      val = 0;
      sign = 0;
      fdpos = 9;
      modes = FALSE;

      if (*bp == '-' || *bp == '+') 
      {
         sign = *bp++;
      }

      if (!strncmp (bp, "RDW", 3))
      {
	 chkpacked ();
	 chkindcrdw();

#ifdef DEBUGDCRDW
	 fprintf (stderr, "p1-DC_RDW: pc = %d, sign = %c, rdwndx = %d\n",
		  pc, sign, rdwndx);
#endif
         indcrdw = TRUE;
	 modes = TRUE;
	 rdwlen = 0;
	 rdws[rdwndx].sign = sign;
	 rdws[rdwndx].origin = pc + 1;
	 rdwcount++;
	 val = 1;
      }

      else if (*bp == STRINGSYM)
      {
	 char *ep;
	 int len;

	 if (inpacked && (packedsign != '@'))
	    chkpacked();

	 bp++;
	 inpacked = TRUE;
	 packedsign = '@';

#ifdef DEBUGDCPACKED
	 fprintf (stderr,
		  "p1-DC_CHAR: pc = %d, packedpc = %d, packedlen = %d, str = '",
		  pc, packedpc, packedlen);
#endif

	 if ((ep = strrchr (bp, STRINGSYM)) != NULL)
	 {
	    fdpos = (packedlen * 2) * 10;
	    for (len = 0; bp < ep; bp++, len++) 
	    {
#ifdef DEBUGDCPACKED
	       fprintf (stderr, "%c", *bp);
#endif
	       if (packedlen && ((packedlen % 5) == 0))
	       {
		  packedlen = 0;
		  val++;
		  packedpc++;
	       }
	       packedlen++;
	    }
	    fdpos += ((packedlen * 2) - 1);
#ifdef DEBUGDCPACKED
	    fprintf (stderr, "'\n  len = %d, val = %d, fdpos = %02d\n",
		     len, val, fdpos);
#endif
	 }
      }

      else
      {
	 char *sp, *ep;

	 if (inpacked && (packedsign != sign))
	    chkpacked();

#if defined(DEBUGDCNUMS) || defined(DEBUGDCPACKED)
	 fprintf (stderr,
		  "p1-DC_NUM: pc = %d, packedpc = %d, packedlen = %d\n",
		  pc, packedpc, packedlen);
#endif

	 sp = bp;
	 for (bp++; *bp && *bp != ' '; )
	 {
	    if (isdigit (*bp))
	    {
	       bp++;
	       continue;
	    }
	    else if (*bp == '.')
	    {
	       bp++;
	       continue;
	    }
	    else if (*bp == ',')
	    {
	       bp++;
	       sp = bp;
	       continue;
	    }
	    else if (*bp == 'F')
	    {
	       bp++;
	       if (*bp == '+' || *bp == '-') bp++;
	       for (; *bp && isdigit(*bp); bp++) ;
	       chkpacked();
#ifdef DEBUGDCNUMS
	       fprintf (stderr,
		     "p1-DCfloat: pc = %d, sp = %12.12s\n", pc+val, sp);
#endif
	    }
	    else if (*bp == '+' || *bp == '-')
	    {
#if defined(DEBUGDCNUMS) || defined(DEBUGDCPACKED)
	       fprintf (stderr, "   New num: val = %d, bp = %20.20s\n",
			val, bp);
#endif
	       if (!inpacked)
	       {
		  chkpacked();
		  val++;
	       }
	       sp = bp;
	       sign = *bp++;
	    }
	    else if (*bp == '(')
	    {
	       int sv, ev;

	       inpacked = TRUE;
	       packedsign = sign;
	       sv = ev = 0;
	       bp++;
	       if ((ep = strchr (bp, ')')) != NULL)
	       {
		  *ep++ = '\0';
		  bp = exprscan (bp, &sv, &pos, &ix, &term, chksyms);
		  if (term == ',')
		     bp = exprscan (bp, &ev, &pos, &ix, &term, chksyms);
		  fdpos = sv * 10 + ev;
		  if (ev < packedlen)
		     chkpacked ();
	          packedlen = ev;
		  bp = ep;
	       }
	    }
         }
	 if (fdpos == 9) val++;
#if defined(DEBUGDCNUMS) || defined(DEBUGDCPACKED)
	 fprintf (stderr, "  val = %d, fdpos = %02d\n",
		     val, fdpos);
#endif
      }

      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
	    if (modes)
	    {
	       s->flags |= RDWVAR;
	    }
	    else
	    {
	       s->flags |= CONVAR;
	    }
	    s->value = pc;
	    s->length = val;
	    s->fdpos = fdpos;
	 }
      }
      rdwlen += val;
      pc += val;
      break;

   case DRDW_T:			/* Define Record Definition Word */
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
	    s->flags |= RDWVAR;
	    s->value = pc;
	    s->fdpos = 9;
	 }
      }
      pc ++;
      break;

   case DSW_T:			/* DSW */
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
	    s->flags |= DSWVAR;
	    s->value = pc;
	 }
      }
      if (*bp != ' ')
      {
	 for (pos = 0; *bp && *bp != ' ' && pos < 10; pos++)
	 {
	    sign = '+';
	    if (*bp == '-' || *bp == '+')
	       sign = *bp++;
	    bp = tokscan (bp, &token, &tokentype, &val, &term);
	    if (tokentype == SYM)
	    {
	       if ((s = symlookup (token, TRUE, TRUE)) == NULL)
	       {
		  sprintf (errtmp, "Duplicate symbol: %s", cursym);
		  logerror (errtmp);
	       }
	       else
	       {
	          s->flags |= DSWSVAR;
		  s->fdpos = (pos * 10) + pos;
		  s->length = (sign == '+') ? 1 : 0;
	       }
	    }
	 }
      }
      pc++;
      break;

   case EQU_T:			/* EQU */
#ifdef DEBUGEQU
      fprintf (stderr, "p1pop: EQU: cursym = '%s'\n", cursym);
#endif
      if (*bp != ' ')
      {
	 int sv, ev;
	 char *sp, *ep;
	 char temp[MAXSRCLINE+2];

	 if (*bp == '@')
	 {
	    break;
	 }
	 fdpos = 9;
	 modes = EQUVAR;
         if ((term = *bp) != ',')
	 {
	    if ((sp = strrchr (bp, '(')) != NULL)
	    {
	       sv = ev = 0;
	       *sp++ = '\0';
	       if ((ep = strchr (sp, ')')) != NULL)
	       {
		  *ep++ = '\0';
		  sp = exprscan (sp, &sv, &pos, &ix, &term, chksyms);
		  if (term == ',')
		     sp = exprscan (sp, &ev, &pos, &ix, &term, chksyms);
	          fdpos = sv * 10 + ev;
		  strcpy (temp, bp);
		  strcat (temp, ep);
		  bp = temp;
#ifdef DEBUGEQU
		  fprintf (stderr, "   new bp = %s\n", bp);
#endif
	       }
	       else
	          logerror ("Invalid position expression");
	    }
	    bp = exprscan (bp, &val, &pos, &ix, &term, chksyms);
	 }
	 if (term == ',')
	 {
	    char code[4];

	    sp = code;
	    for (ix = 0; ix < 4 && *bp && *bp != ' '; ix++) *sp++ = *bp++;
	    *sp = '\0';
#ifdef DEBUGEQU
	    fprintf (stderr, "   code = %s\n", code);
#endif

	    if (!strcmp (code, "AF")) 
	    {
	       modes |= AFVAR;
	    }
	    else if (!strcmp (code, "CU"))
	    {
	       modes |= TCUVAR;
	       if (val < 10 || val > MAXTAPES)
	       {
	          logerror ("Invalid channel unit value");
		  val = 0;
	       }
	    }
	    else if (!strcmp (code, "C"))
	    {
	       modes |= TCVAR;
	       if (val < 1 || val > MAXCHANNELS)
	       {
	          logerror ("Invalid channel value");
		  val = 0;
	       }
	    }
	    else if (!strcmp (code, "I")) 
	    {
	       modes |= ULVAR;
	    }
	    else if (!strcmp (code, "P")) 
	    {
	       modes |= UPVAR;
	    }
	    else if (!strcmp (code, "Q")) 
	    {
	       modes |= IQVAR;
	    }
	    else if (!strcmp (code, "R")) 
	    {
	       modes |= URVAR;
	    }
	    else if (!strcmp (code, "SN"))
	    {
	       modes |= SNVAR;
	       if (val < 1 || val > MAXSWITCHES)
	       {
	          logerror ("Invalid switch value");
	          val = 0;
	       }
	    }
	    else if (!strcmp (code, "S"))
	    {
	       modes |= SWVAR;
	       if (val < 1 || val > MAXSWITCHES)
	       {
	          logerror ("Invalid switch value");
		  val = 0;
	       }
	    }
	    else if (!strcmp (code, "T")) 
	    {
	       modes |= TYVAR;
	    }
	    else if (!strcmp (code, "U")) 
	    {
	       modes |= TUVAR;
	       if (val < 1 || val > MAXUNITS)
	       {
	          logerror ("Invalid unit value");
		  val = 0;
	       }
	    }
	    else if (!strcmp (code, "W")) 
	    {
	       modes |= UWVAR;
	    }
	    else if (!strcmp (code, "X")) 
	    {
	       modes |= IWVAR;
	       if (val < 1 || val > MAXINDEX)
	       {
	          logerror ("Invalid index value");
		  val = 0;
	       }
	    }
	    else
	       logerror ("Invalid mode selector");
	 }

	 /*
	 ** Set symbol to value
	 */

	 if (cursym[0])
	 {
#ifdef DEBUGEQU
	    fprintf (stderr, "   val = %d, fdpos = %02d\n",
		  val, fdpos);
#endif
	    if (!(s = symlookup (cursym, FALSE, FALSE)))
	       s = symlookup (cursym, TRUE, FALSE);

	    if (s)
	    {
	       s->line = linenum;
	       s->value = val;
	       s->fdpos = fdpos;
	       s->flags |= modes;
	    }
	 }
      }
      break;

   default: ;
   }
   return (FALSE);
}

/***********************************************************************
* asmpass1 - Pass 1
***********************************************************************/

int
asmpass1 (FILE *tmpfd, int symonly)
{
   OpCode *op;
   char *token;
   char *bp;
   char *cp;
   int status = 0;
   int tokentype;
   int val;
   int done;
   int srcmode;
   char term;
   char opcode[MAXSYMLEN+2];
   char srcbuf[MAXLINE];

#ifdef DEBUGP1RDR
   fprintf (stderr, "asmpass1: Entered: symonly = %s\n",
	    symonly ? "TRUE" : "FALSE");
#endif

   /*
   ** Rewind the input.
   */

   if (fseek (tmpfd, 0, SEEK_SET) < 0)
   {
      perror ("asm7070: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Process the source.
   */

   chksyms = symonly;

   pc = DEFAULTPCLOC;

   pgmlength = 0;
   litorigin = 0;
   linenum = 0;
   litpoolsize = 0;
   packedlen = 0;
   packedpc = 0;
   packedsign = 0;
   rdwcount = 0;
   rdwndx = 0;
   dacount = 0;
   dandx = 0;

   inpacked = FALSE;
   indcrdw = FALSE;
   inda = FALSE;

   ttlbuf[0] = '\0';
   strcpy (opcode, "   ");
   op = lastop = NULL;

   done = FALSE;
   while (!done)
   {
      /*
      ** Read source line mode and text
      */

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

#ifdef DEBUGP1RDR
      fprintf (stderr,
        "asmpass1: linenum = %d, srcmode = %06x\n",
	       linenum + 1, srcmode);
      fprintf (stderr, "   srcbuf = %s", srcbuf);
#endif

      /*
      ** On the first record and the deckname is not set, get the
      ** deckname from the sequence field.
      */

      if (!linenum && !deckname[0])
      {
         if (srcbuf[RIGHTMARGIN] != ' ')
	 {
	    strncpy (deckname, &srcbuf[RIGHTMARGIN], MAXDECKNAMELEN);
	    deckname[MAXDECKNAMELEN] = '\0';
	    for (cp = deckname; *cp && isalpha(*cp); cp++) ;
	    *cp = '\0';
	 }
      }

      linenum++;

#ifdef DEBUGP1RDRM
      if (srcmode & MACDEF)
	 fprintf (stderr, "min = %s", inbuf);
#endif
      /*
      ** If not a MACRO definition line, process
      */

      if (!(srcmode & (MACDEF | SKPINST)))
      {
	 inmode = srcmode;
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
#ifdef DEBUGP1RDR
	       fprintf (stderr, "   srcbuf = %s", srcbuf);
#endif
	       if (strlen (srcbuf) > RIGHTMARGIN)
		  srcbuf[RIGHTMARGIN+1] = '\0';
	       linenum++;
	       cp = &srcbuf[VARSTART];
	       while (*cp && !isspace(*cp)) cp++;
	       *cp = '\0';
	       strcat (inbuf, &srcbuf[VARSTART]);
	    }
	 }
	 strcat (inbuf, "\n");
#ifdef DEBUGP1RDRC
	 if (inmode & CONTINU)
	    fprintf (stderr, "cin = %s", inbuf);
#endif
	 bp = &inbuf[SYMSTART];
	 exprtype = ADDREXPR;
	 radix = 10;

	 /*
	 ** If not a comment, then process.
	 */

	 if (*bp != COMMENTSYM)
	 {
	    SymNode *s;

	    /*
	    ** If label present, scan it off. Check later to add.
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
	       fprintf (stderr, "asmpass1: cursym = %s\n", cursym);
	       fprintf (stderr, "    rem = %s", bp);
#endif
	    }
	    else 
	       cursym[0] = '\0';

	    /*
	    ** Scan off opcode.
	    */

	    bp = &inbuf[OPCSTART];
	    if (*bp != ' ')
	    {
	       bp = tokscan (bp, &token, &tokentype, &val, &term);
	       strcpy (opcode, token);
	       op = NULL;
	    }

	    lastop = op;
	    op = oplookup (opcode);

	    /*
	    ** Add symbol now if not specially handled.
	    */

	    if (cursym[0])
	    {
	       int addit;

	       addit = TRUE;
	       if (inmode & MACALT || inmode & MACCALL)
		  addit = FALSE;
	       else if (op && (op->optype == TYPE_P))
	          switch (op->opvalue)
		  {
		  case CNTRL_T:
		  case EQU_T:
		     addit = FALSE;
		     break;
		  default: ;
		  }

	       if (chksyms)
	       {
		  if (addit)
		  {
#ifdef DEBUGEQU
		     fprintf (stderr,
			   "asmpass1: Add: cursym = '%s', val = %05o\n",
			   cursym, pc);
		     fprintf (stderr, "   cursym = '%s'\n", cursym);
#endif
		     if ((s = symlookup (cursym, TRUE, TRUE)) == NULL)
		     {
			sprintf (errtmp, "Duplicate symbol: %s", cursym);
			logerror (errtmp);
		     }

		  }
	       }
	       else
	       {
		  if (addit)
		  {
#ifdef DEBUGEQU
		     fprintf (stderr,
			   "asmpass1: Mod: cursym = '%s', val = %05o\n",
				 cursym, pc);
		     fprintf (stderr, "D  cursym = '%s'\n", cursym);
#endif
		     if ((s = symlookup (cursym, FALSE, FALSE)) != NULL)
			s->value = pc;
		  }
	       }
	    }

	    /*
	    ** If not a macro call ref, then process
	    */

	    if (!(inmode & MACCALL))
	    {
	       bp = &inbuf[VARSTART];
	       if (op)
	       {
		  switch (op->optype)
		  {
		     case TYPE_P:
			done = p1pop (op, bp);
			if (done) status = 0;
			break;

		     default:
			p1allop (op, bp);
		  }
	       }
	       else
	       {
		  pc += 1; 
	       }
	    }
	 }
      }
   }

   return (status);
}
