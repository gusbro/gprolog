/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : FD constraint solver buit-in predicates                         *
 * File  : fd_bool_c.c                                                     *
 * Descr.: boolean and Meta-constraint predicate management - C part       *
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

/* $Id$ */

#define OBJ_INIT Fd_Bool_Initializer

#include "engine_pl.h"
#include "bips_pl.h"

#include "engine_fd.h"
#include "bips_fd.h"


/*---------------------------------*
 * Constants                       *
 *---------------------------------*/

#define BOOL_STACK_SIZE            1024
#define VARS_STACK_SIZE            1024


#define NOT                        0

#define EQUIV                      1
#define NEQUIV                     2
#define IMPLY                      3
#define NIMPLY                     4
#define AND                        5
#define NAND                       6
#define OR                         7
#define NOR                        8

#define EQ                         9	/* warning EQ must have same */
#define NEQ                        10	/* parity than EQ_F and ZERO */
#define LT                         11
#define GTE                        12
#define GT                         13
#define LTE                        14

#define EQ_F                       15
#define NEQ_F                      16
#define LT_F                       17
#define GTE_F                      18
#define GT_F                       19
#define LTE_F                      20

#define ZERO                       21
#define ONE                        22	/* must be last */

#define IsVar(op)  ((op)>=ONE)

#define NB_OF_OP                   ZERO




/*---------------------------------*
 * Type Definitions                *
 *---------------------------------*/

/*---------------------------------*
 * Global Variables                *
 *---------------------------------*/

static WamWord bool_tbl[NB_OF_OP];
static WamWord bool_xor;

static WamWord stack[BOOL_STACK_SIZE];
static WamWord *sp;

static WamWord vars_tbl[VARS_STACK_SIZE];
static WamWord *vars_sp;

static Bool (*func_tbl[NB_OF_OP + 2]) ();




/*---------------------------------*
 * Function Prototypes             *
 *---------------------------------*/

static WamWord *Simplify(int sign, WamWord e_word);

static void Add_Fd_Variables(WamWord e_word);

static Bool Load_Bool_Into_Word(WamWord *exp, int result,
				WamWord *load_word);



static Bool Set_Var(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Not(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Equiv(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Nequiv(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Imply(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Nimply(WamWord *exp, int result, WamWord *load_word);

static Bool Set_And(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Nand(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Or(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Nor(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Eq(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Neq(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Lt(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Lte(WamWord *exp, int result, WamWord *load_word);

static Bool Set_Zero(WamWord *exp, int result, WamWord *load_word);

static Bool Set_One(WamWord *exp, int result, WamWord *load_word);



#ifdef DEBUG

static void Display_Stack(WamWord *exp);

void Write_1(WamWord term_word);

#endif



	  /* defined in fd_math_c.c */

Bool Fd_Eq_2(WamWord le_word, WamWord re_word);

Bool Fd_Neq_2(WamWord le_word, WamWord re_word);

Bool Fd_Lt_2(WamWord le_word, WamWord re_word);

Bool Fd_Lte_2(WamWord le_word, WamWord re_word);



#define BOOL_CSTR_2(f,a1,a2)                                                \
    do {                                                                    \
     if (!Fd_Check_For_Bool_Var(a1)) return FALSE;                          \
     if (!Fd_Check_For_Bool_Var(a2)) return FALSE;                          \
     PRIM_CSTR_2(f,a1,a2);                                                  \
    } while(0)

#define BOOL_CSTR_3(f,a1,a2,a3)                                             \
    do {                                                                    \
     if (!Fd_Check_For_Bool_Var(a1)) return FALSE;                          \
     if (!Fd_Check_For_Bool_Var(a2)) return FALSE;                          \
                                                             /* a3 is OK */ \
     PRIM_CSTR_3(f,a1,a2,a3);                                               \
    } while(0)




/*-------------------------------------------------------------------------*
 * FD_BOOL_INITIALIZER                                                     *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Fd_Bool_Initializer(void)
{
  bool_tbl[NOT] = Functor_Arity(Create_Atom("#\\"), 1);

  bool_tbl[EQUIV] = Functor_Arity(Create_Atom("#<=>"), 2);
  bool_tbl[NEQUIV] = Functor_Arity(Create_Atom("#\\<=>"), 2);
  bool_tbl[IMPLY] = Functor_Arity(Create_Atom("#==>"), 2);
  bool_tbl[NIMPLY] = Functor_Arity(Create_Atom("#\\==>"), 2);
  bool_tbl[AND] = Functor_Arity(Create_Atom("#/\\"), 2);
  bool_tbl[NAND] = Functor_Arity(Create_Atom("#\\/\\"), 2);
  bool_tbl[OR] = Functor_Arity(Create_Atom("#\\/"), 2);
  bool_tbl[NOR] = Functor_Arity(Create_Atom("#\\\\/"), 2);

  bool_tbl[EQ] = Functor_Arity(Create_Atom("#="), 2);
  bool_tbl[NEQ] = Functor_Arity(Create_Atom("#\\="), 2);
  bool_tbl[LT] = Functor_Arity(Create_Atom("#<"), 2);
  bool_tbl[GTE] = Functor_Arity(Create_Atom("#>="), 2);
  bool_tbl[GT] = Functor_Arity(Create_Atom("#>"), 2);
  bool_tbl[LTE] = Functor_Arity(Create_Atom("#=<"), 2);

  bool_tbl[EQ_F] = Functor_Arity(Create_Atom("#=#"), 2);
  bool_tbl[NEQ_F] = Functor_Arity(Create_Atom("#\\=#"), 2);
  bool_tbl[LT_F] = Functor_Arity(Create_Atom("#<#"), 2);
  bool_tbl[GTE_F] = Functor_Arity(Create_Atom("#>=#"), 2);
  bool_tbl[GT_F] = Functor_Arity(Create_Atom("#>#"), 2);
  bool_tbl[LTE_F] = Functor_Arity(Create_Atom("#=<#"), 2);

  bool_xor = Functor_Arity(Create_Atom("##"), 2);


  func_tbl[NOT] = Set_Not;

  func_tbl[EQUIV] = Set_Equiv;
  func_tbl[NEQUIV] = Set_Nequiv;
  func_tbl[IMPLY] = Set_Imply;
  func_tbl[NIMPLY] = Set_Nimply;
  func_tbl[AND] = Set_And;
  func_tbl[NAND] = Set_Nand;
  func_tbl[OR] = Set_Or;
  func_tbl[NOR] = Set_Nor;

  func_tbl[EQ] = Set_Eq;
  func_tbl[NEQ] = Set_Neq;
  func_tbl[LT] = Set_Lt;
  func_tbl[GTE] = NULL;
  func_tbl[GT] = NULL;
  func_tbl[LTE] = Set_Lte;

  func_tbl[EQ_F] = NULL;
  func_tbl[NEQ_F] = NULL;
  func_tbl[LT_F] = NULL;
  func_tbl[GTE_F] = NULL;
  func_tbl[GT_F] = NULL;
  func_tbl[LTE_F] = NULL;

  func_tbl[ZERO] = Set_Zero;
  func_tbl[ONE] = Set_One;
}




/*-------------------------------------------------------------------------*
 * FD_BOOL_META_3                                                          *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Fd_Bool_Meta_3(WamWord le_word, WamWord re_word, WamWord op_word)
{
  WamWord word, tag, *adr;
  WamWord *fdv_adr;
  WamWord *exp;
  int op;


  Deref(op_word, word, tag, adr);
  op = UnTag_INT(op_word);

  H[0] = bool_tbl[op];		/* also works for NOT/1 */
  H[1] = le_word;
  H[2] = re_word;

  sp = stack;
  vars_sp = vars_tbl;

  exp = Simplify(1, Tag_Value(STC, H));

#ifdef DEBUG
  Display_Stack(exp);
  DBGPRINTF("\n");
#endif

  if (!Load_Bool_Into_Word(exp, 1, NULL))
    return FALSE;

  while (--vars_sp >= vars_tbl)
    if (*vars_sp-- == 0)	/* bool var */
      {
	if (!Fd_Check_For_Bool_Var(*vars_sp))
	  return FALSE;
      }
    else			/* FD var */
      {
	Deref(*vars_sp, word, tag, adr);
	if (tag == REF)
	  {
	    fdv_adr = Fd_New_Variable();
	    Bind_UV(adr, Tag_Value(REF, fdv_adr));
	  }
      }


  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SIMPLIFY                                                                *
 *                                                                         *
 * This function returns the result of the simplified boolean expression   *
 * given in e_word. NOT operators are only applied to variables.           *
 *                                                                         *
 * Input:                                                                  *
 *    sign  : current sign of the boolean term (-1 (inside a ~) or +1)     *
 *    e_word: boolean term to simplify                                     *
 *                                                                         *
 * Output:                                                                 *
 *    The returned result is a pointer to a node of the following form:    *
 *                                                                         *
 *    for binary boolean not operator (~):                                 *
 *        [1]: variable involved (tagged word)                             *
 *        [0]: operator NOT                                                *
 *                                                                         *
 *    for unary boolean operators (<=> ~<=> ==> ~==> /\ ~/\ \/ ~\/):       *
 *        [2]: right boolean exp (pointer to node)                         *
 *        [1]: left  boolean exp (pointer to node)                         *
 *        [0]: operator (EQUIV, NEQUIV, IMPLY, NIMPLY, AND, NAND, OR, NOR) *
 *                                                                         *
 *    for boolean false value (0):                                         *
 *        [0]: ZERO                                                        *
 *                                                                         *
 *    for boolean true value (1):                                          *
 *        [0]: ONE                                                         *
 *                                                                         *
 *    for boolean variable:                                                *
 *        [0]: tagged word                                                 *
 *                                                                         *
 *    for binary math operators (= \= < >= > <=) (partial / full AC):      *
 *        [2]: right math exp (tagged word)                                *
 *        [1]: left  math exp (tagged word)                                *
 *        [0]: operator (EQ, NEQ, LT, LTE, EQ_F, NEQ_F, LT_F, LTE_F)       *
 *             (GT, GTE, GT_F, and GTE_F becomes LT, LTE, GT_F and GTE_F)  *
 *                                                                         *
 * These nodes are stored in a hybrid stack. NB: XOR same as NEQUIV        *
 *-------------------------------------------------------------------------*/
static WamWord *
Simplify(int sign, WamWord e_word)
{
  WamWord word, tag, *adr;
  WamWord f_n, le_word, re_word;
  int op, n;
  WamWord *exp, l, r;


  exp = sp;

  if (sp - stack > BOOL_STACK_SIZE - 5)
    Pl_Err_Resource(resource_too_big_fd_constraint);

  Deref(e_word, word, tag, adr);
  switch (tag)
    {
    case REF:
    case FDV:
      adr = UnTag_REF(word);
      if (vars_sp - vars_tbl == VARS_STACK_SIZE)
	Pl_Err_Resource(resource_too_big_fd_constraint);

      *vars_sp++ = word;
      *vars_sp++ = 0;		/* bool var */

      if (sign != 1)
	*sp++ = NOT;

      *sp++ = Tag_Value(REF, adr);
      return exp;

    case INT:
      n = UnTag_INT(word);
      if ((unsigned) n > 1)
	goto type_error;

      *sp++ = ZERO + ((sign == 1) ? n : 1 - n);
      return exp;

    case ATM:
      word = Put_Structure(ATOM_CHAR('/'), 2);
      Unify_Value(e_word);
      Unify_Integer(0);
      goto type_error;

    case STC:
      break;

    default:
    type_error:
      Pl_Err_Type(type_fd_bool_evaluable, word);
    }


  adr = UnTag_STC(word);

  f_n = Functor_And_Arity(adr);
  if (bool_xor == f_n)
    op = NEQUIV;
  else
    {
      for (op = 0; op < NB_OF_OP; op++)
	if (bool_tbl[op] == f_n)
	  break;

      if (op == NB_OF_OP)
	{
	  word = Put_Structure(ATOM_CHAR('/'), 2);
	  Unify_Atom(Functor(adr));
	  Unify_Integer(Arity(adr));
	  goto type_error;
	}
    }

  le_word = Arg(adr, 0);
  re_word = Arg(adr, 1);

  if (op == NOT)
    return Simplify(-sign, le_word);

  if (sign != 1)
    op = (op % 2 == EQ % 2) ? op + 1 : op - 1;

  if (op >= EQ && op <= LTE_F)
    {
      Add_Fd_Variables(le_word);
      Add_Fd_Variables(re_word);

      n = (op == GT || op == GT_F) ? op - 2 : (op == GTE
					       || op ==
					       GTE_F) ? op + 2 : op;

      *sp++ = n;
      *sp++ = (n == op) ? le_word : re_word;
      *sp++ = (n == op) ? re_word : le_word;
      return exp;
    }

  sp += 3;
  exp[0] = op;
  exp[1] = (WamWord) Simplify(1, le_word);
  exp[2] = (WamWord) Simplify(1, re_word);

  l = *(WamWord *) (exp[1]);
  r = *(WamWord *) (exp[2]);

  switch (op)
    {
    case EQUIV:
      if (l == ZERO)		/* 0 <=> R is ~R */
	return sp = exp, Simplify(-1, re_word);

      if (l == ONE)		/* 1 <=> R is R */
	return (WamWord *) exp[2];

      if (r == ZERO)		/* L <=> 0 is ~L */
	return sp = exp, Simplify(-1, le_word);

      if (r == ONE)		/* L <=> 1 is L */
	return (WamWord *) exp[1];

      if (l == NOT)		/* ~X <=> R is X <=> ~R */
	{
	  exp[1] += sizeof(WamWord);
	  exp[2] = (WamWord) Simplify(-1, re_word);
	  break;
	}

      if (r == NOT)		/* L <=> ~X is ~L <=> X */
	{
	  exp[1] = (WamWord) Simplify(-1, le_word);
	  exp[2] += sizeof(WamWord);
	  break;
	}
      break;

    case NEQUIV:
      if (l == ZERO)		/* 0 ~<=> R is R */
	return (WamWord *) exp[2];

      if (l == ONE)		/* 1 ~<=> R is ~R */
	return sp = exp, Simplify(-1, re_word);

      if (r == ZERO)		/* L ~<=> 0 is L */
	return (WamWord *) exp[1];

      if (r == ONE)		/* L ~<=> 1 is ~L */
	return sp = exp, Simplify(-1, le_word);

      if (l == NOT)		/* ~X ~<=> R is X <=> R */
	{
	  exp[0] = EQUIV;
	  exp[1] += sizeof(WamWord);
	  break;
	}

      if (r == NOT)		/* L ~<=> ~X is L <=> X */
	{
	  exp[0] = EQUIV;
	  exp[2] += sizeof(WamWord);
	  break;
	}

      if (IsVar(l) && !IsVar(r))	/* X ~<=> R is X <=> ~R */
	{
	  exp[0] = EQUIV;
	  exp[2] = (WamWord) Simplify(-1, re_word);
	  break;
	}

      if (IsVar(r) && !IsVar(l))	/* L ~<=> X is L <=> ~X */
	{
	  exp[0] = EQUIV;
	  exp[1] = (WamWord) Simplify(-1, le_word);
	  break;
	}
      break;

    case IMPLY:
      if (l == ZERO || r == ONE)	/* 0 ==> R is 1 , L ==> 1 is 1 */
	{
	  sp = exp;
	  *sp++ = ONE;
	  break;
	}

      if (l == ONE)		/* 1 ==> R is R */
	return (WamWord *) exp[2];

      if (r == ZERO)		/* L ==> 0 is ~L */
	return sp = exp, Simplify(-1, le_word);

      if (l == NOT)		/* ~X ==> R is X \/ R */
	{
	  exp[0] = OR;
	  exp[1] += sizeof(WamWord);
	  break;
	}

      if (r == NOT)		/* L ==> ~X is X ==> ~L */
	{
	  exp[1] = exp[2] + sizeof(WamWord);
	  exp[2] = (WamWord) Simplify(-1, le_word);
	  break;
	}
      break;

    case NIMPLY:
      if (l == ZERO || r == ONE)	/* 0 ~==> R is 0 , L ~==> 1 is 0 */
	{
	  sp = exp;
	  *sp++ = ZERO;
	  break;
	}

      if (l == ONE)		/* 1 ~==> R is ~R */
	return sp = exp, Simplify(-1, re_word);

      if (r == ZERO)		/* L ~==> 0 is L */
	return (WamWord *) exp[1];

      if (l == NOT)		/* ~X ~==> R is X ~\/ R */
	{
	  exp[0] = NOR;
	  exp[1] += sizeof(WamWord);
	  break;
	}

      if (r == NOT)		/* L ~==> ~X is L /\ X */
	{
	  exp[0] = AND;
	  exp[2] += sizeof(WamWord);
	  break;
	}
      break;

    case AND:
      if (l == ZERO || r == ZERO)	/* 0 /\ R is 0 , L /\ 0 is 0 */
	{
	  sp = exp;
	  *sp++ = ZERO;
	  break;
	}

      if (l == ONE)		/* 1 /\ R is R */
	return (WamWord *) exp[2];

      if (r == ONE)		/* L /\ 1 is L */
	return (WamWord *) exp[1];

      if (l == NOT)		/* ~X /\ R is R ~==> X */
	{
	  exp[0] = NIMPLY;
	  word = exp[1];
	  exp[1] = exp[2];
	  exp[2] = word + sizeof(WamWord);
	  break;
	}

      if (r == NOT)		/* L /\ ~X is L ~==> X */
	{
	  exp[0] = NIMPLY;
	  exp[2] += sizeof(WamWord);
	  break;
	}
      break;

    case NAND:
      if (l == ZERO || r == ZERO)	/* 0 ~/\ R is 1 , L ~/\ 0 is 1 */
	{
	  sp = exp;
	  *sp++ = ONE;
	  break;
	}

      if (l == ONE)		/* 1 ~/\ R is ~R */
	return sp = exp, Simplify(-1, re_word);

      if (r == ONE)		/* L ~/\ 1 is ~L */
	return sp = exp, Simplify(-1, le_word);

      if (l == NOT)		/* ~X ~/\ R is R ==> X */
	{
	  exp[0] = IMPLY;
	  word = exp[1];
	  exp[1] = exp[2];
	  exp[2] = word + sizeof(WamWord);
	  break;
	}

      if (r == NOT)		/* L ~/\ ~X is L ==> X */
	{
	  exp[0] = IMPLY;
	  exp[2] += sizeof(WamWord);
	  break;
	}
      break;

    case OR:
      if (l == ONE || r == ONE)	/* 1 \/ R is 1 , L \/ 1 is 1 */
	{
	  sp = exp;
	  *sp++ = ONE;
	  break;
	}

      if (l == ZERO)		/* 0 \/ R is R */
	return (WamWord *) exp[2];

      if (r == ZERO)		/* L \/ 0 is L */
	return (WamWord *) exp[1];

      if (l == NOT)		/* ~X \/ R is X ==> R */
	{
	  exp[0] = IMPLY;
	  exp[1] += sizeof(WamWord);
	  break;
	}

      if (r == NOT)		/* L \/ ~X is X ==> L */
	{
	  exp[0] = IMPLY;
	  word = exp[1];
	  exp[1] = exp[2] + sizeof(WamWord);
	  exp[2] = word;
	  break;
	}
      break;

    case NOR:
      if (l == ONE || r == ONE)	/* 1 ~\/ R is 0 , L ~\/ 1 is 0 */
	{
	  sp = exp;
	  *sp++ = ZERO;
	  break;
	}

      if (l == ZERO)		/* 0 ~\/ R is ~R */
	return sp = exp, Simplify(-1, re_word);

      if (r == ZERO)		/* L ~\/ 0 is ~L */
	return sp = exp, Simplify(-1, le_word);

      if (l == NOT)		/* ~X ~\/ R is X ~==> R */
	{
	  exp[0] = NIMPLY;
	  exp[1] += sizeof(WamWord);
	  break;
	}

      if (r == NOT)		/* L ~\/ ~X is X ~==> L */
	{
	  exp[0] = NIMPLY;
	  word = exp[1];
	  exp[1] = exp[2] + sizeof(WamWord);
	  exp[2] = word;
	  break;
	}
      break;
    }

  return exp;
}




#ifdef DEBUG
/*-------------------------------------------------------------------------*
 * DISPLAY_STACK                                                           *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Display_Stack(WamWord *exp)
{
  int op = exp[0];
  WamWord *le = (WamWord *) (exp[1]);
  WamWord *re = (WamWord *) (exp[2]);

  switch (op)
    {
    case NOT:
      DBGPRINTF("%s", atom_tbl[Functor_Of(bool_tbl[op])].name);
      DBGPRINTF(" ");
      Write_1(exp[1]);
      break;

    case EQUIV:
    case NEQUIV:
    case IMPLY:
    case NIMPLY:
    case AND:
    case NAND:
    case OR:
    case NOR:
      DBGPRINTF("(");
      Display_Stack(le);
      DBGPRINTF(" ");
      DBGPRINTF("%s", atom_tbl[Functor_Of(bool_tbl[op])].name);
      DBGPRINTF(" ");
      Display_Stack(re);
      DBGPRINTF(")");
      break;

    case EQ:
    case NEQ:
    case LT:
    case LTE:
    case GT:
    case GTE:

    case EQ_F:
    case NEQ_F:
    case LT_F:
    case LTE_F:
    case GT_F:
    case GTE_F:
      Write_1(exp[1]);
      DBGPRINTF(" ");
      DBGPRINTF("%s", atom_tbl[Functor_Of(bool_tbl[op])].name);
      DBGPRINTF(" ");
      Write_1(exp[2]);
      break;

    case ZERO:
      DBGPRINTF("0");
      break;

    case ONE:
      DBGPRINTF("1");
      break;

    default:
      Write_1(*exp);
    }
}
#endif




/*-------------------------------------------------------------------------*
 * ADD_FD_VARIABLES                                                        *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Add_Fd_Variables(WamWord e_word)
{
  WamWord word, tag, *adr;
  int i;

  Deref(e_word, word, tag, adr);

  switch (tag)
    {
    case REF:
      if (vars_sp - vars_tbl == VARS_STACK_SIZE)
	Pl_Err_Resource(resource_too_big_fd_constraint);

      *vars_sp++ = word;
      *vars_sp++ = 1;		/* FD var */
      break;

    case LST:
      Add_Fd_Variables(Car(adr));
      Add_Fd_Variables(Cdr(adr));
      break;

    case STC:			/* tag==STC */
      adr = UnTag_STC(word);

      i = Arity(adr);
      do
	Add_Fd_Variables(Arg(adr, --i));
      while (i);
    }
}




/*-------------------------------------------------------------------------*
 * LOAD_BOOL_INTO_WORD                                                     *
 *                                                                         *
 * This loads a boolean term into a tagged word.                           *
 * Input:                                                                  *
 *    exp      : boolean term to load                                      *
 *    result   : which result is expected:                                 *
 *               0=false, 1=true,                                          *
 *               2=result into the word pointed by load_word.              *
 *                                                                         *
 * Output:                                                                 *
 *    load_word: if result=2 it  will contain the tagged word of the term: *
 *               a <INT,0/1> or a <REF,adr>                                *
 *-------------------------------------------------------------------------*/
static Bool
Load_Bool_Into_Word(WamWord *exp, int result, WamWord *load_word)
{
  WamWord op = *exp;

  if (op >= EQ_F && op <= LTE_F)
    {
      full_ac = 1;
      op = op - EQ_F + EQ;
    }
  else
    full_ac = 0;

  return (*((op <= ONE) ? func_tbl[op] : Set_Var)) (exp, result, load_word);
}




/*-------------------------------------------------------------------------*
 * SET_ZERO                                                                *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Zero(WamWord *exp, int result, WamWord *load_word)
{
  if (result == 0)		/* 0 is false */
    return TRUE;

  if (result == 1)		/* 0 is true */
    return FALSE;

  return Get_Integer(0, *load_word);	/* 0=B */
}




/*-------------------------------------------------------------------------*
 * SET_ONE                                                                 *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_One(WamWord *exp, int result, WamWord *load_word)
{
  if (result == 0)		/* 1 is false */
    return FALSE;

  if (result == 1)		/* 1 is true */
    return TRUE;

  return Get_Integer(1, *load_word);	/* 1=B */
}




/*-------------------------------------------------------------------------*
 * SET_VAR                                                                 *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Var(WamWord *exp, int result, WamWord *load_word)
{
  if (result == 0)		/* X is false */
    return Get_Integer(0, *exp);

  if (result == 1)		/* X is true */
    return Get_Integer(1, *exp);

  *load_word = *exp;		/* X=B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_NOT                                                                 *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Not(WamWord *exp, int result, WamWord *load_word)
{
  if (result == 0)		/* ~X is false */
    return Get_Integer(1, exp[1]);

  if (result == 1)		/* ~X is true */
    return Get_Integer(0, exp[1]);

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_2(not_x_eq_b, exp[1], *load_word);	/* ~X=B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_EQUIV                                                               *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Equiv(WamWord *exp, int result, WamWord *load_word)
{
  WamWord load_l, load_r;

  if (!Load_Bool_Into_Word((WamWord *) (exp[1]), 2, &load_l) ||
      !Load_Bool_Into_Word((WamWord *) (exp[2]), 2, &load_r))
    return FALSE;

  if (result == 0)		/* L <=> R is false */
    {
      BOOL_CSTR_2(not_x_eq_b, load_l, load_r);
      return TRUE;
    }

  if (result == 1)		/* L <=> R is true */
    return Fd_Math_Unify_X_Y(load_l, load_r);

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_3(x_equiv_y_eq_b, load_l, load_r, *load_word);	/* L <=> R = B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_NEQUIV                                                              *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Nequiv(WamWord *exp, int result, WamWord *load_word)
{
  WamWord load_l, load_r;

  if (result <= 1)		/* L ~<=> R is true or false */
    return Set_Equiv(exp, 1 - result, load_word);

  if (!Load_Bool_Into_Word((WamWord *) (exp[1]), 2, &load_l) ||
      !Load_Bool_Into_Word((WamWord *) (exp[2]), 2, &load_r))
    return FALSE;

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_3(x_nequiv_y_eq_b, load_l, load_r, *load_word);	/* L ~<=> R = B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_IMPLY                                                               *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Imply(WamWord *exp, int result, WamWord *load_word)
{
  WamWord load_l, load_r;

  if (result == 0)		/* L ==> R is false */
    return Load_Bool_Into_Word((WamWord *) (exp[1]), 1, &load_l) &&
      Load_Bool_Into_Word((WamWord *) (exp[2]), 0, &load_r);

  if (!Load_Bool_Into_Word((WamWord *) (exp[1]), 2, &load_l) ||
      !Load_Bool_Into_Word((WamWord *) (exp[2]), 2, &load_r))
    return FALSE;

  if (result == 1)		/* L ==> R is true */
    {
      BOOL_CSTR_2(x_imply_y_eq_1, load_l, load_r);
      return TRUE;
    }

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_3(x_imply_y_eq_b, load_l, load_r, *load_word);	/* L ==> R = B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_NIMPLY                                                              *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Nimply(WamWord *exp, int result, WamWord *load_word)
{
  WamWord load_l, load_r;

  if (result <= 1)		/* L ~==> R is true or false */
    return Set_Imply(exp, 1 - result, load_word);

  if (!Load_Bool_Into_Word((WamWord *) (exp[1]), 2, &load_l) ||
      !Load_Bool_Into_Word((WamWord *) (exp[2]), 2, &load_r))
    return FALSE;

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_3(x_nimply_y_eq_b, load_l, load_r, *load_word);	/* L ~==> R = B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_AND                                                                 *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_And(WamWord *exp, int result, WamWord *load_word)
{
  WamWord load_l, load_r;

  if (result == 1)		/* L /\ R is true */
    return Load_Bool_Into_Word((WamWord *) (exp[1]), 1, NULL) &&
      Load_Bool_Into_Word((WamWord *) (exp[2]), 1, NULL);

  if (!Load_Bool_Into_Word((WamWord *) (exp[1]), 2, &load_l) ||
      !Load_Bool_Into_Word((WamWord *) (exp[2]), 2, &load_r))
    return FALSE;

  if (result == 0)		/* L /\ R is false */
    {
      BOOL_CSTR_2(x_and_y_eq_0, load_l, load_r);
      return TRUE;
    }

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_3(x_and_y_eq_b, load_l, load_r, *load_word);	/* L /\ R = B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_NAND                                                                *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Nand(WamWord *exp, int result, WamWord *load_word)
{
  WamWord load_l, load_r;

  if (result <= 1)		/* L ~/\ R is true or false */
    return Set_And(exp, 1 - result, load_word);

  if (!Load_Bool_Into_Word((WamWord *) (exp[1]), 2, &load_l) ||
      !Load_Bool_Into_Word((WamWord *) (exp[2]), 2, &load_r))
    return FALSE;

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_3(x_nand_y_eq_b, load_l, load_r, *load_word);	/* L ~/\ R = B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_OR                                                                  *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Or(WamWord *exp, int result, WamWord *load_word)
{
  WamWord load_l, load_r;

  if (result == 0)		/* L \/ R is false */
    return Load_Bool_Into_Word((WamWord *) (exp[1]), 0, NULL) &&
      Load_Bool_Into_Word((WamWord *) (exp[2]), 0, NULL);

  if (!Load_Bool_Into_Word((WamWord *) (exp[1]), 2, &load_l) ||
      !Load_Bool_Into_Word((WamWord *) (exp[2]), 2, &load_r))
    return FALSE;

  if (result == 1)		/* L \/ R is true */
    {
      BOOL_CSTR_2(x_or_y_eq_1, load_l, load_r);
      return TRUE;
    }

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_3(x_or_y_eq_b, load_l, load_r, *load_word);	/* L \/ R = B */
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_NOR                                                                 *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Nor(WamWord *exp, int result, WamWord *load_word)
{
  WamWord load_l, load_r;

  if (result <= 1)		/* L ~\/ R is true or false */
    return Set_Or(exp, 1 - result, load_word);

  if (!Load_Bool_Into_Word((WamWord *) (exp[1]), 2, &load_l) ||
      !Load_Bool_Into_Word((WamWord *) (exp[2]), 2, &load_r))
    return FALSE;

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());
  BOOL_CSTR_3(x_nor_y_eq_b, load_l, load_r, *load_word);	/* L ~\/ R = B */
  return TRUE;
}








/*-------------------------------------------------------------------------*
 * SET_EQ                                                                  *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Eq(WamWord *exp, int result, WamWord *load_word)
{
  WamWord le_word, re_word;
  int mask;
  WamWord c_word, l_word, r_word;

  le_word = exp[1];
  re_word = exp[2];

  if (result == 0)		/* L = R is false */
    return Fd_Neq_2(le_word, re_word);

  if (result == 1)		/* L = R is true */
    return Fd_Eq_2(le_word, re_word);

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());

#ifdef DEBUG
  cur_op = (full_ac) ? "truth#=#" : "truth#=";
#endif

  if (!Load_Left_Right
      (FALSE, le_word, re_word, &mask, &c_word, &l_word, &r_word)
      || !Term_Math_Loading(l_word, r_word))
    return FALSE;

  switch (mask)
    {
    case MASK_EMPTY:
      return Get_Integer(c_word == 0, *load_word);

    case MASK_LEFT:
      if (c_word > 0)
	return Get_Integer(0, *load_word);

      MATH_CSTR_3(truth_x_eq_c, l_word, -c_word, *load_word);
      return TRUE;

    case MASK_RIGHT:
      if (c_word < 0)
	return Get_Integer(0, *load_word);

      MATH_CSTR_3(truth_x_eq_c, r_word, c_word, *load_word);
      return TRUE;
    }

  if (c_word > 0)
    {
      MATH_CSTR_4(truth_x_plus_c_eq_y, l_word, c_word, r_word, *load_word);
      return TRUE;
    }

  if (c_word < 0)
    {
      MATH_CSTR_4(truth_x_plus_c_eq_y, r_word, -c_word, l_word, *load_word);
      return TRUE;
    }

  MATH_CSTR_3(truth_x_eq_y, l_word, r_word, *load_word);
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_NEQ                                                                 *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Neq(WamWord *exp, int result, WamWord *load_word)
{
  WamWord le_word, re_word;
  int mask;
  WamWord c_word, l_word, r_word;

  le_word = exp[1];
  re_word = exp[2];

  if (result == 0)		/* L \= R is false */
    return Fd_Eq_2(le_word, re_word);

  if (result == 1)		/* L \= R is true */
    return Fd_Neq_2(le_word, re_word);

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());

#ifdef DEBUG
  cur_op = (full_ac) ? "truth#\\=#" : "truth#\\=";
#endif

  if (!Load_Left_Right
      (FALSE, le_word, re_word, &mask, &c_word, &l_word, &r_word)
      || !Term_Math_Loading(l_word, r_word))
    return FALSE;

  switch (mask)
    {
    case MASK_EMPTY:
      return Get_Integer(c_word != 0, *load_word);

    case MASK_LEFT:
      if (c_word > 0)
	return Get_Integer(1, *load_word);

      MATH_CSTR_3(truth_x_neq_c, l_word, -c_word, *load_word);
      return TRUE;

    case MASK_RIGHT:
      if (c_word < 0)
	return Get_Integer(1, *load_word);

      MATH_CSTR_3(truth_x_neq_c, r_word, c_word, *load_word);
      return TRUE;
    }

  if (c_word > 0)
    {
      MATH_CSTR_4(truth_x_plus_c_neq_y, l_word, c_word, r_word, *load_word);
      return TRUE;
    }

  if (c_word < 0)
    {
      MATH_CSTR_4(truth_x_plus_c_neq_y, r_word, -c_word, l_word,
		  *load_word);
      return TRUE;
    }


  MATH_CSTR_3(truth_x_neq_y, l_word, r_word, *load_word);
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SET_LT                                                                  *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Lt(WamWord *exp, int result, WamWord *load_word)
{
  WamWord le_word, re_word;
  int mask;
  WamWord c_word, l_word, r_word;

  le_word = exp[1];
  re_word = exp[2];

  if (result == 0)		/* L < R is false */
    return Fd_Lte_2(re_word, le_word);

  if (result == 1)		/* L < R is true */
    return Fd_Lt_2(le_word, re_word);

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());

#ifdef DEBUG
  cur_op = (full_ac) ? "truth#<#" : "truth#<";
#endif

  if (!Load_Left_Right
      (FALSE, le_word, re_word, &mask, &c_word, &l_word, &r_word)
      || !Term_Math_Loading(l_word, r_word))
    return FALSE;

  switch (mask)
    {
    case MASK_EMPTY:
      return Get_Integer(c_word < 0, *load_word);

    case MASK_LEFT:
      if (c_word >= 0)
	return Get_Integer(0, *load_word);

      PRIM_CSTR_3(truth_x_lte_c, l_word, -c_word - TAGGED_1, *load_word);
      return TRUE;

    case MASK_RIGHT:
      if (c_word < 0)
	return Get_Integer(1, *load_word);

      PRIM_CSTR_3(truth_x_gte_c, r_word, c_word + TAGGED_1, *load_word);
      return TRUE;
    }

  if (c_word > 0)
    {
      PRIM_CSTR_4(truth_x_plus_c_lte_y, l_word, c_word + TAGGED_1, r_word,
		  *load_word);
      return TRUE;
    }

  if (c_word < 0)
    {
      PRIM_CSTR_4(truth_x_plus_c_gte_y, r_word, -c_word - TAGGED_1, l_word,
		  *load_word);
      return TRUE;
    }


  PRIM_CSTR_3(truth_x_lt_y, l_word, r_word, *load_word);
  return TRUE;
}



/*-------------------------------------------------------------------------*
 * SET_LTE                                                                 *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Set_Lte(WamWord *exp, int result, WamWord *load_word)
{
  WamWord le_word, re_word;
  int mask;
  WamWord c_word, l_word, r_word;

  le_word = exp[1];
  re_word = exp[2];

  if (result == 0)		/* L <= R is false */
    return Fd_Lt_2(re_word, le_word);

  if (result == 1)		/* L <= R is true */
    return Fd_Lte_2(le_word, re_word);

  *load_word = Tag_Value(REF, Fd_New_Bool_Variable());

#ifdef DEBUG
  cur_op = (full_ac) ? "truth#=<#" : "truth#=<";
#endif

  if (!Load_Left_Right
      (FALSE, le_word, re_word, &mask, &c_word, &l_word, &r_word)
      || !Term_Math_Loading(l_word, r_word))
    return FALSE;

  switch (mask)
    {
    case MASK_EMPTY:
      return Get_Integer(c_word <= 0, *load_word);

    case MASK_LEFT:
      if (c_word > 0)
	return Get_Integer(0, *load_word);

      PRIM_CSTR_3(truth_x_lte_c, l_word, -c_word, *load_word);
      return TRUE;

    case MASK_RIGHT:
      if (c_word <= 0)
	return Get_Integer(1, *load_word);

      PRIM_CSTR_3(truth_x_gte_c, r_word, c_word, *load_word);
      return TRUE;
    }

  if (c_word > 0)
    {
      PRIM_CSTR_4(truth_x_plus_c_lte_y, l_word, c_word, r_word, *load_word);
      return TRUE;
    }

  if (c_word < 0)
    {
      PRIM_CSTR_4(truth_x_plus_c_gte_y, r_word, -c_word, l_word,
		  *load_word);
      return TRUE;
    }


  PRIM_CSTR_3(truth_x_lte_y, l_word, r_word, *load_word);
  return TRUE;
}