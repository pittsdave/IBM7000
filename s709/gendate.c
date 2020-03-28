/***********************************************************************
*
* gendate.c - IBM 7090 emulator generate $DATE record. 
*
* Changes:
*   02/14/05   DGP   Original.
*   
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int
main ()
{
   time_t curtime;
   struct tm *timeblk;

   curtime = time (NULL);
   timeblk = localtime (&curtime);

   printf ("$LIST \n");
   printf ("$DATE          %02d%02d%02d \n",
	    timeblk->tm_mon+1, timeblk->tm_mday, timeblk->tm_year - 100);
   return (0);
}
