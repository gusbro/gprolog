/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : foreign facility test                                           *
 * File  : for_main_c.c                                                    *
 * Descr.: test file - C part                                              *
 * Author: Daniel Diaz                                                     *
 *                                                                         *
 * Copyright (C) 1999,2000 Daniel Diaz                                     *
 *                                                                         *
 * GNU Prolog is free software; you can redistribute it and/or modify it   *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2, or any later version.       *
 *                                                                         *
 * GNU Prolog is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc.  *
 * 59 Temple Place - Suite 330, Boston, MA 02111, USA.                     *
 *-------------------------------------------------------------------------*/

#include <string.h>

#include "gprolog.h"


/*---------------------------------*
 * Constants                       *
 *---------------------------------*/

/*---------------------------------*
 * Type Definitions                *
 *---------------------------------*/

/*---------------------------------*
 * Global Variables                *
 *---------------------------------*/

/*---------------------------------*
 * Function Prototypes             *
 *---------------------------------*/




/*-------------------------------------------------------------------------*
 * MAIN                                                                    *
 *                                                                         *
 *-------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
  int func;
  WamWord arg[10];
  char str[100];
  char *sol[100];
  int i, nb_sol = 0;
  Bool res;

  Start_Prolog(argc, argv);

  func = Find_Atom("anc");
  for (;;)
    {
      printf("\nEnter a name (or 'end' to finish): ");
      scanf("%s", str);

      if (strcmp(str, "end") == 0)
	break;

      Pl_Query_Begin(TRUE);

      arg[0] = Mk_Variable();
      arg[1] = Mk_String(str);
      nb_sol = 0;
      res = Pl_Query_Call(func, 2, arg);
      while (res)
	{
	  sol[nb_sol++] = Rd_String(arg[0]);
	  res = Pl_Query_Next_Solution();
	}
      Pl_Query_End(PL_RECOVER);

      for (i = 0; i < nb_sol; i++)
	printf("  solution: %s\n", sol[i]);
      printf("%d solution(s)\n", nb_sol);
    }

  Stop_Prolog();
  return 0;
}
