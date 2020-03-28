/***********************************************************************
*
* asmfps.c - Floating Point Support routines for the IBM 7090 assembler.
*
* Changes:
*   12/30/04   DGP   Hacked from s709 arith routines.
*   03/15/05   DGP   Added dblprecision parm to ibm_strtod.
*   12/20/10   DGP   Totally revamped with new s709 arith routines.
*   
***********************************************************************/

#include <stdio.h>
#include <ctype.h>

#include "asmdef.h"
#include "asmfps.h"
#include "asmnums.h"


typedef struct
{
   uint32   s;
   int32    exp;
   t_uint64 frac;
} FP_t;


static t_uint64 ac, mq, sr, sr2, si;
static int spill;

/***********************************************************************
* fp_unpack - Unpack the floating point number.
***********************************************************************/

static void
fp_unpack (t_uint64 h, t_uint64 l, int ifac, FP_t *op)
{
#ifdef DEBUGFP
   fprintf (stderr,
	    "fp_unpack: ifac = %d, h = %012llo, l = %012llo\n",
	    ifac, h, l);
#endif

   if (ifac)
   {
      op->s = (h & ACSIGN) ? 1 : 0;
      op->exp = (uint32) ((h & (Q|P|EXPMASK)) >> EXPSHIFT);
   }
   else
   {
      op->s = (h & SIGN) ? 1 : 0;
      op->exp = (uint32) ((h & EXPMASK) >> EXPSHIFT);
   }
   op->frac = ((h & FRACMASK) << FRACSHIFT) | (l & FRACMASK);
#ifdef DEBUGFP
   fprintf (stderr, "   op = %c %03o %018llo\n",
	    op->s ? '-' : '+', op->exp, op->frac);
#endif
}

/***********************************************************************
* fp_pack - Pack the floating point number.
***********************************************************************/

static void
fp_pack (t_uint64 *h, t_uint64 *l,
	 int ifac, int actest, uint32 ls, int32 lexp, FP_t *op)
{
#ifdef DEBUGFP
   fprintf (stderr,
"fp_pack: ifac = %d, actest = %d, lh = %c, lexp = %03o, op = %c %03o %018llo\n",
	    ifac, actest,
	    ls ? '-' : '+', lexp,
	    op->s ? '-' : '+', op->exp, op->frac);
#endif

   if (ifac)
   {
      *h = ((op->s) ? ACSIGN : 0) |
	    (((t_uint64)op->exp << EXPSHIFT) & (Q|P|EXPMASK)) |
	    ((op->frac >> EXPSHIFT) & FRACMASK);
   }
   else
   {
      *h = ((op->s) ? SIGN : 0) |
	    (((t_uint64)op->exp << EXPSHIFT) & EXPMASK) |
	    ((op->frac >> EXPSHIFT) & FRACMASK);
   }
   if (l)
   {
      *l = ((ls) ? SIGN : 0) |
	    (((t_uint64)lexp << EXPSHIFT) & EXPMASK) |
	    (op->frac & FRACMASK);
   }
   if (ifac && actest)
   {
      if (op->exp > 0377) spill = 1;
      else if (op->exp < 0) spill = 1;
      else spill = 0;
      if (l)
      {
	 if (lexp > 0377) spill = 1;
	 else if (lexp < 0) spill = 1;
      }
   }
#ifdef DEBUGFP
   fprintf (stderr, "    spill = %o, h = %012llo", spill, *h);
   if (l)
      fprintf (stderr, ", l = %012llo", *l);
   fprintf (stderr, "\n");
#endif
}

/***********************************************************************
* dofrnd - Process floating point Rounding.
***********************************************************************/

static void
dofrnd ()
{
   FP_t op;
   int test;

#ifdef DEBUGFP
   fprintf (stderr, "frnd: \n");
#endif

   fp_unpack (ac, 0, 1, &op);
   test = (mq & NRMMASK) ? 1: 0;

   spill = 0;
   if (test)
   {
      op.frac += HIADD;
      if (op.frac & DCRYMASK)
      {
	 op.frac >>= 1;
	 op.exp++;
	 if (op.exp == 0400)
	    spill = 1;
      }

      fp_pack (&ac, NULL, 1, 0, 0, 0, &op);
   }
}

/***********************************************************************
* dodfmpy - Process double floating point Multiply.
***********************************************************************/

static void
dodfmpy (int nrm)
{
   FP_t op1, op2;
   int32 mqexp;
   uint32 f1h, f2h, f1l, f2l;
   t_uint64 tx;

#ifdef DEBUGFP
   fprintf (stderr, "dfmpy: nrm = %d\n", nrm);
#endif

   fp_unpack (ac, mq, 1, &op1);
   fp_unpack (sr, sr2, 0, &op2);

   op1.s ^= op2.s;
   f1h = (uint32)((op1.frac >> FRACSHIFT) & FRACMASK);
   f1l = (uint32)(op1.frac & FRACMASK);
   f2h = (uint32)((op2.frac >> FRACSHIFT) & FRACMASK);
   f2l = (uint32)(op2.frac & FRACMASK);
   if (((op1.exp == 0) && (op1.frac == 0)) ||
       ((op2.exp == 0) && (op2.frac == 0)) ||
       ((f1h == 0) && (f2h == 0)))
   {
      ac = op1.s ? ACSIGN: 0;
      mq = op1.s ? SIGN: 0;
      si = sr;
      return;
   }
   op1.exp = (op1.exp & 0377) + op2.exp - 0200;
   if (op1.frac)
   {
      op1.frac = ((t_uint64) f1h) * ((t_uint64) f2h);
      tx = ((t_uint64) f1h) * ((t_uint64) f2l);
      op1.frac += (tx >> FRACSHIFT);
      tx = ((t_uint64) f1l) * ((t_uint64) f2h);
      op1.frac += (tx >> FRACSHIFT);
      si = tx >> FRACSHIFT;
   }
   else
   {
      if (nrm)
      {
	 si = sr;
      }
      else
      {
         FP_t siop = op2;

	 siop.frac = 0;
	 fp_pack (&si, NULL, 0, 0, 0, 0, &siop);
      }
   }
   if (nrm)
   {
      if (!(op1.frac & DNRMMASK))
      {
	 op1.frac <<= 1;
	 op1.exp--;
      }
      if ((op1.frac >> FRACSHIFT) & FRACMASK)
      {
         mqexp = op1.exp - FRACSHIFT;
      }
      else op1.exp = mqexp = 0;
   }
   else mqexp = op1.exp - FRACSHIFT;

   fp_pack (&ac, &mq, 1, 1, op1.s, mqexp, &op1);
}

static int
ibm_frnd (t_uint64 *op1, int nrm)
{
   int ret;

   spill = 0;
   ret = 0;

#ifdef DEBUGSTRTOD
   fprintf (stderr, "ibm_frnd: op1 = %012llo, nrm = %d\n",
   	    *op1, nrm);
#endif
   ac = *op1 >> 27;
   mq = *op1 & 0777777777;
#ifdef DEBUGSTRTOD
   fprintf (stderr, "   ac = %012llo\n", ac);
   fprintf (stderr, "   mq = %012llo\n", mq);
#endif

   dofrnd (nrm);

   ac = (ac << 27) | (mq & 0777777777);
   *op1 = ac;
   if (spill) ret = -1;
   return (ret);
}

static int
ibm_dfmpy (t_uint64 *op1, t_uint64 *op2, int nrm)
{
   int ret;

   spill = 0;
   ret = 0;

#ifdef DEBUGSTRTOD
   fprintf (stderr, "ibm_dfmpy: op1 = %012llo, op2 = %012llo, nrm = %d\n",
   	    *op1, *op2, nrm);
#endif

   ac = *op1 >> 27;
   mq = *op1 & 0777777777;
   sr = *op2 >> 27;
   sr2 = *op2 & 0777777777;
#ifdef DEBUGSTRTOD
   fprintf (stderr, "   ac = %012llo\n", ac);
   fprintf (stderr, "   mq = %012llo\n", mq);
   fprintf (stderr, "   sr = %012llo\n", sr);
   fprintf (stderr, "   s2 = %012llo\n", sr2);
#endif

   dodfmpy (nrm);

   ac = (ac << 27) | (mq & 0777777777);
   *op1 = ac;
   if (spill) ret = -1;
   return (ret);
}

int
ibm_strtod (char *num, t_uint64 *result, int dblprecision)
{
   double cvtfrac;
   t_uint64 retval;
   t_uint64 fraction = 0;
   t_uint64 residual = 0;
   t_uint64 mantissa = 0;
   t_uint64 evalue = 0;
   t_uint64 temp;
   int maxshft;
   int ret;
   int sign;
   int expsgn;
   uint16 exponent = 0200;
   char ftemp[82];

#ifdef DEBUGSTRTOD
   fprintf (stderr, "ibm_strtod: num = %s\n", num);
#endif
   ret = 0;

   /*
   ** Scan off sign
   */

   sign = 0;
   if (*num == '+')
   {
      num++;
   }
   else if (*num == '-')
   {
      sign = 1;
      num++;
   }

   /*
   ** Scan off leading digits
   */

   while (*num && isdigit(*num)) evalue = evalue * 10 + (*num++ - '0');

   /*
   ** Scan off fraction and convert
   */

   if (*num == '.')
   {
      char *fp;

      fp = ftemp;;
      *fp++ = *num++;
      while (*num && isdigit(*num)) *fp++ = *num++;
      *fp = '\0';
      sscanf (ftemp, "%lf", &cvtfrac);

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

#ifdef DEBUGSTRTOD
   fprintf (stderr, "   exponent = %o\n", exponent);
   fprintf (stderr, "   evalue   = %o\n", evalue);
   fprintf (stderr, "   fraction = %o\n", fraction);
   fprintf (stderr, "   residual = %o\n", residual);
#endif

   /*
   ** Merge whole and fractional parts
   */

   if (evalue > 0)
   {
      int sc;

      temp = evalue;
      while (temp)
      {
	 temp >>= 1;
	 exponent++;
      }

      sc = 0;
      mantissa = evalue;
      maxshft = 9;
      while (maxshft && !(mantissa & 0000700000000))
      {
	 mantissa <<= 3;
	 maxshft--;
	 sc++;
      }
      if (fraction)
      {
	 fraction &= 0000777777777;
	 mantissa |= (fraction >>= ((9-sc) * 3));
	 fraction <<= (sc * 3);
      }
      maxshft = 27;
      while (maxshft && !(mantissa & 0000400000000))
      {
	 mantissa <<= 1;
	 mantissa |= ((fraction & 0000400000000) >> 26);
	 fraction = (fraction << 1) & 0000777777777;
	 fraction |= ((residual & 0000400000000) >> 26);
	 residual = (residual << 1) & 0000777777777;
	 maxshft--;
      }
   }
   else
   {
      mantissa = fraction & 0000777777777;
      maxshft = 27;
      while (maxshft && !(mantissa & 0000400000000))
      {
	 mantissa <<= 1;
	 mantissa |= ((residual & 0000400000000) >> 26);
	 residual = (residual << 1) & 0000777777777;
	 exponent --;
	 maxshft--;
      }
   }

   retval = ((t_uint64)(sign & 1) << (SIGNSHIFT+EXPSHIFT)) |
	   ((t_uint64)(exponent & SEXPMASK) << (EXPSHIFT+EXPSHIFT)) |
	   ((mantissa & FRACMASK) << EXPSHIFT) |
	   (residual & FRACMASK);
   
#ifdef DEBUGSTRTOD
   fprintf (stderr, "   exponent = %o\n", exponent);
   fprintf (stderr, "   mantissa = %o\n", mantissa);
   fprintf (stderr, "   residual = %o\n", residual);
   fprintf (stderr, "   retval   = %llo\n", retval);
#endif

   /*
   ** Check for exponent
   */

   if (*num == 'E')
   {
      t_uint64 op2;
      int sexp;

      num++;
      if (*num == 'E') num++;

      expsgn = 1;
      if (*num == '+')
      {
	 num++;
      }
      else if (*num == '-')
      {
	 num++;
	 expsgn = -1;
      }

      sexp = 0;
      while (*num && isdigit(*num)) sexp = sexp * 10 + (*num++ - '0');

      if (expsgn > 0)
	 op2 = dptens[sexp];
      else
	 op2 = dmtens[sexp-1];
#ifdef DEBUGSTRTOD
      fprintf (stderr, "   sexp = %d\n", sexp);
#endif

      ret = ibm_dfmpy (&retval, &op2, 1);

      /*
      ** If not dblprecision and positive exponent, round up 
      */

      if (!dblprecision && expsgn > 0) ibm_frnd (&retval, 1);

#ifdef DEBUGSTRTOD
      fprintf (stderr, "   retval   = %21.21llo\n", retval);
#endif
   }
   *result = retval;
   return (ret);
}
