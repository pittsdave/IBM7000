/***********************************************************************
*
* scanner.h - Scanner actions.
*
***********************************************************************/

   case ADDREXPRSTART: /* Address operator found */
      switch ( lachar )
      {

	 case '[':
	 case '{':
	    lachar = '(';
	 case '(':
	    token = lachar;
	    break;

	 case ']':
	 case '}':
	    lachar = ')';
	 case ')':
	    token = lachar;
	    break;

	default:
	    if (latran == 1)
	       token = EOS;
	    else
	       token = lachar;
      }
      break;

   case 2: /* Address symbol found */
      {
	 int iii;

	 value = 0;
	 token = SYM;
	 for (iii = 0; symbol[iii] != '\0'; iii++ ) 
	    if (islower(symbol[iii]))
	       symbol[iii] = toupper(symbol[iii]);
      }
      break;

   case 3: /* Index reg symbol found */
      {
	 int iii;

	 value = 0;
	 token = XREG;
	 for (iii = 0; symbol[iii] != '\0'; iii++ ) 
	    if (islower(symbol[iii]))
	       symbol[iii] = toupper(symbol[iii]);
      }
      break;

   case 4: /* Address number found */
      if (radix == 10)
      {
	 value = value * 10 + dignum;
	 token = DECNUM;
      }
      else if (radix == 8)
      {
	 token = OCTNUM;
	 if (dignum > 7)
	 {
	    Parse_Error (SCAN_ERROR, INVOCTNUM, defer);
	    *svalue = 0;
	    strcpy (toksym, "0");
	    return (0);
	 }
	 else
	    value = (value << 3) | dignum;
      }
      break;

   case 5: /* PC symbol found */
   case 7: /* recognize nulsym "**" */
      value = pc;
      token = ASTER;
      break;

   case 6: /* End of expression */
      token = EOS;
      break;

