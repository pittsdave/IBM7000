/***********************************************************************
*
* userreportctss.c - CTSS report users program.
*
* This program does the work of generating user reports for CTSS.
*
* Files read:
*    timusd.data - Users time used file.
*    uaccnt.data - Users account file.
*    report.data - Users report file.
*
* Changes:
*   07/23/13   DGP   Original.
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

static char *loginerr[] =
{
   "Success",
   "System full",
   "No time on present shift",
   "Exceeded time on present shift",
   "Out of funds",
   "Past expiration date",
   "Already logged in",
   "Has FIB job running",
   "Bad username or password",
   "Illegal UnitID",
   "System error"
};

static char *timemsg[] =
{
   "System shutdown",
   "Exceeded shift allotment",
   "Inactive too long",
   "Hung up",
   "Exceeded FIB time",
   "Load leveler termination"
};

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *fd;
   TIMUSD_t *ptimusd;
   UACCNT_t *puaccnt;
   REPORT_t *preport;
   char *programmername;
   char *projectname;
   char *password;
   char *comment;
   int i;
   int diskquota, drumquota, tapequota;
   int detail;
   int allusers;
   int allprojects;
   int maxuaccnt, maxtimusd, maxreport;
   TIMUSD_t timusd[MAXUSERS];
   UACCNT_t uaccnt[MAXUSERS];
   REPORT_t report[MAXREPORT];

   memset (timusd, 0, sizeof (timusd));
   memset (uaccnt, 0, sizeof (uaccnt));
   memset (report, 0, sizeof (report));
   allusers = FALSE;
   allprojects = FALSE;

   /* Sanity check arguments */

   if (argc != 4)
   {
      fputs ("Usage: userreportctss programmername projectname detail\n",
             stderr);
      exit (1);
   }

   programmername = argv[1];
   projectname = argv[2];
   detail = atoi (argv[3]);

   upcase (programmername);
   upcase (projectname);

   if (!strcmp (programmername, "___ALL"))
      allusers = TRUE;
   if (!strcmp (projectname, "___ALL"))
      allprojects = TRUE;

#ifdef DEBUG
   fprintf (stderr,
        "userreportctss: programmername = %s, projectname = %s, detail = %d\n",
            programmername, projectname, detail);
   fprintf (stderr,
            "   size UACCNT_t = %d, size TIMUSD_t = %d, size REPORT_t = %d\n",
            sizeof(UACCNT_t), sizeof(TIMUSD_t), sizeof(REPORT_t));
#endif

   if (strlen (programmername) > NAMELEN)
   {
      fprintf (stderr, "Programmer name too long: %s\n", programmername);
      exit (1);
   }
   if (strlen (projectname) > NAMELEN)
   {
      fprintf (stderr, "Project name too long: %s\n", projectname);
      exit (1);
   }

   /* Read in UACCNT info */

   if ((fd = fopen (UACCNTFILE, "r")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", UACCNTFILE, strerror (errno));
      exit (1);
   }

   for (i = 0; i < MAXUSERS; i++)
   {
      if (fgets ((char *)&uaccnt[i], sizeof(UACCNT_t), fd) == NULL)
      {
         if (feof (fd)) break;
	 fprintf (stderr, "Can't read: %s: %s\n", UACCNTFILE, strerror(errno));
	 fclose (fd);
	 exit (1);
      }
      puaccnt = &uaccnt[i];
#ifdef DEBUG
      fprintf (stderr, "   uaccnt[%d]:\n", i);
      printuaccnt (puaccnt);
#endif
   }
   if (i == MAXUSERS)
   {
      fprintf (stderr, "Too many users: MAX = %d\n", MAXUSERS);
      fclose (fd);
      exit (1);
   }
   maxuaccnt = i;
   fclose (fd);

   /* Read in TIMUSD info */

   if ((fd = fopen (TIMUSDFILE, "r")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", TIMUSDFILE, strerror (errno));
      exit (1);
   }

   for (i = 0; i < MAXUSERS; i++)
   {
      if (fgets ((char *)&timusd[i], sizeof(TIMUSD_t), fd) == NULL)
      {
         if (feof (fd)) break;
	 fprintf (stderr, "Can't read: %s: %s\n", TIMUSDFILE, strerror(errno));
	 fclose (fd);
	 exit (1);
      }
      ptimusd = &timusd[i];
#ifdef DEBUG
      fprintf (stderr, "   timusd[%d]:\n", i);
      printtimusd (ptimusd);
#endif
   }
   if (i == MAXUSERS)
   {
      fprintf (stderr, "Too many users: MAX = %d\n", MAXUSERS);
      fclose (fd);
      exit (1);
   }
   maxtimusd = i;
   fclose (fd);

   /* Read in REPORT info if detail */

   maxreport = 0;
   if (detail)
   {
      if ((fd = fopen (REPORTFILE, "r")) == NULL)
      {
	 fprintf (stderr, "Can't open: %s: %s\n", REPORTFILE, strerror (errno));
	 exit (1);
      }

      for (i = 0; i < MAXREPORT; i++)
      {
	 if (fgets ((char *)&report[i], sizeof(REPORT_t), fd) == NULL)
	 {
	    if (feof (fd)) break;
	    fprintf (stderr, "Can't read: %s: %s\n",
		     REPORTFILE, strerror(errno));
	    fclose (fd);
	    exit (1);
	 }
	 preport = &report[i];
#ifdef DEBUG
	 fprintf (stderr, "   report[%d]:\n", i);
	 printreport (preport);
#endif
      }
      if (i == MAXREPORT)
      {
	 fprintf (stderr, "Too many report lines: MAX = %d\n", MAXREPORT);
	 fclose (fd);
	 exit (1);
      }
      maxreport = i;
      fclose (fd);
   }

   /* Check for existence of user */

   if (!allusers)
   {
      for (i = 0; i < maxtimusd; i++)
      {
	 char proj[NAMELEN+2], user[NAMELEN+2];

	 getalphanum (proj, timusd[i].probno, NAMELEN);
	 getalphanum (user, timusd[i].uname, NAMELEN);
	 if (!strcmp (proj, projectname) && !strcmp (user, programmername))
	    break;
      }
      if (i == maxtimusd)
      {
	 fprintf (stderr, "Unknown user: %s %s\n",
		  projectname, programmername);
	 exit (1);
      }
   }

   /* Print the user report info */

   for (i = 0; i < maxtimusd; i++)
   {
      int printit;
      char proj[NAMELEN+2], user[NAMELEN+2];

      getalphanum (proj, timusd[i].probno, NAMELEN);
      getalphanum (user, timusd[i].uname, NAMELEN);

      printit = FALSE;
      if (allusers && allprojects)
         printit = TRUE;
      if (allusers && !strcmp (proj, projectname))
         printit = TRUE;
      else if (!strcmp (proj, projectname) && !strcmp (user, programmername))
         printit = TRUE;

      if (printit)
      {
	 int j;
	 char comment[COMMENTLEN+2];
	 char datetime[DATETIMELEN+2];
	 char converteddate[DATETIMELEN+6];

	 for (j = 0; j < maxuaccnt; j++)
	 {
	    char uuser[NAMELEN+2];

	    if (uaccnt[j].uname[0] == '*') continue;
	    getalphanum (uuser, uaccnt[j].uname, NAMELEN);
	    if (!strcmp (uuser, user))
	       break;
	 }
	 if (j == maxuaccnt)
	 {
	    fputs ("Internal error: User mismatch\n", stderr);
	    exit (1);
	 }

	 getalphanum (comment, timusd[i].comment, COMMENTLEN);
	 getalphanum (datetime, timusd[i].lastlogout, DATETIMELEN);
	 if (!strcmp (datetime, "000000000000"))
	    strcpy (converteddate, "** NEVER USED **");
         else
	    convertdate (converteddate, datetime);

	 printf ("User: %s %s progno: %d lastlogout: %s\n",
		 proj, user, getnum (uaccnt[j].progno, NUMLEN), converteddate);
	 printf ("   quotas: disk %d, drum %d, tape %d\n",
	         getnum (uaccnt[j].diskquota, NUMLEN),
	         getnum (uaccnt[j].drumquota, NUMLEN),
	         getnum (uaccnt[j].tapequota, NUMLEN));
	 printf ("   comment: %s\n", comment);
	 printf (
	   "   s1: %d/%d  s2: %d/%d  s3: %d/%d  s4: %d/%d  s5: %d/%d  ct: %d\n",
	         getnum (timusd[i].s1time, NUMLEN),
	         getnum (uaccnt[j].s1time, NUMLEN),
	         getnum (timusd[i].s2time, NUMLEN),
	         getnum (uaccnt[j].s2time, NUMLEN),
	         getnum (timusd[i].s3time, NUMLEN),
	         getnum (uaccnt[j].s3time, NUMLEN),
	         getnum (timusd[i].s4time, NUMLEN),
	         getnum (uaccnt[j].s4time, NUMLEN),
	         getnum (timusd[i].s5time, NUMLEN),
	         getnum (uaccnt[j].s5time, NUMLEN),
	         getnum (timusd[i].contime, NUMLEN));

	 if (detail)
	 {
	    int k;

	    for (k = 0; k < maxreport; k++)
	    {
	       int flags;
	       int command;
	       int notime;
	       int errcode;
	       char tuser[NAMELEN+2];

	       getalphanum (tuser, report[k].uname, NAMELEN);
	       if (!strcmp (tuser, user))
	       {
		  getalphanum (datetime, report[k].datetime, DATETIMELEN);
		  convertdate (converteddate, datetime);

		  flags = getoctnum (report[k].flags, NUMLEN);
		  command = (flags >> 15) & 07;
		  notime = (flags >> 9) & 077;
		  errcode = flags & 0777;

		  if (command == 0)
		  {
		     if (errcode == 0)
			printf ("      Logged in:    %s\n", converteddate);
		     else
		        printf ("      Login failed: %s: %s\n",
				converteddate, loginerr[errcode]);
		  }
		  else
		  {
		     int nt, ti;

		     for (nt = notime, ti = 0; nt; nt >>= 1) ti++;

		     if (command == 1)
			printf ("      Logged out:   %s\n", converteddate);
		     else
			printf ("      Auto logout:  %s: code %o: %s\n",
				converteddate, notime, timemsg[ti]);
		     printf (
		     "        s1: %d  s2: %d  s3: %d  s4: %d  s5: %d  ct: %d\n",
			     getnum (report[k].s1time, NUMLEN),
			     getnum (report[k].s2time, NUMLEN),
			     getnum (report[k].s3time, NUMLEN),
			     getnum (report[k].s4time, NUMLEN),
			     getnum (report[k].s5time, NUMLEN),
			     getnum (report[k].contime, NUMLEN));
		  }
	       }
	    }
	 }

         if (allusers) putchar ('\n');
      }
   }
}
