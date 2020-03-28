/***********************************************************************
*
* asmpass0 - Pass 0 for the IBM 7090 assembler.
*
* Changes:
*   12/12/04   DGP   Original.
*   02/02/05   DGP   Correct Macro/IRP expansions.
*   02/03/05   DGP   Added JOBSYM.
*   02/09/05   DGP   Added FAP mode.
*   03/01/05   DGP   Change CALL parm parsing.
*   03/10/05   DGP   Added 7090 macros for 7094 instructions.
*   03/11/05   DGP   Fix null argument in macro expansion.
*                    And chained IF[FT] statements.
*   03/15/05   DGP   Correct macros for 7090 mode and add macro generated
*                    labels.
*   03/17/05   DGP   Fixed CALL error return generation.
*   06/07/05   DGP   Added DUP in macros and fixed FAP IFF.
*   06/09/05   DGP   Fixed 'X''Y' parm bug in macros.
*   06/10/05   DGP   Added RMT support.
*   07/12/05   DGP   Fixed ETC on MACRO call and added Alternate MACRO def.
*   08/01/05   DGP   Allow ETC/w parens on macro call.
*   12/15/05   DGP   Correct EOF detection in source processing.
*   08/09/06   DGP   Correct NULL op relocatability.
*   10/17/07   DGP   Fix nested macro definitions and embedded CALL.
*   05/14/09   DGP   Fix commented END/MACRO/DUP in macros.
*                    Nested macro fixes.
*                    Fix macro token scanner to allow: BCI  1,   XYZ
*   09/18/09   DGP   Fix up opcode/operand when IRP arg is an opcode and
*                    it contains a blank.
*   09/30/09   DGP   Added FAP Linkage Director.
*   10/01/09   DGP   Generalize macro end processing.
*   10/05/09   DGP   Fixup RMT processing to scan as normal code.
*   10/09/09   DGP   More fixes including null IRP.
*   10/16/09   DGP   Fixup EVEN op.
*   11/03/09   DGP   Allow opcode terminated by '('.
*   11/11/09   DGP   Fixed IFF symbol processing.
*   03/22/10   DGP   Fixed token parsing for (...,*,...) to put in comma.
*   03/31/10   DGP   Fixed "IFF N" processing.
*   04/01/10   DGP   Lookup macros in reverse order in case of a redef.
*   06/17/10   DGP   Handle TAPENO in macro expansion correctly.
*   06/20/10   DGP   Fix Hollerith literal in macro argument.
*   07/07/10   DGP   Bump PC in macro expand w/ instruction.
*   12/08/10   DGP   Fixed BCI in macro.
*   12/13/10   DGP   Insure MAP labels get qualified.
*   12/15/10   DGP   Added line number to temporary file.
*                    Added file sequence for errors.
*   12/31/10   DGP   Allow NULSYM in p0mactokens().
*   11/16/11   DGP   Process macros with comma separated operands.
*   11/21/11   DGP   Fix DUP in RMT. Fix macro parm scan.
*   12/02/11   DGP   Added LINK opcode.
*   07/09/12   DGP   Fixed buffer overrun in p0expand.
*   05/06/13   DGP   Fixed QUAL processing.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "asmdef.h"
#include "asmdmem.h"

extern int pc;			/* the assembler pc */
extern int symbolcount;		/* Number of symbols in symbol table */
extern int absolute;		/* In absolute section */
extern int radix;		/* Number scanner radix */
extern int linenum;		/* Source line number */
extern int exprtype;		/* Expression type */
extern int pgmlength;		/* Length of program */
extern int absmod;		/* In absolute module */
extern int monsym;		/* Include IBSYS Symbols (MONSYM) */
extern int jobsym;		/* Include IBJOB Symbols (JOBSYM) */
extern int termparn;		/* Parenthesis are terminals (NO()) */
extern int genxref;		/* Generate cross reference listing */
extern int addext;		/* Add extern for undef'd symbols (!absolute) */
extern int addrel;		/* ADDREL mode */
extern int qualindex;		/* QUALifier table index */
extern int fapmode;		/* FAP assembly mode */
extern int headcount;		/* Number of entries in headtable */
extern int cpumodel;		/* CPU model */
extern int errcount;		/* Number of errors in assembly */
extern int genlnkdir;		/* FAP generate linkage director */
extern int nolnkdir;		/* FAP DO NOT generate linkage director */
extern int filenum;		/* Current file number */
extern int fileseq;		/* INSERT file sequence */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char deckname[MAXLBLLEN+2];/* The assembly deckname */
extern char lbl[MAXLBLLEN+2];	/* The object label */
extern char titlebuf[TTLSIZE+2];/* The assembly TTL buffer */
extern char qualtable[MAXQUALS][MAXSYMLEN+2];/* The QUALifier table */
extern char headtable[MAXHEADS];/* HEAD table */
extern char lnkdirsym[MAXSYMLEN+2];/* Linkage director symbol */

extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern char *pscanbuf;		/* Pointer for tokenizers */
extern RMTSeq *rmthead;		/* Pointer to start of remote sequences */
extern RMTSeq *rmttail;		/* Pointer to end of remote sequences */

static FILE *tmpfd;		/* Input fd */

static long curpos;		/* Current file position */
static int asmskip;		/* In skip line mode IFF/IFT */
static int gotoskip;		/* In GOTO mode */
static int lblgennum;		/* LBL generated number */
static int macrocount;		/* Number of macros defined */
static int ifcont;		/* IFF/IFT continued */
static int ifrel;		/* IFF/IFT continue relation OR/AND */
static int ifskip;		/* IFF/IFT prior skip result */
static int eofflg = FALSE;	/* EOF on input file */
static int macsloaded;          /* 7094 macros loaded flag */
static int rmtflag;		/* In a remote sequence flag */
static int macscan;		/* Currently scanning a MACRO definition */
static int macron;		/* Currently scanning a MACRON definition */
static int readrmtseqflag;	/* Read remote sequences */
static int rmtseqnum;		/* rmtseq saved line number */
static int genlinenum;		/* Generated line number */

static char cursym[MAXSYMLEN+2];/* Current label field symbol */
static char gotosym[MAXSYMLEN+2];/* GOTO target symbol */
static char srcbuf[MAXSRCLINE];	/* Source line buffer */
static char etcbuf[MAXSRCLINE];	/* ETC (continuation) buffer */
static char errtmp[256];	/* Error print string */

static MacLines *freemaclines = NULL;/* resusable Macro lines */
static MacDef macdef[MAXMACROS];/* Macro template table */

#include "asmmacros.h"

/***********************************************************************
* maclookup - Lookup macro in table. Lookup in reverse order in case of
*             a redefinition.
***********************************************************************/

static MacDef *
maclookup (char *name)
{
   MacDef *mac = NULL;
   char *cp;
   int i;
   char lclname[MAXFIELDSIZE];

   strcpy (lclname, name);
   if ((cp = strchr (lclname, ',')) != NULL)
      *cp = '\0';

   for (i = 0; i < macrocount; i++)
   {
      mac = &macdef[macrocount - 1 - i];
      if (!strcmp (mac->macname, lclname))
      {
	 return (mac);
      }
   }
   return (NULL);
}

/***********************************************************************
* p0mactokens - Process macro tokens
***********************************************************************/

static char *
p0mactokens (char *cp, int field, int index, MacDef *mac)
{
   OpCode *op;
   char *token;
   char *ocp;
   char *bcicp;
   int tokentype;
   int val;
   int j;
   int barequote;
   int bcilen;
   char term;
   char targ[TOKENLEN];
   char largs[MAXLINE];
   char lcltoken[TOKENLEN];

#ifdef DEBUGMACRO
   fprintf (stderr, "p0mactokens: field = %d, index = %d, cp = %s",
	    field, index, cp);
#endif

   /*
   ** Check for NULL operands 
   */

   bcilen = 0;
   bcicp = NULL;

   if (field == 2)
   {
      ocp = cp;
      while (*cp && isspace (*cp))
      {
         cp++;
	 if (cp - ocp > (NOOPERAND))
	    return (cp);
      }
   }

   /*
   ** Substitute #NN for each macro parameter. The number, NN, corresponds to
   ** the parameter number.
   */

   term = '\0';
   largs[0] = '\0';
   barequote = FALSE;
   do {

      targ[0] = '\0';
      cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGMACRO
      fprintf (stderr,
	    "   token1 = %s, tokentype = %d, val = %o, term = %c(%x)\n",
	    token, tokentype, val, term, term);
      fprintf (stderr, "   cp = %s", cp);
#endif
      /*
      ** If operator in input stream, copy it out
      */

      if (token[0] == '\0')
      {
	 if (tokentype == ASTER)
	 {
	    strcpy (targ, "*");
	    if (term == ',')
	       strcat (targ, ",");
	 }
	 else if (tokentype == NULSYM)
	 {
	    strcpy (targ, "**");
	    if (term == ',')
	       strcat (targ, ",");
	 }
	 else if (term == '\'')
	 {
	    barequote = TRUE;
	 }
	 else if (term == '=' && *cp == 'H')
	 {
	    char *hp = targ;
	    char *bp = cp;

	    *hp++ = term;
	    while (*cp && ((cp-bp) < 7) && *cp >= ' ')
	    {
	       if (*cp == '\'') break;
	       *hp++ = *cp++;
	    }
	    *hp = '\0';
	 }
	 else
	    sprintf (targ, "%c", term);
      }

      /*
      ** If subsitution field, marked with apostophe, process.
      */

      else if (term == '\'')
      {
	 barequote = FALSE;
	 strcpy (lcltoken, token);
	 for (j = 0; j < mac->macargcount; j++)
	 {
	    if (!strcmp (token, mac->macargs[j]))
	    {
	       sprintf (lcltoken, "#%02d", j);
	       break;
	    }
	 }
	 strcat (largs, lcltoken);

	 cp++;
	 if (*cp != '\'')
	 {
	    cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGMACRO
	    fprintf (stderr,
	       "   token2 = %s, tokentype = %d, val = %o, term = %c(%x)\n",
	       token, tokentype, val, term, term);
	    fprintf (stderr, "   cp = %s", cp);
#endif
	    if (token[0])
	    {
	       for (j = 0; j < mac->macargcount; j++)
	       {
		  if (!strcmp (token, mac->macargs[j]))
		  {
		     sprintf (targ, "#%02d", j);
		     break;
		  }
	       }
	       if (j == mac->macargcount)
		  strcpy (targ, token);
	    }
	    if (term == '\'')
	       cp++;
	    else if (!isspace (term))
	    {
	       strncat (targ, &term, 1);
	       if (!strchr (",+-/*", term)) cp++;
	    }
	 }
      }

      /*
      ** If token is a parameter, process
      */

      else
      {
	 for (j = 0; j < mac->macargcount; j++)
	 {
	    int xx;

	    if (token[0] == '$') xx = 1;
	    else                 xx = 0;
	    if (!strcmp (&token[xx], mac->macargs[j]))
	    {
	       sprintf (targ, "%s#%02d", xx ? "$" : "", j);
	       break;
	    }
	 }
	 if (j == mac->macargcount)
	    strcpy (targ, token);
	 if (!isspace(term))
	 {
	    if ((field < 2) && (term == ','))
	    {
	       term = ' ';
	    }
	    else
	    {
	       strncat (targ, &term, 1);
	       if (term != ',') cp++;
	    }
	 }
      }

      /*
      ** If operand and BCI or BCD scan off the string.
      ** I may have leading blanks 
      */

      if (field == 2 && (mac->macopcode == BCI_T || mac->macopcode == BCD_T))
      {
	 if (term == ',')
	 {
	    int gotquote;
	    int kk;

#ifdef DEBUGMACRO
	    fprintf (stderr, "   targ = %s\n", targ);
	    fprintf (stderr, "   BCI in MACRO = %d:'%s'\n", val, cp);
#endif

	    ocp = cp;
	    bcilen = val * 6;
	    gotquote = FALSE;
	    for (kk = 0; kk < bcilen && *ocp && *ocp >= ' '; kk++, ocp++)
	    {
	       if (*ocp == '\'')
	       {
		  gotquote = TRUE;
	       }
	    }
	    if (!gotquote && (val > 1 || *cp == ' '))
	    {
	       bcicp = &targ[strlen(targ)];
	       for (kk = 0; kk < bcilen && *cp && *cp >= ' '; kk++)
	       {
		  if (*cp == ' ')
		     *cp = '_';
		  *bcicp++ = *cp++;
	       }
	       for (; kk < bcilen; kk++)
		  *bcicp++ = '_';
	       *bcicp = '\0';
	       term = *cp;
	    }
	 }
      }
      strcat (largs, targ);
   } while (*cp && !isspace(term) && !isspace(*cp));

   if (bcicp)
   {
      bcicp = largs;
      while (*bcicp)
      {
         if (*bcicp == '_')
	    *bcicp = ' ';
         bcicp++;
      }
   }

#ifdef DEBUGMACRO
   fprintf (stderr, "   largs = %s\n", largs);
#endif

   /*
   ** Place processed field into macro template
   */

   switch (field)
   {
   case 0:
      strcat (mac->maclines[index]->label, largs);
      break;
   case 1:
      strcat (mac->maclines[index]->opcode, largs);
      if ((ocp = strchr (largs, ',')) != NULL)
         *ocp = '\0';
      if (!strchr (largs, '#') && ((op = oplookup (largs)) != NULL))
         mac->macopcode = op->opvalue;
      break;
   case 2:
      strcat (mac->maclines[index]->operand, largs);
      break;
   }
   return (cp);
}

/***********************************************************************
* p0macrostring - Process macro templates from a string array
***********************************************************************/

static void
p0macrostring (char **infd)
{
   MacDef *mac;
   char *token;
   char *cp;
   char *bp;
   int macindex;
   int tokentype;
   int val;
   int i;
   int done;
   int genmode;
   int oldtermparn;
   char term;
   char genline[MAXLINE];

   oldtermparn = termparn;
#ifdef DEBUGMACRO
   fprintf (stderr, "p0macrostring: termparn = %s\n",
	    termparn ? "TRUE" : "FALSE");
#endif
   termparn = TRUE;

   mac = &macdef[macrocount];
   memset (mac, '\0', sizeof (MacDef));
   macindex = 1;

   strcpy (genline, infd[0]);
   bp = genline;
   pscanbuf = bp;
   cp = cursym;

   while (*bp && !isspace(*bp)) *cp++ = *bp++;
   *cp = '\0';
   strcpy (mac->macname, cursym);

   bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
   if (strncmp (token, "MACRO", 5))
   {
      termparn = oldtermparn;
      pscanbuf = inbuf;
      return;
   }

   if (term == INDIRECTSYM)
   {
      bp++;
      mac->macindirect = TRUE;
   }

   /*
   ** Scan off the parameters
   */

   i = 0;
   do {
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      strcpy (mac->macargs[i], token);
      i++;
   } while (*bp && !isspace(term));
   mac->macargcount = i;

#ifdef DEBUGMACRO
   fprintf (stderr, "\nMACRO0: name = %s, indirect = %s\n",
      mac->macname, mac->macindirect ? "TRUE" : "FALSE");
   for (i = 0; i < mac->macargcount; i++)
      fprintf (stderr, "   arg%02d = %s\n", i, mac->macargs[i]);
#endif

   /*
   ** Read the source into the template until END[M].
   */

   done = FALSE;
   i = 0;
   while (!done)
   {
      /*
      ** Write source line to intermediate file with MACDEF mode
      */

      genmode = MACDEF;
      if (infd[macindex])
	 strcpy (genline, infd[macindex++]);
      else
      {
	 linenum = 1;
	 sprintf (errtmp, "Non terminated MACRO definition: %s",
		  mac->macname);
	 logerror (errtmp);
	 return;
      }

      if (genline[0] != COMMENTSYM)
      {
	 /*
	 ** If END, we're done
	 */

	 cp = genline;
	 if (strncmp (cp, "      ", 6))
	 {
	     while (*cp && isspace(*cp)) cp++;
	 }
	 while (*cp && !isspace(*cp)) cp++;
	 while (*cp && isspace(*cp)) cp++;
	 if (!strncmp (cp, "END", 3))
	    done = TRUE;

	 /*
	 ** Process template line
	 */

	 if (!done)
	 {

	    /*
	    ** Allocate line template
	    */

	    if (freemaclines)
	    {
	       mac->maclines[i] = freemaclines;
	       freemaclines = mac->maclines[i]->next;
	    }
	    else if ((mac->maclines[i] =
			   (MacLines *)DBG_MEMGET (sizeof(MacLines))) == NULL)
	    {
	       fprintf (stderr, "asm7090: Unable to allocate memory\n");
	       exit (ABORT);
	    }
	    memset (mac->maclines[i], '\0', sizeof(MacLines));

	    /*
	    ** If label field process label
	    */

	    cp = genline;
	    if (strncmp (cp, "      ", 6))
	    {
	       while (*cp && isspace (*cp)) cp++;
	       if (isalnum(*cp) || *cp == '.' || *cp == '(' || *cp == ')')
		  cp = p0mactokens (cp, 0, i, mac);
#ifdef DEBUGMACRO
	       fprintf (stderr, "   label = %s\n", mac->maclines[i]->label);
#endif
	    }

	    /*
	    ** Process opcode field
	    */

	    cp = p0mactokens (cp, 1, i, mac);
#ifdef DEBUGMACRO
	    fprintf (stderr, "   opcode = %s\n", mac->maclines[i]->opcode);
#endif

	    /*
	    ** Process operand field
	    */

	    cp = p0mactokens (cp, 2, i, mac);
#ifdef DEBUGMACRO
	    fprintf (stderr, "   operand = %s\n", mac->maclines[i]->operand);
#endif
	    i++;
	 }
      }
   }
   mac->maclinecount = i;
#ifdef DEBUGMACRO
   fprintf (stderr, "MACRO: %s\n", mac->macname);
   for (i = 0; i < mac->macargcount; i++)
      fprintf (stderr, "arg%02d: %s\n", i, mac->macargs[i]);
   for (i = 0; i < mac->maclinecount; i++)
      fprintf (stderr, "%-8.8s %-8.8s %s\n",
	    mac->maclines[i]->label,
	    mac->maclines[i]->opcode,
	    mac->maclines[i]->operand);
#endif
   macrocount++;

   termparn = oldtermparn;
   pscanbuf = inbuf;
}

/***********************************************************************
* p0macro - Process macro templates
***********************************************************************/

static int
p0macro (char *bp, int flag, FILE *infd, FILE *outfd)
{
   MacDef *mac;
   char *token;
   char *cp;
   int tokentype;
   int val;
   int i;
   int done;
   int genmode;
   int oldtermparn;
   int indup, dupcnt, dupin, dupout, dupstart;
   int nestcnt;
   char term;
   char genline[MAXLINE];

   oldtermparn = termparn;
#ifdef DEBUGPARN
   fprintf (stderr, "p0macro: termparn = %s\n", termparn ? "TRUE" : "FALSE");
#endif
#ifdef DEBUGMACRO
   fprintf (stderr, "p0macro: cursym[0] = %x\n", cursym[0]);
#endif
   termparn = TRUE;
   macscan = TRUE;
   indup = FALSE;

   mac = &macdef[macrocount];
   memset (mac, '\0', sizeof (MacDef));
   mac->macstartline = linenum;

   if (cursym[0])
   {
      strcpy (mac->macname, cursym);
      i = 0;
   }
   else
   {
      pscanbuf = genline;
      genmode = MACDEF;
      if (etcbuf[0])
      {
         strcpy (genline, etcbuf);
	 etcbuf[0] = '\0';
      }
      else
	 fgets (genline, MAXSRCLINE, infd);

      if (feof(infd))
      {
	 sprintf (errtmp, "%d: Non terminated MACRO definition: %s",
		  mac->macstartline, mac->macname);
	 logerror (errtmp);
#ifdef DEBUGMACRO
         fprintf (stderr, "1:%s: %s\n", cursym, errtmp);
#endif
	 return(-1);
      }

      writesource (outfd, FALSE, genmode, ++linenum, genline, "m00");

      bp = genline;
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      strcpy (mac->macargs[0], token);
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      strcpy (mac->macname, token);
      mac->macaltdef = TRUE;
      i = 1;
   }
   if (flag) mac->macindirect = TRUE;

   /*
   ** Scan off the parameters
   */

   do {
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGMACRO
      fprintf (stderr, "p0macro: token = %s, tokentype = %d, term = %c\n",
               token, tokentype, term);
#endif
      if (tokentype == SYM)
      {
	 strcpy (mac->macargs[i], token);
	 i++;
      }
   } while (*bp && !isspace(term));
   mac->macargcount = i;

#ifdef DEBUGMACRO
   fprintf (stderr, "\nMACRO0: name = %s\n", mac->macname);
   for (i = 0; i < mac->macargcount; i++)
      fprintf (stderr, "   arg%02d = %s\n", i, mac->macargs[i]);
#endif

   /*
   ** Read the source into the template until END[M].
   */

   done = FALSE;
   i = 0;
   nestcnt = 0;
   pscanbuf = genline;
   while (!done)
   {
      /*
      ** Write source line to intermediate file with MACDEF mode
      */

      genmode = MACDEF;
      if (etcbuf[0])
      {
         strcpy (genline, etcbuf);
	 etcbuf[0] = '\0';
      }
      else
	 fgets (genline, MAXSRCLINE, infd);

      if (feof(infd))
      {
	 sprintf (errtmp, "%d: Non terminated MACRO definition",
		  mac->macstartline);
	 logerror (errtmp);
#ifdef DEBUGMACRO
         fprintf (stderr, "2: %s: %s\n", cursym, errtmp);
#endif
	 return(-1);
      }

      writesource (outfd, FALSE, genmode, ++linenum, genline, "m01");

      if (genline[0] != COMMENTSYM)
      {
	 /*
	 ** If END[M], we're done
	 */

	 cp = genline;
	 if (strncmp (cp, "      ", 6))
	 {
	     while (*cp && isspace(*cp)) cp++;
	 }
	 while (*cp && !isspace(*cp)) cp++;
	 while (*cp && isspace(*cp)) cp++;
	 if (!strncmp (cp, "END", 3))
	 {
	    if (fapmode)
	    {
	       cp = genline;
	       pscanbuf = cp;
	       if (strncmp (cp, "      ", 6))
	       {
		  char *sp;
		  while (*cp && isspace (*cp)) cp++;
		  sp = cp;
		  if (isalnum(*cp) || *cp == '.')
		  {
		     char maclabel[MAXFIELDSIZE];

		     while (*cp && !isspace(*cp)) cp++;
		     strncpy (maclabel, sp, cp-sp);
		     maclabel[cp-sp] = '\0';
#ifdef DEBUGMACRO
		     fprintf (stderr, "   macname  = %s\n", mac->macname);
		     fprintf (stderr, "   maclabel = %s\n", maclabel);
#endif
		     if (nestcnt == 0 && strcmp (mac->macname, maclabel))
		     {
			logerror ("MACRO label mismatch");
		     }
		  }
	       }
	       else
	       {
#ifdef DEBUGMACRO
		  fprintf (stderr, "   nestcnt = %d -> 0\n", nestcnt);
#endif
		  if (nestcnt) nestcnt = 0;
		  cp = &genline[OPCSTART+3];
		  cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
		  if (tokentype != EOS)
		  {
		     sprintf (errtmp, "%d: Non terminated MACRO definition",
			      mac->macstartline);
		     logerror (errtmp);
#ifdef DEBUGMACRO
		     fprintf (stderr, "3: %s: %s\n", cursym, errtmp);
#endif
		  }
	       }
	       pscanbuf = genline;
	    }
	    else
	    {
	       if (strncmp (cp, "ENDM", 4))
	       {
		  sprintf (errtmp, "%d: Non terminated MACRO definition",
			   mac->macstartline);
		  logerror (errtmp);
#ifdef DEBUGMACRO
		  fprintf (stderr, "4: %s: %s\n", cursym, errtmp);
#endif
	       }
	       else
	       {
		  cp = &genline[OPCSTART+4];
		  cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
		  if (tokentype == SYM)
		  {
		     if (nestcnt == 0 && strcmp (mac->macname, token))
		     {
			logerror ("MACRO label mismatch");
		     }
		  }
	       }
	    }
	    if (nestcnt == 0)
	    {
#ifdef DEBUGMACRO
	       fprintf (stderr, "   nestcnt = %d == 0\n", nestcnt);
#endif
	       macscan = FALSE;
	       done = TRUE;
	    }
	    else
	    {
#ifdef DEBUGMACRO
	       fprintf (stderr, "   nestcnt = %d --\n", nestcnt);
#endif
	       nestcnt--;
	    }
	 }

	 /*
	 ** If MACRO, bump nest count
	 */

	 else if (!strncmp (cp, "MACRO", 5))
	 {
#ifdef DEBUGMACRO
	    fprintf (stderr, "   nestcnt = %d ++\n", nestcnt);
#endif
	    nestcnt++;
	 }

	 /*
	 ** Check for DUP in macro
	 */

	 else if (!strncmp (cp, "DUP", 3))
	 {
	    int junk;


	    /*
	    ** Scan off DUP input line count
	    */

	    bp = &genline[OPCSTART+3];
	    bp = exprscan (bp, &dupin, &term, &junk, 1, FALSE, 0);
	    if (dupin > MAXDUPLINES)
	    {
	       logerror ("Too many DUPlicate lines");
	    }
	    else
	    {
	       /*
	       ** Scan off DUP output line count
	       */

	       bp = exprscan (bp, &dupout, &term, &junk, 1, FALSE, 0);
	    }
	    indup = TRUE;
	    dupstart = i;
	    dupcnt = dupin;
	    continue;
	 }

	 /*
	 ** Process template line
	 */

	 if (!done)
	 {

	    /*
	    ** Allocate line template
	    */

	    if (freemaclines)
	    {
	       mac->maclines[i] = freemaclines;
	       freemaclines = mac->maclines[i]->next;
	    }
	    else if ((mac->maclines[i] =
			   (MacLines *)DBG_MEMGET (sizeof(MacLines))) == NULL)
	    {
	       fprintf (stderr, "asm7090: Unable to allocate memory\n");
	       exit (ABORT);
	    }
	    memset (mac->maclines[i], '\0', sizeof(MacLines));

	    /*
	    ** If label field process label
	    */

	    cp = genline;
	    pscanbuf = cp;
	    if (strncmp (cp, "      ", 6))
	    {
	       while (*cp && isspace (*cp)) cp++;
	       if (isalnum(*cp) || *cp == '.' || *cp == '(' || *cp == ')' ||
	           *cp == '$')
		  cp = p0mactokens (cp, 0, i, mac);
#ifdef DEBUGMACRO
	       fprintf (stderr, "   label = %s\n", mac->maclines[i]->label);
#endif
	    }
	    pscanbuf = genline;

	    /*
	    ** Process opcode field
	    */

	    if (strncmp (&genline[OPCSTART], "      ", 6))
	       cp = p0mactokens (cp, 1, i, mac);
	    else
	       mac->maclines[i]->opcode[0] = '\0';
#ifdef DEBUGMACRO
	    fprintf (stderr, "   opcode = %s\n", mac->maclines[i]->opcode);
#endif

	    /*
	    ** Process operand field
	    */

	    cp = p0mactokens (cp, 2, i, mac);
#ifdef DEBUGMACRO
	    fprintf (stderr, "   operand = %s\n", mac->maclines[i]->operand);
#endif
	    i++;
	    if (indup)
	    {
	       dupcnt--;
	       if (dupcnt == 0) 
	       {
		  int j, k, l;

		  /*
		  **  DUP lines in template
		  */

		  indup = FALSE;
		  for (k = 0; k < dupout-1; k++)
		  {
		     
		     j = dupstart;
		     for (l = 0; l < dupin; l++)
		     {
			if (freemaclines)
			{
			   mac->maclines[i] = freemaclines;
			   freemaclines = mac->maclines[i]->next;
			}
			else if ((mac->maclines[i] =
			     (MacLines *)DBG_MEMGET (sizeof(MacLines))) == NULL)
			{
			   fprintf (stderr,
				    "asm7090: Unable to allocate memory\n");
			   exit (ABORT);
			}
			memset (mac->maclines[i], '\0', sizeof(MacLines));

			strcpy (mac->maclines[i]->label,
				mac->maclines[j]->label);
			strcpy (mac->maclines[i]->opcode,
				mac->maclines[j]->opcode);
			strcpy (mac->maclines[i]->operand,
				mac->maclines[j]->operand);
			i++;
			j++;
		     }
		  }
	       }
	    }
	 }
      }
   }
   mac->maclinecount = i;
#ifdef DEBUGMACRO
   fprintf (stderr, "MACRO: %s\n", mac->macname);
   for (i = 0; i < mac->macargcount; i++)
      fprintf (stderr, "arg%02d: %s\n", i, mac->macargs[i]);
   for (i = 0; i < mac->maclinecount; i++)
      fprintf (stderr, "%-8.8s %-8.8s %s\n",
	    mac->maclines[i]->label,
	    mac->maclines[i]->opcode,
	    mac->maclines[i]->operand);
#endif
   macrocount++;

   termparn = oldtermparn;
   pscanbuf = inbuf;
   return (0);
}

/***********************************************************************
* p0call - Process CALL Pseudo operation code.
***********************************************************************/

static void
p0call (char *bp, int mode, FILE *outfd)
{
   char *token;
   char *cp;
   int tokentype;
   int val;
   int i;
   int genmode;
   int argcnt = 0;
   int retcnt = 0;
   int gencnt = 0;
   int genlinenum = 0;
   int calline;
   int oldtermparn;
   char term;
   char name[MAXSYMLEN+2];
   char lclargs[MAXMACARGS][MAXFIELDSIZE];
   char retargs[MAXMACARGS][MAXFIELDSIZE];
   char genline[MAXLINE];

   oldtermparn = termparn;
#ifdef DEBUGPARN
   fprintf (stderr, "p0call: CALL: termparn = %s\n",
	    termparn ? "TRUE" : "FALSE");
#endif
   termparn = TRUE;
   calline = linenum;

   /*
   ** Get name of called routine
   */

#ifdef DEBUGCALL
   fprintf (stderr, "p0CALL: operand = %s", bp);
#endif

   bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
   strcpy (name, token);
#ifdef DEBUGCALL
   fprintf (stderr, "   name = %s\n", token);
#endif

   /*
   ** If arguments specified, then scan them off 
   */

   if ((fapmode && term == ',') || term == '(')
   {
      if (term == '(')
	 bp++;
      cp = bp;
      while (*bp && *bp != ')' && !isspace (*bp)) bp++;
      if (!fapmode && *bp != ')')
      {
	 logerror ("Missing ')' in CALL");
	 termparn = oldtermparn;
	 return;
      }
      bp++;
      do {
	 token = cp;
	 while (*cp != ',' && *cp != ')' && !isspace(*cp)) cp++;
	 term = *cp;
	 *cp++ = '\0';
	 strcpy (lclargs[argcnt], token);
#ifdef DEBUGCALL
	 fprintf (stderr, "   arg[%d] = %s\n", argcnt, token);
#endif
	 argcnt++;
      } while (term != ')' && !isspace(term));
      if (!isspace(*bp)) goto GET_RETURNS;
   }

   /*
   ** Scan off return fields
   */

   else if (term == ',')
   {
GET_RETURNS:
      do {
	 if (*bp == '\'') goto GET_ID;
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 strcpy (retargs[retcnt], token);
#ifdef DEBUGCALL
	 fprintf (stderr, "   retarg[%d] = %s\n", retcnt, token);
#endif
	 retcnt++;
      } while (term == ',');
      if (term == '\'')
      {
   GET_ID:
	 bp++;
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGCALL
	 fprintf (stderr, "   id = %s\n", token);
#endif
	 calline = val;
      }
   }

   /*
   ** Expand the CALL
   */

   genmode = GENINST | mode;

   sprintf (genline, "%-6.6s %-7.7s %s,4\n",
	    "", "TSX", name);
   pc++;
   gencnt++;

   writesource (outfd, FALSE, genmode, ++genlinenum, genline, "c01");
   
   if (!fapmode)
   {
      sprintf (genline, "%-6.6s %-7.7s *+2+%d+%d,,%d\n",
	       "", "TXI", argcnt, retcnt, argcnt);
      pc++;
      gencnt++;
      writesource (outfd, FALSE, genmode, ++genlinenum, genline, "c02");
      sprintf (genline, "%-6.6s %-7.7s %d,,..LDIR\n",
	       "", "PZE", calline);
      pc++;
      gencnt++;
      writesource (outfd, FALSE, genmode, ++genlinenum, genline, "c03");
   }

   for (i = 0; i < argcnt; i++)
   {
      sprintf (genline, "%-6.6s %-7.7s %s\n",
	       "", fapmode ? "TSX" : "PZE", lclargs[i]);
      pc++;
      gencnt++;
      writesource (outfd, FALSE, genmode, ++genlinenum, genline, "c04");
   }

   for (i = retcnt; i > 0; i--)
   {
      sprintf (genline, "%-6.6s %-7.7s %s\n",
	       "", "TRA", retargs[i-1]);
      pc++;
      gencnt++;
      writesource (outfd, FALSE, genmode, ++genlinenum, genline, "c05");
   }

   if (fapmode && (cpumodel == 7096) && !nolnkdir && genlnkdir)
   {
      SymNode *s;

      sprintf (genline, "%-6.6s %-7.7s *+2,0,0\n",
	       "", "NTR");
      pc++;
      gencnt++;
      writesource (outfd, FALSE, genmode, ++genlinenum, genline, "c06");
      sprintf (genline, "%-6.6s %-7.7s .LKDIR,0,*-%d\n",
	       "", "PZE", gencnt);
      if (!(s = symlookup (".LKDIR", "", FALSE, FALSE)))
      {
	 s = symlookup (".LKDIR", "", TRUE, FALSE);
	 if (s)
	 {
	    s->flags |= (P0SYM | RELOCATABLE);
	    s->value = 0;
	 }
      }
      pc++;
      gencnt++;
      writesource (outfd, FALSE, genmode, ++genlinenum, genline, "c07");
   }

   termparn = oldtermparn;
}

/***********************************************************************
* p0pop - Process Pseudo operation codes.
***********************************************************************/

static int
p0pop (OpCode *op, int flag, char *bp, FILE *infd, FILE *outfd, int inmacro)
{
   SymNode *s;
   OpCode *addop;
   char *token;
   char *cp;
   int tokentype;
   int relocatable;
   int val;
   int i, j;
   int genmode;
   int savenmode;
   int boolrl;
   int boolr;
   int genlinenum;
   char term;
   char genline[MAXLINE];

   savenmode = FALSE;
   macron = FALSE;
   genlinenum = 0;
   switch (op->opvalue)
   {

   case ABS_T:			/* ABSoulute assembly (FAP mode) */
      if (fapmode)
      {
	 addext = FALSE;
         absolute = TRUE;
         absmod = TRUE;
      }
      break;

   case LBOOL_T:		/* Left BOOL */
      boolrl = TRUE;
      boolr = FALSE;
      goto PROCESS_BOOL;

   case RBOOL_T:		/* Right BOOL */
      boolrl = TRUE;
      boolr = TRUE;
      goto PROCESS_BOOL;

   case BOOL_T:			/* BOOL */
      boolrl = FALSE;
      boolr = FALSE;
PROCESS_BOOL:
      radix = 8;
      exprtype = BOOLEXPR | BOOLVALUE;
      bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);

      /*
      ** Set boolean value into symbol
      */

      if (cursym[0])
      {
	 if (fapmode)
	 {
	    if (headcount && (strlen(cursym) < MAXSYMLEN))
	    {
	       for (i = 0; i < headcount; i++)
	       {
		  char temp[32];

		  sprintf (temp, "%c", headtable[i]);
		  if (!(s = symlookup (cursym, temp, FALSE, FALSE)))
		  {
		     s = symlookup (cursym, temp, TRUE, FALSE);
		  }
		  if (s)
		  {
#ifdef DEBUGBOOLSYM
		     fprintf (stderr,
		 "asmpass1: BOOL sym = %s, val = %o, lrbool = %s, rbool = %s\n",
			      cursym, val, 
			      boolrl ? "TRUE" : "FALSE",
			      boolr ? "TRUE" : "FALSE");
#endif
		     s->value = val;
		     s->flags = P0SYM | BOOL; 
		     if (boolrl) s->flags |= LRBOOL;
		     if (boolr) s->flags |= RBOOL;
		  }
	       }
	       s = NULL;
	    }
	    else
	    {
	       if (!(s = symlookup (cursym, "", FALSE, FALSE)))
	       {
		  s = symlookup (cursym, "", TRUE, FALSE);
	       }
	    }
	 }
	 else
	 {
	    if (!(s = symlookup (cursym, qualtable[qualindex], FALSE, FALSE)))
	    {
	       s = symlookup (cursym, qualtable[qualindex], TRUE, FALSE);
	    }
	 }

	 if (s)
	 {
#ifdef DEBUGBOOLSYM
            fprintf (stderr,
	       "asmpass1: BOOL sym = %s, val = %o, lrbool = %s, rbool = %s\n",
		  cursym, val, 
		  boolrl ? "TRUE" : "FALSE",
		  boolr ? "TRUE" : "FALSE");
#endif
	    s->value = val;
	    s->flags = P0SYM | BOOL; 
	    if (boolrl) s->flags |= LRBOOL;
	    if (boolr) s->flags |= RBOOL;
	 }
      }
      radix = 10;
      break;

   case CALL_T:			/* Process CALL */
      p0call (bp, CALL, outfd);
      break;

   case DUP_T:			/* DUPlicate lines */
      {
         int dupin, dupout;
	 char *duplines[MAXDUPLINES];

	 genmode = DUPINST;

	 /*
	 ** Scan off DUP input line count
	 */

	 bp = exprscan (bp, &dupin, &term, &relocatable, 1, FALSE, 0);
	 if (dupin > MAXDUPLINES)
	 {
	    logerror ("Too many DUPlicate lines");
	 }
	 else
	 {
	    /*
	    ** Scan off DUP output line count
	    */

	    bp = exprscan (bp, &dupout, &term, &relocatable, 1, FALSE, 0);

	    /*
	    ** Read the DUP input lines
	    */

	    for (i = 0; i < dupin; i++)
	    {
	       if (readrmtseqflag)
	       {
		  rmtseqget (&genmode, genline);
		  genmode |= DUPINST;
	       }
	       else if (etcbuf[0])
	       {
		  strcpy (genline, etcbuf);
		  etcbuf[0] = '\0';
	       }
	       else
		  fgets (genline, MAXSRCLINE, infd);

	       if (feof (infd))
	       {
		  errcount++;
	          fprintf (stderr, "asm7090: %d: EOF processing DUP\n",
			   linenum);
		  return (TRUE);
	       }
	       if ((duplines[i] = (char *) DBG_MEMGET (MAXSRCLINE)) == NULL)
	       {
		  fprintf (stderr, "asm7090: Unable to allocate memory\n");
		  exit (ABORT);
	       }
	       strcpy (duplines[i], genline);
	    }

	    /*
	    ** Write out the DUP'd lines
	    */

	    for (i = 0; i < dupout; i++)
	    {
	       for (j = 0; j < dupin; j++)
	       {
		  if (!rmtflag) pc++;
		  writesource (outfd, rmtflag, genmode, ++genlinenum,
			       duplines[j], "p06");
		  if (strncmp (duplines[j], "      ", 6))
		  {
		     strncpy (duplines[j], "       ", 6);
		  }
	       }
	       if (rmtflag) break;
	    }
	    for (i = 0; i < dupin; i++)
	       DBG_MEMREL (duplines[i]);
	 }
      }
      break;

   case END_T:			/* END */

      if (inmacro) break;

      /*
      ** Because of ETC read ahead we reset the input file back one.
      */

      if (!eofflg && etcbuf[0])
      {
	 if (fseek (infd, curpos, SEEK_SET) < 0)
	    perror ("asm7090: Can't reset input temp file");
      }
      if (fapmode && rmthead)
      {
         readrmtseqflag = TRUE;
	 rmtseqnum = linenum;
	 linenum = 0;
      }

      return (TRUE);

   case ENDQ_T:			/* ENDQ */
      qualindex --;
      if (qualindex < 0) qualindex = 0;
      break;

   case ENT_T:			/* ENTRY */
      /* CTSS version of FAP generates the LINKAGE DIRECTOR */
      if (fapmode && (cpumodel == 7096) && !nolnkdir && !genlnkdir)
      {
	  bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	  memset (lnkdirsym, 0, sizeof(lnkdirsym));
	  if (tokentype == SYM)
	  {
	     strcpy (lnkdirsym, token);
	     genlnkdir = TRUE;
	     pc += 2;
	  }
      }
      break;

   case EQU_T:			/* EQU */
      bp = exprscan (bp, &val, &term, &relocatable, 1, TRUE, 0);

DO_EQU:
      if (cursym[0])
      {
#ifdef DEBUGEQU
	 fprintf (stderr, "p0pop: EQU: cursym = '%s', val = %05o\n",
	       cursym, val);
#endif
	 if (fapmode)
	 {
	    if (headcount && (strlen(cursym) < MAXSYMLEN))
	    {
	       int i;

	       for (i = 0; i < headcount; i++)
	       {
		  char temp[32];

		  sprintf (temp, "%c", headtable[i]);
#ifdef DEBUGEQU
		  fprintf (stderr, "   cursym = '%s', head = '%s' \n",
			   cursym, temp);
#endif
		  s = symlookup (cursym, temp, TRUE, FALSE);
		  if (s)
		  {
		     s->flags |= P0SYM | EQUVAR;
		     s->value = val;
		     if (relocatable) s->flags |= RELOCATABLE;
		     else             s->flags &= ~RELOCATABLE;
		  }
	       }
	       s = NULL;
	    }
	    else
	    {
#ifdef DEBUGEQU
	       fprintf (stderr, "   cursym = '%s', head = '%s' \n", cursym, "");
#endif
	       s = symlookup (cursym, "", TRUE, FALSE);
	    }
	 }
	 else
	 {
#ifdef DEBUGEQU
	    fprintf (stderr, "   cursym = '%s', qual = '%s' \n",
		    cursym, qualtable[qualindex]);
#endif
	    s = symlookup (cursym, qualtable[qualindex], TRUE, FALSE);
	 }

	 if (s)
	 {
	    s->flags |= P0SYM | EQUVAR;
	    s->value = val;
	    if (relocatable) s->flags |= RELOCATABLE;
	    else             s->flags &= ~RELOCATABLE;
	 }
      }
      break;

   case EVEN_T:			/* EVEN */
      if (pc & 00001) pc++;
      break;

   case GOTO_T:			/* GOTO */
      /*
      ** In this pass, only process if in a macro
      */
      
      if (inmacro)
      {
	 /*
	 ** Scan off target symbol
	 */

	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 if (strlen(token) > MAXSYMLEN) token[MAXSYMLEN] = '\0';
	 strcpy (gotosym, token);
#ifdef DEBUGGOTO
         fprintf (stderr, "asmpass0: GOTO: gotosym = '%s'\n", gotosym);
#endif
	 gotoskip = TRUE;
      }
      break;

   case HEAD_T:			/* HEAD */
      headcount = 0;
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 headtable[headcount++] = token[0];
	 if (headcount >= MAXHEADS)
	 {
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
      /*
      ** In this pass, only process if in a macro
      */
      
      if (fapmode)
      {
	 int foundexpr = FALSE;
	 int alldigits = TRUE;
	 char tok1[TOKENLEN];
	 char tok2[TOKENLEN];

#ifdef DEBUGP0IFF
         fprintf (stderr, "p0IFF: bp = %s", bp);
#endif

	 tok1[0] = '\0';
	 tok2[0] = '\0';
	 asmskip = 0;
	 for (cp = bp; *cp; cp++)
	 {
	    if (*cp == ',' || *cp <= ' ')
	       break;
	    if (*cp == '+' || *cp == '*' || *cp == '/' || *cp == '-')
	    {
	       alldigits = FALSE;
	       foundexpr = TRUE;
	       break;
	    }
	    if (!isdigit(*cp))
	       alldigits = FALSE;
	 }
	 if (alldigits || foundexpr)
	 {
	     bp = exprscan (bp, &val, &term, &relocatable, 1, TRUE, 0);
#ifdef DEBUGP0IFF
             fprintf (stderr, "p0IFF: val = %d\n", val);
#endif
	 }
	 else
	 {
	     cp = bp;
	     bp = exprscan (bp, &val, &term, &relocatable, 1, TRUE, 0);
	     if (term == ' ')
		*(bp) = '\0';
	     else
		*(bp-1) = '\0';
	     if (*cp == '$') cp++;
#ifdef DEBUGP0IFF
             fprintf (stderr, "p0IFF: term = %X(%c), val = %d, sym = %s\n",
	     		term, term, val, cp);
#endif
	     /*
	     ** Look up the symbol, If undefined set val == 0.
	     ** If defined and is a SET or EQU variable use the value from
	     ** exprscan.
	     ** Otherwise, set val == 1 to indicate as defined.
	     */

	     s = symlookup (cp, "", FALSE, FALSE);
	     if (s)
	     {
	        if (!(s->flags & (SETVAR | EQUVAR)))
		{
		   val = 1;
		}
	     }
	     else
	     {
		val = 0;
	     }
	 }

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
#ifdef DEBUGP0IFF
	 fprintf (stderr,
	    "p0IFF: asmskip = %d, val = %d, i = %d, tok1 = '%s', tok2 = '%s'\n",
		asmskip, val, i, tok1, tok2);
#endif
      }
      else if (inmacro)
      {
	 /*
	 ** Scan the conditional expression and get result
	 */

	 bp = condexpr (bp, &val, &term);
	 if (val >= 0)
	 {
	    int skip;

#ifdef DEBUGIF
	    fprintf (stderr, "p0IFF: val = %d, ifcont = %s\n",
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
		     asmskip = 1;
	       }
	       else if (ifrel == IFOR)
	       {
		  if (ifskip && skip)
		     asmskip = 1;
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
#ifdef DEBUGIF
	       fprintf (stderr, "   ifskip = %s\n",
			ifskip ? "TRUE" : "FALSE");
#endif
	    }

	    /*
	    ** Neither, just do it
	    */

	    else if (skip)
	    {
	       asmskip = 1;
#ifdef DEBUGIF
	       fprintf (stderr, "   asmskip = %d\n", asmskip);
#endif
	    }
	 }
      }
      break;

   case IFT_T:			/* IF True */
      /*
      ** In this pass, only process if in a macro
      */

      if (inmacro)
      {

	 /*
	 ** Scan the conditional expression and get result
	 */

	 bp = condexpr (bp, &val, &term);
	 if (val >= 0)
	 {
	    int skip;

#ifdef DEBUGIF
	    fprintf (stderr, "p0IFT: val = %d, ifcont = %s\n",
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
		     asmskip = 1;
	       }
	       else if (ifrel == IFOR)
	       {
		  if (ifskip && skip)
		     asmskip = 1;
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
#ifdef DEBUGIF
	       fprintf (stderr, "   ifskip = %s\n", ifskip ? "TRUE" : "FALSE");
#endif
	    }

	    /*
	    ** Neither, just do it
	    */

	    else if (skip)
	    {
	       asmskip = 1;
#ifdef DEBUGIF
	       fprintf (stderr, "   asmskip = %d\n", asmskip);
#endif
	    }
	 }
      }
      break;

   case INSERT_T:
      tmpfd = opencopy (bp, tmpfd, etcbuf);
      break;

   case LBL_T:			/* LBL */
      if (!deckname[0])
      {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 if (strlen(token) > MAXLBLLEN) token[MAXLBLLEN] = '\0';
	 strcpy (deckname, token);
	 strcpy (lbl, token);
      }
      break;

   case LDIR_T:			/* Linkage DIRector */
      genmode = GENINST;
      sprintf (genline, "%-6.6s %-7.7s %s\n", "..LDIR", "PZE", "**");
      pc++;
      writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p07");
      sprintf (genline, "%-6.6s %-7.7s 1,%s\n", "", "BCI", deckname);
      pc++;
      writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p08");
      break;

   case LINK_T:			/* LINK */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (tokentype == SYM)
      {
         if (!strcmp (token, "ON"))
	 {
	    genlnkdir = TRUE;
	    nolnkdir = FALSE;
	 }
	 else
	 {
	    genlnkdir = FALSE;
	    nolnkdir = TRUE;
	 }
      }
      break;

   case MACRON_T:		/* Macro */
      macron = TRUE;
   case MACRO_T:		/* Macro */
      if (p0macro (bp, flag, infd, outfd))
      {
	 return (TRUE);
      }
      break;

   case NOLNK_T:		/* NO LiNKage director */
      genlnkdir = FALSE;
      nolnkdir = TRUE;
      break;

   case NULL_T:			/* NULL */
      val = pc;
      relocatable = !absmod;
      goto DO_EQU;

   case OPSYN_T:		/* OP SYNonym */
      /*
      ** Scan off opcode
      */

      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGOPSYN
      fprintf (stderr,
      "OPSYN0: cursym = %s, token = %s, tokentype = %d, val = %o, term = %c\n",
	      cursym, token, tokentype, val, term);
#endif
      /*
      ** Delete any previous definition and add
      */

      if ((addop = oplookup (token)) != NULL)
      {
	 opdel (cursym);
	 opadd (cursym, addop->opvalue, addop->opmod, addop->optype);
      }

      /*
      ** Synonym opcode not found, error
      */

      else
      {
	 MacDef *oldmac;
	 MacDef *newmac;

	 if ((oldmac = maclookup (token)) != NULL)
	 {
	    newmac = &macdef[macrocount];
	    memcpy (newmac, oldmac, sizeof (MacDef));
	    strcpy (newmac->macname, cursym);
	    macrocount++;
	 }
	 else
	 {
	    sprintf (errtmp, "Invalid opcode for OPSYN: %s", token);
	    logerror (errtmp);
	 }
      }
      break;

   case QUAL_T:			/* QUALified section */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (strlen(token) > MAXSYMLEN) token[MAXSYMLEN] = '\0';
      qualindex ++;
      strcpy (qualtable[qualindex], token);
      break;

   case RETURN_T:		/* RETURN */
      {
	 char name[MAXSYMLEN+2];

	 genmode = GENINST | RETURN;
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 strcpy (name, token);
	 if (term == ',')
	 {
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	    sprintf (genline, "%-6.6s %-7.7s %d,4\n", "", "AXT", val);
	    pc++;
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p09");
	    sprintf (genline, "%-6.6s %-7.7s %s,4\n", "", "SXD", name);
	    pc++;
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p10");
	 }
	 sprintf (genline, "%-6.6s %-7.7s %s+1\n", "", "TRA", name);
	 pc++;
	 writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p11");
      }
      break;

   case RMT_T:			/* ReMoTe sequence */
      cp = bp;
      cp = tokscan (cp, &token, &tokentype, &val, &term, FALSE);
      if (tokentype != EOS)
      {
	 writesource (outfd, FALSE, 0, ++linenum, srcbuf, "r01");
	 readrmtseqflag = TRUE;
	 rmtseqnum = linenum;
	 linenum = 0;
      }
      else
      {
	 rmtflag = rmtflag ? FALSE : TRUE;
      }
      break;

   case SAVEN_T:		/* SAVEN cpu context */
      savenmode = TRUE;

   case SAVE_T:			/* SAVE cpu context */
      {
	 int oldtermparn;
	 int regs[8];
	 int modes[4];
	 char label[MAXSYMLEN+2];
	 char operand[MAXFIELDSIZE];

	 oldtermparn = termparn;
#ifdef DEBUGPARN
	 fprintf (stderr, "p0pop: SAVE: termparn = %s\n",
		  termparn ? "TRUE" : "FALSE");
#endif
	 termparn = TRUE;
	 for (i = 0; i < 8; i++) regs[i] = FALSE;
	 for (i = 0; i < 4; i++) modes[i] = FALSE;

	 /*
	 ** Scan off SAVE args and regs. Order regs.
	 */

	 do
	 {
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	    if (term == '(' || term == ')') term = ',';
	    if (tokentype == DECNUM)
	    {
	       if (val < 1 || val > 7)
	       {
		  sprintf (errtmp, "Invalid register for %s: %d",
			   savenmode ? "SAVEN" : "SAVE", val);
		  logerror (errtmp);
		  termparn = oldtermparn;
		  return(FALSE);
	       }
	       regs[val] = TRUE;
	    }
	    else if (tokentype == SYM)
	    {
	       if (!strcmp (token, "I")) modes[0] = TRUE;
	       else if (!strcmp (token, "D")) modes[1] = TRUE;
	       else if (!strcmp (token, "E")) modes[2] = TRUE;
	       else 
	       {
		  sprintf (errtmp, "Invalid mode for %s: %s",
			   savenmode ? "SAVEN" : "SAVE", token);
		  logerror (errtmp);
		  termparn = oldtermparn;
		  return(FALSE);
	       }
	    }
	 } while (term == ',');

	 /*
	 ** Expand the SAVE[N] 
	 */

	 genmode = GENINST | SAVE;

	 /*
	 ** If symbol, generate ENTRY
	 */

	 if (cursym[0])
	 {
	    if (!savenmode && !absmod) /* No ENTRY for SAVEN mode */
	    {
	       sprintf (genline, "%-6.6s %-7.7s %s\n", "", "ENTRY", cursym);
	       writesource (outfd, FALSE, genmode, ++genlinenum, genline,
			    "p12");
	    }
	    sprintf (operand, "..%04d,,0", lblgennum+3);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", cursym, "TXI", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p13");
	    pc++;
	 }
	 else 
	 {
	    sprintf (errtmp, "Label required for %s",
		     savenmode ? "SAVEN" : "SAVE");
	    logerror (errtmp);
	    return(FALSE);
	 }

	 /*
	 ** Generate E mode, Error return
	 */

	 if (modes[2])
	 {
	    sprintf (operand, "%s,%d", cursym, 4);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "LDC", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p14");
	    pc++;
	    sprintf (operand, "%s,%d", "*+5", 4);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "SXD", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p15");
	    pc++;
	    sprintf (operand, "..%04d,%d", lblgennum+1, 4);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "LAC", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p16");
	    pc++;
	    sprintf (operand, "%s,%d", "*+1,4", 1);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "TXI", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p17");
	    pc++;
	    sprintf (operand, "%s,%d", "*+1", 4);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "SXA", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p18");
	    pc++;
	    sprintf (operand, "%s,%d", "**", 4);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "LXA", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p19");
	    pc++;
	    sprintf (operand, "%s,%d", "*+1,4", 0);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "TXI", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p20");
	    pc++;
	    sprintf (operand, "..%04d,%d", lblgennum+2, 4);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "SXA", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p21");
	    pc++;
	 }

	 /*
	 ** Generate index register loads
	 */

	 for (i = 7; i > 0; i--)
	 {
	       if (i == 4) continue;
	       if (regs[i])
	       {
		  sprintf (operand, "%s,%d", "**", i);
		  sprintf (genline, "%-6.6s %-7.7s %s\n", "", "AXT", operand);
		  writesource (outfd, FALSE, genmode, ++genlinenum,
		  		genline, "p22");
		  pc++;
	       }
	 }
	 
	 sprintf (label, "..%04d", lblgennum+1);
	 sprintf (operand, "%s,%d", "**", 4);
	 sprintf (genline, "%-6.6s %-7.7s %s\n", label, "AXT", operand);
	 writesource (outfd, FALSE, genmode, ++genlinenum, genline, "p23");
	 pc++;
	 
	 /* 
	 ** Generate Indicator load
	 */

	 if (modes[0])
	 {
	    sprintf (operand, "..%04d+1", lblgennum+2);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "LDI", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum,
	    		genline, "p24");
	    pc++;
	 }

	 /*
	 ** Generate Trap enables
	 */

	 if (modes[1])
	 {
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "NZT", ".TRPSW");
	    writesource (outfd, FALSE, genmode, ++genlinenum,
	    		genline, "p25");
	    pc++;
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "ENB*", ".TRAPX");
	    writesource (outfd, FALSE, genmode, ++genlinenum,
	    		genline, "p26");
	    pc++;
	 }
	 sprintf (label, "..%04d", lblgennum+2);
	 if (modes[2])
	 {
	    strcpy (operand, "**");
	 }
	 else
	 {
	    strcpy (operand, "1,4");
	 }
	 sprintf (genline, "%-6.6s %-7.7s %s\n", label, "TRA", operand);
	 writesource (outfd, FALSE, genmode, ++genlinenum,
	 		genline, "p27");
	 pc++;

	 if (modes[0])
	 {
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "PZE", "**");
	    writesource (outfd, FALSE, genmode, ++genlinenum,
	    		genline, "p28");
	    pc++;
	 }
	 sprintf (label, "..%04d", lblgennum+3);
	 sprintf (genline, "%-6.6s %-7.7s %s\n", label, "EQU", "*");
	 writesource (outfd, FALSE, genmode, ++genlinenum,
	 		genline, "p29");
	 pc++;
	 if (modes[1])
	 {
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "XEC", "SYSDSB");
	    writesource (outfd, FALSE, genmode, ++genlinenum,
	    		genline, "p30");
	    pc++;
	 }

	 /*
	 ** Generate Indicator save
	 */

	 if (modes[0])
	 {
	    sprintf (operand, "..%04d+1", lblgennum+2);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "STI", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum,
	    		genline, "p31");
	    pc++;
	 }

	 /*
	 ** Generate Trap save
	 */

	 if (modes[2])
	 {
	    sprintf (operand, "%s,%d", cursym, 0);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "SXD", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum,
	    		genline, "p32");
	    pc++;
	 }
	 sprintf (operand, "%s,%d", "SYSLOC", 4);
	 sprintf (genline, "%-6.6s %-7.7s %s\n", "", "SXA", operand);
	 writesource (outfd, FALSE, genmode, ++genlinenum,
	 		genline, "p33");
	 pc++;

	 /*
	 ** Generate Linkage Director, if not SAVEN
	 */

	 if (!savenmode)
	 {
	    sprintf (operand, "%s,%d", "..LDIR", 4);
	    sprintf (genline, "%-6.6s %-7.7s %s\n", "", "SXA", operand);
	    writesource (outfd, FALSE, genmode, ++genlinenum,
	    		genline, "p34");
	    pc++;
	 }

	 /*
	 ** Generate save registers
	 */

	 sprintf (operand, "..%04d,%d", lblgennum+1, 4);
	 sprintf (genline, "%-6.6s %-7.7s %s\n", "", "SXA", operand);
	 writesource (outfd, FALSE, genmode, ++genlinenum,
	 		genline, "p35");
	 pc++;
	 j = 1;
	 for (i = 1; i < 8; i++)
	 {
	    if (i == 4) continue;
	    if (regs[i])
	    {
	       sprintf (operand, "..%04d-%d,%d", lblgennum+1, j, i);
	       sprintf (genline, "%-6.6s %-7.7s %s\n", "", "SXA", operand);
	       writesource (outfd, FALSE, genmode, ++genlinenum,
	       		genline, "p36");
	       pc++;
	       j++;
	    }
	 }
	 lblgennum += 3;
	 termparn = oldtermparn;
      }
      break;

   case SET_T:			/* SET value immediately */
      bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      if (cursym[0])
      {
	 if (!(s = symlookup (cursym, "", FALSE, FALSE)))
	    s = symlookup (cursym, "", TRUE, FALSE);
	 if (s)
	 {
	    s->value = val;
	    s->flags = P0SYM | SETVAR;
	 }
      }
      break;

   case SST_T:
      monsym = TRUE;
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGMONSYM
      fprintf (stderr, "SST %s\n", token);
#endif
      definemonsyms(0);
      if (!strcmp (token, "FORTRAN"))
      {
	 definemonsyms(1);
      }
      break;

   case TAPENO_T:		/* TAPE Number */
      while (*bp && isspace (*bp)) bp++;
#ifdef DEBUGTAPENO
      fprintf (stderr, "p0TAPENO: bp = %s", bp);
#endif
      if (*bp >= 'A' && *bp <= 'H')
      {
	 val = ((*bp - 'A' + 1) << 9) | 0200;
	 bp++;
	 if (isdigit (*bp))
	 {
	    j = *bp - '0';
	    bp++;
	    if (isdigit (*bp))
	    {
	       j = (j * 10) + (*bp - '0');
	       bp++;
	    }
	    val |= j;
	    if (*bp) switch (*bp)
	    {
	       case 'B':
	       case 'H':
		  val |= 020;
		  break;
	       default: ;
	    }
#ifdef DEBUGTAPENO
	    fprintf (stderr, "   val = %05o\n", val);
#endif
	    if (cursym[0])
	    {
	       if (!(s = symlookup (cursym, "", FALSE, FALSE)))
		  s = symlookup (cursym, "", TRUE, FALSE);
	       if (s)
	       {
		  s->value = val;
		  s->flags = P0SYM | TAPENO;
	       }
	    }

	 }
      }
      break;

   default: ;
   }

   return (FALSE);
}

/***********************************************************************
* p0expand - Expand macros
***********************************************************************/

static void
p0expand (MacDef *mac, int flag, char *bp, FILE *infd, FILE *outfd)
{
   SymNode *sym;
   OpCode *op;
   MacDef *lclmac;
   char *token;
   char *cp;
   char *ocp;
   int tokentype;
   int inirp;
   int doirp;
   int irpline;
   int irpnum;
   int val;
   int i, j;
   int done;
   int genmode;
   int lclflag;
   int argcnt;
   int oldtermparn;
   int nestndx;
   int noexp;
   char term;
   char genline[MAXLINE];
   char lclargs[MAXMACARGS][MAXFIELDSIZE*2];
   char irpargs[MAXFIELDSIZE*2];
   char irparg[MAXFIELDSIZE];
   char *nestmac[MAXMACROLINES];


#ifdef DEBUGMACROEXP
   fprintf (stderr, "p0expand: macro = %s, args = %s", mac->macname, bp);
#endif

   oldtermparn = termparn;
#ifdef DEBUGPARN
   fprintf (stderr, "p0expand: termparn = %s\n", termparn ? "TRUE" : "FALSE");
#endif
   termparn = TRUE;

   for (i = 0; i < MAXMACARGS; i++)
   {
      lclargs[i][0] = '\0';
   }

   /*
   ** If an indirect Macro and flag is set, the first arg is "*".
   */

   argcnt = 0;
   if (mac->macindirect)
   {
      if (flag)
	 strcpy (lclargs[argcnt], "*");
      argcnt++;
   }

   /*
   ** Scan off MACRO arguments
   */

   if (mac->macaltdef)
   {
      strcpy (lclargs[argcnt], cursym);
      argcnt++;
   }

   do {
      char tok[MAXLINE];

      ocp = bp;
      tok[0] = '\0';
      token = tok;
      while (*bp && !isspace(*bp) && *bp != ',' && *bp != '(')
      {
	  *token++ = *bp++;
      }
      *token = '\0';
      token = tok;
      term = *bp++;
#ifdef DEBUGMACROEXP
      fprintf (stderr, "   token = %s, term = %c(%x)\n",
	       token, term, term);
#endif
      if (token[0] == '\0' && term == '(')
      {
	 ocp = bp;
	 i = 1;
         while (*bp && i > 0/* && !isspace(*bp)*/)
	 {
	    if (*bp == '(')
	    {
	       i++;
	       bp++;
	    }
	    else if (*bp == ')')
	    {
	       i--;
	       if (i)
		  bp++;
	    }
	    else bp++;
	 }
#ifdef DEBUGMACROEXP
         fprintf (stderr, "   *bp = %c(%x) %s, pocp = %s\n", *bp, *bp, bp, ocp);
#endif
	 term = *bp;
	 *bp++ = '\0';
	 strcat (lclargs[argcnt], ocp);
	 if (term != ')' || *bp == ',')
	    term = *bp;
	 if (term == ',') bp++;
	 else if (isalnum(term) || term == '.' || term == ')')
	    term = ',';
      }
      else
      {
	 strcat (lclargs[argcnt], token);
	 if (term == '(')
	 {
	     bp--;
	     argcnt++;
	 }
      }
      if (isspace (term) || term == ',') argcnt++;
   } while (!isspace(term));

   termparn = oldtermparn;

   /* If CTSS FAP mode and 1st args start with a "$", assume not
      a TIA external */
   if (!absolute && fapmode && (cpumodel == 7096) && lclargs[0][0] == '$')
   {
      strcpy (genline, &lclargs[0][1]);
      if (!(sym = symlookup (genline, "", FALSE, FALSE)))
      {
	 sym = symlookup (genline, "", TRUE, FALSE);
      }
#ifdef DEBUGMACROEXP
      fprintf (stderr, "  FORCE External: sym = %s\n", genline);
#endif
      if (sym)
      {
	 sym->flags = P0SYM | RELOCATABLE | EXTERNAL;
	 sym->value = 0;
      }
   }

   /*
   ** Go through MACRO and fill in missing labels with generated labels.
   */

   for (i = 0; i < mac->maclinecount; i++)
   {
      char genindex[3];

      cp = mac->maclines[i]->label;
      if ((ocp = strchr(cp, '#')) != NULL)
      {
	 ocp++;
	 strncpy (genindex, ocp, 2);
	 genindex[2] = '\0';
	 j = atoi (genindex);
	 if (!lclargs[j][0])
	 {
	    sprintf (lclargs[j], "..%04d", lblgennum);
	    lblgennum++;
	 }
      }
   }

#ifdef DEBUGMACROEXP
   for (i = 0; i < argcnt; i++)
      fprintf (stderr, "   arg%02d = %s\n", i, lclargs[i]);
#endif

   /*
   ** Go through MACRO template and replace parameters with the
   ** user arguments.
   */

   nestndx = 0;
   inirp = FALSE;
   noexp = FALSE;
   doirp = TRUE;

   for (i = 0; i < mac->maclinecount; i++)
   {
      char genlabel[MAXFIELDSIZE];
      char genopcode[MAXFIELDSIZE];
      char genoperand[MAXFIELDSIZE];
      char genindex[3];


      /*
      ** Macro is nested, replace parameters and pass on
      */

      genmode = 0;
      genlabel[0] = '\0';
      genopcode[0] = '\0';
      genoperand[0] = '\0';

      if (mac->macnested) 
      {
         if ((nestmac[nestndx] = (char *)DBG_MEMGET (MAXLINE)) == NULL)
	 {
	    fprintf (stderr, "asm7090: Unable to allocate memory\n");
	    exit (ABORT);
	 }
      }

      /*
      ** Process label field
      */

      done = FALSE;
      cp = mac->maclines[i]->label;

      while (*cp && !done)
      {
	 if ((ocp = strchr(cp, '#')) != NULL)
	 {
	    if (ocp > cp)
	       strncat (genlabel, cp, ocp-cp);
	    ocp++;
	    strncpy (genindex, ocp, 2);
	    ocp += 2;
	    cp = ocp;
	    genindex[2] = '\0';
	    j = atoi (genindex);
	    if (inirp && j == irpnum)
	    {
	       strcat(genlabel, irparg);
	    }
	    else
	    {
	       strcat (genlabel, lclargs[j]);
	    }
	 }
	 else
	 {
	    strcat (genlabel, cp);
	    done = TRUE;
	 }
      }
      if (strlen(genlabel) > MAXSYMLEN) genlabel[MAXSYMLEN] = '\0';
      strcpy (cursym, genlabel);
#ifdef DEBUGMACROEXP
      fprintf (stderr, "   genlabel = %s\n", genlabel);
#endif

      /*
      ** Process opcode field
      */

      done = FALSE;
      cp = mac->maclines[i]->opcode;

      while (*cp && !done)
      {
	 if ((ocp = strchr(cp, '#')) != NULL)
	 {
	    if (ocp > cp)
	       strncat (genopcode, cp, ocp-cp);
	    ocp++;
	    strncpy (genindex, ocp, 2);
	    ocp += 2;
	    cp = ocp;
	    genindex[2] = '\0';
	    j = atoi (genindex);
	    if (inirp && j == irpnum)
	    {
	       strcat(genopcode, irparg);
	    }
	    else
	    {
	       strcat (genopcode, lclargs[j]);
	    }
	 }
	 else
	 {
	    strcat (genopcode, cp);
	    done = TRUE;
	 }
      }
#ifdef DEBUGMACROEXP
      fprintf (stderr, "   genopcode = %s\n", genopcode);
#endif

      /*
      ** Process operand field
      */

      /* If the opcode contains a blank, prepend the operand with it */
      if ((ocp = strchr (genopcode, ' ')) != NULL)
      {
         *ocp++ = '\0'; 
	 strcat (genoperand, ocp);
	 strcat (genoperand, ",");
      }
      /* If opcode has an embedded comma, set it null and copy remainder
         to the operand. */
      else if ((ocp = strchr (genopcode, ',')) != NULL)
      {
	 *ocp++ = '\0';
	 strcat (genoperand, ocp);
	 strcat (genoperand, ",");
      }

      done = FALSE;
      cp = mac->maclines[i]->operand;

      while (*cp && !done)
      {
	 if ((ocp = strchr(cp, '#')) != NULL)
	 {
	    if (ocp > cp)
	       strncat (genoperand, cp, ocp-cp);
	    ocp++;
	    strncpy (genindex, ocp, 2);
	    ocp += 2;
	    cp = ocp;
	    genindex[2] = '\0';
	    j = atoi (genindex);
	    if (inirp && j == irpnum)
	    {
	       strcat(genoperand, irparg);
	    }
	    else
	    {
	       strcat (genoperand, lclargs[j]);
	    }
	 }
	 else
	 {
	    strcat (genoperand, cp);
	    done = TRUE;
	 }
      }
      strcat (genoperand, "\n");
#ifdef DEBUGMACROEXP
      fprintf (stderr, "   genoperand = %s", genoperand);
#endif

      sprintf (genline, "%-6.6s %-7.7s %s", genlabel, genopcode, genoperand);
      
      /*
      ** Lookup opcode for macro expansion control
      */

      lclflag = 0;
      termparn = FALSE;
      tokscan (genopcode, &token, &tokentype, &val, &term, FALSE);
      termparn = oldtermparn;
      op = oplookup (token);
      if (rmtflag)
      {
	 if (op && (op->optype == TYPE_P) && (op->opvalue == RMT_T))
	 {
	    rmtflag = FALSE;
	 }
	 else
	 {
	    genmode |= MACEXP;
	    rmtseqadd (genmode, genline);
	 }
	 continue;
      }

      if (op && op->optype == TYPE_P &&
          (op->opvalue == MACRO_T || op->opvalue == MACRON_T))
      {
#ifdef DEBUGMACROEXP
	 fprintf (stderr, "   NEST: %s TRUE1\n", cursym);
#endif
         mac->macnested = TRUE;
      }
      if (!asmskip && cursym[0] && !mac->macnested)
      {
#ifdef DEBUGMACROEXP
	 fprintf (stderr, "   ADD sym: cursym = %s\n", cursym);
#endif
	 if (!(sym = symlookup (cursym, "", FALSE, FALSE)))
	 {
	    sym = symlookup (cursym, "", TRUE, FALSE);
	 }
	 if (sym)
	 {
	    sym->flags |= P0SYM;
         }
      }

      if (term == INDIRECTSYM) lclflag = 060;

      /*
      ** If not skipping, IF[F/T], process
      */

      if (!asmskip)
      {
	 /*
	 ** Process GOTO in macro
	 */

	 if (gotoskip)
	 {
	    if (cursym[0] && !strcmp(cursym, gotosym))
	    {
#ifdef DEBUGGOTO
	       fprintf (stderr, "   cursym = '%s', gotosym = '%s'\n",
			cursym, gotosym);
#endif
	       gotoskip = FALSE;
	       goto GOTO_ENDS;
	    }
	    else
	    {
#ifdef DEBUGGOTO
	       fprintf (stderr, "   genopcode = '%s'\n", genopcode);
#endif
	       if (op && op->optype == TYPE_P && op->opvalue == IRP_T)
	       {
	          gotoskip = FALSE;
		  genmode |= (MACEXP | MACCALL);
		  goto GOTO_IRP;
	       }
	       else
	       {
		  genmode |= (MACEXP | SKPINST);
	       }

	       if (mac->macnested)
		  strcpy (nestmac[nestndx++], genline);
	       else
	       {
		  writesource (outfd, FALSE, genmode, ++genlinenum,
		  		genline, "g00");
	       }
	    }
	 }

	 /*
	 ** Not in GOTO, check for other controls
	 */

	 else
	 {
	 GOTO_ENDS:
	    /*
	    ** Check Pseudo ops that control macro
	    */

	    if (op && op->optype == TYPE_P)
	    {
	       char psopline[MAXLINE];

	       switch (op->opvalue)
	       {
	       case CALL_T:
		  if (mac->macnested)
		     strcpy (nestmac[nestndx++], genline);
		  else
		  {
		     genmode |= MACEXP;
		     writesource (outfd, FALSE, genmode, ++genlinenum,
		     		genline, "g01");
		     p0call (genoperand, MACEXP, outfd);
		  }
		  break;

	       case IFF_T:
	       case IFT_T:
	       case GOTO_T:
		  genmode |= MACCALL;
	       case SET_T:
	       case TAPENO_T:
		  genmode |= MACEXP;
		  if (mac->macnested)
		     strcpy (nestmac[nestndx++], genline);
		  else
		  {
		     writesource (outfd, FALSE, genmode, ++genlinenum,
		     		genline, "g02");
		  }
	       case RMT_T:
		  strcpy (psopline, genoperand);
		  cp = psopline;
		  if (!mac->macnested)
		     p0pop (op, lclflag, cp, infd, outfd, TRUE);
		  break;

	       case IRP_T:

		  /*
		  ** Process IRP here
		  */
	       GOTO_IRP:
		  genmode |= (MACEXP | MACCALL);

		  if (mac->macnested && nestndx)
		     strcpy (nestmac[nestndx++], genline);
#if DEBUGMACROEXP
		  else
		  {
		     writesource (outfd, FALSE, genmode, ++genlinenum,
		     		genline, "g03");
		  }
#endif

		  /*
		  ** If in IRP block, get next IRP arg
		  */

		  if (inirp)
		  {
		     irparg[0] = '\0';
		     strcpy (genoperand, irpargs);
		     cp = genoperand;

		     /*
		     ** If another IRP args, scan it off and loop
		     */

		     if (*cp)
		     {
			if (*cp == '(')
			{
			   int ii;

			   cp++;
			   ocp = cp;
			   ii = 1;
			   while (*cp && ii > 0 /*&& !isspace(*cp)*/)
			   {
			      if (*cp == '(')
			      {
				 ii++;
				 cp++;
			      }
			      else if (*cp == ')')
			      {
				 if (ii)
				 {
				    ii--;
				    if (ii) cp++;
				 }
			      }
			      else cp++;
			   }
#ifdef DEBUGMACROEXP
			   fprintf (stderr, "   *cp = %c(%x), pocp = %s\n",
				 *cp, *cp, ocp);
#endif
			   if (*cp == ')')
			   {
			      *cp++ = '\0';
			      if (*cp == ',') cp++;
			   }
			   strcpy (irparg, ocp);
			}
			else do {
			   term = *cp++;
			   if (term == ',')
			      break;
			   if (term == '(')
			   {
			      cp--;
			      break;
			   }
			   strncat (irparg, &term, 1);
			} while (*cp);
			strcpy (irpargs, cp);
#ifdef DEBUGMACROEXP
			fprintf (stderr, "   irpargs1 = '%s'\n", irpargs);
			fprintf (stderr, "   irparg1  = '%s'\n", irparg);
#endif
			i = irpline;
		     }
		     /*
		     ** No more args, exit loop
		     */
		     else
		     {
			inirp = FALSE;
			doirp = TRUE;
		     }
		  }

		  /*
		  ** New IRP, get first arg and start loop
		  */

		  else
		  {
		     inirp = TRUE;
		     irpline = i;
		     irpnum = j;
		     irparg[0] = '\0';
		     cp = strchr (genoperand, '\n');
		     *cp = '\0';
		     cp = genoperand;
		     if (*cp == '(')
		     {
			int ii;

		        cp++;
			ocp = cp;
			ii = 1;
			while (*cp && ii > 0 /*&& !isspace(*cp)*/)
			{
			   if (*cp == '(')
			   {
			      ii++;
			      cp++;
			   }
			   else if (*cp == ')')
			   {
			      if (ii)
			      {
				 ii--;
				 if (ii) cp++;
			      }
			   }
			   else cp++;
			}
#ifdef DEBUGMACROEXP
			fprintf (stderr, "   *cp = %c(%x), pocp = %s\n",
				 *cp, *cp, ocp);
#endif
			if (*cp == ')')
			{
			   *cp++ = '\0';
			   if (*cp == ',') cp++;
			}
			strcpy (irparg, ocp);
		     }
		     else do {
			term = *cp++;
		        if (term == ',')
			   break;
			if (term == '(')
			{
			   cp--;
			   break;
			}
			strncat (irparg, &term, 1);
		     } while (*cp);
		     strcpy (irpargs, cp);
#ifdef DEBUGMACROEXP
		     fprintf (stderr, "   irpargs0 = '%s'\n", irpargs);
		     fprintf (stderr, "   irparg0  = '%s'\n", irparg);
#endif
		     if (irparg[0] == '\0') 
		        doirp = FALSE;
		  }
		  break;

	       case END_T:
	       case ENDM_T:
		  if (mac->macnested)
		  {
		     genmode |= (MACEXP | MACDEF);
		     strcpy (nestmac[nestndx++], genline);
#ifdef DEBUGMACROEXP
		     fprintf (stderr, " NESTED macro:\n");
#endif
		     for (j = 0; j < nestndx; j++)
		     {
			writesource (outfd, FALSE, genmode, ++genlinenum,
				     nestmac[j], "g09");
#ifdef DEBUGMACROEXP
		        fprintf (stderr,"%s", nestmac[j]);
#endif
		     }
		     p0macrostring (nestmac);   
		     for (j = 0; j < nestndx; j++)
			DBG_MEMREL (nestmac[j]);
		     nestndx = 0;
		     mac->macnested = FALSE;
		     noexp = FALSE;
		  }
		  else
		  {
		     genmode |= MACEXP;
		     writesource (outfd, FALSE, genmode, ++genlinenum,
		     		genline, "g08");
		  }
		  break;

	       case MACRO_T:
	          noexp = TRUE;
	       case MACRON_T:
	          mac->macnested = TRUE;
		  if ((nestmac[nestndx] = (char *)DBG_MEMGET (MAXLINE)) == NULL)
		  {
		     fprintf (stderr, "asm7090: Unable to allocate memory\n");
		     exit (ABORT);
		  }
		  strcpy (nestmac[nestndx++], genline);
	          break;

	       default:
		  genmode |= MACEXP;
		  if (doirp)
		  {

		     if (mac->macnested)
			strcpy (nestmac[nestndx++], genline);
		     else
		     {
			writesource (outfd, FALSE, genmode, ++genlinenum,
					genline, "g04");
		     }
		  }
	       }
	    }

	    /*
	    ** Check if macro is called in macro
	    */

	    else if ((lclmac = maclookup (genopcode)) != NULL)
	    {
#ifdef DEBUGMACROEXP
	       fprintf (stderr,"macro opcode = %s, noexp = %d\n",
	       		genopcode, noexp);
#endif
	       genmode |= (MACEXP | MACCALL);
	       if (lclmac->macaltdef) genmode |= MACALT;

	       if (doirp)
	       {
		  if (mac->macnested)
		  {
		     strcpy (nestmac[nestndx++], genline);
		  }
		  else
		  {
		     writesource (outfd, FALSE, genmode, ++genlinenum,
		     		genline, "g05");
		     if (!noexp)
			p0expand (lclmac, lclflag, genoperand, infd, outfd);
		  }
	       }
	    }

	    /*
	    ** None of the above, send it on to next pass
	    */

	    else
	    {
	       pc++;
	       genmode |= MACEXP;
	       if (doirp)
	       {
		  if (mac->macnested)
		     strcpy (nestmac[nestndx++], genline);
		  else
		  {
		     writesource (outfd, FALSE, genmode, ++genlinenum,
		     		genline, "g06");
		  }
	       }
	    }
	 }
      }

      /*
      ** In skip, decrement skip count
      */

      else
      {
	 genmode |= (MACEXP | SKPINST);
	 if (mac->macnested)
	    strcpy (nestmac[nestndx++], genline);
	 else
	 {
	    writesource (outfd, FALSE, genmode, ++genlinenum, genline, "g07");
	 }
#ifdef DEBUGIF
         fprintf (stderr, "SKIP: %s", genline);
#endif
	 asmskip--;
      }
   }

   /*
   ** If we expanded a nested macro, go process it
   */

   if (nestndx && mac->macnested)
   {
      p0macrostring (nestmac);   
      for (i = 0; i < nestndx; i++)
         DBG_MEMREL (nestmac[i]);
   }

}

/***********************************************************************
* p0ibsys - process IBSYS cards.
***********************************************************************/

static void
p0ibsys (char *bp, FILE *infd, FILE *outfd)
{
   char *token;
   int genmode;
   int tokentype;
   int val;
   int oldtermparn;
   char term;

#ifdef DEBUGCTLCARD
   fprintf (stderr, "p0ibsys: %s", bp);
#endif

   /*
   ** Send control card to next pass
   */
NEW_CONTROL:
   genmode = CTLCARD;
   writesource (outfd, FALSE, genmode, ++genlinenum, bp, "i00");

   oldtermparn = termparn;
#ifdef DEBUGPARN
   fprintf (stderr, "p0ibsys: termparn = %s\n", termparn ? "TRUE" : "FALSE");
#endif
   termparn = FALSE;

   bp++;
   /*
   ** If not a comment, process
   */

   if (*bp != COMMENTSYM)
   {
      /*
      ** Scan off command
      */

      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGCTLCARD
      fprintf (stderr, 
	    "   token1 = %s, tokentype = %d, val = %o, term = %c(%x)\n",
	    token, tokentype, val, term, term);
#endif
      /*
      ** If $TITLE then scan off the title
      */

      if (!strcmp (token, "TITLE"))
      {
	 char *cp;

	 bp = &inbuf[VARSTART];
	 while (*bp && isspace (*bp)) bp++;
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

      /*
      ** If $EXECUTE then scan and check if IBSFAP.
      */

      else if (!strcmp (token, "EXECUTE"))
      {
	 bp = &inbuf[VARSTART];
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 if (!strcmp (token, "IBSFAP"))
	 {
	    fapmode = TRUE;
	    cpumodel = 7094;
	    oldtermparn = FALSE;
	 }
      }

      /*
      ** If $IBMAP then scan and process the options.
      */

      else if (!strcmp (token, "IBMAP"))
      {
         bp = &inbuf[OPCSTART];
	 fapmode = FALSE;
	 cpumodel = 7090;

	 /*
	 ** Get the deckname
	 */

	 if (!isspace(*bp))
	 {
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGCTLCARD
	    fprintf (stderr,
	       "   token2 = %s, tokentype = %d, val = %o, term = %c(%x)\n",
	       token, tokentype, val, term, term);
#endif
	    if (strlen (token) > MAXSYMLEN) token[MAXSYMLEN] = '\0';
	    strcpy (deckname, token);
	    if (strlen (token) > 4) token[4] = '\0';
	    strcpy (lbl, token);
	    
	 }

	 /*
	 ** Parse the options
	 */

	 bp = &inbuf[VARSTART];
	 do {
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	    if (term == '(')
	    {
	       strcpy (token, "(");
	       strncat (token, bp, 3);
	       bp += 3;
	       term = *bp;
	       if (*bp == ',') bp++;
	    }
#ifdef DEBUGCTLCARD
	    fprintf (stderr,
	       "   token3 = %s, tokentype = %d, val = %o, term = %c(%x)\n",
	       token, tokentype, val, term, term);
#endif
	    if (!strcmp (token, "ABSMOD"))
	    {
	       addext = FALSE;
	       absmod = TRUE;
	    }
	    else if (!strcmp (token, "ADDREL"))
	    {
	       addrel = TRUE;
	       addext = TRUE;
	       absmod = FALSE;
	    }
	    else if (!strcmp (token, "RELMOD"))
	    {
	       addext = TRUE;
	       absmod = FALSE;
	    }
	    else if (!strcmp (token, "JOBSYM"))
	    {
	       jobsym = TRUE;
	       definejobsyms();
	    }
	    else if (!strcmp (token, "M90"))
	    {
	       cpumodel = 7090;
	    }
	    else if (!strcmp (token, "M94"))
	    {
	       cpumodel = 7094;
	    }
	    else if (!strcmp (token, "MONSYM"))
	    {
	       monsym = TRUE;
	       definemonsyms(0);
	    }
	    else if (!strcmp (token, "NO()"))
	    {
	       oldtermparn = TRUE;
	    }
	    else if (!strcmp (token, "()OK"))
	    {
	       oldtermparn = FALSE;
	    }
	    else if (!strcmp (token, "REF"))
	    {
	       genxref = TRUE;
	    }
	 } while (term == ',');
      }

      /*
      ** If $IBEDT then slurp up cards until EOF or another CONTROL card.
      */

      else if (!strcmp (token, "IBEDT"))
      {
	 if (etcbuf[0])
	 {
#ifdef DEBUGP0RDR
	    fprintf (stderr, "P0in = %s", etcbuf);
#endif
	    writesource (outfd, FALSE, genmode, ++genlinenum, etcbuf, "i01");
	    etcbuf[0] = '\0';
	 }

	 bp = errtmp;
	 while (fgets (bp, 256, infd))
	 {
	    if (*bp == '\f' || *bp == '~') continue;
#ifdef DEBUGP0RDR
	    fprintf (stderr, "P0in = %s", bp);
#endif
	    if (*bp == IBSYSSYM)
	       goto NEW_CONTROL;
	    writesource (outfd, FALSE, genmode, ++genlinenum, bp, "i02");
	 }
      }
   }

   termparn = oldtermparn;
#ifdef DEBUGPARN
   fprintf (stderr, "p0ibsys: exit: termparn = %s\n",
	 termparn ? "TRUE" : "FALSE");
#endif
}

/***********************************************************************
* asmpass0 - Pass 0
***********************************************************************/

int
asmpass0 (FILE *tmpfd0, FILE *tmpfd1)
{
   MacDef *mac;
   char *token;
   int i;
   int done;
   int enddone;
   int flag;
   int genmode;
   int endmode;
   int status = 0;
   int tokentype;
   int val;
   int begcomments;
   char term;
   char opcode[MAXSYMLEN+2];
   char endbuf[MAXSRCLINE];

#ifdef DEBUGP0RDR
   fprintf (stderr, "asmpass0: Entered\n");
#endif

   /*
   ** Clear out macro table.
   */

   for (i = 0; i < MAXMACROS; i++)
   {
      memset ((char *)&macdef[i], '\0', sizeof(MacDef));
   }

   lblgennum = 1;
   macrocount = 0;
   macsloaded = FALSE;
   macscan = FALSE;
   readrmtseqflag = FALSE;

   /*
   ** Rewind the output.
   */

   if (fseek (tmpfd1, 0, SEEK_SET) < 0)
   {
      perror ("asm7090: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Process the source.
   */

   pc = 0;
   qualindex = 0;
   linenum = 0;
   headcount = 0;
   endmode = 0;
   genlinenum = 0;

   done = FALSE;
   enddone = FALSE;
   rmtflag = FALSE;

   etcbuf[0] = '\0';
   endbuf[0] = '\0';

   begcomments = TRUE;

   tmpfd = tmpfd0;

DOREAD:
   while (!done)
   {
      char *bp;

      srcbuf[0] = '\0';
      genmode = 0;
      curpos = ftell (tmpfd);
      if (readrmtseqflag)
      {
         if (!rmtseqget (&genmode, srcbuf))
	 {
	    readrmtseqflag = FALSE;
	    linenum = rmtseqnum;
	    if (enddone)
	    {
	       status = 0;
	       done = TRUE;
	       break;
	    }
	    continue;
	 }
      }
      else 
      {
	 if (etcbuf[0])
	 {
	    strcpy (srcbuf, etcbuf);
	    if (!eofflg && !fgets(etcbuf, MAXSRCLINE, tmpfd))
	    {
	       etcbuf[0] = '\0';
	       eofflg = TRUE;
	    }
	 }
	 else
	 {
	    if (!eofflg && !fgets(srcbuf, MAXSRCLINE, tmpfd))
	    {
	       srcbuf[0] = '\0';
	       eofflg = TRUE;
	    }
	    else if (!eofflg && !fgets(etcbuf, MAXSRCLINE, tmpfd))
	    {
	       etcbuf[0] = '\0';
	       eofflg = TRUE;
	    }
	 }

	 if (eofflg && !srcbuf[0])
	 {
	    if ((linenum > 1) && (tmpfd == tmpfd0))
	    {
	       logerror ("No END record");
	       status = 0;
	    }
	    else
	    {
	       status = 1;
	    }
	    done = TRUE;
	    break;
	 }
      }

      gotoskip = FALSE;
      exprtype = ADDREXPR;
      radix = 10;

      strcpy (inbuf, srcbuf);
#ifdef DEBUGP0RDR
      fprintf (stderr, "P0in = %s", inbuf);
#endif
      bp = inbuf;

      if (asmskip)
      {
	 genmode |= SKPINST;
	 writesource (tmpfd1, rmtflag, genmode, ++linenum, srcbuf, "s01");
	 asmskip--;
	 continue;
      }

      /*
      ** Check if IBSYS control card.
      */

      if (*bp == IBSYSSYM)
      {
         p0ibsys (bp, tmpfd, tmpfd1);
      }
      else if (srcbuf[0] == '~')
      {
         continue;
      }

      /*
      ** If not a comment, then process.
      */

      else if (*bp != COMMENTSYM)
      {
	 OpCode *op;

	 genlinenum = 0;
	 begcomments = FALSE;

	 /*
	 ** If not 7094 mode assmebly, define macros for 7094 instructions.
	 */

	 if (!macsloaded && cpumodel > 704 && cpumodel < 7094)
	 {
	    macsloaded = TRUE;

	    for (i = 0; macros7094[i]; i++)
	    {
	       p0macrostring (macros7094[i]);
	    }

	 }
	 
	 /*
	 ** If label present, add to symbol table.
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
	    for (cp = temp; *cp; cp++); /* allow for short label only lines */
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
	    fprintf (stderr, "asmpass0: cursym = %s\n", cursym);
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
	 ** Scan off opcode.
	 */

	 if (*bp == '\0')
	 {
	    strcpy (opcode, "PZE"); /* Make a null opcode a PZE */
	 }
	 else if (!strncmp (&inbuf[OPCSTART], "   ", 3))
	 {
	    strcpy (opcode, "PZE"); /* Make a blank opcode a PZE */
	    bp = &inbuf[OPCSTART+3];
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
	 ** Check etcbuf for ETC
	 */

	 if (etcbuf[0])
	 {
	    if ((strlen(etcbuf) > OPCSTART+4) &&
	        !strncmp (&etcbuf[OPCSTART], "ETC", 3))
	    {
	       genmode |= CONTINU;
	    }
	 }

	 /*
	 ** Check for indirect addressing.
	 ** Maybe either "TRA*  SYM" or "TRA  *SYM".
	 */

	 flag = 0;
	 if (term == INDIRECTSYM)
	 {
	    flag = 060;
	    bp++;
	 }
	 else
	 {
	    int nooperands;

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
	 ** Check macro table first, in case of name override.
	 */

	 if ((mac = maclookup (opcode)) != NULL)
	 {
	    genmode |= MACCALL;
	    if (mac->macaltdef) genmode |= MACALT;
	    writesource (tmpfd1, FALSE, genmode, ++linenum, srcbuf, "s01");
	    if (genmode & CONTINU)
	    {
	       char *pp;
	       int paren;

	       if (strlen (inbuf) > RIGHTMARGIN)
		  inbuf[RIGHTMARGIN+1] = '\0';
	       pp = &inbuf[VARSTART];
	       paren = 0;
	       while (*pp)
	       {
		  if ((paren <= 0) && isspace(*pp)) break;
		  if (*pp == '$') break;
		  if (*pp == '(') paren++;
		  else if (*pp == ')') paren--;
		  pp++;
	       }
	       *pp = '\0';
	       if (*(pp-1) == '$') *(pp-1) = '\0';
	       while (genmode & CONTINU)
	       {
		  int etcmode = genmode & ~CONTINU;

		  writesource (tmpfd1, FALSE, etcmode, ++linenum,
			       etcbuf, "s02");
		  if (strlen (etcbuf) > RIGHTMARGIN)
		     etcbuf[RIGHTMARGIN+1] = '\0';
		  linenum++;
		  pp = &etcbuf[VARSTART];
		  while (*pp && !isspace(*pp)) pp++;
		  *pp = '\0';
		  if (*(pp-1) == '$') *(pp-1) = '\0';
		  strcat (inbuf, &etcbuf[VARSTART]);
		  if (fgets(etcbuf, MAXSRCLINE, tmpfd) == NULL)
		  {
		     done = TRUE;
		     break;
		  }
		  if ((strlen(etcbuf) > OPCSTART+4) &&
		      !strncmp (&etcbuf[OPCSTART], "ETC", 3))
		     genmode |= CONTINU;
		  else 
		     genmode &= ~CONTINU;
	       }
	       strcat (inbuf, "\n");
#ifdef DEBUGP0RDR
	       fprintf (stderr, "P0in-1 = %s", inbuf);
#endif
	    }
	    if (!asmskip)
	       p0expand (mac, flag, bp, tmpfd, tmpfd1);
	    asmskip = 0;
	    gotoskip = FALSE;
	 }

	 else
	 {

	    /*
	    ** Check opcodes
	    */

	    op = oplookup (opcode);

	    /*
	    ** If pseudo op, process
	    */

	    if (op && op->optype == TYPE_P)
	    {
	       int writebefore;

	       /*
	       ** Set MACCALL for CALL, SAVE[N] and RETURN.
	       */

	       writebefore = FALSE;
	       switch (op->opvalue)
	       {
	       case CALL_T:
	       case SAVE_T:
	       case SAVEN_T:
	       case RETURN_T:
		  genmode |= MACCALL;
	       case DUP_T:
	       case LDIR_T:
	       case MACRO_T:
	       case MACRON_T:
	       case INSERT_T:
		  writesource (tmpfd1, rmtflag, genmode, ++linenum,
		  		srcbuf, "s03");
	       case RMT_T:
		  writebefore = TRUE;
		  break;
	       default: ;
	       }

	       i = p0pop (op, flag, bp, tmpfd, tmpfd1, FALSE);

	       if (((genmode & (MACEXP | RMTSEQ)) == (MACEXP | RMTSEQ)) &&
		  op->opvalue == IFF_T)
	       {
	          genmode |= SKPINST;
	       }

	       if (i)
	       {
		  enddone = TRUE;
		  endmode = genmode;
		  strcpy (endbuf, srcbuf);
	          if (fapmode && rmthead)
		  {
		     readrmtseqflag = TRUE;
		     rmtseqnum = linenum;
		     linenum = 0;
		  }
		  else
		  {
		     done = TRUE;
		     status = 0;
		  }
	       }
	       else if (!writebefore)
	       {
		  writesource (tmpfd1, rmtflag, genmode, ++linenum,
			       srcbuf, "s04");
	       }
	    }

	    /*
	    ** None of the above, send on
	    */

	    else
	    {
	       if (cursym[0])
	       {
		  SymNode *sym;
		  if (fapmode)
		  {
		     if (!(sym = symlookup (cursym, "", FALSE, FALSE)))
		     {
			sym = symlookup (cursym, "", TRUE, FALSE);
		     }
		  }
		  else
		  {
		     if (!(sym = symlookup (cursym, qualtable[qualindex],
					    FALSE, FALSE)))
			sym = symlookup (cursym, qualtable[qualindex],
					 TRUE, FALSE);
		  }
		  if (sym)
		  {
		     sym->flags |= P0SYM;
		  }
	       }
	       pc++;
	       writesource (tmpfd1, rmtflag, genmode, ++linenum,
			    srcbuf, "s05");
	    }
	 }
      }

      /*
      ** A comment, in FAP there is some meaning.
      */

      else
      {
	 writesource (tmpfd1, FALSE, genmode, ++linenum, srcbuf, "s06");
	 if (begcomments && fapmode && !titlebuf[0])
	 {
	    char *cp;

	    bp++;
	    while (*bp && isspace(*bp)) bp++;
	    cp = bp;
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	    if (!strcmp (token, "FAP")) ;
	    else if (!strcmp (token, "ID")) ;
	    else
	    {
	       while (*bp != '\n')
	       {
		  if (bp - inbuf > RIGHTMARGIN) break;
		  if (bp - cp == TTLSIZE) break;
		  bp++;
	       }
	       *bp = '\0';
	       strcpy (titlebuf, cp);
	    }
	 }
      }
   }

   /*
   ** Go through table and clear externals.
   */

   for (i = 0; i < symbolcount; i++)
   {
      if (symbols[i])
      {
         if (symbols[i]->flags & EXTERNAL)
	    symbols[i]->value = 0;
      }
   }

   if ((tmpfd = closecopy (tmpfd, etcbuf)) != NULL)
   {
      done = FALSE;
      eofflg = FALSE;
      goto DOREAD;
   }

   if (endbuf[0])
   {
      writesource (tmpfd1, FALSE, endmode, ++linenum, endbuf, "s07");
   }

   /*
   ** Clean up
   */

   for (i = 0; i < macrocount; i++)
   {
      int j;
      for (j = 0; j < macdef[i].maclinecount; j++)
      {
#ifdef DEBUGMALLOCS
	 DBG_MEMREL (macdef[i].maclines[j]);
#else
         macdef[i].maclines[j]->next = freemaclines;
	 freemaclines = macdef[i].maclines[j];
#endif
      }
   }

#ifdef DEBUGMULTIFILE
   fprintf (stderr, "asmpass0: %d lines in file\n", linenum);
#endif

   pscanbuf = inbuf;
   return (status);
}
