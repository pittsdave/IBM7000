/***********************************************************************
*
* userdelctss.c - CTSS delete users program.
*
* This program does the work of deleting users information the the CTSS control
* files.
*
* Files read/written:
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
   int i;
   int prognum;
   int maxuaccnt, maxtimusd;
   TIMUSD_t timusd[MAXUSERS];
   UACCNT_t uaccnt[MAXUSERS];

   memset (timusd, 0, sizeof (timusd));
   memset (uaccnt, 0, sizeof (uaccnt));

   /* Sanity check arguments */

   if (argc != 3)
   {
      fputs ("Usage: userdelctss programmername projectname\n", stderr);
      exit (1);
   }

   programmername = argv[1];
   projectname = argv[2];

   upcase (programmername);
   upcase (projectname);

#ifdef DEBUG
   printf ("userdelctss: programmername = %s, projectname = %s\n",
           programmername, projectname);
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

   /* Check for existence of user */

   for (i = 0; i < maxtimusd; i++)
   {
      char proj[NAMELEN+2], user[NAMELEN+2];

      getalphanum (proj, timusd[i].probno, NAMELEN);
      getalphanum (user, timusd[i].uname, NAMELEN);
      if (!strcmp (proj, projectname) && !strcmp (user, programmername))
      {
	 prognum = getnum (timusd[i].progno, NUMLEN);
	 memset (timusd[i].uname, '-', NAMELEN);
         break;
      }
   }
   if (i == maxtimusd)
   {
      fprintf (stderr, "Unknown user: %s %s\n",
	       projectname, programmername);
      exit (1);
   }

   /* Look through UACCNT */

   for (i = 0; i < maxuaccnt; i++)
   {
      char user[NAMELEN+2];

      getalphanum (user, uaccnt[i].uname, NAMELEN);
      if (getnum (uaccnt[i].progno, NUMLEN) == prognum && 
          !strcmp (user, programmername))
      {
	 memset (uaccnt[i].uname, '-', NAMELEN);
	 break;
      }
   }

   /* Write out UACCNT file */

   if ((fd = fopen (UACCNTFILE, "w")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", UACCNTFILE, strerror (errno));
      exit (1);
   }

   for (i = 0; i < maxuaccnt; i++)
   {
      if (uaccnt[i].uname[0] == '-') continue;
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
      if (timusd[i].uname[0] == '-') continue;
      if (fputs ((char *)&timusd[i], fd) < 0)
      {
	 fprintf (stderr, "Can't write: %s: %s\n", TIMUSDFILE, strerror(errno));
	 fclose (fd);
	 exit (1);
      }
   }
   fclose (fd);
}
