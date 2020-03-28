/***********************************************************************
*
* genext.c Generate extract record for the extract program.
*
* Changes:
*   12/12/11   DGP   Original.
*   
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
   char *prog, *proj;
   char *fname1, *fname2;
   char *mode;
   int i, j;

   proj = "M1416";
   prog = "GUEST";
   fname1 = NULL;
   fname2 = NULL;
   mode = NULL;

   j = 0;
   for (i = 1; i < argc; i++)
   {
      if (*argv[i] == '-')
         goto USAGE;
      switch (j++)
      {
      case 0:
	 mode = argv[i];
	 break;
      case 1:
	 fname1 = argv[i];
	 break;
      case 2:
	 fname2 = argv[i];
	 break;
      case 3:
	 proj = argv[i];
	 break;
      case 4:
	 prog = argv[i];
	 break;
      default:
         goto USAGE;
      }
   }

   if ((fname1 == NULL) || (fname2 == NULL) || (mode == NULL))
   {
   USAGE:
      fputs ("usage: genext mode fname1 fname2 [proj [prog]]\n", stderr);
      return (12);
   }

   printf ("%6.6s%6.6s%6.6s%6.6s%-6.6s\n", fname1, fname2 ,mode, proj, prog);
   return (0);
}
