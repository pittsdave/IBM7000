/***********************************************************************
*
* lnksupt.c - Support routines for the IBM 7090 linker.
*
* Changes:
*   05/21/03   DGP   Original.
*   03/29/10   DGP   Added module numbers to listing.
*   03/30/10   DGP   Added cross reference and wide listing.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "lnkdef.h"

extern int pc;
extern int symbolcount;
extern int absolute;
extern int pgmlength;
extern int modcount;
extern int pass;
extern int genxref;
extern char inbuf[MAXLINE];
extern SymNode *symbols[MAXSYMBOLS];
extern Memory memory[MEMSIZE];


/***********************************************************************
* symlookup - Lookup a symbol in the symbol table.
***********************************************************************/

SymNode *
symlookup (char *sym, char *module, int add)
{
   SymNode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;

#ifdef DEBUGSYM
   printf ("symlookup: Entered: sym = %s, module = %s\n", sym, module);
#endif

   /*
   ** Empty symbol table.
   */

   if (symbolcount == 0)
   {
      if (!add) return (NULL);

#ifdef DEBUGSYM
      printf ("add symbol at top\n");
#endif
      if ((symbols[symbolcount] = (SymNode *)malloc (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "lnk7090: Unable to allocate memory\n");
	 exit (ABORT);
      }
      strcpy (symbols[0]->symbol, sym);
      strcpy (symbols[0]->module, module);
      symbols[0]->xref_head = NULL;
      symbols[0]->xref_tail = NULL;
      symbols[0]->value = pc;
      symbols[0]->external = FALSE;
      symbols[0]->global = FALSE;
      symbols[0]->undef = FALSE;
      symbols[0]->muldef = FALSE;
      symbols[0]->modnum = modcount + 1;
      if (absolute)
	 symbols[0]->relocatable = FALSE;
      else
	 symbols[0]->relocatable = TRUE;
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
#ifdef DEBUGSYM
      printf (" mid = %d, last = %d\n", mid, last);
#endif
      if (last == mid) break;
      r = strcmp (symbols[mid]->symbol, sym);
      if (r == 0)
      {
	 if (add) return (NULL); /* must be a duplicate */
         return (symbols[mid]);
      }
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
      printf ("add new symbol\n");
#endif
      if (symbolcount+1 > MAXSYMBOLS)
      {
         fprintf (stderr, "lnk7090: Symbol table exceeded\n");
	 exit (ABORT);
      }

      if ((new = (SymNode *)malloc (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "lnk7090: Unable to allocate memory\n");
	 exit (ABORT);
      }

      strcpy (new->symbol, sym);
      strcpy (new->module, module);
      new->xref_head = NULL;
      new->xref_tail = NULL;
      new->value = pc;
      new->external = FALSE;
      new->global = FALSE;
      new->undef = FALSE;
      new->muldef = FALSE;
      new->modnum = modcount + 1;
      if (absolute)
	 new->relocatable = FALSE;
      else
	 new->relocatable = TRUE;

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

