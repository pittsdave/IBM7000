/***********************************************************************
*
* asmpass1 - Pass 1 for the IBM 7090 assembler.
*
* Changes:
*   05/21/03   DGP   Original.
*   08/20/03   DGP   Changed to call the SLR(1) parser.
*   12/20/04   DGP   Add MAP functions and 2.0 additions.
*   02/02/05   DGP   Correct VFD processing.
*   02/09/05   DGP   Added FAP support.
*   03/03/05   DGP   Fixed bugs in FAP EQU processing.
*   03/09/05   DGP   Fixed BCD and BCI processing.
*   03/11/05   DGP   Fixed chained IF[FT] statements.
*   03/15/05   DGP   Fixed "LOC  blank" in FAP mode.
*   06/07/05   DGP   Fixed FAP IFF.
*   06/08/05   DGP   Made ORG always relocatable in relocatable assembly.
*                    Added character delimiter to BCI.
*   08/09/06   DGP   Correct NULL op relocatability.
*   05/14/09   DGP   Allow BCD to start with 0.
*   09/30/09   DGP   Added FAP Linkage Director.
*   10/16/09   DGP   Fixup EVEN op.
*   11/03/09   DGP   Allow opcode terminated by '('.
*                    Allow EOS to terminate a VFD/ETC block.
*   03/31/10   DGP   Fixed "IFF N" processing.
*   05/27/10   DGP   Fix BES relocation.
*   06/08/10   DGP   Fix IFF skip count.
*   11/01/10   DGP   Added COMMON Common mode.
*   12/13/10   DGP   Correct spacing for VFD.
*   12/15/10   DGP   Added line number to temporary file.
*                    Added file sequence for errors.
*   12/16/10   DGP   Added a unified source read function readsource().
*   12/23/10   DGP   Fixed COMMON to generate correct address for label.
*   05/06/13   DGP   Fixed QUAL processing.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "asmdef.h"


extern int pc;			/* the assembler pc */
extern int symbolcount;		/* Number of symbols in symbol table */
extern int absolute;		/* In absolute section */
extern int radix;		/* Number scanner radix */
extern int linenum;		/* Source line number */
extern int exprtype;		/* Expression type */
extern int p1errcnt;		/* Number of pass 0/1 errors */
extern int pgmlength;		/* Length of program */
extern int absmod;		/* In absolute module */
extern int termparn;		/* Parenthesis are terminals (NO()) */
extern int genxref;		/* Generate cross reference listing */
extern int addext;		/* Add extern for undef'd symbols (!absolute) */
extern int addrel;		/* ADDREL mode */
extern int qualindex;		/* QUALifier table index */
extern int begincount;		/* Number of BEGINs */
extern int litorigin;		/* Literal pool origin */
extern int litpool;		/* Literal pool size */
extern int fapmode;		/* FAP assembly mode */
extern int headcount;		/* Number of entries in headtable */
extern int inpass;		/* Which pass are we in */
extern int errcount;		/* Number of errors in assembly */
extern int genlnkdir;		/* FAP generate linkage director */
extern int commonctr;		/* Common counter */
extern int commonused;		/* Common used */
extern int filenum;		/* Current file number */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char deckname[MAXLBLLEN+2];/* The assembly deckname */
extern char lbl[MAXLBLLEN+2];	/* The object label */
extern char ttlbuf[TTLSIZE+2];	/* The assembly TTL buffer */
extern char qualtable[MAXQUALS][MAXSYMLEN+2]; /* The QUALifier table */
extern char headtable[MAXHEADS];/* HEAD table */

extern SymNode *addextsym;	/* Added symbol for externals */
extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern ErrTable p1error[MAXERRORS];/* The pass 0/1 error table */
extern BeginTable begintable[MAXBEGINS];/* The USE/BEGIN table */


static int curruse;		/* Current USE index */
static int prevuse;		/* Previous USE index */
static int chksyms;		/* Check symbols in this pass */
static int inmode;		/* Source line input mode */
static int asmskip;		/* In skip line mode IFF/IFT */
static int gotoskip;		/* In GOTO mode */
static int gotoblank;		/* BLANK option on GOTO */
static int ifcont;		/* IFF/IFT continued */
static int ifrel;		/* IFF/IFT continue relation OR/AND */
static int ifskip;		/* IFF/IFT prior skip result */
static int prevloc;		/* Previous LOC pc value */
static int locorg;		/* Origin of current LOC */

static char cursym[MAXSYMLEN+2];/* Current label field symbol */
static char gotosym[MAXSYMLEN+2];/* GOTO target symbol */
static char errtmp[256];	/* Error print string */

/***********************************************************************
* p1allop - Operation processors (all we do here is advance the pc).
***********************************************************************/

static void
p1allop (OpCode *op, char *bp)
{
   int relocatable;
   int val;
   char term;

   while (*bp && isspace(*bp)) bp++;
   if (*bp == LITERALSYM)
   {
      bp = exprscan (bp, &val, &term, &relocatable, 1, chksyms, 0);
   }
   pc++;
}

/***********************************************************************
* p1diskop - Disk I/O processors (all we do here is advance the pc).
***********************************************************************/

static void
p1diskop (OpCode *op, char *bp)
{
   pc+=2;
}

/***********************************************************************
* opdadd - Process OPD/OPVFD op adds
***********************************************************************/

static void
opdadd (char *opcode, unsigned op0, unsigned op1)
{
   int newtype;
   unsigned newop, newmode;

   newop = 0;
   newmode = 0;
   newtype = 0;

   if (fapmode)
   {
	 if (op0 & 0300000) newtype = TYPE_A;
	 else if (op0 & 0000060) newtype = TYPE_B;
	 else if (op0 & 0000001) newtype = TYPE_E;
	 else if (op1 & 0000001) newtype = TYPE_D;
	 else if (op1 & 0000002) newtype = TYPE_C;

	 newop = (op0 & 0777700) >> 6;
	 if (op0 & 0000001) newmode = op1 & 0017777;
   }
   else switch ((op0 & 0770000) >> 12)
   {
      case 040: 		/* Type A */
      case 042: 		/* Type A */

	 newtype = TYPE_A;
	 newop = op1 & 0007777;
	 break;

      case 043: 		/* Type B */

	 newtype = TYPE_B;
	 newop = op1 & 0007777;
	 break;

      case 044: 		/* Type C */

	 newtype = TYPE_C;
	 newop = op1 & 0007777;
	 break;

      case 045: 		/* Type D */

	 newtype = TYPE_D;
	 newop = op1 & 0007777;
	 break;

      case 046: 		/* Type E */

	 newtype = TYPE_E;
	 if ((op1 & 0700000) == 0100000)
	    newop = 04760;
	 else
	    newop = 00760;
	 newmode = op1 & 0077777;
	 break;

      default: 

	 logerror ("Unsupported opcode addition");
	 return;
   }

   /*
   ** Add new opcode, if all OK
   */

   opadd (opcode, newop, newmode, newtype);
}

/***********************************************************************
* processliterals - Process literals.
***********************************************************************/

static void
processliterals ()
{
   int i;
   int literalcount = 0;
   int escapecount = 0;

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
	 if (symbols[i]->symbol[0] == LITERALSYM)
	 {
	    SymNode *s;

#ifdef DEBUGLIT
	    fprintf (stderr, "p1-processliterals: pc = %o\n", pc);
#endif
	    s = symbols[i];
	    symdelete (s);
	    pc++;
	    literalcount--;
         }
      }
   }
}

/***********************************************************************
* p1lookup - lookup cursym in current context.
***********************************************************************/

static SymNode *
p1lookup (void)
{
   SymNode *s;
   char temp[32];

#ifdef DEBUGSYM
   fprintf (stderr, "p1lookup: cursym = %s\n", cursym);
#endif
   s = NULL;
   if (fapmode)
   {
      if (strlen(cursym) == MAXSYMLEN)
      {
	 temp[0] = cursym[0];
	 temp[1] = '\0';
	 s = symlookup (&cursym[1], temp, FALSE, FALSE);
      }
      else if (headcount)
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
* p1pop - Process Pseudo operation codes. Most ops just advance the pc.
* Most errors are ignored here and reported in pass 2.
***********************************************************************/

static int
p1pop (OpCode *op, char *bp)
{
   SymNode *s;
   OpCode *addop;
   char *token;
   char *cp;
   int tokentype;
   int relocatable;
   int val;
   int i, j;
   int boolrl;
   int boolr;
   int bitcount;
   int tmpnum0;
   int tmpnum1;
   int bit12mode;
   int combssflag;
   char term;

   bit12mode = FALSE;
   combssflag = FALSE;

   switch (op->opvalue)
   {

   case BCD_T:			/* BCD */
      while (*bp && isspace (*bp)) bp++;
      if (isdigit (*bp))
      {
         val = *bp++ - '0';
	 if (val > 0)
	     pc += val;
	 else
	     pc += 10;
      }
      else
      {
         pc += 10;
      }
      break;

   case BIT12_T:		/* 12BIT */
      bit12mode = TRUE;

   case BCI_T:			/* BCI */
      while (*bp && isspace (*bp)) bp++;
      if (bit12mode) goto DELIM_ONLY;

      if (*bp == ',')
	 val = 10;
      else
      {
	 if (!isdigit (*bp))
	 {
	    int  chcnt;
	    char cterm ;

   DELIM_ONLY:
	    chcnt = 0;
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
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      }
      pc += val;
      break;

   case BEGIN_T:		/* BEGIN */
      if (begincount < MAXBEGINS)
      {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGUSE
	 fprintf (stderr, "p1: BEGIN: token = %s, curruse = %d, pc = %05o\n",
		  token, curruse, pc);
#endif
	 bp = exprscan (bp, &val, &term, &relocatable, 1, chksyms, 0);
	 for (i = 0; i < begincount; i++)
	 {
	    if (!strcmp (token, begintable[i].symbol))
	    {
	       begintable[i].bvalue = val;
	       break;
	    }
	 }
	 if (i == begincount)
	 {
	    strcpy (begintable[begincount].symbol, token);
	    begintable[begincount].bvalue = val;
	    begincount++;
	 }
      }
      break;

   case BES_T:			/* BES */
      bp = exprscan (bp, &val, &term, &relocatable, 1, chksyms, 0);
      pc += val;
      pc &= 077777;
      if (cursym[0])
      {
	 if ((s = p1lookup()) != NULL)
	 {
	    s->value = pc;
	 }
      }
      break;

   case BFT_T:			/* BFT extended op */
   case BNT_T:			/* BNT extended op */
   case IIB_T:			/* IIB extended op */
   case RIB_T:			/* RIB extended op */
   case SIB_T:			/* SIB extended op */
      pc++;
      break;

   case COMBSS_T:		/* COMMON BSS */
      combssflag = TRUE;

   case COMMON_T:		/* COMMON storage */
   case COMBES_T:		/* COMMON BES */
      /*
      ** FAP COMMON
      */

      if (fapmode)
      {
	 commonused = TRUE;
#ifdef DEBUGCOMMON
         fprintf (stderr, "COMMON: commonctr = %05o\n", commonctr);
#endif
	 if (!combssflag && cursym[0])
	 {
	    s = p1lookup();
	    if (s)
	    {
	       s->value = commonctr;
#ifdef DEBUGCOMMON
	       fprintf (stderr, "   BES symbol = %s, value = %05o\n",
	       		cursym, s->value);
#endif
	       s->flags = COMMON;
	    }
	 }
	 bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	 if (val < 0)
	    commonctr += -val;
	 else
	    commonctr -= val;
	 commonctr &= 077777;
	 if (combssflag && cursym[0])
	 {
	    s = p1lookup();
	    if (s)
	    {
	       if (val == 0)
		  s->value = commonctr;
	       else
		  s->value = commonctr+1;
#ifdef DEBUGCOMMON
	       fprintf (stderr, "   BSS symbol = %s, value = %05o\n",
	       		cursym, s->value);
	       fprintf (stderr, "   commonctr = %05o, val = %05o\n",
	       		commonctr, val);
#endif
	       s->flags = COMMON;
	    }
	 }
      }

      /*
      ** MAP COMMON
      */

      else if (inpass == 012)
      {
	 i = begincount - 1;
	 if (!begintable[i].value)
	    begintable[i].value = begintable[i].bvalue;
	 val = begintable[i].value;
#ifdef DEBUGUSE
         fprintf (stderr, "asmpass0: COMMON: common = %d, val = %05o\n",
	       i, val);
#endif
         if (cursym[0])
	 {
	    if ((s = p1lookup()) != NULL)
	       s->value = val;
	 }
	 bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	 begintable[i].value += val;
#ifdef DEBUGUSE
         fprintf (stderr, "   new val = %05o\n", begintable[i].value);
#endif
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
		     s->flags = BOOL; 
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
	    s->flags = BOOL; 
	    if (boolrl) s->flags |= LRBOOL;
	    if (boolr) s->flags |= RBOOL;
	 }
      }
      radix = 10;
      break;

   case BSS_T:			/* BSS */
      bp = exprscan (bp, &val, &term, &relocatable, 1, chksyms, 0);
      pc += val;
      pc &= 077777;
      break;

   case DEC_T:			/* DEC */
   case OCT_T:			/* OCT */
      exprtype = DATAEXPR | DATAVALUE;
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 if (tokentype == '-' || tokentype == '+')
	 {
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 }
	 if (tokentype == DBLFNUM || tokentype == DBLBNUM) pc++;
	 pc ++;
      } while (term == ',');
      break;

   case END_T:			/* END */
      val = pc;
      if (!litorigin) litorigin = pc;
      processliterals();
      pgmlength = val + litpool;
      litpool = 0;
      return (TRUE);

   case ENDQ_T:			/* ENDQ */
      qualindex --;
      if (qualindex < 0) qualindex = 0;
      break;

   case EQU_T:			/* EQU */
      bp = exprscan (bp, &val, &term, &relocatable, 1, chksyms, 0);

      /*
      ** Set symbol to value
      */

DO_EQU:
      if (cursym[0])
      {
#ifdef DEBUGEQU
	 fprintf (stderr, "p1pop: EQU: cursym = '%s', val = %05o\n",
	       cursym, val);
#endif
	 if (fapmode)
	 {
	    if (headcount && (strlen(cursym) < MAXSYMLEN))
	    {
	       for (i = 0; i < headcount; i++)
	       {
		  char temp[32];

		  sprintf (temp, "%c", headtable[i]);
#ifdef DEBUGEQU
		  fprintf (stderr, "E  cursym = '%s', head = '%s' \n",
			   cursym, temp);
#endif
		  if (!(s = symlookup (cursym, temp, FALSE, FALSE)))
		     s = symlookup (cursym, temp, TRUE, FALSE);

		  if (s)
		  {
		     s->line = linenum;
		     if (addextsym)
		     {
#ifdef DEBUGEQU
			fprintf (stderr, "   cursym value = 0%o\n", s->value);
			fprintf (stderr, "   addextsym = '%s', val = 0%o\n",
				 addextsym->symbol, addextsym->value);
#endif
			addextsym->value = s->value;
			if (addextsym->flags & EXTERNAL) s->flags |= EXTERNAL;
			s->flags |= NOEXPORT;
			s->vsym = addextsym;
		     }
		     else
		     {
			s->value = val;
#ifdef DEBUGEQU
			fprintf (stderr, "   cursym value = 0%o\n", s->value);
#endif
			s->line = linenum;
			if (relocatable) s->flags |= RELOCATABLE;
			else             s->flags &= ~RELOCATABLE;
		     }
		  }
	       }
	       s = NULL;
	    }
	    else
	    {
#ifdef DEBUGEQU
	       fprintf (stderr, "F  cursym = '%s', head = '%s' \n", cursym, "");
#endif
	       if (!(s = symlookup (cursym, "", FALSE, FALSE)))
		  s = symlookup (cursym, "", TRUE, FALSE);
	    }
	 }
	 else
	 {
#ifdef DEBUGEQU
	    fprintf (stderr, "G  cursym = '%s', qual = '%s' \n",
		    cursym, qualtable[qualindex]);
#endif
	    if (!(s = symlookup (cursym, qualtable[qualindex], FALSE, FALSE)))
	       s = symlookup (cursym, qualtable[qualindex], TRUE, FALSE);
	 }

	 if (s)
	 {
	    s->line = linenum;
	    if (addextsym)
	    {
#ifdef DEBUGEQU
	       fprintf (stderr, "   cursym value = 0%o\n", s->value);
	       fprintf (stderr, "   addextsym = '%s', val = 0%o\n",
			addextsym->symbol, addextsym->value);
#endif
	       addextsym->value = s->value;
	       if (addextsym->flags & EXTERNAL) s->flags |= EXTERNAL;
	       s->flags |= NOEXPORT;
	       s->vsym = addextsym;
	    }
	    else
	    {
	       s->value = val;
#ifdef DEBUGEQU
	       fprintf (stderr, "   cursym value = 0%o\n", s->value);
#endif
	       if (relocatable) s->flags |= RELOCATABLE;
	       else             s->flags &= ~RELOCATABLE;
	    }
	 }
      }
      break;

   case EVEN_T:			/* EVEN */
      if (pc & 00001) pc++;
      break;

   case EXT_T:			/* EXTern */
      if (!chksyms) do
      {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 if (tokentype != SYM)
	 {
	    sprintf (errtmp, "EXTRN requires symbol: %s",
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
	 if (!(s = symlookup (token, qualtable[qualindex], FALSE, TRUE)))
	    s = symlookup (token, qualtable[qualindex], TRUE, TRUE);
	 if (s)
	 {
	    s->value = 0;
	    if (absmod || absolute)
	       s->flags &= ~RELOCATABLE;
	    else
	       s->flags |= RELOCATABLE;
	    s->flags |= EXTERNAL;
	 }
      } while (term == ',');
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
      fprintf (stderr, "asmpass1: GOTO: gotosym = '%s', gotoblank = %s\n",
	    gotosym, gotoblank ? "TRUE" : "FALSE");
#endif
      gotoskip = TRUE;
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
      if (fapmode)
      {
	 char tok1[TOKENLEN];
	 char tok2[TOKENLEN];

	 tok1[0] = '\0';
	 tok2[0] = '\0';
	 asmskip = 0;
	 bp = exprscan (bp, &val, &term, &relocatable, 1, TRUE, 0);
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
#ifdef DEBUGP1IFF
	 fprintf (stderr,
	    "p1IFF: asmskip = %d, val = %d, i = %d, tok1 = '%s', tok2 = '%s'\n",
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
	    fprintf (stderr, "p1IFF: val = %d, ifcont = %s\n",
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
	 fprintf (stderr, "p1IFT: val = %d, ifcont = %s\n",
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
      break;

   case LIT_T:			/* LITeral to pool */
      exprtype = DATAEXPR | DATAVALUE;
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 if (tokentype == '-' || tokentype == '+')
	 {
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 }
	 if (tokentype == DBLFNUM || tokentype == DBLBNUM) litpool++;
	 litpool++;
      } while (term == ',');
      break;

   case LOC_T:			/* LOC */
      val = -1;
      if (fapmode)
      {
         cp = tokscan (bp, &token, &tokentype, &j, &term, FALSE);
	 if (tokentype == EOS) val = prevloc + (pc - locorg);
      }
      if (val < 0)
	 bp = exprscan (bp, &val, &term, &relocatable, 1, chksyms, 0);
      absolute = TRUE;
      prevloc = pc;
      pc = val;
      locorg = val;
      if (cursym[0])
      {
	 s = p1lookup();
	 if (s);
	 {
	    s->value = pc;
	    s->flags &= ~RELOCATABLE;
	 }
      }
      break;

   case LORG_T:			/* Literal ORiGin */
#ifdef DEBUGLIT
      fprintf (stderr,
	       "p1-LORG: litorigin = %o, litpool = %d, new pc = %o\n",
	       pc, litpool, pc+litpool);
#endif
      litorigin = pc;
      val = pc;
      processliterals();
      pc = val + litpool;
      litpool = 0;
      break;

   case MAX_T:			/* MAX */
      val = 0;
      do {
	 bp = exprscan (bp, &i, &term, &relocatable, 1, chksyms, 0);
	 if (i > val) val = i;
      } while (term == ',');
      if (cursym[0])
      {
	 s = p1lookup();
	 if (s)
	 {
	    s->value = val;
	    s->flags &= ~RELOCATABLE;
	 }
      }
      break;

   case MIN_T:			/* MIN */
      val = 0777777777;
      do {
	 bp = exprscan (bp, &i, &term, &relocatable, 1, chksyms, 0);
	 if (i < val) val = i;
      } while (term == ',');
      if (cursym[0])
      {
	 s = p1lookup();
	 if (s)
	 {
	    s->value = val;
	    s->flags &= ~RELOCATABLE;
	 }
      }
      break;

   case NULL_T:			/* NULL */
      val = pc;
      relocatable = !absmod;
      goto DO_EQU;

   case OPD_T:			/* OP Definition */
      if (chksyms)
      {
	 radix = 8;
	 exprtype = DATAEXPR | DATAVALUE;

	 /*
	 ** Scan off new opcode definition value 
	 */

	 bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	 if (strlen(token) != 12)
	 {
	    sprintf (errtmp, "Invalid octal number for OPD: %s", bp);
	    logerror (errtmp);
	    return (FALSE);
	 }
#ifdef DEBUGOPD
	 fprintf (stderr,
		 "OPD: token = %s, tokentype = %d, val = %o, term = %c\n",
		 token, tokentype, val, term);
#endif
	 sscanf (&token[6], "%o", &tmpnum1);
	 token[6] = '\0';
	 sscanf (token, "%o", &tmpnum0);
#ifdef DEBUGOPD
	 fprintf (stderr, " tmpnum0 = %o, tmpnum1 = %o\n", tmpnum0, tmpnum1);
#endif
	 /*
	 ** Go add it
	 */

	 opdadd (cursym, tmpnum0, tmpnum1);
	 radix = 10;
      }
      break;

   case OPSYN_T:		/* OP SYNonym */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGOPSYN
      fprintf (stderr, 
       "OPSYN1: cursym = %s, token = %s, tokentype = %d, val = %o, term = %c\n",
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
      break;

   case OPVFD_T:		/* OP Variable Field Definition */
      if (!chksyms)
      {
	 /*
	 ** Scan off VFD fields and build op definition
	 */

	 bitcount = 0;
	 tmpnum0 = 0;
	 tmpnum1 = 0;

	 while (bp)
	 {
	    int bits;
	    unsigned mask;
	    char ctype;

	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	    
#ifdef DEBUGOPD
	    fprintf (stderr,
		  "OPVFD: token = %s, tokentype = %d, val = %o, term = %c\n",
		  token, tokentype, val, term);
#endif
	    if (tokentype == EOS) break;

	    ctype = token[0];
	    if (term != '/')
	    {
	       sprintf (errtmp, "Invalid OPVFD specification: %s", token);
	       logerror (errtmp);
	       return (FALSE);
	    }
	    if (ctype == 'O')
	    {
	       bits = atoi (&token[1]);
	       radix = 8;
	    }
	    else
	    {
	       bits = atoi (token);
	       radix = 10;
	    }
	    bp++;
	    mask = 0;
	    for (i = 0; i < bits; i++) mask = (mask << 1) | 1;
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
#ifdef DEBUGOPD
	    fprintf (stderr,
		  "   token = %s, tokentype = %d, val = %o, term = %c\n",
		  token, tokentype, val, term);
	    fprintf (stderr,
		  "   tmpnum0 = %o, tmpnum1 = %o, mask = %o, bitcount = %d\n", 
		  tmpnum0, tmpnum1, mask, bitcount);
#endif
	    val &= mask;
	    if (bitcount < 18)
	    {
	       if (bitcount + bits <= 18)
		  tmpnum0 = (tmpnum0 << bits) | val;
#ifdef DEBUGOPD
	       else
	       {
		  int resid = 18 - (bitcount + bits);
		  printf ("   resid = %d\n", resid);
	       }
#endif
	    }
	    else
	    {
	       tmpnum1 = (tmpnum1 << bits) | val;
	    }
	    bitcount += bits;
	    if (isspace(term)) break;
	 }
#ifdef DEBUGOPD
	 fprintf (stderr, "   tmpnum0 = %o, tmpnum1 = %o\n", 
		 tmpnum0, tmpnum1);
#endif
	 /*
	 ** Go add it
	 */

	 opdadd (cursym, tmpnum0, tmpnum1);
	 radix = 10;
      }
      break;

   case ORG_T:			/* ORiGin */
      bp = exprscan (bp, &val, &term, &relocatable, 1, chksyms, 0);
      if (absmod)
	 absolute = TRUE;
      else
	 absolute = FALSE;
      pc = val;
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
		  s = symlookup (cursym, temp, FALSE, FALSE);
		  if (s) 
		     s->value = pc;
	       }
	       s = NULL;
	    }
	    else
	    {
	       s = symlookup (cursym, "", FALSE, FALSE);
	    }
	 }
	 else
	 {
	    s = symlookup (cursym, qualtable[qualindex], FALSE, FALSE);
	 }

	 if (s)
	 {
	    s->value = pc;
	 }
      }
      break;

   case QUAL_T:			/* QUALified section */
      bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
      if (strlen(token) > MAXSYMLEN) token[MAXSYMLEN] = '\0';
      qualindex ++;
      strcpy (qualtable[qualindex], token);
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
	    s->flags &= ~RELOCATABLE;
	 }
      }
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
	    if (i == begincount)
	    {
#ifdef DEBUGUSE
	       fprintf (stdder,
		     "   Add USE: token = %s, chain = %d, value = %05o\n",
		     token, curruse, begintable[curruse].value);
#endif
	       prevuse = begincount - 1;
	       if (prevuse < 0) prevuse = 0;
	       strcpy (begintable[begincount].symbol, token);
	       begintable[begincount].chain = curruse;
	       begintable[begincount].bvalue = 0;
	       begintable[begincount].value = begintable[prevuse].value;
	       curruse = begincount;
	       begincount++;
	    }
	 }
	 pc = begintable[curruse].value;
#ifdef DEBUGUSE
	 fprintf (stderr, "   new curruse = %d, prevuse = %d, pc = %05o\n",
		  curruse, prevuse, pc);
#endif
      }
      break;

   case VFD_T:			/* Variable Field Definition */
      bitcount = 0;

      /*
      ** Skip leading blanks
      */

      i = FALSE;
#ifdef DEBUGP1VFD
      fprintf (stderr, "VFD-p1: addr bp = %p, addr inbuf = %p\n", bp, inbuf);
      fprintf (stderr, "   bp = %s\n", bp);
      fprintf (stderr, "   inbuf = %s\n", inbuf);
#endif
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
#ifdef DEBUGP1VFD
	 fprintf (stderr, "VFD-p1: NO Operand\n");
#endif
         bitcount = 36;
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
#ifdef DEBUGP1VFD
	 fprintf (stderr, "VFD-p1: bp = %s\n", bp);
#endif

	 /*
	 ** Scan off VFD fields and calulate length
	 */

	 while (*bp)
	 {
	    int bits;
	    int chartype;
	    char ctype;

	    exprtype = ADDREXPR;
	    bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
	    
#ifdef DEBUGP1VFD
	    fprintf (stderr,
		  "   token = %s, tokentype = %d, val = %o, term = %c\n",
		  token, tokentype, val, term);
#endif
	    if (tokentype == EOS) break;

	    ctype = token[0];
	    chartype = FALSE;
	    if (term != '/')
	    {
	       sprintf (errtmp, "Invalid VFD specification: %s", token);
#ifdef DEBUGP1VFD
	       fprintf (stderr, "ERROR: %d: %s\n", linenum, errtmp);
#endif
	       logerror (errtmp);
	       return (FALSE);
	    }
	    if (ctype == 'O')
	    {
	       if (*(bp+1) == '/')
		  exprtype = BOOLEXPR | BOOLVALUE;
	       else
		  exprtype = ADDREXPR | BOOLVALUE;
	       bits = atoi (&token[1]);
	       radix = 8;
	    }
	    else if (ctype == 'H')
	    {
	       bits = atoi (&token[1]);
	       chartype = TRUE;
	    }
	    else
	    {
	       exprtype = ADDREXPR | BOOLVALUE;
	       bits = atoi (token);
	       radix = 10;
	    }
	    bp++;
	    if (chartype)
	    {
	       char lcltoken[TOKENLEN];

	       cp = bp;
	       while (*bp && *bp != ',' && !isspace(*bp)) bp++;
	       term = *bp;
	       *bp++ = '\0';
	       strcpy (lcltoken, cp);
	       val = 0;
#ifdef DEBUGP1VFD
	       fprintf (stderr,
		"   H%d: token = %s, tokentype = %d, val = %o, term = %c(%x)\n",
		     bits, lcltoken, tokentype, val, term, term);
#endif
	    }
	    else
	    {
	       if (*bp == '/')
	       {
	          bp++;
		  val = 037777777777;
		  term = *bp;
		  if (term == ',')
		     bp++;
		  else 
		     bp = exprscan (bp, &val, &term, &relocatable, 1,
				    chksyms, 0);
	       }
	       else
	       {
		  bp = exprscan (bp, &val, &term, &relocatable, 1, chksyms, 0);
	       }
	    }
#ifdef DEBUGP1VFD
	    fprintf (stderr, "   bits = %d, val = %o, term = %c(%x)\n",
		    bits, val, term, term);
#endif
	    radix = 10;
	    bitcount += bits;
	    if (term == '\n') break;
	 }
      }
#ifdef DEBUGP1VFD
      fprintf (stderr, "   bitcount = %d\n", bitcount);
#endif

      /*
      ** Convert bits to words
      */

      val = bitcount / 36;
      val = val + ((bitcount - (val * 36)) ? 1 : 0);
#ifdef DEBUGP1VFD
      fprintf (stderr, "   val = %d\n", val);
#endif
      radix = 10;

      /*
      ** Add word count to pc
      */

      pc += val;
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
   char *token;
   char *bp;
   char *cp;
   int i;
   int status = 0;
   int tokentype;
   int val;
   int done;
   int srcmode;
   char term;
   char opcode[MAXSYMLEN+2];
   char srcbuf[MAXLINE];

#ifdef DEBUGP1RDR
   if (inpass == 010)
      fprintf (stderr, "asmpass1: Entered: symonly = %s\n",
	       symonly ? "TRUE" : "FALSE");
#endif

   /*
   ** Rewind the input.
   */

   if (fseek (tmpfd, 0, SEEK_SET) < 0)
   {
      perror ("asm7090: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Process the source.
   */

   chksyms = symonly;

   commonctr = FAPCOMMONSTART;

   gotoskip = FALSE;
   gotoblank = FALSE;
   pgmlength = 0;
   litorigin = 0;
   headcount = 0;
   curruse = 0;
   prevuse = 0;
   qualindex = 0;
   asmskip = 0;
   linenum = 0;
   litpool = 0;
   litorigin = 0;
   ttlbuf[0] = '\0';
   gotosym[0] = '\0';

   if (genlnkdir)
   {
       pc = 2;
   }
   else
   {
       pc = 0;
   }

   for (i = 0; i < begincount; i++)
      begintable[i].value = 0;

   done = FALSE;
   while (!done)
   {
      /*
      ** Read source line mode and text
      */

      if (readsource (tmpfd, &srcmode, &linenum, &filenum, srcbuf) < 0)
      {
         done = TRUE;
	 break;
      }
#ifdef DEBUGP1RDR
      if (inpass == 010)
	 fprintf (stderr, "   gotoskip = %s, asmskip = %s, pc = %o\n",
		  gotoskip ? "TRUE" : "FALSE",
		  asmskip ? "TRUE" : "FALSE", pc);
#endif

      /*
      ** If IBSYS control card, ignore
      */

      if (srcmode & CTLCARD)
      {
	 linenum = 0;
	 continue;
      }

      linenum++;

#ifdef DEBUGP1RDRM
      if ((inpass == 010) && (srcmode & MACDEF))
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
	       if (readsource (tmpfd, &srcmode, &linenum, &filenum, srcbuf) < 0)
	       {
		  done = TRUE;
		  break;
	       }
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
	 if ((inpass == 010) && (inmode & CONTINU))
	    fprintf (stderr, "cin = %s", inbuf);
#endif
	 bp = inbuf;
	 exprtype = ADDREXPR;
	 radix = 10;
	 addextsym = NULL;
	 pc &= 077777;

	 /*
	 ** If not skip, IF[FT] mode, process
	 */

	 if (!asmskip)
	 {
	    /*
	    ** If not a comment, then process.
	    */

	    if (*bp != COMMENTSYM)
	    {
	       SymNode *s;
	       OpCode *op;

	       /*
	       ** If label present, scan it off. Check later to add.
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
		  fprintf (stderr, "asmpass1: cursym = %s\n", cursym);
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
		  fprintf (stderr, "H  cursym = '%s', gotosym = '%s'\n",
			   cursym, gotosym);
#endif
		  if (cursym[0] && !strcmp (cursym, gotosym))
		  {
		     gotosym[0] = '\0';
		     if (gotoblank) cursym[0] = '\0';
		     gotoskip = FALSE;
		     gotoblank = FALSE;
		  }
		  else continue;
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
	       }
	       else if (!strncmp (&inbuf[OPCSTART], "   ", 3))
	       {
		  strcpy (opcode, "PZE");
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
		     case BOOL_T:
		     case EQU_T:
		     case HED_T:
		     case HEAD_T:
		     case LBOOL_T:
		     case MACRO_T:
		     case MACRON_T:
		     case NULL_T:
		     case OPD_T:
		     case OPSYN_T:
		     case OPVFD_T:
		     case RBOOL_T:
		     case SAVE_T:
		     case SAVEN_T:
		     case SET_T:
		     case ENT_T:
		     case REM_T:
		     case SPC_T:
		     case EJE_T:
		     case TAPENO_T:
		     case TTL_T:
			addit = FALSE;
		        break;
		     default: ;
		     }
		  }

		  if (chksyms)
		  {
		     if (addit)
		     {
			if (fapmode)
			{
			   if (headcount && (strlen(cursym) < MAXSYMLEN))
			   {
			      int i;

#ifdef DEBUGEQU
			      fprintf (stderr, 
				   "asmpass1: Add: cursym = '%s', val = %05o\n",
			            cursym, pc);
			      printf ("   headcount = %d\n", headcount);
#endif
			      for (i = 0; i < headcount; i++)
			      {
				 char temp[32];

				 sprintf (temp, "%c", headtable[i]);
#ifdef DEBUGEQU
				 fprintf (stderr,
					  "A cursym = '%s', head = '%s' \n",
					  cursym, temp);
#endif
				 s = symlookup (cursym, temp, TRUE, TRUE);
				 if (!s)
				 {
				    sprintf (errtmp,
					  "Duplicate-0 symbol: %s$%s",
					     temp, cursym);
				    logerror (errtmp);
				    status = -1;
				 }
			      }
			   }
			   else
			   {
#ifdef DEBUGEQU
			      fprintf (stderr,
				       "B  cursym = '%s', head = '%s' \n",
				       cursym, "");
#endif
			      s = symlookup (cursym, "", TRUE, TRUE);
			      if (!s)
			      {
				 sprintf (errtmp, "Duplicate-1 symbol: %s",
					  cursym);
#ifdef DEBUGP1RDR
				 if (inpass == 010)
				    fprintf (stderr, "asmpass1: %s\n", errtmp);
#endif
				 logerror (errtmp);
				 status = -1;
			      }
			   }
			}
			else
			{
#ifdef DEBUGEQU
                           fprintf (stderr,
				 "asmpass1: Add: cursym = '%s', val = %05o\n",
				 cursym, pc);
			   fprintf (stderr, "   cursym = '%s', qual = '%s' \n",
				   cursym, qualtable[qualindex]);
#endif
			   s = symlookup (cursym, qualtable[qualindex],
					  TRUE, TRUE);
			   if (!s)
			   {
			      if (qualtable[qualindex][0] == '\0')
				 sprintf (errtmp, "Duplicate-2 symbol: %s",
					  cursym);
			      else
				 sprintf (errtmp, "Duplicate-3 symbol: %s$%s",
					  qualtable[qualindex], cursym);
			   }
			}
		     }
		  }
		  else
		  {
		     if (addit)
		     {
			if (fapmode)
			{
#ifdef DEBUGEQU
                           fprintf (stderr,
				 "asmpass1: Mod: cursym = '%s', val = %05o\n",
				 cursym, pc);
#endif
			   if (headcount && (strlen(cursym) < MAXSYMLEN))
			   {
			      int i;

			      for (i = 0; i < headcount; i++)
			      {
				 char temp[32];

				 sprintf (temp, "%c", headtable[i]);
#ifdef DEBUGEQU
				 fprintf (stderr,
				       "C  cursym = '%s', head = '%s' \n",
					  cursym, temp);
#endif
				 s = symlookup (cursym, temp, FALSE, FALSE);
				 if (s)
				 {
				    s->value = pc;
				 }
			      }
			      s = NULL;
			   }
			   else
			   {
#ifdef DEBUGEQU
			      fprintf (stderr,
			       "asmpass1: NOHED: cursym = '%s', head = '%s' \n",
				       cursym, "");
#endif
			      s = symlookup (cursym, "", FALSE, FALSE);
			   }
			}
			else
			{
#ifdef DEBUGEQU
                           fprintf (stderr,
				 "asmpass1: Mod: cursym = '%s', val = %05o\n",
			            cursym, pc);
			   fprintf (stderr, "D  cursym = '%s', qual = '%s' \n",
				   cursym, qualtable[qualindex]);
#endif
			   s = symlookup (cursym, qualtable[qualindex],
					  FALSE, FALSE);
			}

			if (s)
			{
			   s->value = pc;
			}
		     }
		  }
	       }

	       /*
	       ** If not a macro call ref, then process
	       */

	       if (!(inmode & MACCALL))
	       {
		  if (op)
		  {
		     switch (op->optype)
		     {

			case TYPE_DISK:
			   p1diskop (op, bp);
			   break;

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
		     if (!gotoskip)
		     {
			/* Missing op may not be a problem */
			pc++; 
		     }
		  }
	       }
	       
	    }
	 }

	 /*
	 ** If skip mode, check if skipping a GOTO for label BLANK option.
	 */

	 if (asmskip)
	 {
	    if (asmskip == 1)
	    {
	       if (strncmp (&inbuf[OPCSTART], "   ", 3))
	       {
	          bp = &inbuf[OPCSTART];
		  bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
		  if (!strcmp(token, "GOTO"))
		  {
		     bp = tokscan (bp, &token, &tokentype, &val, &term, FALSE);
		     if (strlen(token) > MAXSYMLEN) token[MAXSYMLEN] = '\0';
		     gotoblank = FALSE;
		     strcpy (gotosym, token);
		     if (term == ',')
		     {
			bp = tokscan (bp, &token, &tokentype, &val, &term,
				      FALSE);
			if (!strcmp (token, "BLANK"))
			gotoblank = TRUE;
		     }
#ifdef DEBUGGOTO
		     fprintf (stderr,
			"asmpass1: GOTO: gotosym = '%s', gotoblank = %s\n",
			gotosym, gotoblank ? "TRUE" : "FALSE");
#endif
		  }
	       }
	    }
	    asmskip--;   
	 }
      }

   }

   /*
   ** If MAP, Set BEGIN/USE beginning values.
   */

   if (!fapmode)
   {
      if (inpass == 011)
      {
	 /*
	 ** Add blank common location counter, always last
	 */

	 begintable[0].value = pc;
	 strcpy (begintable[begincount].symbol, "//");
	 begintable[begincount].bvalue = 0;
	 begintable[begincount].value = 0;
	 begintable[begincount].chain = 0;
	 begincount++;

	 for (i = begincount-1; i > 0; i--)
	 {
	    if (!begintable[i].bvalue)
	       begintable[i].bvalue = begintable[i-1].value;
#ifdef DEBUGUSE
	    fprintf (stderr, 
	      "use[%d]: symbol = %s, value = %05o, bvalue = %05o, chain = %d\n",
	       i, begintable[i].symbol, begintable[i].value, 
		  begintable[i].bvalue, begintable[i].chain); 
#endif
	 }
#ifdef DEBUGUSE
	 i = 0;
	 fprintf (stderr, 
	    "use[%d]: symbol = %s, value = %05o, bvalue = %05o, chain = %d\n",
	       i, begintable[i].symbol, begintable[i].value, 
		  begintable[i].bvalue, begintable[i].chain); 
#endif
      }
      else if (inpass == 012)
      {
	 i = begincount - 1;
	 if (begintable[i].value)
	    pgmlength += (begintable[i].value - begintable[i].bvalue);
#ifdef DEBUGUSE
	 fprintf (stderr, 
	    "cmn[%d]: symbol = %s, value = %05o, bvalue = %05o, chain = %d\n",
	       i, begintable[i].symbol, begintable[i].value, 
		  begintable[i].bvalue, begintable[i].chain); 
#endif
      }
   }

   return (status);
}
