/***********************************************************************
*
* useraddctss.c - CTSS add users program.
*
* This program does the work of adding users information the the CTSS control
* files.
*
* Files read/written:
*    ufd.data	 - UFD creation file.
*    quota.data  - User quotas file.
*    timusd.data - Users time used file.
*    uaccnt.data - Users account file.
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
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *fd;
   TIMUSD_t *ptimusd;
   UACCNT_t *puaccnt;
   char *programmername;
   char *projectname;
   char *password;
   char *comment;
   int i;
   int diskquota, drumquota, tapequota;
   int s1time, s2time, s3time, s4time, s5time;
   int programmernumber;
   int maxuaccnt, maxtimusd;
   int privs;
   TIMUSD_t timusd[MAXUSERS];
   UACCNT_t uaccnt[MAXUSERS];

   memset (timusd, 0, sizeof (timusd));
   memset (uaccnt, 0, sizeof (uaccnt));

   s1time = s2time = s3time = s4time = s5time = SHIFTTIME;

   /* Sanity check arguments */

   if (argc != 9)
   {
      fputs ("Usage: useraddctss programmername programmernumber projectname"
	     " password diskquota drumquota tapequota comment\n", stderr);
      exit (1);
   }

   programmername = argv[1];
   programmernumber = atoi (argv[2]);
   projectname = argv[3];
   password = argv[4];
   diskquota = atoi (argv[5]);
   drumquota = atoi (argv[6]);
   tapequota = atoi (argv[7]);
   comment = argv[8];

   upcase (programmername);
   upcase (projectname);
   upcase (password);
   upcase (comment);

   privs = RCOMBT;
   if (tapequota) 
      privs |= R636BT;

#ifdef DEBUG
   printf (
  "useraddctss: programmername = %s, programmernumber = %d, projectname = %s\n",
           programmername, programmernumber, projectname);
   printf ("   password = %s, diskquota = %d, drumquota = %d, tapequota = %d\n",
           password, diskquota, drumquota, tapequota);
   printf ("   comment = '%s'\n", comment);
   printf ("   sizeof UACCNT_t = %d, sizeof TIMUSD_t = %d\n",
           sizeof(UACCNT_t), sizeof(TIMUSD_t));
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
   if (strlen (password) > NAMELEN)
   {
      fprintf (stderr, "Password too long: %s\n", password);
      exit (1);
   }
   if (strlen (comment) > COMMENTLEN)
   {
      fprintf (stderr, "Comment too long: '%s'\n", comment);
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
      printf ("   uaccnt[%d]:\n", i);
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
      printf ("   timusd[%d]:\n", i);
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

   /* Check for a duplicate user */

   for (i = 0; i < maxtimusd; i++)
   {
      char user[NAMELEN+2];

      getalphanum (user, timusd[i].uname, NAMELEN);
      if (!strcmp (user, programmername))
      {
         fprintf (stderr, "Duplicate user: %s %s\n",
	          projectname, programmername);
	 exit (1);
      }

      if (getnum (timusd[i].progno, NUMLEN) == programmernumber)
      {
         fprintf (stderr, "Duplicate programmer number: %d\n",
	          programmernumber);
	 exit (1);
      }
   }
   if ((maxuaccnt + 1) >= MAXUSERS)
   {
      fprintf (stderr, "Too many users: MAX = %d\n", MAXUSERS);
      exit (1);
   }

   /* Write out UFD info */

   if ((fd = fopen (UFDFILE, "w")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", UFDFILE, strerror (errno));
      exit (1);
   }

   fprintf (fd, "%6.6s%-6.6s\n", projectname, programmername);
   fclose (fd);

   /* Write out Quota info */

   if ((fd = fopen (QUOTAFILE, "w")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", QUOTAFILE, strerror (errno));
      exit (1);
   }

   fprintf (fd, "%6.6s%-6.6s%06d%06d%06d\n",
	    projectname, programmername, drumquota, diskquota, tapequota);
   fclose (fd);

   /* Add user to TIMUSD */

   sprintf ((char *)&timusd[maxtimusd++], 
	    "%6.6s%6d%6.6s%-66.66s0000000000000000000000000000000000000"
	    "00000000000000000000000\n",
	    projectname, programmernumber, programmername, comment);
   
   /* Add user to UACCNT */

   sprintf ((char *)&uaccnt[maxuaccnt++], 
            "%-6.6s%6d000001000000%-6.6s000001%06o000000%6.6s"
	    "                              %06d%06d%06d"
	    "%06d%06d%06d%06d%06d\n",
	    programmername, programmernumber, programmername, privs, password,
	    drumquota, diskquota, tapequota,
	    s1time, s2time, s3time, s4time, s5time);

   /* Write out UACCNT file */

   if ((fd = fopen (UACCNTFILE, "w")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", UACCNTFILE, strerror (errno));
      exit (1);
   }

   for (i = 0; i < maxuaccnt; i++)
   {
      if (fputs ((char *)&uaccnt[i], fd) < 0)
      {
	 fprintf (stderr, "Can't write: %s: %s\n", UACCNTFILE, strerror(errno));
	 fclose (fd);
	 exit (1);
      }
   }
   fclose (fd);

   /* Write out TIMUSD file */

   if ((fd = fopen (TIMUSDFILE, "w")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", TIMUSDFILE, strerror (errno));
      exit (1);
   }

   for (i = 0; i < maxtimusd; i++)
   {
      if (fputs ((char *)&timusd[i], fd) < 0)
      {
	 fprintf (stderr, "Can't write: %s: %s\n", TIMUSDFILE, strerror(errno));
	 fclose (fd);
	 exit (1);
      }
   }
   fclose (fd);
}
