/***********************************************************************
*
* prsf2.h - Parse files.
*
* Changes:
*   ??/??/??   PRP   Original.
*   05/30/06   DGP   Added second integer return for block length.
*	
***********************************************************************/

/*
 * Parse arguments for two files,
 * one input one output,
 * specified explicitly or
 * as one base name with different extensions
 */

void parsefiles (int, char **, char *, char *, int *, int *);

