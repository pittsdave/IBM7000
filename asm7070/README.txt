Version: 0.2.1
Date: 06/26/2013


I. Introduction

asm7070 is a cross assembler for the IBM 7070 computer. It is Autocoder 
compatible, and as such requires the same coding standard. Macros are also
supported, although undefined. The object format is undefined in the IBM 
Autocoder manual, so I use a tagged object format.


II. Build asm7070

Lunix/Unix:

$ make

The make attempts to figure out which system to make for using uname.

WinBlows:

$ nmake nt


III. To run asm7070

$ asm7070 [-options] -o file.obj file.asm

Where options are:

   -c CPU       - CPU model (7070, 7074)
   -d deck      - Deckname for output
   -l listfile  - Generate listing to listfile
   -o outfile   - Object file name
   -s           - Standard listing format
   -t title     - Title for listing
   -w           - Generate wide listing
   -x           - Generate cross reference


IV. Pseudo Ops

   CNTRL  - CoNTRoL directive.
   CODE   - CODE directive (not implemented).
   DA     - Define Area.
   DC     - Define Constant.
   DIOCS  - Define Input/Output Control System (not implemented).
   DLINE  - Define LINE (not implemented).
   DRDW   - Define Record Defintion Word.
   DSW    - Define SWitch.
   DTF    - Define Tape File (not implemented).
   DUF    - Define Unit record File (not implemented).
   EQU    - EQUate.


V. Symbol Table Flags

The symbol table is listed with tags after the value field that specify the
symbols usage. For symbols that have a position or associated length it
follows the tag character. The tags are:

   blank - Absolute symbol
   AF    - Disk arm and unit
   C     - Tape Channel
   CU    - Tape Channel and Unit
   DA    - Area definition symbol
   DC    - Constant definition symbol
   DF    - Area field definition
   DS    - DSW definition.
   ES    - DSW switch.
   I     - Unit record latch
   P     - Unit record punch
   Q     - Inquiry synchronizer
   R     - Unit record reader
   RW    - DRW definition.
   S     - Electronic switch
   SN    - Alteration switch
   T     - Typewriter
   U     - Tape Unit
   W     - Unit record printer
   X     - Index word


VI. References

C28-6121-0 IBM 7070 Autocoder
GA22-7003-6 IBM 7070-7074 Principles of Operation


VII. Known problems/issues

Without software to debug the assembler; many problems may exist.
