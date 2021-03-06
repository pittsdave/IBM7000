/***********************************************************************
*
* nativebcd.h - IBM 7090 emulator BCD to native conversion tables.
*
* Changes:
*   ??/??/??   PP    Original. Version 1.0.0
*   01/28/05   DGP   Changed to make IBSYS codes primary and other is alt.
*                    Also, changed alt codes for source tape correctness.
*   02/15/05   DGP   Changed ASCII to native, since EBCDIC works too.
*   06/25/10   DGP   Added CTSS characters.
*	
***********************************************************************/

/*
 * BCD to native table
 */

char tonative[64] = {
/* 00 */ '0', '1', '2', '3', '4', '5', '6', '7',
/* 10 */ '8', '9', ' ', '=', '\'', ' ', ' ', ' ',
/* 20 */ '+', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
/* 30 */ 'H', 'I', '?', '.', ')', ':', ' ', ' ',
/* 40 */ '-', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
/* 50 */ 'Q', 'R', '!', '$', '*', ' ', ' ', ' ',
/* 60 */ ' ', '/', 'S', 'T', 'U', 'V', 'W', 'X',
/* 70 */ 'Y', 'Z', ' ', ',', '(', ' ', ' ', ' '
};

char toaltnative[64] = {
/* 00 */ ' ', '1', '2', '3', '4', '5', '6', '7',
/* 10 */ '8', '9', '0', '=', '\'', ':', '>', '{',
/* 20 */ ' ', '/', 'S', 'T', 'U', 'V', 'W', 'X',
/* 30 */ 'Y', 'Z', '|', ',', '(', '~', '\\', '"',
/* 40 */ '-', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
/* 50 */ 'Q', 'R', '!', '$', '*', ']', ';', '_',
/* 60 */ '+', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
/* 70 */ 'H', 'I', '?', '.', ')', '[', '<', '}'
};
