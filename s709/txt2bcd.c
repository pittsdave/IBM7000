/***********************************************************************
*
* txt2bcd.c - IBM 7090 Text to BCD converter.
*
* Changes:
*   ??/??/??   PRP   Original.
*   01/28/05   DGP   Changed to use IBSYS standard characters as default.
*   03/18/05   DGP   Changed 072 usage, in altbcd it is a '?'.
*   01/03/06   DGP   Added alternate BCD support.
*   04/13/06   DGP   Added '~' as alternate eof char.
*   05/31/06   DGP   Added simh format support and blocking.
*   10/24/06   DGP   Remove length mask in tapewriteint.
*   08/19/11   DGP   Slurp to newline on EOF records.
*   
***********************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "nativebcd.h"
#include "cvtpar.h"
#include "prsf2.h"

#define MAXREC 32768

char fin[300], fon[300];

static int altchars;
static int simhfmt;
static int reclen;
static int blklen;
static int chrcnt;
static int chrblk;

static char datbuf[MAXREC];

#if '\n' == 0x0A && ' ' == 0x20 && '0' == 0x30 \
  && 'A' == 0x41 && 'a' == 0x61 && '!' == 0x21

/*
** ASCII to BCD conversion table. 
*/

static unsigned char tobcd[128] =
{
 /*00  NL   SH   SX   EX   ET   NQ   AK   BL */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*08  BS   HT   LF   VT   FF   CR   SO   SI */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*10  DL   D1   D2   D3   D4   NK   SN   EB */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*18  CN   EM   SB   EC   FS   GS   RS   US */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*20  SP    !    "    #    $    %    &    ' */
      060, 052, 060, 060, 053, 060, 060, 014,
 /*28   (    )    *    +    ,    -    .    / */
      074, 034, 054, 020, 073, 040, 033, 061,
 /*30   0    1    2    3    4    5    6    7 */
      000, 001, 002, 003, 004, 005, 006, 007,
 /*38   8    9    :    ;    <    =    >    ? */
      010, 011, 060, 060, 060, 013, 060, 032,
 /*40   @    A    B    C    D    E    F    G */
      060, 021, 022, 023, 024, 025, 026, 027,
 /*48   H    I    J    K    L    M    N    O */
      030, 031, 041, 042, 043, 044, 045, 046,
 /*50   P    Q    R    S    T    U    V    W */
      047, 050, 051, 062, 063, 064, 065, 066,
 /*58   X    Y    Z    [    \    ]    ^    _ */
      067, 070, 071, 060, 060, 060, 060, 060,
 /*60   `    a    b    c    d    e    f    g */
      060, 021, 022, 023, 024, 025, 026, 027,
 /*68   h    i    j    k    l    m    n    o */
      030, 031, 041, 042, 043, 044, 045, 046,
 /*70   p    q    r    s    t    u    v    w */
      047, 050, 051, 062, 063, 064, 065, 066,
 /*78   x    y    z    {    |    }    ~   DL */
      067, 070, 071, 060, 060, 060, 060, 060,
};


static unsigned char toaltbcd[128] =
{
 /*00  NL   SH   SX   EX   ET   NQ   AK   BL */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*08  BS   HT   LF   VT   FF   CR   SO   SI */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*10  DL   D1   D2   D3   D4   NK   SN   EB */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*18  CN   EM   SB   EC   FS   GS   RS   US */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*20  SP    !    "    #    $    %    &    ' */
      020, 052, 037, 020, 053, 020, 020, 014,
 /*28   (    )    *    +    ,    -    .    / */
      034, 074, 054, 060, 033, 040, 073, 021,
 /*30   0    1    2    3    4    5    6    7 */
      012, 001, 002, 003, 004, 005, 006, 007,
 /*38   8    9    :    ;    <    =    >    ? */
      010, 011, 020, 056, 076, 013, 016, 072,
 /*40   @    A    B    C    D    E    F    G */
      020, 061, 062, 063, 064, 065, 066, 067,
 /*48   H    I    J    K    L    M    N    O */
      070, 071, 041, 042, 043, 044, 045, 046,
 /*50   P    Q    R    S    T    U    V    W */
      047, 050, 051, 022, 023, 024, 025, 026,
 /*58   X    Y    Z    [    \    ]    ^    _ */
      027, 030, 031, 075, 036, 055, 020, 057,
 /*60   `    a    b    c    d    e    f    g */
      020, 061, 062, 063, 064, 065, 066, 067,
 /*68   h    i    j    k    l    m    n    o */
      070, 071, 041, 042, 043, 044, 045, 046,
 /*70   p    q    r    s    t    u    v    w */
      047, 050, 051, 022, 023, 024, 025, 026,
 /*78   x    y    z    {    |    }    ~   DL */
      027, 030, 031, 017, 032, 077, 035, 020,
};



#endif /* ASCII */

#if '\n' == 0x15 && ' ' == 0x40 && '0' == 0xf0 \
  && 'A' == 0xC1 && 'a' == 0x81 && '!' == 0x5A

/*
** EBCDIC to BCD conversion table. 
*/

static unsigned char tobcd[256] =
{
 /*00  NU   SH   SX   EX   PF   HT   LC   DL */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*08            SM   VT   FF   CR   SO   SI */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*10  DE   D1   D2   TM   RS   NL   BS   IL */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*18  CN   EM   CC   C1   FS   GS   RS   US */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*20  DS   SS   FS        BP   LF   EB   EC */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*28            SM   C2   EQ   AK   BL      */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*30            SY        PN   RS   UC   ET */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*38                 C3   D4   NK        SU */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*40  SP                                    */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*48          CENT    .    <    (    +    | */
      060, 060, 060, 033, 060, 074, 020, 060,
 /*50   &                                    */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*58             !    $    *    )    ;    ^ */
      060, 060, 052, 053, 054, 034, 060, 060,
 /*60   -    /                               */
      040, 061, 060, 060, 060, 060, 060, 060,
 /*68             |    ,    %    _    >    ? */
      060, 060, 060, 073, 060, 060, 060, 032,
 /*70                                        */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*78        `    :    #    @    '    =    " */
      060, 060, 060, 060, 060, 014, 013, 060,
 /*80        a    b    c    d    e    f    g */
      060, 021, 022, 023, 024, 025, 026, 027,
 /*88   h    i         {                     */
      030, 031, 060, 060, 060, 060, 060, 060,
 /*90        j    k    l    m    n    o    p */
      060, 041, 042, 043, 044, 045, 046, 047,
 /*98   q    r         }                     */
      050, 051, 060, 060, 060, 060, 060, 060,
 /*A0        ~    s    t    u    v    w    x */
      060, 060, 062, 063, 064, 065, 066, 067,
 /*A8   y    z                   [           */
      070, 071, 060, 060, 060, 060, 060, 060,
 /*B0                                        */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*B8                            ]           */
      060, 060, 060, 060, 060, 060, 060, 060,
 /*C0   {    A    B    C    D    E    F    G */
      060, 021, 022, 023, 024, 025, 026, 027,
 /*C8   H    I                               */
      030, 031, 060, 060, 060, 060, 060, 060,
 /*D0   }    J    K    L    M    N    O    P */
      060, 041, 042, 043, 044, 045, 046, 047,
 /*D8   Q    R                               */
      050, 051, 060, 060, 060, 060, 060, 060,
 /*E0   \         S    T    U    V    W    X */
      060, 060, 062, 063, 064, 065, 066, 067,
 /*E8   Y    Z                               */
      070, 071, 060, 060, 060, 060, 060, 060,
 /*F0   0    1    2    3    4    5    6    7 */
      000, 001, 002, 003, 004, 005, 006, 007,
 /*F8   8    9                               */
      010, 011, 060, 060, 060, 060, 060, 060
};

static unsigned char toaltbcd[256] =
{
 /*00  NU   SH   SX   EX   PF   HT   LC   DL */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*08            SM   VT   FF   CR   SO   SI */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*10  DE   D1   D2   TM   RS   NL   BS   IL */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*18  CN   EM   CC   C1   FS   GS   RS   US */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*20  DS   SS   FS        BP   LF   EB   EC */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*28            SM   C2   EQ   AK   BL      */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*30            SY        PN   RS   UC   ET */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*38                 C3   D4   NK        SU */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*40  SP                                    */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*48          CENT    .    <    (    +    | */
      020, 020, 020, 073, 076, 034, 060, 033,
 /*50   &                                    */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*58             !    $    *    )    ;    ^ */
      020, 020, 052, 053, 054, 074, 056, 020,
 /*60   -    /                               */
      040, 021, 020, 020, 020, 020, 020, 020,
 /*68             |    ,    %    _    >    ? */
      020, 020, 032, 033, 020, 057, 016, 072,
 /*70                                        */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*78        `    :    #    @    '    =    " */
      020, 020, 020, 020, 020, 014, 013, 037,
 /*80        a    b    c    d    e    f    g */
      020, 061, 062, 063, 064, 065, 066, 067,
 /*88   h    i         {                     */
      070, 071, 020, 017, 020, 020, 020, 020,
 /*90        j    k    l    m    n    o    p */
      020, 041, 042, 043, 044, 045, 046, 047,
 /*98   q    r         }                     */
      050, 051, 020, 077, 020, 020, 020, 020,
 /*A0        ~    s    t    u    v    w    x */
      020, 035, 022, 023, 024, 025, 026, 027,
 /*A8   y    z                   [           */
      030, 031, 020, 020, 020, 075, 020, 020,
 /*B0                                        */
      020, 020, 020, 020, 020, 020, 020, 020,
 /*B8                            ]           */
      020, 020, 020, 020, 020, 055, 020, 020,
 /*C0   {    A    B    C    D    E    F    G */
      017, 061, 062, 063, 064, 065, 066, 067,
 /*C8   H    I                               */
      070, 071, 020, 020, 020, 020, 020, 020,
 /*D0   }    J    K    L    M    N    O    P */
      077, 041, 042, 043, 044, 045, 046, 047,
 /*D8   Q    R                               */
      050, 051, 020, 020, 020, 020, 020, 020,
 /*E0   \         S    T    U    V    W    X */
      036, 020, 022, 023, 024, 025, 026, 027,
 /*E8   Y    Z                               */
      030, 031, 020, 020, 020, 020, 020, 020,
 /*F0   0    1    2    3    4    5    6    7 */
      012, 001, 002, 003, 004, 005, 006, 007,
 /*F8   8    9                               */
      010, 011, 020, 020, 020, 020, 020, 020
};

#endif /* EBCDIC */

/***********************************************************************
* tapewriteint - Write an integer.
***********************************************************************/

static void
tapewriteint (FILE *fd, int v)
{
   fputc (v & 0xFF, fd);
   fputc ((v >> 8) & 0xFF, fd);
   fputc ((v >> 16) & 0xFF, fd);
   fputc ((v >> 24) & 0xFF, fd);
}

/***********************************************************************
* mapchar - Map char to BCD and even parity.
***********************************************************************/

static int
mapchar (int ch)
{
   return evenpar[altchars ? toaltbcd[ch] : tobcd[ch]];
}

/***********************************************************************
* padrec - Pad out record to length.
***********************************************************************/

static void
padrec (char pad)
{
   char ch;

   while (chrcnt < reclen)
   {
      ch = mapchar (pad);

      if (!simhfmt && chrblk == 0)
	 ch |= 0200;
      datbuf[chrblk++] = ch;
      chrcnt++;
   }
   chrcnt = 0;
}

/***********************************************************************
* padblk - Pad out block to length.
***********************************************************************/

static void
padblk (FILE *fo, char pad, int truncate)
{
   char ch;

   if (truncate)
   {
      if (simhfmt) tapewriteint (fo, chrblk);
      fwrite (datbuf, 1, chrblk, fo);
      if (simhfmt) tapewriteint (fo, chrblk);
      chrblk = 0;
      return;
   }

   while (chrblk < blklen)
   {
      ch = mapchar (pad);

      if (!simhfmt && chrblk == 0)
	 ch |= 0200;
      datbuf[chrblk++] = ch;
   }

   chrblk = 0;
   if (simhfmt) tapewriteint (fo, blklen);
   fwrite (datbuf, 1, blklen, fo);
   if (simhfmt) tapewriteint (fo, blklen);
}

/***********************************************************************
* main - Main procedure.
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *fi, *fo;
   char *optarg;
   int optind;
   int c;

   altchars = FALSE;
   simhfmt = FALSE;

   for (optind = 1, optarg = argv[optind];
       (optind < argc) && (*optarg == '-');
       optind++, optarg = argv[optind])
   {
      ++optarg;
      while (*optarg)
      {
         switch (*optarg++)
	 {

         case 's':
	    simhfmt = TRUE;
	    /* Fall through */

         case 'a':
            altchars = TRUE;
            break;

         default:
            fprintf (stderr,
	     "Usage: txt2bcd [-options] infile [outfile] [reclen [blklen]]]\n");
            fprintf (stderr, "  -a     Use Alternate character set\n");
            fprintf (stderr, "  -s     Use simh format\n");
            exit (1);
         }
      }
   }

   reclen = 80;
   blklen = 84;

   parsefiles (argc - (optind-1), &argv[optind-1], "txt", "bcd",
	       &reclen, &blklen);

   if ((fi = fopen (fin, "r")) == NULL)
   {
      perror (fin);
      exit (1);
   }
   if ((fo = fopen (fon, "wb")) == NULL)
   {
      perror (fon);
      exit (1);
   }

   chrcnt = 0;
   chrblk = 0;

   while ((c = fgetc (fi)) != EOF)
   {
      if (c == '\r') ;
      else if (c == '\n')
      {
	 padrec (' ');
	 if (chrblk + reclen > blklen)
	    padblk (fo, 0, 0);
      }
      else if (c == '\f' || c == '~')
      {
	 if (chrblk) padblk (fo, 0, 1);
	 if (!simhfmt) fputc (0217, fo);
	 else          tapewriteint (fo, 0);
	 while ((c = fgetc (fi)) != EOF)
	    if (c == '\n') break;
      }
      else if (c == '\t')
      {
	 c = mapchar (' ');
         if (!simhfmt && chrblk == 0)
	    c |= 0200;
	 datbuf[chrblk++] = c;
         chrcnt++;

	 c &= 0177;
         while (chrcnt % 8 != 0)
	 {
	    datbuf[chrblk++] = c;
            chrcnt++;
         }
      }
      else
      {
	 c = mapchar (c);
         if (!simhfmt && chrblk == 0)
	    c |= 0200;
	 datbuf[chrblk++] = c;
         chrcnt++;
      }
   }

   if (chrcnt > 0)
   {
      padrec (' ');
   }
   if (chrblk > 0)
   {
      padblk (fo, 0, 1);
   }

   if (simhfmt)
   {
      tapewriteint (fo, 0);
      tapewriteint (fo, 0);
   }
   else
   {
      fputc (0217, fo);
      fputc (0217, fo);
   }
   return (0);
}
