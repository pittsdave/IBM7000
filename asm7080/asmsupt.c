/***********************************************************************
*
* asmsupt.c - Support routines for the IBM 7080 assembler.
*
* Changes:
*   02/12/07   DGP   Original.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "asmdef.h"
#include "asmdmem.h"
#include "errors.h"

extern int pc;			/* the assembler pc */
extern int symbolcount;		/* Number of symbols in symbol table */
extern int radix;		/* Number scanner radix */
extern int linenum;		/* Source line number */
extern int errcount;		/* Number of errors in assembly */
extern int errnum;		/* Number of pass 2 errors for current line */
extern int exprtype;		/* Expression type */
extern int p1errcnt;		/* Number of pass 0/1 errors */
extern int p1erralloc;		/* Number of pass 0/1 errors allocated */
extern int pgmlength;		/* Length of program */
extern int genxref;		/* Generate cross reference listing */
extern int inpass;		/* Which pass are we in */
extern int litorigin;		/* Literal pool origin */
extern int litpoolsize;		/* Literal pool size */
extern int widemode;		/* Generate wide listing */
extern int filenum;		/* INSERT file index */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char errline[10][120];	/* Pass 2 error lines for current statment */
extern char incldir[MAXLINE];	/* -I include directory */

extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern SymNode *freesymbols;	/* Reusable symbols nodes */
extern XrefNode *freexrefs;	/* Reusable xref nodes */
extern ErrTable p1error[MAXERRORS];/* The pass 0/1 error table */
extern char *pscanbuf;		/* Pointer for tokenizers */

static char errtmp[120];	/* Error print string */
#ifdef DEBUGSYM
static int ssdebug;
#endif

/***********************************************************************
* logerror - Log an error for printing.
***********************************************************************/

void
logerror (char *errmsg)
{
#ifdef DEBUGERROR
   fprintf (stderr, "logerror: linenum = %d, errcount = %d, inpass = %o\n",
	   linenum, errcount, inpass);
   fprintf (stderr, "   p1errcnt = %d, p1erralloc = %d, errnum = %d\n",
	   p1errcnt, p1erralloc, errnum);
   fprintf (stderr, "   errmsg = %s\n", errmsg);
#endif
   if (!chkerror (linenum))
   {
      errcount++;
      if (inpass == 020)
      {
	 strcpy (errline[errnum++], errmsg);
      }
      else
      {
	 if (p1errcnt >= p1erralloc)
	 {
	    p1error[p1errcnt].errortext = (char *)DBG_MEMGET (120);
	    p1erralloc++;
	 }
	 p1error[p1errcnt].errorline = linenum;
	 strcpy (p1error[p1errcnt].errortext, errmsg);
	 p1errcnt++;
      }
   }
}

/***********************************************************************
* Parse_Error - Print parsing error
***********************************************************************/

void
Parse_Error (int cause, int state, int defer)
{
   char errorstring[256];

#ifdef DEBUGPARSEERROR
   fprintf (stderr, 
   "Parse_Error: pass = %o, defer = %s, cause = %d, state = %d, linenum = %d\n",
	 inpass, defer ? "TRUE " : "FALSE", cause, state, linenum);
#endif
   if (defer) return;

   strcpy (errorstring, "Unknown error");
   if (cause == PARSE_ERROR)
   {
      switch (state)
      {
      /* Include generated parser errors */
#include "asm7080.err"
      default: ;
      }
   }
   else if (cause == SCAN_ERROR)
   {
      switch (state)
      {
      case INVOCTNUM:
	 strcpy (errorstring, "Invalid octal number");
	 break;
      case INVEXPONENT:
	 strcpy (errorstring, "Exponent out of range");
	 break;
      case INVSYMBOL:
	 strcpy (errorstring, "Invalid symbol");
	 break;
      default: ;
      }
   }
   else if (cause == INTERP_ERROR)
   {
      strcpy (errorstring, "Divide by zero");
   }

   sprintf (errtmp, "%s in expression", errorstring);
   logerror (errtmp);

} /* Parse_Error */

/***********************************************************************
* tokscan - Scan a token.
***********************************************************************/

char *
tokscan (char *buf, char **token, int *tokentype, int *tokenvalue, char *term)
{
   int index;
   toktyp tt;
   tokval val;
   static char t[80];
   char c;

#ifdef DEBUGTOK
   fprintf (stderr, "tokscan: Entered: buf = %s", buf);
#endif

   /*
   ** Call the scanner to get the token
   */

   index = 0;
#ifdef DEBUGTOK
   fprintf (stderr, "  calling scanner: buf = %s\n", buf);
#endif
   tt = Scanner (buf, &index, &val, t, &c, inpass == 020 ? FALSE : TRUE); 
#ifdef DEBUGTOK
   fprintf (stderr, "  token = %s, tokentype = %d, term = %02x, val = %d\n",
	    t, tt, c, val);
   fprintf (stderr, " index = %d, *(buf+index) = %02X\n",  index, *(buf+index));
#endif

   /*
   ** Return proper token and index
   */

   if (tt == DECNUM || tt == OCTNUM)
      c = *(buf+index);
   if (c == ',') index++;
   *token = t;
   *tokentype = tt;
   *tokenvalue = val;
   *term = c;
   return (buf+index);
}

/***********************************************************************
* freexref - Link an xref entry on free chain.
***********************************************************************/

void
freexref (XrefNode *xr)
{
   xr->next = freexrefs;
   freexrefs = xr;
}

/***********************************************************************
* freesym - Link a symbol entry on free chain.
***********************************************************************/

void
freesym (SymNode *sr)
{
   sr->next = freesymbols;
   freesymbols = sr;
}

/***********************************************************************
* symdelete - Delete a symbol from the symbol table.
***********************************************************************/

void
symdelete (SymNode *sym)
{
   int i;

   for (i = 0; i < symbolcount; i++)
   {
      if (symbols[i] == sym)
      {
	 XrefNode *xr, *nxr;

	 xr = symbols[i]->xref_head;
	 while (xr)
	 {
	    nxr = xr->next;
	    freexref (xr);
	    xr = nxr;
	 }
         freesym (symbols[i]);
	 for (; i < symbolcount; i++)
	 {
	    symbols[i] = symbols[i+1];
	 }
	 symbolcount --;
         return;
      }
   }
}

/***********************************************************************
* symlookup - Lookup a symbol in the symbol table.
***********************************************************************/

SymNode *
symlookup (char *sym, int add, int xref)
{
   SymNode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;

#ifdef DEBUGSYM
#ifdef DEBUGSYMVAL
   ssdebug = !strcmp (sym, "UPDMFD");
#else
   ssdebug = TRUE;
#endif
   if (ssdebug)
   {
      fprintf (stderr, "symlookup: Entered: sym = '%s'\n", sym);
      fprintf (stderr,
	    "   add = %s, xref = %s, inpass = %03o, pc = %05o, line = %d\n",
	    add ? "TRUE" : "FALSE",
	    xref ? "TRUE" : "FALSE",
	    inpass, pc, linenum);
   }
#endif

   /*
   ** Empty symbol table.
   */

   if (symbolcount == 0)
   {
      if (!add) return (NULL);

#ifdef DEBUGSYM
      if (ssdebug)
	 fprintf (stderr, "   add symbol at top\n");
#endif
      if (freesymbols)
      {
         symbols[0] = freesymbols;
	 freesymbols = symbols[0]->next;
      }
      else if ((symbols[0] = (SymNode *)DBG_MEMGET (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "asm7080: Unable to allocate memory\n");
	 exit (ABORT);
      }
      memset (symbols[0], '\0', sizeof (SymNode));
      strcpy (symbols[0]->symbol, sym);
      symbols[0]->value = pc;
      symbols[0]->line = linenum;
#ifdef DEBUGXREF
      fprintf (stderr, "symlookup: sym = %s defline = %d\n", sym, linenum);
#endif
      symbols[0]->flags &= ~RELOCATABLE;
      if (inpass == 000)
	 symbols[0]->flags |= P0SYM;
      symbolcount++;
      return (symbols[0]);
   }

   /*
   ** Locate using binary search
   */

   lo = 0;
   up = symbolcount;
   last = -1;
   
   while (!done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGSYMSEARCH
      fprintf (stderr, "   mid = %d, last = %d\n", mid, last);
#endif
      if (symbolcount == 1)
	 done = TRUE;
      else if (last == mid) break;

      r = strcmp (symbols[mid]->symbol, sym);

      /*
      ** We have a hit
      */

      if (r == 0)
      {
	 SymNode *symp = symbols[mid];

	 if (add)
	 {
	    if (!(symbols[mid]->flags & P0SYM))
	       return (NULL); /* must be a duplicate */

#ifdef DEBUGSYM
	    if (ssdebug)
	       fprintf (stderr, "   update symbol\n");
#endif
	    symbols[mid]->flags &= ~P0SYM;
	    symbols[mid]->value = pc;
	    symbols[mid]->line = linenum;
	    symbols[mid]->flags &= ~RELOCATABLE;
	 }

#ifdef DEBUGSYM
	 if (ssdebug)
	    fprintf (stderr, "   found symbol, value = %05o\n",
		  symbols[mid]->value);
#endif

	 /*
	 ** If xref mode, chain a line reference
	 */

	 if (genxref && xref)
	 {
	    XrefNode *new;

	    if (freexrefs)
	    {
	       new = freexrefs;
	       freexrefs = new->next;
	    }
	    else if ((new = (XrefNode *)DBG_MEMGET (sizeof (XrefNode))) == NULL)
	    {
	       fprintf (stderr, "asm7080: Unable to allocate memory\n");
	       exit (ABORT);
	    }
#ifdef DEBUGXREF
	    fprintf (stderr, "symlookup: sym = %s refline = %d\n",
		     sym, linenum);
#endif
	    memset (new, '\0', sizeof(XrefNode));
	    new->next = NULL;
	    new->line = linenum;
	    if (symp->xref_head == NULL)
	    {
	       symp->xref_head = new;
	    }
	    else
	    {
	       (symp->xref_tail)->next = new;
	    }
	    symp->xref_tail = new;
	 }
         return (symbols[mid]);
      }

      /*
      ** Otherwise, get next symbol
      */

      else if (r < 0)
      {
         lo = mid;
      }
      else 
      {
         up = mid;
      }
      last = mid;
   }

   /*
   ** Not found, check to add
   */

   if (add)
   {
      SymNode *new;

#ifdef DEBUGSYM
      if (ssdebug)
	 fprintf (stderr, "   add new symbol\n");
#endif
      /*
      ** Add to table, allocate storage and initialize symbol
      */

      if (symbolcount+1 > MAXSYMBOLS)
      {
         fprintf (stderr, "asm7080: Symbol table exceeded\n");
	 exit (ABORT);
      }

      if (freesymbols)
      {
         new = freesymbols;
	 freesymbols = new->next;
      }
      else if ((new = (SymNode *)DBG_MEMGET (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "asm7080: Unable to allocate memory\n");
	 exit (ABORT);
      }

      memset (new, '\0', sizeof (SymNode));
      strcpy (new->symbol, sym);
#ifdef DEBUGXREF
      fprintf (stderr, "symlookup: sym = %s defline = %d\n", sym, linenum);
#endif
      new->value = pc;
      new->line = linenum;
      new->flags &= ~RELOCATABLE;
      if (inpass == 000)
	 new->flags |= P0SYM;

      /*
      ** Insert pointer in sort order.
      */

      for (lo = 0; lo < symbolcount; lo++)
      {
         if (strcmp (symbols[lo]->symbol, sym) > 0)
	 {
	    for (up = symbolcount + 1; up > lo; up--)
	    {
	       symbols[up] = symbols[up-1];
	    }
	    symbols[lo] = new;
	    symbolcount++;
	    return (symbols[lo]);
	 }
      }
      symbols[symbolcount] = new;
      ret = symbols[symbolcount];
      symbolcount++;
   }
   return (ret);
}

/***********************************************************************
* exprscan - Scan an expression and return its value.
***********************************************************************/

char *
exprscan (char *bp, int *val, char *tterm, int defer)
{
   char *cp;
   int spaces = FALSE;
   int retsize = FALSE;
   int index = 0;
   int sval;
   tokval lval = 0;

#ifdef DEBUGEXPR
   fprintf (stderr, "exprscan: entered: bp = %s\n", bp);
#endif
   cp = bp;

   /*
   ** Skip leading blanks
   */

   while (*bp && isspace(*bp))
   {
      spaces = TRUE;
      bp++;
   }

   /*
   ** If no operands, return EOL
   */

   if (spaces && (bp - pscanbuf >= NOOPERAND))
   {
      bp = cp;
      *tterm = '\n';
      *val = lval;
      return (bp);
   }

#ifdef DEBUGEXPR
   fprintf (stderr, "   bp = %s\n", bp);
#endif

   /*
   ** Check for operand modifiers.
   */

   if (!strncmp (bp, "S,", 2))
   {
      retsize = TRUE;
      bp += 2;
   }

   /*
   ** If literal then scan it off and put in symbol table
   */

   if (*bp == LITERALSYM)
   {
      SymNode *s;
      char *cp, *sp;
      int fpnum;
      int litsigned;
      int litlen;
      int badliteral;

      badliteral = FALSE;
      cp = bp+1;
      while (*cp && *cp != LITERALSYM) cp++;
      if (*cp != LITERALSYM)
      {
         if ((inpass & 070) == 020)
	    logerror ("Non terminated literal");
         badliteral = TRUE;
	 lval = 0;
      }
      cp++;
      *tterm = *cp;
      index = cp - bp;
      *cp = '\0';

#ifdef DEBUGLIT
      fprintf (stderr,
	 "exprscan: pc = %d, litpoolsize = %d, litorigin = %d, inpass = %o\n",
	       pc, litpoolsize, litorigin, inpass);
#endif
      if (!badliteral)
      {
	 if (!(s = symlookup (bp, FALSE, TRUE)))
	 {
	    /*
	    ** Figure in memory length of literal
	    */

	    sp = bp+1;
	    litlen = 0;
	    fpnum = FALSE;

	    /* Signed number (maybe floating point). */
	    if (*sp == '-' || *sp == '+')
	    {
	       litsigned = TRUE;
	       for (sp++; *sp != LITERALSYM; sp++)
	       {
		  if (*sp == '-' || *sp == '+')
		  {
		     litlen = FLOATPOINTLENGTH;
		     fpnum = TRUE;
		     break;
		  }
		  if (*sp != '.')
		     litlen++;
	       }
	    }

	    /* UnSigned number or mixed chracters */
	    else
	    {
	       litsigned = FALSE;
	       for (;*sp != LITERALSYM; sp++) litlen++;
	    }

#ifdef DEBUGLIT
	    fprintf (stderr,
	    "exprscan: litsigned = %d, fpnum = %d, litlen = %d, literal = %s\n",
		     litsigned, fpnum, litlen, bp);
#endif

	    if (!(s = symlookup (bp, TRUE, TRUE)))
	    {
	       sprintf (errtmp, "Unable to add Literal: %s", bp);
	       logerror (errtmp);
	       lval = 0;
	    }
	    else
	    {
	       if (litsigned)
	       {
		  s->flags |= SIGNEDLITERAL;
		  if (fpnum)
		     s->flags |= FLOATLITERAL;
	       }
	       else
	       {
		  s->flags |= MIXEDLITERAL;
	       }
	       s->length = litlen;

	       if ((inpass & 070) == 010)
	       {
		  s->value = pc;
		  litpoolsize += litlen;
	       }
	       else
	       {
		  if (litorigin)
		  {
		     s->value = litorigin + litpoolsize;
		     litpoolsize += litlen;
		  }
		  else
		  {
		     s->value = pgmlength;
		     pgmlength += litlen;
		  }
	       }
	    }
	 }
      }
      *cp = *tterm;
      if (s)
      {
	 if (retsize)
	    lval = s->length;
	 else
	    lval = s->value;
      }
   }

   /*
   ** Call the parser to scan the expression
   */

   else if (strlen(bp) && *bp != ',')
   {
      if (*bp == '-')
      {
         if (*(bp+1) == ',' || isspace(*(bp+1)))
	 {
	    index = 1;
	    *tterm = *(bp+index);
	    *val = lval;
	    if (*tterm == ',') index++;
	    return (bp+index);
	 }
	 if (*(bp+1) == '1' && (*(bp+2) == ',' || isspace(*(bp+2))))
	 {
	    lval = (exprtype & BOOLVALUE) ? 0777777 : 077777 ;
	    index = 2;
	    *tterm = *(bp+index);
	    *val = lval;
	    if (*tterm == ',') index++;
	    return (bp+index);
	 }
      }
      else if (*bp == '/' && (*(bp+1) == ',' || isspace (*(bp+1))))
      {
	 lval = 0;
	 index = 1;
	 *tterm = *(bp+index);
	 *val = lval;
	 if (*tterm == ',') index++;
	 return (bp+index);
      }
      lval = Parser (bp, &index, retsize, defer);
   }
#ifdef DEBUGEXPR
   fprintf (stderr, " index = %d, *(bp+index) = %02X\n",
	 index, *(bp+index));
   fprintf (stderr, " sval = %d, lval = %d\n", sval, lval);
#endif
   
   /*
   ** Return proper value and index
   */

   *tterm = *(bp+index);
   if (*tterm == ',') index++;
   *val = lval;
   return (bp+index);
}
