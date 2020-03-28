/***********************************************************************
*
* scanner.h - Scanner actions.
*
***********************************************************************/

#if defined(WIN32) && !defined(MINGW)
#define OVERFLOW_MASK  0000000000000007777400I64
#define HIGH_MASK      0000000000000000000377I64
#define LOW_MASK       0777777777777777777777I64
#define TOP_MASK       0777777777000000000000I64
#define BIT63_MASK     0400000000000000000000I64
#define LONGZERO       0I64
#else
#define OVERFLOW_MASK  0000000000000007777400LL
#define HIGH_MASK      0000000000000000000377LL
#define LOW_MASK       0777777777777777777777LL
#define TOP_MASK       0777777777000000000000LL
#define BIT63_MASK     0400000000000000000000LL
#define LONGZERO       0LL
#endif

   case ADDREXPRSTART: /* Address operator found */
      switch ( lachar )
      {

	 case '[':
	 case '{':
	    lachar = '(';
	 case '(':
	    token = lachar;
	    break;

	 case ']':
	 case '}':
	    lachar = ')';
	 case ')':
	    token = lachar;
	    break;

	default:
	    if (latran == 1)
	       token = EOS;
	    else
	       token = lachar;
      }
      break;

   case 2: /* Address symbol found */
   case 11: /* Data symbol found */
      {
	 int iii;

	 value = 0;
	 token = SYM;
	 for (iii = 0; symbol[iii] != '\0'; iii++ ) 
	    if (islower(symbol[iii]))
	       symbol[iii] = toupper(symbol[iii]);
	 if (fapmode || !usequaltable)
	    qualsym[0] = '\0';
	 else
	    strcpy (qualsym, qualtable[qualindex]);
      }
      break;

   case 3: /* Address qualified Symbol found */
   case 12: /* Data qualified Symbol found */
      {
	 char *iiicp;
	 int iii;

	 value = 0;
	 token = SYM;
	 if (fapmode && symbol[0] == '$' && symbol[1] == 0)
	 {
	    qualsym[0] = '\0';
	    strcpy (toksym, symbol);
	    return (0);
	 }
	 for (iii = 0; symbol[iii] != '\0'; iii++ ) 
	    if (islower(symbol[iii]))
	       symbol[iii] = toupper(symbol[iii]);
	 iiicp = (char *)strchr(symbol, '$');
	 *iiicp = '\0';
	 iiicp++;
	 strcpy (qualsym, symbol);
	 strcpy (symbol, iiicp);
	 if (fapmode && strlen (qualsym) == 0)
	 {
	    strcpy (qualsym, "0");
	 }
	 if (strlen(qualsym) == 0 && strlen(symbol) == 0)
	 {
	    Parse_Error (SCAN_ERROR, INVSYMBOL, defer);
	    *svalue = 0;
	    strcpy (toksym, "0");
	    return (0);
	 }
      }
      break;

   case 4: /* Address number found */
      if (radix == 10)
      {
	 value = value * 10 + dignum;
	 evalue = evalue * 10 + dignum;
	 token = DECNUM;
      }
      else if (radix == 8)
      {
	 token = OCTNUM;
	 if (dignum > 7)
	 {
	    Parse_Error (SCAN_ERROR, INVOCTNUM, defer);
	    *svalue = 0;
	    strcpy (toksym, "0");
	    return (0);
	 }
	 else
	    value = (value << 3) | dignum;
      }
      break;

   case 5: /* PC symbol found */
   case 24: /* recognize nulsym "**" */
      value = pc;
      token = ASTER;
      break;

   case 6: /* End of expression */
      token = EOS;
      break;

   case BOOLEXPRSTART: /* Boolean Operator found */
      switch ( lachar )
      {
      case '+':
	 token = OR;
	 break;
      case '-':
	 token = XOR;
	 break;
      case '/':
	 token = NOT;
	 break;
      case '*':
	 token = AND;
	 break;
      default:
	 if (latran == 1)
	    token = EOS;
	 else
	    token = lachar;
      }
      break;

   case 8: /* Boolean Symbol found */
      {
	 int iii;

	 value = 0;
	 token = BSYM;
	 for (iii = 0; symbol[iii] != '\0'; iii++ ) 
	    if (islower(symbol[iii]))
	       symbol[iii] = toupper(symbol[iii]);
	 if (fapmode || !usequaltable)
	    qualsym[0] = '\0';
	 else
	    strcpy (qualsym, qualtable[qualindex]);
      }
      break;

   case 9: /* Boolean Number found */
      if (radix == 10)
      {
	    Parse_Error (SCAN_ERROR, INVOCTNUM, defer);
	    *svalue = 0;
	    strcpy (toksym, "0");
	    return (0);
      }
      else if (radix == 8)
      {
	 token = BOCTNUM;
	 if (dignum > 7)
	 {
	    Parse_Error (SCAN_ERROR, INVOCTNUM, defer);
	    *svalue = 0;
	    strcpy (toksym, "0");
	    return (0);
	 }
	 else
	    value = (value << 3) | dignum;
      }
      break;

   case DATAEXPRSTART: /* Data operator found */
      switch ( lachar )
      {

	 case '[':
	 case '{':
	    lachar = '(';
	 case '(':
	    token = lachar;
	    break;

	 case ']':
	 case '}':
	    lachar = ')';
	 case ')':
	    token = lachar;
	    break;

	default:
	    if (latran == 1)
	       token = EOS;
	    else
	       token = lachar;
      }
      break;

   case 13: /* Data number found */
      if (radix == 10)
      {
	 value = value * 10 + dignum;
	 evalue = evalue * 10 + dignum;
#ifdef DEBUG_FLOAT
         fprintf (stderr, "evalue = %lld\n", evalue);
#endif
	 token = DECNUM;
      }
      else if (radix == 8)
      {
	 token = OCTNUM;
	 if (dignum > 7)
	 {
	    Parse_Error (SCAN_ERROR, INVOCTNUM, defer);
	    *svalue = 0;
	    strcpy (toksym, "0");
	    return (0);
	 }
	 else
	    value = (value << 3) | dignum;
      }
      break;

   case 14: /* process data fraction */
      token = SNGLFNUM;
      if (dignum == 0)
      {
	 if (!digitfound)
	    leadingzero++;
      }
      else digitfound = TRUE;
      fract = fract * 10 + dignum;
#ifdef DEBUG_FLOAT
      fprintf (stderr, "fract = %lld\n", fract);
#endif
      break;

   case 15: /* recognize floating point exponent sign*/
      token = SNGLFNUM;
      if (lachar == 'E') /* Two E's ddddEEddd, DBL precision */
	 dblprecision = TRUE;
      break;

   case 16: /* process floating point exponent sign */
   case 33:
      token = SNGLFNUM;
      if (lachar == '-')
	 fexpsgn = -1;
      break;

   case 17: /* recognize floating point exponent */
   case 34:
      token = SNGLFNUM;
      fexp = fexp * 10 + dignum;
#ifdef DEBUG_FLOAT
      fprintf (stderr, "fexp = %d\n", fexp);
#endif
      break;

   case 18: /* build floating point number */
      if (dblprecision)
	 token = DBLFNUM;
      else
	 token = SNGLFNUM;
      makefloat (fexpsgn, fexp, fract, evalue, dblprecision, defer,
		leadingzero, svalue, toksym, symbol);
      break;

   case 19: /* recognize binary point exponent sign*/
   case 28: /* recognize nnEnnBnn number */
      token = SNGLBNUM;
      if (lachar == 'B') /* Two B's ddddBBddd, DBL precision */
	 dblprecision = TRUE;
      break;

   case 20: /* process binary point exponent sign */
   case 29:
      token = SNGLBNUM;
      if (lachar == '-')
	 bexpsgn = -1;
      break;

   case 21: /* recognize binary point exponent */
   case 30:
      token = SNGLBNUM;
      bexp = bexp * 10 + dignum;
      break;

   case 22: /* build binary point number */
      if (dblprecision)
	 token = DBLBNUM;
      else
	 token = SNGLBNUM;
      makebinpoint (bexpsgn, bexp, fract, evalue, dblprecision, defer,
		leadingzero, svalue, toksym, symbol);
      break;

   case 23: /* recognize nulsym "**" */
      token = NULSYM;
      break;
	 
   case 25: /* recognize boolean AND "*" */
   case 27:
	 token = AND;
	 break;

   case 26: /* recognize boolean nulsym "**" */
      token = BNULSYM;
      break;

   case 31: /* recognize nnEnnBnn number */
   case 35: /* recognize nnBnnEnn number */
      {
	 t_uint64 resulth, resultl;
	 uint32 kk;
	 int jj;
	 int lostprecision;

	 if (dblprecision)
	    token = DBLBNUM;
	 else
	    token = SNGLBNUM;
#ifdef DEBUG_BINPOINT
	 fprintf (stderr, "EXP binary point format: \n");
	 fprintf (stderr, "   bexpsgn = %d, bexp = %d\n", bexpsgn, bexp);
	 fprintf (stderr, "   fexpsgn = %d, fexp = %d\n", fexpsgn, fexp);
	 fprintf (stderr, "   evalue = %d(%o), fract = %d\n",
		  evalue, evalue, fract);
#endif
	 makefloat (fexpsgn, fexp, fract, evalue, dblprecision, defer,
		   leadingzero, svalue, toksym, numsymbol);
#ifdef DEBUG_BINPOINT
	 fprintf (stderr, "   numsymbol = %s\n", numsymbol);
#endif
	 lostprecision = FALSE;
	 resulth = 0;
#if defined(WIN32)
	 sscanf (&numsymbol[3], "%I64o", &resultl);
#else
	 sscanf (&numsymbol[3], "%llo", &resultl);
#endif
	 numsymbol[3] = '\0';
	 sscanf (&numsymbol[0], "%o", &fexp);
#ifdef DEBUG_BINPOINT
	 fprintf (stderr, "   fexp = %o, resultl = %llo\n", fexp, resultl);
	 fprintf (stderr, "   bexp = %d\n", bexp);
#endif
	 for (jj = 0; jj < (54-(fexp-0200)); jj++)
	 {
	    kk = resulth & 1;
	    resulth >>= 1;
	    resulth |= (resultl & 1) ? BIT63_MASK : 0;
	    resultl >>= 1;
	    resultl |= (kk) ? BIT63_MASK : 0;
	 }
#ifdef DEBUG_BINPOINT
	 fprintf (stderr, "   result = %21.21llo %21.21llo\n", resulth,resultl);
#endif
	 for (jj = 0; jj < 71-bexp; jj++)
	 {
	    kk = (resulth & BIT63_MASK) ? 1 : 0;
	    resulth <<= 1;
	    resulth |= (resultl & BIT63_MASK) ? 1 : 0;
	    resultl <<= 1;
	    resultl |= kk;
	 }
#ifdef DEBUG_BINPOINT
	 fprintf (stderr, "   result = %21.21llo %21.21llo\n", resulth,resultl);
#endif
	 if (resulth & OVERFLOW_MASK)
	 {
#ifdef DEBUG_BINPOINT
	    fprintf (stderr, "   OVERFLOW\n");
#endif
	    lostprecision = TRUE;
	 }
	 resultl &= LOW_MASK;
	 resulth &= HIGH_MASK;
	 if (dblprecision)
	 {
	    if (resulth == LONGZERO && resultl == LONGZERO)
	       lostprecision = TRUE;
	 }
	 else
	 {
	    if (resulth == LONGZERO && (resultl & TOP_MASK) == LONGZERO)
	    {
	       lostprecision = TRUE;
#ifdef DEBUG_BINPOINT
	       fprintf (stderr, "   UNDERFLOW\n");
#endif
	    }
	 }
	 if (lostprecision && !defer)
	    logerror ("Lost precision in conversion");
#if defined(WIN32)
	 sprintf (symbol, "%3.3I64o%21.21I64o", resulth, resultl);
#else
	 sprintf (symbol, "%3.3llo%21.21llo", resulth, resultl);
#endif
#ifdef DEBUG_BINPOINT
	 fprintf (stderr, "   symbol = %s\n", symbol);
#endif
      }
      break;

   case 32: /* recognize nnBnnEnn number */
      token = SNGLBNUM;
      if (lachar == 'E') /* Two E's ddddEEddd, DBL precision */
	 dblprecision = TRUE;
      break;

