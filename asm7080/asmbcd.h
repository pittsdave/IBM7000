/***********************************************************************
*
* asmbcd.h - Convert characters to BCD.
*
* Changes:
*   02/08/07   DGP   Original.
*   05/22/13   DGP   Changed to match 705 character set.
*	
***********************************************************************/

/*
 * BCD to native table.
 */

unsigned char tonative[64] = {

/* 00 */ 'a', '1', '2', '3', '4', '5', '6', '7',
/* 10 */ '8', '9', '0', '=','\'', ':', '>', 's',
/* 20 */ ' ', '/', 'S', 'T', 'U', 'V', 'W', 'X',
/* 30 */ 'Y', 'Z', '#', ',', '(', '`','\\', '_',
/* 40 */ '-', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
/* 50 */ 'Q', 'R', '!', '$', '*', ']', ';', '^',
/* 60 */ '+', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
/* 70 */ 'H', 'I', '?', '.', ')', '[', '<', '|'
};

#if '\n' == 0x0A && ' ' == 0x20 && '0' == 0x30 \
  && 'A' == 0x41 && 'a' == 0x61 && '!' == 0x21

/*
** ASCII to BCD conversion table. 
*/

static unsigned char tobcd[128] =
{
 /*00  NL   SH   SX   EX   ET   NQ   AK   BL */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*08  BS   HT   LF   VT   FF   CR   SO   SI */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*10  DL   D1   D2   D3   D4   NK   SN   EB */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*18  CN   EM   SB   EC   FS   GS   RS   US */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*20  SP    !    "    #    $    %    &    ' */
      020, 052, 255, 032, 053, 034, 255, 014,
 /*28   (    )    *    +    ,    -    .    / */
      034, 074, 054, 060, 033, 040, 073, 021,
 /*30   0    1    2    3    4    5    6    7 */
      012, 001, 002, 003, 004, 005, 006, 007,
 /*38   8    9    :    ;    <    =    >    ? */
      010, 011, 015, 056, 076, 013, 016, 072,
 /*40   @    A    B    C    D    E    F    G */
      014, 061, 062, 063, 064, 065, 066, 067,
 /*48   H    I    J    K    L    M    N    O */
      070, 071, 041, 042, 043, 044, 045, 046,
 /*50   P    Q    R    S    T    U    V    W */
      047, 050, 051, 022, 023, 024, 025, 026,
 /*58   X    Y    Z    [    \    ]    ^    _ */
      027, 030, 031, 075, 036, 055, 057, 037,
 /*60   `    a    b    c    d    e    f    g */
      035, 000, 255, 255, 255, 255, 255, 255,
 /*68   h    i    j    k    l    m    n    o */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*70   p    q    r    s    t    u    v    w */
      255, 255, 255, 017, 255, 255, 255, 255,
 /*78   x    y    z    {    |    }    ~   DL */
      255, 255, 255, 255, 077, 255, 255, 255
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
      255, 255, 255, 255, 255, 255, 255, 255,
 /*08            SM   VT   FF   CR   SO   SI */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*10  DE   D1   D2   TM   RS   NL   BS   IL */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*18  CN   EM   CC   C1   FS   GS   RS   US */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*20  DS   SS   FS        BP   LF   EB   EC */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*28            SM   C2   EQ   AK   BL      */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*30            SY        PN   RS   UC   ET */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*38                 C3   D4   NK        SU */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*40  SP                                    */
      020, 255, 255, 255, 255, 255, 255, 255,
 /*48          CENT    .    <    (    +    | */
      255, 255, 255, 073, 076, 034, 060, 077,
 /*50   &                                    */
      060, 255, 255, 255, 255, 255, 255, 255,
 /*58             !    $    *    )    ;    ^ */
      255, 255, 052, 053, 054, 074, 056, 057,
 /*60   -    /                               */
      040, 021, 255, 255, 255, 255, 255, 255,
 /*68             |    ,    %    _    >    ? */
      255, 255, 255, 033, 034, 037, 016, 072,
 /*70                                        */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*78        `    :    #    @    '    =    " */
      255, 035, 015, 032, 014, 014, 013, 255,
 /*80        a    b    c    d    e    f    g */
      255, 000, 255, 255, 255, 255, 255, 255,
 /*88   h    i         {                     */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*90        j    k    l    m    n    o    p */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*98   q    r         }                     */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*A0        ~    s    t    u    v    w    x */
      255, 255, 017, 255, 255, 255, 255, 255,
 /*A8   y    z                   [           */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*B0                                        */
      255, 255, 255, 255, 255, 255, 255, 255,
 /*B8                            ]           */
      255, 255, 255, 255, 255, 055, 255, 255,
 /*C0   {    A    B    C    D    E    F    G */
      255, 061, 062, 063, 064, 065, 066, 067,
 /*C8   H    I                               */
      070, 071, 255, 255, 255, 255, 255, 255,
 /*D0   }    J    K    L    M    N    O    P */
      255, 041, 042, 043, 044, 045, 046, 047,
 /*D8   Q    R                               */
      050, 051, 255, 255, 255, 255, 255, 255,
 /*E0   \         S    T    U    V    W    X */
      036, 255, 022, 023, 024, 025, 026, 027,
 /*E8   Y    Z                               */
      030, 031, 255, 255, 255, 255, 255, 255,
 /*F0   0    1    2    3    4    5    6    7 */
      012, 001, 002, 003, 004, 005, 006, 007,
 /*F8   8    9                               */
      010, 011, 255, 255, 255, 255, 255, 255
};

#endif /* EBCDIC */
