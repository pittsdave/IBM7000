/***********************************************************************
*
* usersupport.c - CTSS users support routines.
*
* Changes:
*   07/18/13   DGP   Original.
*   
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "sysdef.h"
#include "timacc.h"
#include "usersupport.h"

/***********************************************************************
* Upcase field.
***********************************************************************/

void
upcase (char *bp)
{
   for (; *bp; bp++)
   {
      if (islower (*bp))
         *bp = toupper (*bp);
   }
}

/***********************************************************************
* Get alphanumeric field.
***********************************************************************/

char *
getalphanum (char *to, char *from, int len)
{
   char *rp = to;
   int i;
   int first = TRUE;

   for (i = 0; i < len; i++)
   {
      char ch;

      ch = *from++;
      if (first && ch == ' ') continue;
      first = FALSE;
      *to++ = ch;
   }
   *to = 0;
   for (to--; i && isspace(*to); i--, to--) *to = 0;
   return rp;
}

/***********************************************************************
* Get numeric field.
***********************************************************************/

int
getnum (char *from, int len)
{
   int num = 0;
   int i;

   for (i = 0; i < len; i++)
   {
      char ch;

      ch = *from++;
      if (isdigit(ch))
         num = (num * 10) + (ch - '0');
   }
   return num;
}

/***********************************************************************
* Get octal numeric field.
***********************************************************************/

int
getoctnum (char *from, int len)
{
   int num = 0;
   int i;

   for (i = 0; i < len; i++)
   {
      char ch;

      ch = *from++;
      if ((ch >= '0') && (ch < '8'))
         num = (num << 3) | (ch - '0');
   }
   return num;
}

/***********************************************************************
* Convert datetime to "mm/dd/yy hhmm.t"
***********************************************************************/

char *
convertdate (char *todate, char *fromdate)
{
   todate[0] = fromdate[0];
   todate[1] = fromdate[1];
   todate[2] = '/';
   todate[3] = fromdate[2];
   todate[4] = fromdate[3];
   todate[5] = '/';
   todate[6] = fromdate[4];
   todate[7] = fromdate[5];
   todate[8] = ' ';
   todate[9]  = fromdate[6];
   todate[10] = fromdate[7];
   todate[11] = fromdate[8];
   todate[12] = fromdate[9];
   todate[13] = fromdate[10];
   todate[14] = fromdate[11];
   todate[15] = 0;
   return todate;
}

/***********************************************************************
* Print uaccnt record.
***********************************************************************/

void
printuaccnt (UACCNT_t *puaccnt)
{
   char temp[80];

   fprintf (stderr, "      uname     = %s\n",
	    getalphanum (temp, puaccnt->uname, NAMELEN));
   if (puaccnt->uname[0] != '*')
   {
      fprintf (stderr, "      progno    = %d\n",
               getnum (puaccnt->progno, NUMLEN));
      fprintf (stderr, "      group     = %d\n",
               getnum (puaccnt->group, NUMLEN));
      fprintf (stderr, "      stanby    = %d\n",
               getnum (puaccnt->stanby, NUMLEN));
      fprintf (stderr, "      ufd       = %s\n", 
	       getalphanum (temp, puaccnt->ufd, NAMELEN));
      fprintf (stderr, "      unit      = %d\n",
               getnum (puaccnt->unit, NUMLEN));
      fprintf (stderr, "      rcode     = %d\n",
               getnum (puaccnt->rcode, NUMLEN));
      fprintf (stderr, "      flags     = %d\n",
               getnum (puaccnt->flags, NUMLEN));
      fprintf (stderr, "      password  = %s\n",
	       getalphanum (temp, puaccnt->password, NAMELEN));
      fprintf (stderr, "      drumquota = %d\n",
               getnum (puaccnt->drumquota, NUMLEN));
      fprintf (stderr, "      diskquota = %d\n",
               getnum (puaccnt->diskquota, NUMLEN));
      fprintf (stderr, "      tapequota = %d\n",
               getnum (puaccnt->tapequota, NUMLEN));
      fprintf (stderr, "      s1time    = %d\n",
               getnum (puaccnt->s1time, NUMLEN));
      fprintf (stderr, "      s2time    = %d\n",
               getnum (puaccnt->s2time, NUMLEN));
      fprintf (stderr, "      s3time    = %d\n",
               getnum (puaccnt->s3time, NUMLEN));
      fprintf (stderr, "      s4time    = %d\n",
               getnum (puaccnt->s4time, NUMLEN));
      fprintf (stderr, "      s5time    = %d\n",
               getnum (puaccnt->s5time, NUMLEN));
   }
}

/***********************************************************************
* Print timusd record.
***********************************************************************/

void
printtimusd (TIMUSD_t *ptimusd)
{
   char temp[80];

   fprintf (stderr, "      probno     = %s\n",
	    getalphanum (temp, ptimusd->probno, NAMELEN));
   fprintf (stderr, "      progno     = %d\n",
            getnum (ptimusd->progno, NUMLEN));
   fprintf (stderr, "      uname      = %s\n",
	    getalphanum (temp, ptimusd->uname, NAMELEN));
   fprintf (stderr, "      comment    = '%s'\n", 
	    getalphanum (temp, ptimusd->comment, COMMENTLEN));
   fprintf (stderr, "      lastlogout = %s\n",
	    getalphanum (temp, ptimusd->lastlogout, DATETIMELEN));
   fprintf (stderr, "      unit       = %d\n",
            getnum (ptimusd->unit, NUMLEN));
   fprintf (stderr, "      s1time     = %d\n",
            getnum (ptimusd->s1time, NUMLEN));
   fprintf (stderr, "      s2time     = %d\n",
            getnum (ptimusd->s2time, NUMLEN));
   fprintf (stderr, "      s3time     = %d\n",
            getnum (ptimusd->s3time, NUMLEN));
   fprintf (stderr, "      s4time     = %d\n",
            getnum (ptimusd->s4time, NUMLEN));
   fprintf (stderr, "      s5time     = %d\n",
            getnum (ptimusd->s5time, NUMLEN));
   fprintf (stderr, "      contime    = %d\n",
            getnum (ptimusd->contime, NUMLEN));
}

/***********************************************************************
* Print report record.
***********************************************************************/

void
printreport (REPORT_t *preport)
{
   char temp[80];

   fprintf (stderr, "      probno    = %s\n",
	    getalphanum (temp, preport->probno, NAMELEN));
   fprintf (stderr, "      progno    = %d\n",
            getnum (preport->progno, NUMLEN));
   fprintf (stderr, "      uname     = %s\n",
	    getalphanum (temp, preport->uname, NAMELEN));
   fprintf (stderr, "      datetime  = %s\n",
	    getalphanum (temp, preport->datetime, DATETIMELEN));
   fprintf (stderr, "      unit      = %d\n",
            getnum (preport->unit, NUMLEN));
   fprintf (stderr, "      flags     = %d\n",
            getnum (preport->flags, NUMLEN));
   fprintf (stderr, "      s1time    = %d\n",
            getnum (preport->s1time, NUMLEN));
   fprintf (stderr, "      s2time    = %d\n",
            getnum (preport->s2time, NUMLEN));
   fprintf (stderr, "      s3time    = %d\n",
            getnum (preport->s3time, NUMLEN));
   fprintf (stderr, "      s4time    = %d\n",
            getnum (preport->s4time, NUMLEN));
   fprintf (stderr, "      s5time    = %d\n",
            getnum (preport->s5time, NUMLEN));
   fprintf (stderr, "      contime   = %d\n",
            getnum (preport->contime, NUMLEN));
}
