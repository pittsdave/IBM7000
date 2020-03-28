/***********************************************************************
*
* asmpass1 - Pass 1 for the IBM 7080 assembler.
*
* Changes:
*   02/12/07   DGP   Original.
*   03/07/07   DGP   Fixed deckname overrun.
*   05/22/13   DGP   Set default starting PC to 160.
*                    Ensure instruction PC ends in 4 or 9.
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

extern int pccounter[MAXPCCOUNTERS];/* PC counters */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char deckname[MAXDECKNAMELEN+2];/* The assembly deckname */
extern char ttlbuf[TTLSIZE+2];	/* The assembly TTL buffer */
extern char titlebuf[TTLSIZE+2];/* The assembly TITLE buffer */

extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern ErrTable p1error[MAXERRORS];/* The pass 0/1 error table */


static OpCode *lastop;		/* Last opcode, NULL if specified */
static int chksyms;		/* Check symbols in this pass */
static int inmode;		/* Source line input mode */
static int switchpc;		/* PC value for CHRCD, BITCD and switch */
static int switchsize;		/* Size value for CHRCD, BITCD and switch */

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
* p1allop - Operation processors (all we do here is advance the pc).
***********************************************************************/

static void
p1allop (OpCode *op, int numfield, char *bp)
{
   SymNode *s;
   int val;
   char term;

   while ((pc % 5) != 0) pc++;
   if (cursym[0]) 
   {
      if ((s = symlookup (cursym, FALSE, FALSE)) != NULL)
	 s->value = pc + 4;
   }
   if (*bp == LITERALSYM)
      bp = exprscan (bp, &val, &term, chksyms);

   pc += 5;
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
   int signedcount = 0;
   int signedlength = 0;
   int mixedcount = 0;
   int mixedlength = 0;
   int adconcount = 0;
   int adconlength = 0;

   /*
   ** Get number of literals in symbol table per type
   */

   for (i = 0; i < symbolcount; i++)
   {
      s = symbols[i];
      if (s->symbol[0] == LITERALSYM)
      {
	 literalcount++;
	 if (s->flags & SIGNEDLITERAL)
	 {
	    signedcount++;
	    if (s->length > signedlength) signedlength = s->length;
	 }
	 else if (s->flags & MIXEDLITERAL)
	 {
	    mixedcount++;
	    if (s->length > mixedlength) mixedlength = s->length;
	 }
	 else if (s->flags & ADCONLITERAL)
	 {
	    adconcount++;
	    if (s->length > adconlength) adconlength = s->length;
	 }
      }
   }

#ifdef DEBUGLIT
   fprintf (stderr, "p1-processliterals: litorigin = %d, litpoolsize = %d\n",
	    litorigin, litpoolsize);
   fprintf (stderr, "   signedcount = %d, signedlength = %d\n",
	    signedcount, signedlength);
   fprintf (stderr, "   mixedcount = %d, mixedlength = %d\n",
	    mixedcount, mixedlength);
   fprintf (stderr, "   adconcount = %d, adconlength = %d\n",
	    adconcount, adconlength);
#endif

   /*
   ** Process Signed literals
   */

   for (j = 1; signedcount && j < signedlength+1; j++)
   {
      if ((j % 5) == 0) continue;
      for (i = 0; i < symbolcount; i++)
      {
	 s = symbols[i];
	 if ((s->flags & SIGNEDLITERAL) && (s->length == j))
	 {
#ifdef DEBUGLIT
	    fprintf (stderr, "   Signed: litpc = %d, j = %d\n",
		     litpc, j);
#endif
	    s->value = litpc;
	    litpc += s->length;
	    signedcount--;
         }
      }
   }
   for (j = 5; signedcount && j < signedlength+1; j+=5)
   {
      for (i = 0; i < symbolcount; i++)
      {
	 s = symbols[i];
	 if ((s->flags & SIGNEDLITERAL) && (s->length == j))
	 {
#ifdef DEBUGLIT
	    fprintf (stderr, "   Signed: litpc = %d, j = %d\n",
		     litpc, j);
#endif
	    s->value = litpc;
	    litpc += s->length;
	    signedcount--;
         }
      }
   }

   /*
   ** Process Mixed(UnSigned) literals
   */

   for (j = 1; mixedcount && j < mixedlength+1; j++)
   {
      if ((j % 5) == 0) continue;
      for (i = 0; i < symbolcount; i++)
      {
	 s = symbols[i];
	 if ((s->flags & MIXEDLITERAL) && (s->length == j))
	 {
#ifdef DEBUGLIT
	    fprintf (stderr, "   Mixed: litpc = %d, j = %d\n",
		     litpc, j);
#endif
	    s->value = litpc;
	    litpc += s->length;
	    mixedcount--;
         }
      }
   }
   for (j = 5; mixedcount && j < mixedlength+1; j+=5)
   {
      for (i = 0; i < symbolcount; i++)
      {
	 s = symbols[i];
	 if ((s->flags & MIXEDLITERAL) && (s->length == j))
	 {
#ifdef DEBUGLIT
	    fprintf (stderr, "   Mixed: litpc = %d, j = %d\n",
		     litpc, j);
#endif
	    s->value = litpc;
	    litpc += s->length;
	    mixedcount--;
         }
      }
   }

   /*
   ** Process ADCON literals
   */

}

/***********************************************************************
* p1pop - Process Pseudo operation codes. Most ops just advance the pc.
* Most errors are ignored here and reported in pass 2.
***********************************************************************/

static int
p1pop (OpCode *op, int numfield, char *bp)
{
   SymNode *s;
   char *token;
   int val;
   int tokentype;
   char term;

   switch (op->opvalue)
   {

   case ACON6_T:		/* Address CONstant 6 characters */
      pc += 1;
      /* fall through */
   case ACON5_T:		/* Address CONstant 5 characters */
   case ADCON_T:		/* ADdress CONstant */
      pc += 1;
      /* fall through */
   case ACON4_T:		/* Address CONstant 4 characters */
      pc += 4;
      if (*bp == LITERALSYM)
	 bp = exprscan (bp, &val, &term, chksyms);
      break;

   case BITCD_T:		/* BIT CoDe */
      if (lastop)
      {
	 if (cursym[0])
	 {
	    if ((s = symlookup (cursym, FALSE, FALSE)))
	    {
	       s->flags |= BITCDVAR;
	       s->length = switchsize;
	       s->value = switchpc;
	       s->swvalue = *(bp - 1);
	    }
	 }
      }
      else
      {
	 switchpc = pc;
	 switchsize = 1;
	 pc += 1;
      }
      break;

   case CHRCD_T:		/* ChaRacter CoDe */
      if (lastop)
      {
	 if (cursym[0])
	 {
	    if ((s = symlookup (cursym, FALSE, FALSE)))
	    {
	       s->flags |= CHRCDVAR;
	       s->length = switchsize;
	       s->value = switchpc;
	       s->swvalue = *bp++;
	       if (switchsize == 2)
		  s->swvalue = (s->swvalue << 8) | *bp++;
	    }
	 }
      }
      else
      {
	 switchpc = pc;
	 if (numfield == 2)
	 {
	    pc += 2;
	    switchsize = 2;
	 }
         else 
	 {
	    pc += 1;
	    switchsize = 1;
	 }
      }
      break;

   case CON_T:			/* CONstant */
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
	    s->flags |= CONVAR;
	    s->length = numfield;
	    if (isdigit(*bp) || *bp == '+' || *bp == '-')
	    {
	       s->value += numfield - 1;
	       s->flags |= NUMSACCESS;
	    }
	 }
      }
      pc += numfield;
      break;

   case END_T:			/* END */
      val = pc;
      if (!litorigin) litorigin = pc;
      processliterals();
      return (TRUE);

   case FPN_T:			/* Floating Point Number */
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
	    s->flags |= FPNVAR;
	    s->length = FLOATPOINTLENGTH;
	 }
      }
      pc += FLOATPOINTLENGTH;
      break;

   case INCL_T:			/* INCLude */
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      if (tokentype != SYM)
      {
	 sprintf (errtmp, "INCL requires symbol: %s",
		  token);
	 logerror (errtmp);
	 return (FALSE);
      }
      if (strlen(token) > MAXSYMLEN)
      {
	 sprintf (errtmp, "Symbol too long: %s", token);
	 logerror (errtmp);
	 return (FALSE);
      }
      if (!(s = symlookup (token, FALSE, TRUE)))
      {
	 s = symlookup (token, TRUE, TRUE);
	 if (s)
	 {
	    s->value = 0;
	    s->flags |= EXTERNAL;
	 }
      }
      break;

   case LASN_T:			/* Location ASsigNment */
      if (*bp != ' ')
      {
	 bp = exprscan (bp, &val, &term, chksyms);
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
      break;

   case LITOR_T:		/* LITeral ORigin */
      if (*bp != ' ')
      {
	 bp = exprscan (bp, &val, &term, chksyms);
#ifdef DEBUGLIT
	 fprintf (stderr, "p1-LORG: litorigin = %d\n", val);
#endif
	 litorigin = val;
      }
      break;

   case RCD_T:			/* ReCordD definition */
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
	    s->flags |= RCDVAR;
	    s->length = numfield;
	    switch (*bp)
	    {
	    case 'N':
	    case '+':
	       s->value += numfield - 1;
	       s->flags |= NUMSACCESS;
	       break;
	    default: ;
	    }
	 }
      }
      pc += numfield;
      break;

   case RPT_T:			/* RePortT definition */
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)))
	 {
	    s->flags |= RPTVAR;
	    s->length = numfield;
	 }
      }
      pc += numfield;
      break;

   case SASN_T:			/* Special location ASsigNment */
      if (*bp != ' ')
      {
	 bp = exprscan (bp, &val, &term, chksyms);
	 pccounter[0] = pc;
	 pc = val;
      }
      break;

   case TITLE_T:		/* TITLE */
      if (!titlebuf[0])
      {
	 char *cp;

	 cp = bp;
	 while (*bp != '\n')
	 {
	    if (bp - inbuf > RIGHTMARGIN) break;
	    if (bp - cp == TTLSIZE) break;
	    bp++;
	 }
	 *bp = '\0';
	 strcpy (titlebuf, cp);
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
   int i;
   int val;
   int done;
   int srcmode;
   int numfield;
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
      perror ("asm7080: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Process the source.
   */

   chksyms = symonly;

   pgmlength = 0;
   litorigin = 0;
   pc = STARTPC;
   linenum = 0;
   litpoolsize = 0;
   ttlbuf[0] = '\0';
   strcpy (opcode, "   ");
   op = lastop = NULL;
   switchpc = -1;

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
        "asmpass1: linenum = %d, srcmode = %06x, srclen = %d\n",
	       linenum + 1, srcmode, strlen(srcbuf));
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

	    lastop = op;
	    op = oplookup (opcode);

	    /*
	    ** Add symbol now if not specially handled.
	    */

	    if (cursym[0])
	    {
	       int addit;

	       addit = TRUE;
	       if (inmode & MACALT)
		  addit = FALSE;
	       else if (op && op->optype == TYPE_P && !(inmode & MACCALL))
	       {
		  switch (op->opvalue)
		  {
		  case MACRO_T:
		     addit = FALSE;
		     break;
		  default: ;
		  }
	       }

	       if (chksyms)
	       {
		  if (addit)
		  {
#ifdef DEBUGEQU
		     fprintf (stderr,
			   "asmpass1: Add: cursym = '%s', val = %05o\n",
			   cursym, pc);
		     fprintf ("   cursym = '%s'\n", cursym);
#endif
		     if ((s = symlookup (cursym, TRUE, TRUE)) != NULL)
			sprintf (errtmp, "Duplicate-2 symbol: %s", cursym);
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
	       /*
	       ** Scan off numeric field
	       */

	       bp = &inbuf[NUMSTART];
	       if (inbuf[OPCSTART] == ' ' && lastop)
	          bp -= 3;
	       numfield = 0;

	       for (; bp < &inbuf[VARSTART]; bp++)
	       {
		  if (*bp != ' ' && isdigit(*bp))
			numfield = numfield * 10 + (*bp - '0');
	       }

	       bp = &inbuf[VARSTART];
	       if (op)
	       {
		  switch (op->optype)
		  {
		     case TYPE_P:
			done = p1pop (op, numfield, bp);
			if (done) status = 0;
			break;

		     default:
			p1allop (op, numfield, bp);
		  }
	       }
	       else
	       {
		  pc+=5; 
	       }
	    }
	    
	 }

      }

   }

   return (status);
}
