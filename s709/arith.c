/***********************************************************************
*
* arith.c - IBM 7090 emulator arithmetic routines.
*
* Changes:
*   ??/??/??   PRP   Original.
*   12/20/04   DGP   Added double precision.
*   02/07/05   DGP   Added check for negative MQEXP calc and fixed FMP
*                  MQ == 0.
*   11/17/06   DGP   "Borrowed" FP routines from Bob Supnik's SIMH.
*   12/10/06   DGP   General cleanup.
*   05/29/07   DGP   Put CYCLE call back.
*   12/05/11   DGP   Clear CPU_DIVCHK on start of division.
*   
***********************************************************************/

#define EXTERN extern

#include <stdio.h>

#include "sysdef.h"
#include "regs.h"
#include "s709.h"
#include "console.h"

typedef struct
{
   uint32   s;
   int32    exp;
   t_uint64 frac;
} FP_t;


/***********************************************************************
* Integer Operations.
***********************************************************************/

/***********************************************************************
* add - Perform integer Addition.
***********************************************************************/

void
add ()
{
#ifdef USE64
   t_uint64 op1, op2;

   op1 = ac & (Q|DATAMASK);
   op2 = sr & MAGMASK;
   ac &= ACSIGN;
   if ((ac ? 1 : 0) ^ ((sr & SIGN) ? 1 : 0))
   {
      if (op1 >= op2)
         ac |= op1 - op2;
      else
         ac = (ac ^ ACSIGN) | (op2 - op1);
   }
   else
   {
      ac |= (op1 + op2) & (Q|DATAMASK);
      if ((ac ^ op1) & P)
	 cpuflags |= CPU_ACOFLOW;
   }
#else
   if (((srh & SIGN) ^ (ach & SIGN)) == 0)
   {
      adderl = srl + acl;
      adderh = (srh & HMSK) + ach;
      if (adderl < srl)
         adderh++;
      if ((adderh & P) != (ach & P))
	 cpuflags |= CPU_ACOFLOW;
   }
   else
   {
      adderl = srl + ~acl;
      adderh = (srh & HMSK) + (~ach & (SIGN|Q|P|HMSK));
      if (adderl < srl)
         adderh++;
      if (adderh & QCARRY)
      {
         adderl++;
         if (adderl == 0)
            adderh++;
      }
      else
      {
         adderl = ~adderl;
         adderh = ~adderh;
      }
   }
   acl = adderl;
   ach = adderh & (SIGN|Q|P|HMSK);
#endif
}

/***********************************************************************
* rnd - Perform integer rounding.
***********************************************************************/

void
rnd (int useadd)
{
#ifdef USE64
   if (mq & BIT1)
   {
      if (useadd)
      {
	 sr = 1;
	 add ();
      }
      else
	 ac = (ac & ACSIGN) | ((ac+1) & (Q|DATAMASK));
   }
#else
   if (mqh & BIT1)
   {
      if (useadd)
      {
	 srh = 0;
	 srl = 1;
	 add ();
      }
      else
      {
	 acl++;
	 if (acl == 0)
	 {
	    ach++;
	    ach = ach & (SIGN|Q|P|HMSK);
	 }
      }
   }
#endif
}

/***********************************************************************
* mpy - Perform integer Multiply.
***********************************************************************/

void
mpy ()
{
#ifdef USE64
   t_uint64 lac;
   uint32 sign;
   int subcycle;

   if (shcnt == 0) return;
   sign = ((mq & SIGN) ? 1 : 0) ^ ((sr & SIGN) ? 1 : 0);
   lac = 0;
   sr &= MAGMASK;
   mq &= MAGMASK;
   if (sr && mq)
   {
      subcycle = 0;
      while (shcnt--)
      {
         if (mq & 1) lac = (lac + sr) & (Q|DATAMASK);
	 mq = (mq >> 1) | ((lac & 1) << 34);
	 lac >>= 1;
         if (++subcycle == 3)
         {
            DCYCLE ();
            subcycle = 0;
         }
      }
   }
   else
   {
      mq = 0;
      DCYCLE ();
   }
   if (sign)
   {
      lac |= ACSIGN;
      mq |= SIGN;
   }
   ac = lac;
#else
   int subcycle;
   uint8 sign;

   if ((srh & SIGN) == (mqh & SIGN))
      sign = 0;
   else
      sign = SIGN;
   srh &= HMSK;
   mqh &= HMSK;
   ach = 0;
   acl = 0;

   if ((srh & HMSK) == 0 && srl == 0)
   {
      mqh = 0;
      mql = 0;
      DCYCLE ();
   }
   else
   {
      subcycle = 0;
      while (shcnt--)
      {
         if (mql & 1)
            add ();
         mql >>= 1;
         if (mqh & 1)
            mql |= BIT4;
	 mqh >>= 1;
         if (acl & 1)
            mqh |= BIT1;
         acl >>= 1;
         if (ach & 1)
            acl |= BIT4;
	 ach >>= 1;
         if (++subcycle == 3)
         {
            DCYCLE ();
            subcycle = 0;
         }
      }
   }
   ach |= sign;
   mqh |= sign;
#endif
}

/***********************************************************************
* intdiv - Perform integer Division.
***********************************************************************/

void
intdiv ()
{
#ifdef USE64
   t_uint64 acsign, mqsign;
   int subcycle;

   cpuflags &= ~CPU_DIVCHK;
   if (shcnt == 0) return;
   acsign = (ac & ACSIGN) ? 1 : 0;
   mqsign = (sr & SIGN) ? 1 : 0;
   sr &= MAGMASK;
   if ((ac & (Q|DATAMASK)) >= sr)
   {
      cpuflags |= CPU_DIVCHK;
      DCYCLE ();
      return;
   }
   ac &= Q | DATAMASK;
   mq &= MAGMASK;
   subcycle = 0;
   while (shcnt--)
   {
      ac = ((ac << 1) & (Q|DATAMASK)) | (mq >> 34);
      mq = (mq << 1) & MAGMASK;
      if (ac >= sr)
      {
         ac -= sr;
	 mq |= 1;
      }
      if (++subcycle == 3)
      {
         DCYCLE ();
         subcycle = 0;
      }
   }
   if (acsign ^ mqsign) mq |= SIGN;
   if (acsign) ac |= ACSIGN;
#else
   int subcycle;
   uint8 sign, mqsign;

   cpuflags &= ~CPU_DIVCHK;
   if ((srh & SIGN) == (ach & SIGN))
      mqsign = 0;
   else
      mqsign = SIGN;
   srh = (srh & HMSK) | SIGN;

   sign = ach & SIGN;
   ach = ach & (Q|P|HMSK);

   if ((srh & HMSK) < (ach & (Q|P|HMSK)) ||
       ((srh & HMSK) == (ach & (Q|P|HMSK)) && srl <= acl))
   {
      cpuflags |= CPU_DIVCHK;
      DCYCLE ();
   }
   else
   {
      mqh = (mqh & HMSK) | mqsign;
      subcycle = 0;
      while (shcnt--)
      {
         ach = (ach & SIGN) | ((ach << 1) & (Q|P|HMSK));
         if (acl & BIT4)
            ach++;
         acl <<= 1;
         if (mqh & BIT1)
            acl++;
         mqh = (mqh & SIGN) | ((mqh << 1) & HMSK);
         if (mql & BIT4)
            mqh++;
         mql <<= 1;
         if (ach & P)
	    cpuflags |= CPU_ACOFLOW;
         if ((srh & HMSK) < (ach & (Q|P|HMSK)) ||
             ((srh & HMSK) == (ach & (Q|P|HMSK)) &&
              srl <= acl))
	 {
            add ();
            mql |= 1;
         }
         if (++subcycle == 3)
         {
            DCYCLE ();
            subcycle = 0;
         }
      }
   }
   ach |= sign;
#endif
}

/***********************************************************************
* Floating Point Operations
***********************************************************************/


/***********************************************************************
* fp_unpack - Unpack the floating point number.
***********************************************************************/

#ifdef USE64
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
#else
static void
fp_unpack (uint8 hh, uint32 hl, uint8 lh, uint32 ll, int ifac, FP_t *op)
{
#ifdef DEBUGFP
   fprintf (stderr,
	    "fp_unpack: ifac = %d, hh = %c%03o%010lo, ll = %c%03o%010lo\n",
	    ifac,
	    (hh & SIGN) ? '-' : '+',
	    ((hh & 037) << 2) | (short)(hl >> 30),
	    (unsigned long)hl & 07777777777,
	    (lh & SIGN) ? '-' : '+',
	    ((lh & 017) << 2) | (short)(ll >> 30),
	    (unsigned long)ll & 07777777777);
#endif
   op->s = (hh & SIGN) ? 1 : 0;
   if (ifac)
   {
      op->exp = ((hh & (Q|P|HMSK)) << 5) | ((hl & 037000000000) >> EXPSHIFT);
   }
   else
   {
      op->exp = ((hh & HMSK)  << 5) | ((hl & 037000000000) >> EXPSHIFT);
   }
   op->frac = (((t_uint64)hl & FRACMASK) << FRACSHIFT) |
	       ((t_uint64)ll & FRACMASK);
#ifdef DEBUGFP
   fprintf (stderr, "   op = %c %03o %018llo\n",
	    op->s ? '-' : '+', op->exp, op->frac);
#endif
}
#endif

/***********************************************************************
* fp_pack - Pack the floating point number.
***********************************************************************/

#ifdef USE64
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
      if (op->exp > 0377) spill = SPILL_AC | SPILL_OV;
      else if (op->exp < 0) spill = SPILL_AC;
      else spill = 0;
      if (l)
      {
	 if (lexp > 0377) spill |= SPILL_MQ | SPILL_OV;
	 else if (lexp < 0) spill |= SPILL_MQ;
      }
   }
#ifdef DEBUGFP
   fprintf (stderr, "    spill = %o, h = %012llo", spill, *h);
   if (l)
      fprintf (stderr, ", l = %012llo", *l);
   fprintf (stderr, "\n");
#endif
}
#else
static void
fp_pack (uint8 *hh, uint32 *hl, uint8 *lh, uint32 *ll,
	 int ifac, int actest, uint32 ls, int32 lexp, FP_t *op)
{
#ifdef DEBUGFP
   fprintf (stderr,
"fp_pack: ifac = %d, actest = %d, lh = %c, lexp = %03o, op = %c %03o %018llo\n",
	    ifac, actest,
	    ls ? '-' : '+', lexp,
	    op->s ? '-' : '+', op->exp, op->frac);
#endif

   
   *hh = op->s ? ((hh == &sih) ? P : SIGN) : 0;

   if (ifac)
   {
      *hh |= (op->exp & 01740) >> 5;
   }
   else
   {
      *hh |= (op->exp & 00340) >> 5;
   }
   *hl = ((op->exp & 0037) << EXPSHIFT) | ((op->frac >> FRACSHIFT) & FRACMASK);

   if (lh)
   {
      *lh = (ls ? SIGN : 0) | ((lexp & 00340) >> 5);
      *ll = ((lexp & 0037) << EXPSHIFT) | (op->frac & FRACMASK);
   }
   if (ifac && actest)
   {
      if (op->exp > 0377) spill = SPILL_AC | SPILL_OV;
      else if (op->exp < 0) spill = SPILL_AC;
      else spill = 0;
      if (lh)
      {
	 if (lexp > 0377) spill |= SPILL_MQ | SPILL_OV;
	 else if (lexp < 0) spill |= SPILL_MQ;
      }
   }
#ifdef DEBUGFP
   fprintf (stderr, "    spill = %o, hh = %c%03o%010lo",
	    spill,
	    (*hh & SIGN) ? '-' : '+',
	    ((*hh & 037) << 2) | (short)(*hl >> 30),
	    (unsigned long)*hl & 07777777777);
   if (lh)
      fprintf (stderr, ", ll = %c%03o%010lo",
	    (*lh & SIGN)? '-' : '+',
	    ((*lh & 017) << 2) | (short)(*ll >> 30),
	    (unsigned long)*ll & 07777777777);
   fprintf (stderr, "\n");
#endif
}
#endif


/***********************************************************************
* fp_norm - Normalize the number.
***********************************************************************/

static void
fp_norm (FP_t *op)
{
   op->frac &= DFRACMASK;
   if (op->frac == 0) return;
   while ((op->frac & DNRMMASK) == 0)
   {
      op->frac <<= 1;
      op->exp--;
   }
}

/***********************************************************************
* fp_fracdiv - Fractional division.
***********************************************************************/

static t_uint64
fp_fracdiv (t_uint64 dvd, t_uint64 dvr, t_uint64 *rem)
{
   dvr >>= FRACSHIFT;
   if (rem) *rem = dvd % dvr;
   return (dvd / dvr);
}


/***********************************************************************
* Single Precision Floating Point.
***********************************************************************/


/***********************************************************************
* fadd - Process floating point Addition
***********************************************************************/

void
fadd (int nrm)
{
   FP_t op1, op2, t;
   int32 mqexp, diff;

#ifdef DEBUGFP
   fprintf (stderr, "fadd: nrm = %d\n", nrm);
#endif

#ifdef USE64
   fp_unpack (ac, 0, 1, &op1);
   fp_unpack (sr, 0, 0, &op2);
   mq = 0;
   diff = (ac & P) ? 1 : 0;
#else
   fp_unpack (ach, acl, 0, 0, 1, &op1);
   fp_unpack (srh, srl, 0, 0, 0, &op2);
   mqh = 0; mql = 0;
   diff = (ach & P) ? 1 : 0;
#endif

   CYCLE ();
   if (op1.exp > op2.exp)
   {
      if (diff) op1.s = 1;
      t = op1;
      op1 = op2;
      op2 = t;
      op2.exp &= 0377; 
   }
   diff = op2.exp - op1.exp;
   if (diff)
   {
      if ((diff < 0) || (diff > 077)) op1.frac = 0;
      else op1.frac >>= diff;
   }
   if (op1.s ^ op2.s)
   {
      if (op1.frac >= op2.frac)
      {
         op2.frac = op1.frac - op2.frac;
         op2.s = op1.s;
      }
      else op2.frac -= op1.frac;
   }
   else
   {
      op2.frac += op1.frac;
      if (op2.frac & DCRYMASK)
      {
         op2.frac >>= 1;
         op2.exp++;
      }
   }
   if (nrm)
   {
      if (op2.frac)
      {
         fp_norm (&op2);
	 mqexp = op2.exp - FRACSHIFT;
      }
      else op2.exp = mqexp = 0;
      CYCLE ();
   }
   else mqexp = op2.exp - FRACSHIFT;
   /*if (mqexp < 0) mqexp = 0;*/

#ifdef USE64
   fp_pack (&ac, &mq, 1, 1, op2.s, mqexp, &op2);
#else
   fp_pack (&ach, &acl, &mqh, &mql, 1, 1, op2.s, mqexp, &op2);
#endif
}

/***********************************************************************
* frnd - Process floating point Rounding.
***********************************************************************/

void
frnd ()
{
   FP_t op;
   int test;

#ifdef DEBUGFP
   fprintf (stderr, "frnd: \n");
#endif

#ifdef USE64
   fp_unpack (ac, 0, 1, &op);
   test = (mq & NRMMASK) ? 1: 0;
#else
   fp_unpack (ach, acl, 0, 0, 1, &op);
   test = (mql & NRMMASK) ? 1: 0;
#endif

   spill = 0;
   if (test)
   {
      op.frac += HIADD;
      if (op.frac & DCRYMASK)
      {
	 op.frac >>= 1;
	 op.exp++;
	 if (op.exp == 0400)
	    spill = SPILL_AC | SPILL_OV;
      }

#ifdef USE64
      fp_pack (&ac, NULL, 1, 0, 0, 0, &op);
#else
      fp_pack (&ach, &acl, NULL, NULL, 1, 0, 0, 0, &op);
#endif
   }
   DCYCLE ();
}

/***********************************************************************
* fmpy - Process floating point Multiply.
***********************************************************************/

void
fmpy (int nrm)
{
   FP_t op1, op2;
   int32 mqexp;
   uint32 f1h, f2h;

#ifdef DEBUGFP
   fprintf (stderr, "fmpy: nrm = %d\n", nrm);
#endif

#ifdef USE64
   fp_unpack (mq, 0, 0, &op1);
   fp_unpack (sr, 0, 0, &op2);
#else
   fp_unpack (mqh, mql, 0, 0, 0, &op1);
   fp_unpack (srh, srl, 0, 0, 0, &op2);
#endif

   op1.s ^= op2.s;
   if ((op2.exp == 0) && (op2.frac == 0))
   {
#ifdef USE64
      ac = op1.s ? ACSIGN: 0;
      mq = op1.s ? SIGN: 0;
#else
      ach = op1.s ? SIGN: 0; acl = 0;
      mqh = op1.s ? SIGN: 0; mql = 0;
#endif
      return ;
   }
   CYCLE ();

   f1h = (op1.frac >> FRACSHIFT) & FRACMASK;
   f2h = (op2.frac >> FRACSHIFT) & FRACMASK;
   op1.frac = ((t_uint64) f1h) * ((t_uint64) f2h);
   op1.exp = (op1.exp & 0377) + op2.exp - 0200;
   if (nrm)
   {
      if (!(op1.frac & DNRMMASK))
      {
	 op1.frac <<= 1;
	 op1.exp--;
      }
      if ((op1.frac >> EXPSHIFT) & FRACMASK)
         mqexp = op1.exp - EXPSHIFT;
      else
         op1.exp = mqexp = 0;
      CYCLE ();
    }
    else mqexp = op1.exp - FRACSHIFT;

#ifdef USE64
   fp_pack (&ac, &mq, 1, 1, op1.s, mqexp, &op1);
#else
   fp_pack (&ach, &acl, &mqh, &mql, 1, 1, op1.s, mqexp, &op1);
#endif
}

/***********************************************************************
* fdiv - Process floating point Division
***********************************************************************/

void
fdiv ()
{
   FP_t op1, op2;
   t_uint64 rem;
   int32 mqexp;
   uint32 quos;

#ifdef DEBUGFP
   fprintf (stderr, "fdiv: \n");
#endif

   cpuflags &= ~CPU_DIVCHK;
#ifdef USE64
   fp_unpack (ac, 0, 1, &op1);
   fp_unpack (sr, 0, 0, &op2);
#else
   fp_unpack (ach, acl, 0, 0, 1, &op1);
   fp_unpack (srh, srl, 0, 0, 0, &op2);
#endif

   quos = op1.s ^ op2.s;
   if (op1.frac >= (2 * op2.frac))
   {
#ifdef USE64
      mq = quos ? SIGN: 0;
#else
      mqh = quos ? SIGN: 0; mql = 0;
#endif
      cpuflags |= CPU_DIVCHK;
      return;
   }
   if (op1.frac == 0)
   {
#ifdef USE64
      mq = quos ? SIGN: 0;
      ac = 0;
#else
      mqh = quos ? SIGN: 0; mql = 0;
      ach = 0; acl = 0;
#endif
      return;
   }
   CYCLE ();
   op1.exp &= 0377;
   if (op1.frac >= op2.frac)
   {
      op1.frac >>= 1;
      op1.exp++;
   }
   op1.frac = fp_fracdiv (op1.frac, op2.frac, &rem);
   op1.frac |= (rem << FRACSHIFT);
   mqexp = op1.exp - op2.exp + 0200;
   op1.exp -= FRACSHIFT;
   CYCLE ();

#ifdef USE64
   fp_pack (&ac, &mq, 1, 1, quos, mqexp, &op1);
#else
   fp_pack (&ach, &acl, &mqh, &mql, 1, 1, quos, mqexp, &op1);
#endif
   if (spill) spill |= SPILL_SP;
}

/***********************************************************************
* Double Precision Floating Point.
***********************************************************************/

/***********************************************************************
* dfadd - Process double floating point Addition
***********************************************************************/

void
dfadd (int nrm)
{
   FP_t op1, op2, t;
   int32 mqexp, diff;
   int acp, acb9;
   int srb9;

#ifdef DEBUGFP
   fprintf (stderr, "dfadd: nrm = %d\n", nrm);
#endif

#ifdef USE64
   fp_unpack (ac, mq, 1, &op1);
   fp_unpack (sr, sr2, 0, &op2);
   acp = (ac & P) ? 1 : 0;
   acb9 = (ac & NRMMASK) ? 1 : 0;
   srb9 = (sr & NRMMASK) ? 1 : 0;
#else
   fp_unpack (ach, acl, mqh, mql, 1, &op1);
   fp_unpack (srh, srl, sr2h, sr2l, 0, &op2);
   acp = (ach & P) ? 1 : 0;
   acb9 = (acl & NRMMASK) ? 1 : 0;
   srb9 = (srl & NRMMASK) ? 1 : 0;
#endif

   if (op1.exp > op2.exp)
   {
      if (((op1.exp - op2.exp) > 0100) && (acb9)) ;      /* early out */
      else
      {
#ifdef USE64
	 fp_pack (&si, NULL, 0, 0, 0, 0, &op1);
#else
	 fp_pack (&sih, &sil, NULL, NULL, 0, 0, 0, 0, &op1);
#endif
      }
      if (acp) op1.s = 1;
      t = op1;
      op1 = op2;
      op2 = t;
      op2.exp &= 0377;
      CYCLE ();
   }
   else
   {
      if (((op2.exp - op1.exp) > 077) && (srb9))         /* early out */
      {
         FP_t mqop;

	 mqop.s = op2.s;
	 mqop.exp = op2.exp;
#ifdef USE64
	 mqop.frac = (mq & FRACMASK) << FRACSHIFT;
	 fp_pack (&si, NULL, 0, 0, 0, 0, &mqop);
#else
	 mqop.frac = ((t_uint64)mql & FRACMASK) << FRACSHIFT;
	 fp_pack (&sih, &sil, NULL, NULL, 0, 0, 0, 0, &mqop);
#endif
      }
      else
      {
#ifdef USE64
	 fp_pack (&si, NULL, 0, 0, 0, 0, &op2);
#else
	 fp_pack (&sih, &sil, NULL, NULL, 0, 0, 0, 0, &op2);
#endif
      }
      CYCLE ();
    }
   diff = op2.exp - op1.exp;
   if (diff)
   {
      if ((diff < 0) || (diff > 077)) op1.frac = 0;
      else op1.frac >>= diff;
   }
   if (op1.s ^ op2.s)
   {
      if (op1.frac >= op2.frac)
      {
	 op2.frac = op1.frac - op2.frac;
         op2.s = op1.s;
      }
      else 
         op2.frac -= op1.frac;
   }
   else
   {
      op2.frac += op1.frac;
      if (op2.frac & DCRYMASK)
      {
         op2.frac >>= 1;
         op2.exp++;
      }
   }
   if (nrm)
   {
      if (op2.frac)
      {
         fp_norm (&op2);
         mqexp = op2.exp - FRACSHIFT;
      }
      else op2.exp = mqexp = 0;
      CYCLE ();
   }
   else mqexp = op2.exp - FRACSHIFT;

#ifdef USE64
   fp_pack (&ac, &mq, 1, 1, op2.s, mqexp, &op2);
#else
   fp_pack (&ach, &acl, &mqh, &mql, 1, 1, op2.s, mqexp, &op2);
#endif
}

/***********************************************************************
* dfmpy - Process double floating point Multiply.
***********************************************************************/

void
dfmpy (int nrm)
{
   FP_t op1, op2;
   int32 mqexp;
   uint32 f1h, f2h, f1l, f2l;
   t_uint64 tx;

#ifdef DEBUGFP
   fprintf (stderr, "dfmpy: nrm = %d\n", nrm);
#endif

#ifdef USE64
   fp_unpack (ac, mq, 1, &op1);
   fp_unpack (sr, sr2, 0, &op2);
#else
   fp_unpack (ach, acl, mqh, mql, 1, &op1);
   fp_unpack (srh, srl, sr2h, sr2l, 0, &op2);
#endif

   op1.s ^= op2.s;
   f1h = (uint32)((op1.frac >> FRACSHIFT) & FRACMASK);
   f1l = (uint32)(op1.frac & FRACMASK);
   f2h = (uint32)((op2.frac >> FRACSHIFT) & FRACMASK);
   f2l = (uint32)(op2.frac & FRACMASK);
   if (((op1.exp == 0) && (op1.frac == 0)) ||
       ((op2.exp == 0) && (op2.frac == 0)) ||
       ((f1h == 0) && (f2h == 0)))
   {
#ifdef USE64
      ac = op1.s ? ACSIGN: 0;
      mq = op1.s ? SIGN: 0;
      si = sr;
#else
      ach = op1.s ? SIGN: 0; acl = 0;
      mqh = op1.s ? SIGN: 0; mql = 0;
      sih = ((srh & SIGN) >> 4) | (srh & HMSK); sil = srl;
#endif
      return;
   }
   CYCLE ();
   op1.exp = (op1.exp & 0377) + op2.exp - 0200;
   if (op1.frac)
   {
      op1.frac = ((t_uint64) f1h) * ((t_uint64) f2h);
      tx = ((t_uint64) f1h) * ((t_uint64) f2l);
      op1.frac += (tx >> FRACSHIFT);
      tx = ((t_uint64) f1l) * ((t_uint64) f2h);
      op1.frac += (tx >> FRACSHIFT);
#ifdef USE64
      si = tx >> FRACSHIFT;
#else
      sih = 0; sil = (uint32)(tx >> FRACSHIFT);
#endif
   }
   else
   {
      if (nrm)
      {
#ifdef USE64
	 si = sr;
#else
	 sih = ((srh & SIGN) >> 4) | (srh & HMSK); sil = srl;
#endif
      }
      else
      {
         FP_t siop = op2;

	 siop.frac = 0;
#ifdef USE64
	 fp_pack (&si, NULL, 0, 0, 0, 0, &siop);
#else
	 fp_pack (&sih, &sil, NULL, NULL, 0, 0, 0, 0, &siop);
#endif
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
      CYCLE ();
   }
   else mqexp = op1.exp - FRACSHIFT;

#ifdef USE64
   fp_pack (&ac, &mq, 1, 1, op1.s, mqexp, &op1);
#else
   fp_pack (&ach, &acl, &mqh, &mql, 1, 1, op1.s, mqexp, &op1);
#endif
}

/***********************************************************************
* dfdiv - Process double floating point Division.
***********************************************************************/

void
dfdiv ()
{
   FP_t op1, op2, siop;
   int32 mqexp;
   uint32 csign, ac_s;
   t_uint64 f1h, f2h, tr, tq1, tq1d, trmq1d, tq2;
   t_uint64 sr2frac;

#ifdef DEBUGFP
   fprintf (stderr, "dfdiv: \n");
#endif

   cpuflags &= ~CPU_DIVCHK;
#ifdef USE64
   fp_unpack (ac, mq, 1, &op1);
   fp_unpack (sr, 0, 0, &op2);
   sr2frac = sr2 & FRACMASK;
#else
   fp_unpack (ach, acl, mqh, mql, 1, &op1);
   fp_unpack (srh, srl, 0, 0, 0, &op2);
   sr2frac = sr2l & FRACMASK;
#endif

   ac_s = op1.s;
   op1.s ^= op2.s;
   f1h = (op1.frac >> FRACSHIFT) & FRACMASK;
   f2h = (op2.frac >> FRACSHIFT) & FRACMASK;
   if (f1h >= (2 * f2h))
   {
#ifdef USE64
      si = 0;
#else
      sih = 0; sil = 0;
#endif
      cpuflags |= CPU_DIVCHK;
      return;
   }
   if (f1h == 0)
   {
#ifdef USE64
      si = mq = op1.s ? SIGN: 0;
      ac = op1.s ? ACSIGN: 0;
#else
      mqh = op1.s ? SIGN: 0; mql = 0;
      sih = op1.s ? P: 0; sil = 0;
      ach = op1.s ? SIGN: 0; acl = 0;
#endif
      return;
   }
   CYCLE ();
   op1.exp &= 0377;
   if (f1h >= f2h)
   {
      op1.frac >>= 1;
      op1.exp++;
   }
   op1.exp = op1.exp - op2.exp + 0200;
   tq1 = fp_fracdiv (op1.frac, op2.frac, &tr);
   tr <<= FRACSHIFT;
   tq1d = (tq1 * sr2frac) & ~((t_uint64) FRACMASK);
   csign = (tr < tq1d);
   if (csign) trmq1d = tq1d - tr;
   else trmq1d = tr - tq1d;
   siop = op1;
   siop.frac = (tq1 << FRACSHIFT);
#ifdef USE64
   fp_pack (&si, NULL, 0, 0, 0, 0, &siop);
#else
   fp_pack (&sih, &sil, NULL, NULL, 0, 0, 0, 0, &siop);
#endif
   if (trmq1d >= (2 * op2.frac))
   {
      siop.s = csign ^ ac_s;
      siop.exp = 0;
      siop.frac = trmq1d;
#ifdef USE64
      fp_pack (&ac, NULL, 1, 0, 0, 0, &siop);
      mq = (csign ^ ac_s) ? SIGN: 0;
#else
      fp_pack (&ach, &acl, NULL, NULL, 1, 0, 0, 0, &siop);
      mqh = (csign ^ ac_s) ? SIGN: 0; mql = 0;
#endif
      spill = 0;
      cpuflags |= CPU_DIVCHK;
      return;
   }
   CYCLE ();
   tq2 = fp_fracdiv (trmq1d, op2.frac, NULL);
   if (trmq1d >= op2.frac) tq2 &= ~((t_uint64) 1);
   op1.frac = tq1 << FRACSHIFT;
   if (csign) op1.frac -= tq2;
   else op1.frac += tq2;
   fp_norm (&op1);
   if (op1.frac) mqexp = op1.exp - FRACSHIFT;
   else op1.exp = mqexp = 0;

#ifdef USE64
   fp_pack (&ac, &mq, 1, 1, op1.s, mqexp, &op1);
#else
   fp_pack (&ach, &acl, &mqh, &mql, 1, 1, op1.s, mqexp, &op1);
#endif
}
