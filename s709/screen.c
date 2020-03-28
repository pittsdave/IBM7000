/***********************************************************************
*
* screen.c - IBM 7090 emulator Screen control routines.
*
* Changes:
*   10/20/03   DGP   Original.
*   12/28/11   DGP   Added Windows screen support.
*   
***********************************************************************/

#include <stdio.h>

#if defined(USS) || defined(OS390)
#define ESCAPE  0x27
#else
#define ESCAPE  0x1B
#endif
 
#if defined(WIN32) && !defined(MINGW)

#include <windows.h>
#include <wincon.h>

static void
set_screen_pos (short rowY1, short colX1)
{
   CONSOLE_SCREEN_BUFFER_INFO csbi;
   HANDLE hStdErr;
   COORD ptConsole;
   int cons_fd;

   if (!_isatty (cons_fd = fileno (stderr)))
      return;

   if ((hStdErr = (HANDLE) _get_osfhandle (cons_fd)) == NULL)
      return;

   GetConsoleScreenBufferInfo (hStdErr, &csbi);

   ptConsole.X = colX1 - 1;
   ptConsole.Y = rowY1 - 1;

   SetConsoleCursorPosition (hStdErr, ptConsole);
}

static void
clear_screen (void)
{
   CONSOLE_SCREEN_BUFFER_INFO csbi;
   HANDLE hStdErr;
   DWORD dwNumCells, dwCellsWritten;
   COORD ptConsole = { 0, 0 };
   int cons_fd;

   if (!_isatty (cons_fd = fileno (stderr)))
      return;

   if ((hStdErr = (HANDLE) _get_osfhandle (cons_fd)) == NULL)
      return;

   GetConsoleScreenBufferInfo (hStdErr, &csbi);
   dwNumCells = csbi.dwSize.X * csbi.dwSize.Y;
   FillConsoleOutputCharacter (hStdErr, ' ', dwNumCells, ptConsole,
			       &dwCellsWritten);
   GetConsoleScreenBufferInfo (hStdErr, &csbi);
   FillConsoleOutputAttribute (hStdErr, csbi.wAttributes, dwNumCells,
			       ptConsole, &dwCellsWritten);
   SetConsoleCursorPosition (hStdErr, ptConsole);
}

static void
erase_to_eol (void)
{
   CONSOLE_SCREEN_BUFFER_INFO csbi;
   HANDLE hStdErr;
   DWORD dwCellsWritten;
   COORD ptConsole;
   int cons_fd;

   if (!_isatty (cons_fd = fileno (stderr)))
      return;

   if ((hStdErr = (HANDLE) _get_osfhandle (cons_fd)) == NULL)
      return;

   GetConsoleScreenBufferInfo (hStdErr, &csbi);
   ptConsole = csbi.dwCursorPosition;
   FillConsoleOutputAttribute (hStdErr, csbi.wAttributes,
			       csbi.dwSize.X - ptConsole.X, ptConsole,
			       &dwCellsWritten);
   FillConsoleOutputCharacter (hStdErr, ' ', csbi.dwSize.X - ptConsole.X,
			       ptConsole, &dwCellsWritten);
}
#endif

/***********************************************************************
* clearscreen - clear the screen
***********************************************************************/
 
void
clearscreen (void)
{
#ifdef ANSICRT
   fprintf (stdout, "%c[2J", ESCAPE);
#endif

#if defined(WIN32) && !defined(MINGW)
   clear_screen ();
#endif
}
 
/***********************************************************************
* clearline - clear the line
***********************************************************************/
 
void
clearline (void)
{
#ifdef ANSICRT
   fprintf (stdout, "%c[2K", ESCAPE);
#endif

#if defined(WIN32) && !defined(MINGW)
   erase_to_eol ();
#endif
}
 
/***********************************************************************
* screen position - position on the screen
***********************************************************************/
 
void
screenposition (int row, int col)
{
#ifdef ANSICRT
   fprintf (stdout, "%c[%d;%dH", ESCAPE, row, col);
#endif

#if defined(WIN32) && !defined(MINGW)
   set_screen_pos ((short)row, (short)col);
#endif
}
