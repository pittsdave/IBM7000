Version: 0.2.2
Date: 05/22/2013


I. Introduction

asm7080 is a cross assembler for the IBM 7080 computer. It is Autocoder III
compatible, and as such requires the same coding standard. Macros are also
supported, although undefined. The object format is as defined in the IBM 
Autocoder manual.


II. Build asm7080

Lunix/Unix:

$ make

The make attempts to figure out which system to make for using uname.

WinBlows:

$ nmake nt


III. To run asm7080

$ asm7080 [-options] -o file.obj file.asm

Where options are:

   -b           - Boot loader format
   -c CPU       - CPU model (705, 705-III, 7080)
   -d deck      - Deckname for output
   -l listfile  - Generate listing to listfile
   -n           - No loader records or trailers
   -o outfile   - Object file name
   -s           - Standard listing format
   -t title     - Title for listing
   -w           - Generate wide listing
   -x           - Generate cross reference


IV. Supported Pseudo Ops

   ACON4  - Address CONstant 4.
   ACON5  - Address CONstant 5.
   ACON6  - Address CONstant 6.
   ADCON  - ADdress CONstant.
   BITCD  - BIT CoDe.
   CHRCD  - CHaRacter CoDe.
   CON    - CONstant definition.
   EJECT  - EJECT a page.
   END    - END of assembly
   FPN    - Floating Point Number.
   LASN   - Location ASsigNment.
   LITOR  - LITeral ORigin.
   RCD    - ReCorD definition.
   RPT    - RePorT definition.
   SASN   - Special location ASsigNment.
   TCD    - Transfer Control Directive.
   TITLE  - TITLE block, first line is copied to the assembly header.


V. Symbol Table Flags

The symbol table is listed with tags after the value field that specify the
symbols usage. For symbols that have an associated length it follows the tag
character. The tags are:

   blank - Absolute symbol
   B     - BITCD symbol
   C     - Constant symbol
   F     - Floating point symbol
   H     - CHRCD symbol
   I     - INCL symbol
   N     - NAME definition symbol
   R     - Record definition symbol
   T     - Report definition symbol


VI. References

C28-6224-1 IBM 705/7080 Autocoder III Language
A22-6560-4 IBM 7080 Principles of Operation


VII. Known problems/issues


