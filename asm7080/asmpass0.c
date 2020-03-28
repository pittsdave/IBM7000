/***********************************************************************
*
* asmpass0 - Pass 0 for the IBM 7080 assembler.
*
* Changes:
*   02/12/07   DGP   Original.
*   07/31/07   DGP   Correct stack corruption.
*   05/22/13   DGP   Set default starting PC to 160.
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
extern int radix;		/* Number scanner radix */
extern int linenum;		/* Source line number */
extern int exprtype;		/* Expression type */
extern int pgmlength;		/* Length of program */
extern int genxref;		/* Generate cross reference listing */
extern int cpumodel;		/* CPU model */
extern int errcount;		/* Number of errors in assembly */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char deckname[MAXDECKNAMELEN+2];/* The assembly deckname */
extern char titlebuf[TTLSIZE+2];/* The assembly TTL buffer */

extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern char *pscanbuf;		/* Pointer for tokenizers */

static FILE *tmpfd;		/* Input fd */

static long curpos;		/* Current file position */
static int lblgennum;		/* LBL generated number */
static int macrocount;		/* Number of macros defined */
static int eofflg = FALSE;	/* EOF on input file */
static int macscan;		/* Currently scanning a MACRO definition */

static char cursym[MAXSYMLEN+2];/* Current label field symbol */
static char srcbuf[MAXSRCLINE];	/* Source line buffer */
static char etcbuf[MAXSRCLINE];	/* ETC (continuation) buffer */

static MacLines *freemaclines = NULL;/* resusable Macro lines */
static MacDef macdef[MAXMACROS];/* Macro template table */

#include "asmmacros.h"

/***********************************************************************
* maclookup - Lookup macro in table
***********************************************************************/

static MacDef *
maclookup (char *name)
{
   MacDef *mac = NULL;
   int i;

   for (i = 0; i < macrocount; i++)
   {
      mac = &macdef[i];
      if (!strcmp (mac->macname, name))
      {
	 return (mac);
      }
   }
   return (NULL);
}

/***********************************************************************
* p0mactoken - Process macro tokens
***********************************************************************/

static char *
p0mactokens (char *cp, int field, int index, MacDef *mac)
{
   char *token;
   char *ocp;
   int tokentype;
   int val;
   int j;
   int barequote;
   char term;
   char targ[TOKENLEN];
   char largs[MAXLINE];
   char lcltoken[TOKENLEN];

#ifdef DEBUGMACRO
   fprintf (stderr, "p0mactokens: field = %d, index = %d, cp = %s\n",
	    field, index, cp);
#endif

   /*
   ** Check for NULL operands 
   */

   if (field == 3)
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
      cp = tokscan (cp, &token, &tokentype, &val, &term);
#ifdef DEBUGMACRO
      fprintf (stderr,
	    "   token1 = %s, tokentype = %d, val = %d, term = %c(%x)\n",
	    token, tokentype, val, term, term);
      fprintf (stderr, "   cp = %s", cp);
#endif
      /*
      ** If operator in input stream, copy it out
      */

      if (token[0] == '\0')
      {
	 if (tokentype == ASTER)
	    strcpy (targ, "*");
	 else if (term == '\'')
	    barequote = TRUE;
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
	       sprintf (lcltoken, "%c%02d", MACPARMSYM, j);
	       break;
	    }
	 }
	 strcat (largs, lcltoken);

	 cp++;
	 if (*cp != '\'')
	 {
	    cp = tokscan (cp, &token, &tokentype, &val, &term);
#ifdef DEBUGMACRO
	    fprintf (stderr,
	       "   token2 = %s, tokentype = %d, val = %d, term = %c(%x)\n",
	       token, tokentype, val, term, term);
	    fprintf (stderr, "   cp = %s\n", cp);
#endif
	    if (token[0])
	    {
	       for (j = 0; j < mac->macargcount; j++)
	       {
		  if (!strcmp (token, mac->macargs[j]))
		  {
		     sprintf (targ, "%c%02d", MACPARMSYM, j);
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
	 if (token[0] == MACLABELSYM)
	 {
	    j = atoi (&token[1]);
	    sprintf (targ, "%c%02d", MACPARMSYM, j + mac->macargcount);
#ifdef DEBUGMACRO
            fprintf (stderr, "   macro label, token = %s, j = %d, targ = %s\n",
	    token, j, targ);
#endif
	 }
	 else
	 {
	    for (j = 0; j < mac->macargcount; j++)
	    {
	       if (!strcmp (token, mac->macargs[j]))
	       {
		  sprintf (targ, "%c%02d", MACPARMSYM, j);
		  break;
	       }
	    }
	    if (j == mac->macargcount)
	       strcpy (targ, token);
	 }
	 if (!isspace(term))
	 {
	    strncat (targ, &term, 1);
	    if (term != ',') cp++;
	 }
      }
      strcat (largs, targ);
   } while (*cp && !isspace(term) && !isspace(*cp));

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
      break;
   case 2:
      strcat (mac->maclines[index]->numfield, largs);
      break;
   case 3:
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
   char term;
   char genline[MAXLINE];

   if (macrocount >= MAXMACROS)
   {
      logerror ("MACRO table overflow");
      return;
   }
   mac = &macdef[macrocount];
   memset (mac, '\0', sizeof (MacDef));
   macindex = 1;

   strcpy (genline, infd[0]);
   bp = &genline[SYMSTART];
   pscanbuf = bp;
   cp = cursym;

   while (*bp && !isspace(*bp)) *cp++ = *bp++;
   *cp = '\0';
   strcpy (mac->macname, cursym);

   bp = &genline[OPCSTART];
   bp = tokscan (bp, &token, &tokentype, &val, &term);
   if (strncmp (token, "MACRO", 5))
   {
      pscanbuf = inbuf;
      return;
   }

   /*
   ** Scan off the parameters
   */

   i = 0;
   bp = &genline[VARSTART];

   /* Check if numfield is defined in definition */

   if (strncmp (bp-2, "  ", 2))
   {
      char ch;

      ch = *bp;
      *bp = ' ';
      cp = bp-2;
      if (*cp == ' ') cp++;
      cp = tokscan (cp, &token, &tokentype, &val, &term);
      *bp = ch;
      strcpy (mac->macargs[i], token);
      mac->macusenum = TRUE;
      i++;
   }

   do {
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      strcpy (mac->macargs[i], token);
      i++;
   } while (*bp && !isspace(term));
   mac->macargcount = i;

#ifdef DEBUGMACRO
   fprintf (stderr, "\nMACRO0: name = %s, numfield = %d\n",
      mac->macname, mac->macnumfield);
   for (i = 0; i < mac->macargcount; i++)
      fprintf (stderr, "   arg%02d = %s\n", i, mac->macargs[i]);
#endif

   /*
   ** Read the source into the template until MEND.
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

      /*
      ** If MEND, we're done
      */

      if (!strncmp (&genline[OPCSTART], "MEND", 4))
	 done = TRUE;

      /*
      ** If not a comment, process template line
      */

      if (!done && genline[SYMSTART] != COMMENTSYM)
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
	    fprintf (stderr, "asm7080: Unable to allocate memory\n");
	    exit (ABORT);
	 }
	 memset (mac->maclines[i], '\0', sizeof(MacLines));

	 /*
	 ** If label field process label
	 */

	 cp = &genline[SYMSTART];
	 if (strncmp (cp, "      ", 6))
	 {
	    while (*cp && isspace (*cp)) cp++;
	    if (isalnum(*cp) || *cp == MACLABELSYM)
	       cp = p0mactokens (cp, 0, i, mac);
#ifdef DEBUGMACRO
	    fprintf (stderr, "   label = %s\n", mac->maclines[i]->label);
#endif
	 }

	 /*
	 ** Process opcode field
	 */

	 cp = &genline[OPCSTART];
	 cp = p0mactokens (cp, 1, i, mac);
#ifdef DEBUGMACRO
	 fprintf (stderr, "   opcode = %s\n", mac->maclines[i]->opcode);
#endif

	 /*
	 ** Process num field
	 */

	 cp = &genline[NUMSTART];
	 if (strncmp (cp, "  ", 2))
	 {
	    term = genline[VARSTART];
	    genline[VARSTART] = ' ';
	    if (*cp == ' ') cp++;
	    cp = p0mactokens (cp, 2, i, mac);
	    genline[VARSTART] = term;
	 }
	 else
	    strcpy (mac->maclines[i]->numfield, "  ");
#ifdef DEBUGMACRO
	 fprintf (stderr, "   numfield = %s\n", mac->maclines[i]->numfield);
#endif

	 /*
	 ** Process operand field
	 */

	 cp = &genline[VARSTART];
	 cp = p0mactokens (cp, 3, i, mac);
#ifdef DEBUGMACRO
	 fprintf (stderr, "   operand = %s\n", mac->maclines[i]->operand);
#endif
	 i++;
      }
   }
   mac->maclinecount = i;
#ifdef DEBUGMACRO
   fprintf (stderr, "MACRO: %s\n", mac->macname);
   for (i = 0; i < mac->macargcount; i++)
      fprintf (stderr, "arg%02d: %s\n", i, mac->macargs[i]);
   for (i = 0; i < mac->maclinecount; i++)
      fprintf (stderr, "%-10.10s %-5.5s %s %s\n",
	    mac->maclines[i]->label,
	    mac->maclines[i]->opcode,
	    mac->maclines[i]->numfield,
	    mac->maclines[i]->operand);
#endif
   macrocount++;

   pscanbuf = inbuf;
}

/***********************************************************************
* p0macro - Process macro templates
***********************************************************************/

static int
p0macro (char *bp, int numfield, FILE *infd, FILE *outfd)
{
   MacDef *mac;
   char *token;
   char *cp;
   int tokentype;
   int val;
   int i;
   int done;
   int genmode;
   int nestcnt;
   int lclnum;
   char term;
   char genline[MAXLINE];
   char stemp[256];

#ifdef DEBUGMACRO
   fprintf (stderr, "p0macro: cursym[0] = %x, numfield = %d\n",
	    cursym[0], numfield);
#endif
   macscan = TRUE;

   if (macrocount >= MAXMACROS)
   {
      logerror ("MACRO table overflow");
      return;
   }
   mac = &macdef[macrocount];
   memset (mac, '\0', sizeof (MacDef));
   mac->macstartline = linenum;
   mac->macnumfield = numfield;

   if (!cursym[0])
   {
      sprintf (stemp, "%d: MACRO definition requires a name",
	       mac->macstartline);
      logerror (stemp);
      return(-1);
   }

   strcpy (mac->macname, cursym);
   i = 0;

   /*
   ** Scan off the parameters
   */

   /* Check if numfield is defined in definition */

   if (strncmp (bp-2, "  ", 2))
   {
      char ch;

#ifdef DEBUGMACRO
      fprintf (stderr, "   numfield parm = %4.4s\n", bp-2);
#endif
      ch = *bp;
      *bp = ' ';
      cp = bp-2;
      if (*cp == ' ') cp++;
      cp = tokscan (cp, &token, &tokentype, &val, &term);
      *bp = ch;
      strcpy (mac->macargs[0], token);
      mac->macusenum = TRUE;
      i = 1;
   }

   do {
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      strcpy (mac->macargs[i], token);
      i++;
   } while (*bp && !isspace(term));
   mac->macargcount = i;

#ifdef DEBUGMACRO
   fprintf (stderr, "\nMACRO0: name = %s\n", mac->macname);
   for (i = 0; i < mac->macargcount; i++)
      fprintf (stderr, "   arg%02d = %s\n", i, mac->macargs[i]);
#endif

   /*
   ** Read the source into the template until MEND.
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
	 sprintf (stemp, "%d: Non terminated MACRO definition",
		  mac->macstartline);
	 logerror (stemp);
	 return(-1);
      }

      fwrite (&genmode, 1, 4, outfd);
      fputs (genline, outfd);
#ifdef DEBUGP0OUT
      fprintf (stderr, "m01(%06X):%s", genmode, genline);
#endif
      linenum++;

      /*
      ** If MEND, we're done
      */

      if (!strncmp (&genline[OPCSTART], "MEND", 4))
      {
	 if (nestcnt == 0)
	 {
	    macscan = FALSE;
	    done = TRUE;
	 }
         else nestcnt--;
      }

      /*
      ** If not a comment, process template line
      */

      if (!done && genline[SYMSTART] != COMMENTSYM)
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
	    fprintf (stderr, "asm7080: Unable to allocate memory\n");
	    exit (ABORT);
	 }
	 memset (mac->maclines[i], '\0', sizeof(MacLines));

	 /*
	 ** If label field process label
	 */

	 cp = &genline[SYMSTART];
	 pscanbuf = cp;
	 if (strncmp (cp, "      ", 6))
	 {
	    while (*cp && isspace (*cp)) cp++;
	    if (isalnum(*cp) || *cp == '.')
	       cp = p0mactokens (cp, 0, i, mac);
#ifdef DEBUGMACRO
	    fprintf (stderr, "   label = %s\n", mac->maclines[i]->label);
#endif
	 }
	 pscanbuf = genline;

	 /*
	 ** Process opcode field
	 */

	 cp = &genline[OPCSTART];
	 if (strncmp (cp, "      ", 6))
	    cp = p0mactokens (cp, 1, i, mac);
	 else
	    mac->maclines[i]->opcode[0] = '\0';
#ifdef DEBUGMACRO
	 fprintf (stderr, "   opcode = %s\n", mac->maclines[i]->opcode);
#endif

	 /*
	 ** Process numfield field
	 */

	 cp = &genline[NUMSTART];
	 if (strncmp (cp, "  ", 2))
	 {
	    term = genline[VARSTART];
	    genline[VARSTART] = ' ';
	    if (*cp == ' ') cp++;
	    cp = p0mactokens (cp, 2, i, mac);
	    genline[VARSTART] = term;
	 }
	 else
	    strcpy (mac->maclines[i]->numfield, "  ");
#ifdef DEBUGMACRO
	 fprintf (stderr, "   numfield = %s\n", mac->maclines[i]->numfield);
#endif

	 /*
	 ** Process operand field
	 */

	 cp = &genline[VARSTART];
	 cp = p0mactokens (cp, 3, i, mac);
#ifdef DEBUGMACRO
	 fprintf (stderr, "   operand = %s\n", mac->maclines[i]->operand);
#endif
	 i++;
      }
   }
   mac->maclinecount = i;
#ifdef DEBUGMACRO
   fprintf (stderr, "MACRO: %s\n", mac->macname);
   for (i = 0; i < mac->macargcount; i++)
      fprintf (stderr, "arg%02d: %s\n", i, mac->macargs[i]);
   for (i = 0; i < mac->maclinecount; i++)
      fprintf (stderr, "%-10.10s %-5.5s %s %s\n",
	    mac->maclines[i]->label,
	    mac->maclines[i]->opcode,
	    mac->maclines[i]->numfield,
	    mac->maclines[i]->operand);
#endif
   macrocount++;

   pscanbuf = inbuf;
   return (0);
}

/***********************************************************************
* p0pop - Process Pseudo operation codes.
***********************************************************************/

static int
p0pop (OpCode *op, int numfield, char *bp, FILE *infd, FILE *outfd, int inmacro)
{
   switch (op->opvalue)
   {

   case MACRO_T:		/* MACRO */

      if (p0macro (bp, numfield, infd, outfd))
	 return (TRUE);
      break;

   case END_T:			/* END */

      if (inmacro) break;
      /*
      ** Because of ETC read ahead we reset the input file back one.
      */

      if (!eofflg && etcbuf[0])
      {
	 if (fseek (infd, curpos, SEEK_SET) < 0)
	    perror ("asm7080: Can't reset input temp file");
      }
      return (TRUE);

   default: ;
   }

   return (FALSE);
}

/***********************************************************************
* p0expand - Expand macros
***********************************************************************/

static void
p0expand (MacDef *mac, int numfield, char *bp, FILE *infd, FILE *outfd)
{
   SymNode *sym;
   OpCode *op;
   MacDef *lclmac;
   char *token;
   char *cp;
   char *ocp;
   int tokentype;
   int val;
   int i, j;
   int done;
   int genmode;
   int lclnum;
   int argcnt;
   int nestndx;
   char term;
   char genline[MAXLINE];
   char lclargs[MAXMACARGS][MAXFIELDSIZE];
   char *nestmac[MAXMACROLINES];


#ifdef DEBUGMACROEXP
   fprintf (stderr, "p0expand: macro = %s, numfield = %d, args = %s",
	    mac->macname, numfield, bp);
#endif


   for (i = 0; i < MAXMACARGS; i++) lclargs[i][0] = '\0';

   argcnt = 0;

   /*
   ** Scan off MACRO arguments
   */

   if (mac->macusenum)
   {
      if (numfield)
      {
	 sprintf (lclargs[argcnt], "%02d", numfield);
      }
      argcnt++;
   }

   do {
      ocp = bp;
      bp = tokscan (bp, &token, &tokentype, &val, &term);
#ifdef DEBUGMACROEXP
      fprintf (stderr,
	    "   token = %s, tokentype = %d, val = %d, term = %c(%x)\n",
	    token, tokentype, val, term, term);
#endif
      if (token[0] == '\0' && tokentype == NULSYM)
      {
	 strcat (lclargs[argcnt], "**");
	 if (term == '(') argcnt++;
      }
      else if (token[0] == '\0' && tokentype == ASTER)
      {
	 strcat (lclargs[argcnt], "*");
	 if (term == '(') argcnt++;
      }
      else if (token[0] == '\0' && tokentype == EOS)
      {
         break;
      }
      else if (token[0] == '\0' && term == '(')
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
	       if (i)
	       {
		  i--;
		  if (i) bp++;
	       }
	    }
	    else bp++;
	 }
#ifdef DEBUGMACROEXP
         fprintf (stderr, "   *bp = %c(%x) %s, pocp = %s\n", *bp, *bp, bp, ocp);
#endif
	 if (*bp == ')')
	 {
	    *bp++ = '\0';
	    strcat (lclargs[argcnt], ocp);
	 }
	 else
	 {
	    logerror ("Missing ')' in macro");
	 }
	 term = *bp;
	 if (term == ',') bp++;
	 else if (isalnum(term) || term == '.')
	    term = ',';
      }
      else if (token[0] == '\0')
      {
         strncat (lclargs[argcnt], &term, 1);
      }
      else
      {
	 strcat (lclargs[argcnt], token);
	 if (term == '(') argcnt++;
      }
      if (isspace (term) || term == ',') argcnt++;
   } while (!isspace(term));

#ifdef DEBUGMACROEXP
   for (i = 0; i < argcnt; i++)
      fprintf (stderr, "   arg%02d = %s\n", i, lclargs[i]);
#endif

   /*
   ** Go through MACRO and fill in missing labels with generated labels.
   */

   for (i = 0; i < mac->maclinecount; i++)
   {
      char genindex[3];

      cp = mac->maclines[i]->label;
      if ((ocp = strchr(cp, MACPARMSYM)) != NULL)
      {
	 ocp++;
	 strncpy (genindex, ocp, 2);
	 genindex[2] = '\0';
	 j = atoi (genindex);
	 if (!lclargs[j][0])
	 {
	    sprintf (lclargs[j], "%c%04d", MACLABELSYM, lblgennum);
	    lblgennum++;
	 }
      }
   }

   /*
   ** Go through MACRO template and replace parameters with the
   ** user arguments.
   */

   nestndx = 0;

   for (i = 0; i < mac->maclinecount; i++)
   {
      char genlabel[MAXFIELDSIZE];
      char genopcode[MAXFIELDSIZE];
      char gennumfield[MAXFIELDSIZE];
      char genoperand[MAXFIELDSIZE];
      char genindex[3];


      /*
      ** Macro is nested, replace parameters and pass on
      */

      if (mac->macnested) 
      {
         if ((nestmac[nestndx] = (char *)DBG_MEMGET (MAXLINE)) == NULL)
	 {
	    fprintf (stderr, "asm7080: Unable to allocate memory\n");
	    exit (ABORT);
	 }
      }

      /*
      ** Process label field
      */

      done = FALSE;
      genmode = 0;
      genlabel[0] = '\0';
      token = genlabel;
      cp = mac->maclines[i]->label;

      while (*cp && !done)
      {
	 if ((ocp = strchr(cp, MACPARMSYM)) != NULL)
	 {
	    if (ocp > cp)
	       strncat (token, cp, ocp-cp);
	    ocp++;
	    strncpy (genindex, ocp, 2);
	    ocp += 2;
	    cp = ocp;
	    genindex[2] = '\0';
	    j = atoi (genindex);
	    strcat (token, lclargs[j]);
	 }
	 else
	 {
	    strcat (token, cp);
	    done = TRUE;
	 }
      }
      if (strlen(genlabel) > MAXSYMLEN) genlabel[MAXSYMLEN] = '\0';
      strcpy (cursym, genlabel);
      if (cursym[0])
      {
	 if (!(sym = symlookup (cursym, FALSE, FALSE)))
	 {
	    sym = symlookup (cursym, TRUE, FALSE);
	 }
	 if (sym)
	 {
	    sym->flags |= P0SYM;
         }
      }
#ifdef DEBUGMACROEXP
      fprintf (stderr, "   genlabel = %s\n", genlabel);
#endif

      /*
      ** Process opcode field
      */

      done = FALSE;
      genopcode[0] = '\0';
      token = genopcode;
      cp = mac->maclines[i]->opcode;

      while (*cp && !done)
      {
	 if ((ocp = strchr(cp, MACPARMSYM)) != NULL)
	 {
	    if (ocp > cp)
	       strncat (token, cp, ocp-cp);
	    ocp++;
	    strncpy (genindex, ocp, 2);
	    ocp += 2;
	    cp = ocp;
	    genindex[2] = '\0';
	    j = atoi (genindex);
	    strcat (token, lclargs[j]);
	 }
	 else
	 {
	    strcat (token, cp);
	    done = TRUE;
	 }
      }
#ifdef DEBUGMACROEXP
      fprintf (stderr, "   genopcode = %s\n", genopcode);
#endif

      /*
      ** Process num field
      */

      done = FALSE;
      gennumfield[0] = '\0';
      token = gennumfield;
      cp = mac->maclines[i]->numfield;

      while (*cp && !done)
      {
	 if ((ocp = strchr(cp, MACPARMSYM)) != NULL)
	 {
	    if (ocp > cp)
	       strncat (token, cp, ocp-cp);
	    ocp++;
	    strncpy (genindex, ocp, 2);
	    ocp += 2;
	    cp = ocp;
	    genindex[2] = '\0';
	    j = atoi (genindex);
	    strcat (token, lclargs[j]);
	 }
	 else
	 {
	    strcat (token, cp);
	    done = TRUE;
	 }
      }
#ifdef DEBUGMACROEXP
      fprintf (stderr, "   gennumfield = %s\n", gennumfield);
#endif

      /*
      ** Process operand field
      */

      done = FALSE;
      genoperand[0] = '\0';
      token = genoperand;
      cp = mac->maclines[i]->operand;

      while (*cp && !done)
      {
	 if ((ocp = strchr(cp, MACPARMSYM)) != NULL)
	 {
	    if (ocp > cp)
	       strncat (token, cp, ocp-cp);
	    ocp++;
	    strncpy (genindex, ocp, 2);
	    ocp += 2;
	    cp = ocp;
	    genindex[2] = '\0';
	    j = atoi (genindex);
	    strcat (token, lclargs[j]);
	 }
	 else
	 {
	    strcat (token, cp);
	    done = TRUE;
	 }
      }
#ifdef DEBUGMACROEXP
      fprintf (stderr, "   genoperand = %s\n", genoperand);
#endif
      sprintf (genline, "     %-10.10s%-5.5s%-2.2s%-58.58s",
	       genlabel, genopcode, gennumfield, genoperand);
      strcat (genline, "\n");
      
      /*
      ** Lookup opcode for macro expansion control
      */

      lclnum = 0;
      tokscan (genopcode, &token, &tokentype, &val, &term);
      op = oplookup (token);

      /*
      ** Check Pseudo ops that control macro
      */

      if (op && op->optype == TYPE_P)
      {
	 switch (op->opvalue)
	 {
	 case MEND_T:
	    genmode = MACEXP;
	    if (mac->macnested)
	    {
	       strcpy (nestmac[nestndx++], genline);
	       p0macrostring (nestmac);   
	       for (j = 0; j < nestndx; j++)
		  DBG_MEMREL (nestmac[j]);
	       nestndx = 0;
	    }
	    else
	    {
	       fwrite (&genmode, 1, 4, outfd);
	       fputs (genline, outfd);
#ifdef DEBUGP0OUT
	       fprintf (stderr, "g03(%06X):%s", genmode, genline);
#endif
	       linenum++;
	    }
	    break;

	 default:
	    genmode = MACEXP;
	    fwrite (&genmode, 1, 4, outfd);
	    fputs (genline, outfd);
#ifdef DEBUGP0OUT
	    fprintf (stderr, "g04(%06X):%s", genmode, genline);
#endif
	    linenum++;
	 }
      }

      /*
      ** Check if macro is called in macro
      */

      else if ((lclmac = maclookup (genopcode)) != NULL)
      {
	 genmode = (MACEXP | MACCALL);
	 if (mac->macnested)
	    strcpy (nestmac[nestndx++], genline);
	 else
	 {
	    fwrite (&genmode, 1, 4, outfd);
	    fputs (genline, outfd);
#ifdef DEBUGP0OUT
	    fprintf (stderr, "g02(%06X):%s", genmode, genline);
#endif
	    linenum++;
	 }
	 strcat (genoperand, "   \n");
	 p0expand (lclmac, lclnum, genoperand, infd, outfd);
      }

      /*
      ** None of the above, send it on to next pass
      */

      else
      {
	 genmode = MACEXP;
	 if (mac->macnested)
	    strcpy (nestmac[nestndx++], genline);
	 else
	 {
	    fwrite (&genmode, 1, 4, outfd);
	    fputs (genline, outfd);
#ifdef DEBUGP0OUT
	    fprintf (stderr, "g01(%06X):%s", genmode, genline);
#endif
	    linenum++;
	 }
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
* asmpass0 - Pass 0
***********************************************************************/

int
asmpass0 (FILE *tmpfd0, FILE *tmpfd1)
{
   MacDef *mac;
   char *token;
   int i;
   int done;
   int numfield;
   int genmode;
   int status = 0;
   int tokentype;
   int val;
   int begcomments;
   char term;
   char opcode[MAXSYMLEN+2];

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
   macscan = FALSE;

   /*
   ** Rewind the output.
   */

   if (fseek (tmpfd1, 0, SEEK_SET) < 0)
   {
      perror ("asm7080: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Preload system macros
   */

   for (i = 0; sysmacros[i]; i++)
   {
      p0macrostring (sysmacros[i]);
   }

   /*
   ** Process the source.
   */

   pc = STARTPC;
   linenum = 0;
   linenum = 0;

   done = FALSE;

   etcbuf[0] = '\0';

   begcomments = TRUE;

   tmpfd = tmpfd0;

   while (!done)
   {
      char *bp;

      srcbuf[0] = '\0';
      curpos = ftell (tmpfd);
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
	    status = 2;
	 }
	 else
	 {
	    status = 1;
	 }
         done = TRUE;
	 break;
      }

      linenum++;
      genmode = 0;
      exprtype = ADDREXPR;
      radix = 10;

      strcpy (inbuf, srcbuf);
#ifdef DEBUGP0RDR
      fprintf (stderr, "P0in = %s", inbuf);
#endif
      bp = &inbuf[SYMSTART];

      /*
      ** If not a comment, then process.
      */

      if (*bp != COMMENTSYM)
      {
	 OpCode *op;

	 begcomments = FALSE;

	 /*
	 ** If label present, scan it off.
	 */

	 if (strncmp (bp, "      ", 6))
	 {
	    char *cp;

	    strncpy (cursym, bp, MAXSYMLEN);
	    cursym[MAXSYMLEN] = '\0';
	    for (cp = cursym; *cp; cp++); /* allow for short label only lines */
	    *cp-- = '\0';
	    while (isspace(*cp)) *cp-- = '\0';
#ifdef DEBUGCURSYM
	    fprintf (stderr, "asmpass0: cursym = %s\n", cursym);
	    fprintf (stderr, "    rem = %s", bp);
#endif
	 }
	 else 
	    cursym[0] = '\0';

	 /*
	 ** Scan off opcode.
	 */

	 bp = &inbuf[OPCSTART];
	 i = NUMSTART - OPCSTART;
	 strncpy (opcode, bp, i);
	 opcode[i] = '\0';
	 for (i--; i; i--)
	 {
	    if (isspace (opcode[i])) opcode[i] = '\0';
	    else break;
	 }

	 /*
	 ** Scan off numeric field
	 */

	 bp = &inbuf[NUMSTART];
	 numfield = 0;

	 for (; bp < &inbuf[VARSTART]; bp++)
	 {
	    if (*bp != ' ' && isdigit(*bp))
		  numfield = numfield * 10 + (*bp - '0');
	 }
	 bp = &inbuf[VARSTART];

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
	 ** Check macro table first, in case of name override.
	 */

	 if ((mac = maclookup (opcode)) != NULL)
	 {
	    genmode |= MACCALL;
	    fwrite (&genmode, 1, 4, tmpfd1);
	    fputs (srcbuf, tmpfd1);
#ifdef DEBUGP0OUT
	    fprintf (stderr, "s07(%06X):%s", genmode, srcbuf);
#endif
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

		  fwrite (&etcmode, 1, 4, tmpfd1);
		  fputs (etcbuf, tmpfd1);
#ifdef DEBUGP0OUT
		  fprintf (stderr, "s08(%06X):%s", etcmode, etcbuf);
#endif
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
	    p0expand (mac, numfield, bp, tmpfd, tmpfd1);
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

	    if (op)
	    {
	       if (op->optype == TYPE_P)
	       {
		  fwrite (&genmode, 1, 4, tmpfd1);
		  fputs (srcbuf, tmpfd1);
#ifdef DEBUGP0OUT
		  fprintf (stderr, "s01(%06X):%s", genmode, srcbuf);
#endif
		  done = p0pop (op, numfield, bp, tmpfd, tmpfd1, FALSE);
		  if (done) status = 0;
	       }
	       else
	       {
	          char genline[MAXLINE];

		  /*
		  ** Check for indirect address mode.
		  */

		  bp = &inbuf[VARSTART];
	          if (strncmp (bp, "I,", 2))
		     goto SEND_IT_ON;

		  genmode |= MACCALL;
		  fwrite (&genmode, 1, 4, tmpfd1);
		  fputs (srcbuf, tmpfd1);
#ifdef DEBUGP0OUT
		  fprintf (stderr, "s02(%06X):%s", genmode, srcbuf);
#endif
		  genmode = MACEXP;
		  strncpy (genline, srcbuf, VARSTART+1);
		  strcpy (&genline[VARSTART], &srcbuf[VARSTART+2]);
		  strcpy (&genline[RIGHTMARGIN-2], "           \n");
		  strncpy (&genline[OPCSTART], "EIA  ", 5);
		  fwrite (&genmode, 1, 4, tmpfd1);
		  fputs (genline, tmpfd1);
#ifdef DEBUGP0OUT
		  fprintf (stderr, "s03(%06X):%s", genmode, genline);
#endif
		  strncpy (&genline[SYMSTART], "          ", 10);
		  strncpy (&genline[OPCSTART], &srcbuf[OPCSTART], 5);
		  fwrite (&genmode, 1, 4, tmpfd1);
		  fputs (genline, tmpfd1);
#ifdef DEBUGP0OUT
		  fprintf (stderr, "s04(%06X):%s", genmode, genline);
#endif
	       }
	    }

	    /*
	    ** None of the above, send on
	    */

	    else
	    {
	    SEND_IT_ON:
	       pc += 5;
	       fwrite (&genmode, 1, 4, tmpfd1);
	       fputs (srcbuf, tmpfd1);
#ifdef DEBUGP0OUT
	       fprintf (stderr, "s05(%06X):%s", genmode, srcbuf);
#endif
	    }
	 }
      }

      /*
      ** A comment.
      */

      else
      {
	 fwrite (&genmode, 1, 4, tmpfd1);
	 fputs (srcbuf, tmpfd1);
#ifdef DEBUGP0OUT
	 fprintf (stderr, "s06(%06X):%s", genmode, srcbuf);
#endif
      }

   }

   /*
   ** Clean up
   */

   for (i = 0; i < macrocount; i++)
   {
      int j;
      for (j = 0; j < macdef[i].maclinecount; j++)
      {
         macdef[i].maclines[j]->next = freemaclines;
	 freemaclines = macdef[i].maclines[j];
      }
   }

#ifdef DEBUGMULTIFILE
   fprintf (stderr, "asmpass0: %d lines in file\n", linenum);
#endif

   pscanbuf = inbuf;
   return (status);
}
