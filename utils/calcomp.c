/***********************************************************************
*
* calcomp.c - Calcomp plotter routines.
*  These routines were hacked from several Fortran routines.
*
* Changes:
*   12/08/11   DGP   Original.
*   
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>

static FILE *pfd;

static int res = 300;

static float fxold = 0.0;
static float fyold = 0.0;
static float xorigin = 0.0;
static float yorigin = 0.0;
static float curfact = 1.0;

/*********************************************************************
*
* PLOT moves the plotting pen to a new position.
*
*    The PLOT command changes the location of the pen.
*
*    If the pen is down, a line will be drawn as the pen moves.
*    If the pen is up, no line will be drawn.
*
*    In the special case where IPEN is 999, then the call to
*    PLOT does not move the pen; instead, it is interpreted as
*    a request to terminate the current plot.
*
*  Parameters:
*
*    Input: float XPAGE, YPAGE, the coordinates of the new pen
*       location.
*    Input: integer IPEN, determines the pen status.
*        2: the pen is down, and a line is drawn.
*        3: the pen is up, and no line is drawn.
*       -2: the pen is down, a line is drawn, and the location
*           (XPAGE,YPAGE) becomes the new plot origin.
*       -3: the pen is up, no line is drawn, and the location 
*           (XPAGE,YPAGE) becomes the new plot origin.
*       999: the current plot is to be terminated.
*
*********************************************************************/
void
plot (float xpage, float ypage, int ipen)
{
   int x, y;
   int xold, yold;

   x = ((xorigin + xpage) * curfact * res);
   y = ((yorigin + ypage) * curfact * res);
   xold = ((xorigin + fxold) * curfact * res);
   yold = ((yorigin + fyold) * curfact * res);

   if (abs (ipen) == 2)
   {
      fprintf (pfd, " %d %d moveto\n", xold, yold);
      fprintf (pfd, " %d %d lineto stroke\n", x, y);
      fxold = xpage;
      fyold = ypage;
   }
   else if (abs (ipen) == 3)
   {
      fprintf (pfd, " %d %d moveto\n", x, y);
      fxold = xpage;
      fyold = ypage;
   }
   else if (ipen == 999)
   {
      fputs (" restore\n", pfd);
      fputs (" showpage\n", pfd);
      fputs ("%%Trailer\n", pfd);
      fputs ("%%Pages: 1\n", pfd);
      fputs ("%%EOF\n", pfd);
      fclose (pfd);
      return;
   }
   else
   {
      fprintf (stderr, "PLOT - Fatal error: IPEN = %d\n", ipen);
      exit (12);
   }

   if (ipen < 0)
   {
      xorigin = xorigin + xpage;
      yorigin = yorigin + ypage;
   }
}

/*********************************************************************
*
* PLOTS is called to initialize a plot.
*
*  Discussion:
*
*    PLOTS must be the first CALCOMP routine called when creating
*    a plot.
*
*    The corresponding call to finalize a plot is through the
*    "PLOT" command, using a third argument with the flag value
*    of 999:
*
*      plot (x, y, 999);
*
*  Parameters:
*
*    Input: integer I specifies the resolution in DPI.
*    Input: integer J specifes the drawing rotation.
*       0 = no rotation.
*       1 = rotate 90 degrees.
*    Input: integer LDEV, the FORTRAN logical unit number for the plot.
*       In our case we use it in the naming of the output file. The 
*       name is generated as: calcomp_NNN.ps
*
*********************************************************************/
void
plots (int i, int j, int ldev)
{
   time_t curtime;
   char file_name[20];

   sprintf (file_name, "calcomp_%03d.ps", ldev);
   if ((pfd = fopen (file_name, "w")) == NULL)
   {
      fprintf (stderr, "PLOTS - Fatal error: %s: %s\n",
               file_name, strerror (errno));
      exit (12);
   }

   if (i > 0)
      res = i;

   fputs ("%!PS-Adobe-1.0\n", pfd);

   fputs ("%%Creator: calcomp.c\n", pfd);
   fprintf (pfd, "%%Title: %s\n", file_name);

   curtime = time (NULL);
   fprintf (pfd, "%%CreationDate: %s", ctime (&curtime));
   fputs ("%%Document-Fonts: Times-Roman\n", pfd);
   fputs ("%%EndComments\n", pfd);
   fputs ("%%LanguageLevel: 1\n", pfd);
   fputs ("%%EndComments\n", pfd);
   fprintf (pfd, "  72 %d div dup scale\n", 300);
   if (j == 1)
   {
      fputs (" 90 rotate\n", pfd);
      fputs (" 75 -2460 translate\n", pfd);
      xorigin = 0.50;
   }
   else
   {
      xorigin = 0.25;
      yorigin = 0.25;
   }
   fputs (" 0 0 moveto\n", pfd);
   fputs (" 1 setlinewidth\n", pfd);
   fputs (" 1 setlinejoin\n", pfd);
   fputs (" 1 setlinecap\n", pfd);
   fputs (" save\n", pfd);
}
