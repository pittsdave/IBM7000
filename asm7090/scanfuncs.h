/***********************************************************************
*
* scanfuncs.h - Scanner action functions.
*
***********************************************************************/

static void
makefloat (int expsgn, int sexp, t_uint64 sfrc, t_uint64 evalue,
	   int dblprecision, int defer, int leadingzero, tokval *svalue,
	   char *toksym, char *symbol)
{
   t_uint64 result;
   double cvtfrac;
   long temp;
   char ftemp[32];
   char ztemp[32];

   if (sexp > 38)
   {
      Parse_Error (SCAN_ERROR, INVEXPONENT, defer);
      *svalue = 0;
      strcpy (toksym, "0");
      result = 0;
   }
   else
   {
      temp = leadingzero;
      ztemp[0] = '\0';
      while (temp)
      {
	 strcat (ztemp, "0");
	 temp--;
      }
      sprintf (ftemp, "0.%s%lld", 
	       ztemp, sfrc);
      sscanf (ftemp, "%lf", &cvtfrac);
      sprintf (ftemp, "%lld.%s%lld%s%c%d", 
	       evalue, ztemp, sfrc,
	       dblprecision ? "EE": "E", 
	       expsgn > 0 ? '+' : '-', sexp);
#ifdef DEBUG_FLOAT
      fprintf (stderr, "makefloat: Float format: \n");
      fprintf (stderr, 
	    "   evalue = %lld(%llo), leadzero = %d, sfrc = %lld(%llo)\n",
		  evalue, evalue, leadingzero, sfrc, sfrc);
      fprintf (stderr, "   cvtfrac = %f, ftemp = %s\n", cvtfrac, ftemp);
      fprintf (stderr, "   sexp = %d(%o), expsgn = %d\n",
		  sexp, sexp, expsgn);
      fprintf (stderr, "   num = %s\n", ftemp);
#endif
      ibm_strtod (ftemp, &result, dblprecision);
#ifdef DEBUG_FLOAT
      fprintf (stderr, "ibm_strtod returns\n");
#endif
   }
#if defined(WIN32)
   sprintf (symbol, "%21.21I64o", result);
#else
   sprintf (symbol, "%21.21llo", result);
#endif

#ifdef DEBUG_FLOAT
   fprintf (stderr, "   symbol = %s\n", symbol);
#endif
}

static void
makebinpoint (int expsgn, t_uint64 sexp, t_uint64 sfrc, t_uint64 evalue,
	      int dblprecision, int defer, int leadingzero, tokval *svalue,
	      char *toksym, char *symbol)
{
   t_uint64 result;
   double cvtfrac;
   short exponent = 0;
   long fraction = 0;
   long residual = 0;
   long temp;
   char ftemp[32];
   char ztemp[32];

   temp = leadingzero;
   ztemp[0] = '\0';
   while (temp)
   {
      strcat (ztemp, "0");
      temp--;
   }
   sprintf (ftemp, "0.%s%lld", 
	    ztemp, sfrc);
   sscanf (ftemp, "%lf", &cvtfrac);
#ifdef DEBUG_BINPOINT
   fprintf (stderr, "makebinpoint: Binary point format: \n");
   fprintf (stderr, 
   "   evalue = %lld(%llo), leadzero = %d, sfrc = %lld(%llo)\n",
	       evalue, evalue, leadingzero, sfrc, sfrc);
   fprintf (stderr, "   cvtfrac = %f, ftemp = %s\n", cvtfrac, ftemp);
   fprintf (stderr, "   sexp = %d(%o), expsgn = %d\n",
	       sexp, sexp, expsgn);
   sprintf (ftemp, "%lld.%s%lld%s%c%d", 
	    evalue, ztemp, sfrc,
	    dblprecision ? "BB": "B", 
	    expsgn > 0 ? '+' : '-', sexp);
   fprintf (stderr, "   num = %s\n", ftemp);
#endif

   if (evalue > 0)
   {
      temp = evalue;
      while (temp)
      {
	 temp >>= 1;
	 exponent++;
      }
   }
   if (sfrc)
   {
      fraction = 1; /* Guard bit */
      while (!(fraction & 0010000000000))
      {
	 int d;
	 
	 cvtfrac *= 8.0;
	 d = cvtfrac;
	 cvtfrac = cvtfrac - d;
	 fraction = (fraction << 3) | d;
      }
      residual = 010 | (fraction & 07);
      fraction >>= 3;
      while (!(residual & 0001000000000))
      {
	 int d;
	 
	 cvtfrac *= 8.0;
	 d = cvtfrac;
	 cvtfrac = cvtfrac - d;
	 residual = (residual << 3) | d;
      }
   }

#ifdef DEBUG_BINPOINT
   fprintf (stderr, "   exponent = %o\n", exponent);
   fprintf (stderr, "   evalue   = %o\n", evalue);
   fprintf (stderr, "   fraction = %o\n", fraction);
   fprintf (stderr, "   residual = %o\n", residual);
#endif
   result = evalue;
   result <<= 62 - sexp;

   if (sfrc)
   {
      t_uint64 fresult;

      fresult = ((t_uint64)fraction & 0000777777777) << 35;
      fresult |= ((t_uint64)residual & 0000777777777) << 8;
      fresult >>= sexp;
#ifdef DEBUG_BINPOINT
      fprintf (stderr, "   fresult = %llo\n", fresult);
#endif
      result |= fresult;
   }
#if defined(WIN32)
   sprintf (symbol, "%21.21I64o", result);
#else
   sprintf (symbol, "%21.21llo", result);
#endif
#ifdef DEBUG_BINPOINT
   fprintf (stderr, "   symbol = %s\n", symbol);
#endif
}
