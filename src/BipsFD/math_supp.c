/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : FD constraint solver buit-in predicates                         *
 * File  : math_supp.c                                                     *
 * Descr.: mathematical support                                            *
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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define OBJ_INIT Math_Supp_Initializer

#define MATH_SUPP_FILE

#include "engine_pl.h"
#include "bips_pl.h"

#include "engine_fd.h"
#include "bips_fd.h"




/*---------------------------------*
 * Constants                       *
 *---------------------------------*/

#define DELAY_CSTR_STACK_SIZE      128
#define VARS_STACK_SIZE            1024

#define MAX_MONOMS                 1024

#define MAX_COEF_FOR_SORT          100


#define PLUS_1                     0
#define PLUS_2                     1
#define MINUS_1                    2
#define MINUS_2                    3
#define TIMES_2                    4
#define DIV_2                      5
#define POWER_2                    6
#define MIN_2                      7
#define MAX_2                      8
#define DIST_2                     9
#define QUOT_2                     10
#define REM_2                      11
#define QUOT_REM_3                 12
#define NB_OF_OP                   13




#define DC_X2_EQ_Y                 0
#define DC_XY_EQ_Z                 1
#define DC_DIV_A_Y_EQ_Z            2
#define DC_DIV_X_A_EQ_Z            3
#define DC_DIV_X_Y_EQ_Z            4
#define DC_ZERO_POWER_N_EQ_Y       5
#define DC_A_POWER_N_EQ_Y          6
#define DC_X_POWER_A_EQ_Y          7
#define DC_MIN_X_A_EQ_Z            8
#define DC_MIN_X_Y_EQ_Z            9
#define DC_MAX_X_A_EQ_Z            10
#define DC_MAX_X_Y_EQ_Z            11
#define DC_ABS_X_MINUS_A_EQ_Z      12
#define DC_ABS_X_MINUS_Y_EQ_Z      13
#define DC_QUOT_REM_A_Y_R_EQ_Z     14
#define DC_QUOT_REM_X_A_R_EQ_Z     15
#define DC_QUOT_REM_X_Y_R_EQ_Z     16




/*---------------------------------*
 * Type Definitions                *
 *---------------------------------*/

typedef struct			/* Monomial term information      */
{				/* ------------------------------ */
  WamWord a_word;		/* coefficient a tagged <INT,val> */
  WamWord x_word;		/* variable    a tagged <REF,adr> */
}
Monom;




typedef struct			/* Polynomial term information    */
{				/* ------------------------------ */
  WamWord c_word;		/* the const. a tagged <INT,val>  */
  int nb_monom;			/* nb of monomial terms           */
  Monom m[MAX_MONOMS];		/* table of monomial terms        */
}
Poly;



typedef struct			/* Non linear constr information  */
{				/* ------------------------------ */
  int cstr;			/* DC_X2_EQ_Y, DC_XY_EQ_Z,...     */
  WamWord a1, a2, a3;		/* arguments (input)              */
  WamWord res;			/* argument  (result)             */
}
NonLin;




/*---------------------------------*
 * Global Variables                *
 *---------------------------------*/

static WamWord arith_tbl[NB_OF_OP];


static NonLin delay_cstr_stack[DELAY_CSTR_STACK_SIZE];
static NonLin *delay_sp;


static WamWord vars_tbl[VARS_STACK_SIZE];
static WamWord *vars_sp;


static Bool sort;




/*---------------------------------*
 * Function Prototypes             *
 *---------------------------------*/

static
  Bool Load_Left_Right_Rec(Bool optim_eq,
			   WamWord le_word, WamWord re_word,
			   int *mask, WamWord *c_word,
			   WamWord *l_word, WamWord *r_word);

static int Compar_Monom(Monom *m1, Monom *m2);

static Bool Load_Term_Into_Word(WamWord e_word, WamWord *load_word);

static WamWord Push_Delayed_Cstr(int cstr, WamWord a1, WamWord a2,
				 WamWord a3);

static void Add_Monom(Poly *p, int sign, WamWord a_word, WamWord x_word);

static Bool Add_Multiply_Monom(Poly *p, int sign, Monom *m1, Monom *m2);

static Bool Normalize(WamWord e_word, int sign, Poly *p);

static Bool Load_Poly(int nb_monom, Monom *m, WamWord pref_load_word,
		      WamWord *load_word);

static Bool Load_Poly_Rec(int nb_monom, Monom *m, WamWord load_word);

static Bool Load_Delay_Cstr_Part(void);



#ifdef DEBUG

void Write_1(WamWord term_word);

#endif

#define New_Tagged_Fd_Variable  (Tag_Value(REF,Fd_New_Variable()))

#define Add_Cst_To_Poly(p,s,w)  (p->c_word+=(s<0) ? -((long) (w)) : (w))

#define New_Poly(p)             ((p).c_word=(p).nb_monom=0)

#define Tag0_Mul(x,y)           ((x)*((y)>>TAG_SIZE))




/*-------------------------------------------------------------------------*
 * MATH_SUPP_INITIALIZER                                                   *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Math_Supp_Initializer(void)
{
  arith_tbl[PLUS_1] = Functor_Arity(ATOM_CHAR('+'), 1);
  arith_tbl[PLUS_2] = Functor_Arity(ATOM_CHAR('+'), 2);
  arith_tbl[MINUS_1] = Functor_Arity(ATOM_CHAR('-'), 1);
  arith_tbl[MINUS_2] = Functor_Arity(ATOM_CHAR('-'), 2);
  arith_tbl[TIMES_2] = Functor_Arity(ATOM_CHAR('*'), 2);
  arith_tbl[POWER_2] = Functor_Arity(Create_Atom("**"), 2);
  arith_tbl[DIV_2] = Functor_Arity(ATOM_CHAR('/'), 2);
  arith_tbl[MIN_2] = Functor_Arity(Create_Atom("min"), 2);
  arith_tbl[MAX_2] = Functor_Arity(Create_Atom("max"), 2);
  arith_tbl[DIST_2] = Functor_Arity(Create_Atom("dist"), 2);
  arith_tbl[QUOT_2] = Functor_Arity(Create_Atom("//"), 2);
  arith_tbl[REM_2] = Functor_Arity(Create_Atom("rem"), 2);
  arith_tbl[QUOT_REM_3] = Functor_Arity(Create_Atom("quot_rem"), 3);
}




/*-------------------------------------------------------------------------*
 * LOAD_LEFT_RIGHT                                                         *
 *                                                                         *
 * This function loads the left and right term of a constraint into (new)  *
 * variables.                                                              *
 * Input:                                                                  *
 *    optim_eq: is used to optimize loadings of a term1 #= term2 constraint*
 *              when the constant is zero.                                 *
 *    le_word : left  term of the constraint                               *
 *    re_word : right term of the constraint                               *
 *                                                                         *
 * Output:                                                                 *
 *   mask     : indicates if l_word and r_word are used (see MASK_... cst) *
 *   c_word   : the general (signed) constant as a      (tagged <INT,val>) *
 *   l_word   : the variable containing the left  part  (tagged <REF,adr>) *
 *   r_word   : the variable containing the right part  (tagged <REF,adr>) *
 *-------------------------------------------------------------------------*/
Bool
Load_Left_Right(Bool optim_eq, WamWord le_word, WamWord re_word,
		int *mask, WamWord *c_word,
		WamWord *l_word, WamWord *r_word)
{
#ifdef DEBUG
  DBGPRINTF("\n*** Math constraint : ");
  Write_1(le_word);
  DBGPRINTF(" %s ", cur_op);
  Write_1(re_word);
  DBGPRINTF("\n");
#endif

  delay_sp = delay_cstr_stack;
  vars_sp = vars_tbl;

  return Load_Left_Right_Rec(optim_eq, le_word, re_word, mask, c_word,
			     l_word, r_word);
}




/*-------------------------------------------------------------------------*
 * TERM_MATH_LOADING                                                       *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Term_Math_Loading(WamWord l_word, WamWord r_word)
{
  WamWord word, tag, *adr;
  WamWord *fdv_adr;

  if (delay_sp != delay_cstr_stack)
    {
#ifdef DEBUG
      DBGPRINTF("\nnon Linear part\n");
#endif
      if (!Load_Delay_Cstr_Part())
	return FALSE;
    }

  while (--vars_sp >= vars_tbl)
    {
      Deref(*vars_sp, word, tag, adr);
      if (tag == REF && word != l_word && word != r_word)
	{
	  fdv_adr = Fd_New_Variable();
	  Bind_UV(adr, Tag_Value(REF, fdv_adr));
	}
    }

  return TRUE;
}




/*-------------------------------------------------------------------------*
 * LOAD_LEFT_RIGHT_REC                                                     *
 *                                                                         *
 * This function can be called with re_word==NOT_A_WAM_WORD by the function*
 * Load_Term_Into_Word(). In that case, re_word is simply ignored.         *
 *-------------------------------------------------------------------------*/
static Bool
Load_Left_Right_Rec(Bool optim_eq,
		    WamWord le_word, WamWord re_word,
		    int *mask, WamWord *c_word,
		    WamWord *l_word, WamWord *r_word)
{
  Poly p;
  Monom *l_m, *r_m;
  Monom *cur, *pos, *neg, *end;
  int l_nb_monom, r_nb_monom;
  WamWord pref_load_word;	/* to optimize equalities (#=) */
  int i;

  sort = FALSE;
  New_Poly(p);

  if (!Normalize(le_word, +1, &p))
    return FALSE;

  if (re_word != NOT_A_WAM_WORD && !Normalize(re_word, -1, &p))
    return FALSE;

  if (sort || p.nb_monom > MAX_MONOMS / 2)
    {
      qsort(p.m, p.nb_monom, sizeof(Monom),
	    (int (*)(const void *, const void *)) Compar_Monom);

      for (i = 0; i < p.nb_monom; i++)	/* find left monomial terms */
	if (p.m[i].a_word <= 0)
	  break;

      l_m = p.m;
      l_nb_monom = i;

      for (; i < p.nb_monom; i++)	/* find right monomial terms */
	if (p.m[i].a_word >= 0)
	  break;
	else
	  p.m[i].a_word = -p.m[i].a_word;	/* only positive coefs now */

      r_m = l_m + l_nb_monom;
      r_nb_monom = i - l_nb_monom;
    }
  else
    {
      pos = p.m;
      end = pos + p.nb_monom;
      neg = end;

      for (cur = pos; cur < end; cur++)
	{
	  if (cur->a_word < 0)
	    {
	      neg->a_word = -cur->a_word;
	      neg->x_word = cur->x_word;
	      neg++;
	      continue;
	    }

	  if (cur->a_word > 0)
	    {
	      if (cur != pos)
		*pos = *cur;
	      pos++;
	    }
	}

      l_m = p.m;
      l_nb_monom = pos - l_m;
      r_m = end;
      r_nb_monom = neg - r_m;

#ifdef DEBUG
      DBGPRINTF("l_nb_monom:%d   r_nb_monom:%d\n", l_nb_monom, r_nb_monom);
#endif
    }


#ifdef DEBUG
  DBGPRINTF("normalization: ");
  for (i = 0; i < l_nb_monom; i++)
    {
      DBGPRINTF("%d*", UnTag_INT(l_m[i].a_word));
      Write_1(l_m[i].x_word);
      DBGPRINTF(" + ");
    }

  if (p.c_word > 0)
    DBGPRINTF("%d + ", UnTag_INT(p.c_word));
  else if (l_nb_monom == 0)
    DBGPRINTF("0 + ");

  DBGPRINTF("\b\b%s ", (re_word != NOT_A_WAM_WORD) ? cur_op : "=");

  for (i = 0; i < r_nb_monom; i++)
    {
      DBGPRINTF("%d*", UnTag_INT(r_m[i].a_word));
      Write_1(r_m[i].x_word);
      DBGPRINTF(" + ");
    }

  if (p.c_word < 0)
    DBGPRINTF("%d + ", UnTag_INT(-p.c_word));
  else if (r_nb_monom == 0 && re_word != NOT_A_WAM_WORD)
    DBGPRINTF("0 + ");

  if (re_word == NOT_A_WAM_WORD)
    DBGPRINTF("loaded + ");

  DBGPRINTF("\b\b \n\n");
#endif


  pref_load_word = NOT_A_WAM_WORD;

  *mask = MASK_EMPTY;
  if (l_nb_monom)
    {
      *mask |= MASK_LEFT;
      if (optim_eq && p.c_word == 0 && r_nb_monom == 1
	  && r_m[0].a_word == TAGGED_1)
	pref_load_word = r_m[0].x_word;

      if (!Load_Poly(l_nb_monom, l_m, pref_load_word, l_word))
	return FALSE;
    }

  if (r_nb_monom)
    {
      *mask |= MASK_RIGHT;
      if (pref_load_word == NOT_A_WAM_WORD)
	{
	  if (optim_eq && p.c_word == 0 && l_nb_monom)
	    pref_load_word = *l_word;

	  if (!Load_Poly(r_nb_monom, r_m, pref_load_word, r_word))
	    return FALSE;
	}
    }

  *c_word = p.c_word;

  return TRUE;
}




/*-------------------------------------------------------------------------*
 * LOAD_TERM_INTO_WORD                                                     *
 *                                                                         *
 * This function loads a term into a (tagged) word.                        *
 * Input:                                                                  *
 *    e_word  : term to load                                               *
 *                                                                         *
 * Output:                                                                 *
 *   load_word: the tagged word containing the loading of the term:        *
 *              can be a <INT,val> if there is no variable or a <REF,adr>) *
 *                                                                         *
 * This functions acts like T #= NewVar. However, if T is just an integer  *
 * it avoids the creation of a useless FD NewVar.                          *
 *-------------------------------------------------------------------------*/
static Bool
Load_Term_Into_Word(WamWord e_word, WamWord *load_word)
{
  int mask;
  WamWord c_word, l_word, r_word;
  WamWord word;


  if (!Load_Left_Right_Rec
      (FALSE, e_word, NOT_A_WAM_WORD, &mask, &c_word, &l_word, &r_word))
    return FALSE;

  if (mask == MASK_EMPTY)
    {
      if (c_word < 0)
	return FALSE;

      *load_word = c_word;
      return TRUE;
    }

  if (mask == MASK_LEFT && c_word == 0)
    {
      *load_word = l_word;
      return TRUE;
    }

  *load_word = New_Tagged_Fd_Variable;

  switch (mask)
    {
    case MASK_LEFT:		/* here c_word != 0 */
      if (c_word > 0)
	MATH_CSTR_3(x_plus_c_eq_y, l_word, c_word, *load_word);
      else
	MATH_CSTR_3(x_plus_c_eq_y, *load_word, -c_word, l_word);
      return TRUE;

    case MASK_RIGHT:
      if (c_word < 0)
	return FALSE;

      word = New_Tagged_Fd_Variable;
      MATH_CSTR_3(x_plus_y_eq_z, r_word, *load_word, word);
      PRIM_CSTR_2(x_eq_c, word, c_word);
      return TRUE;
    }

  if (c_word == 0)
    {
      MATH_CSTR_3(x_plus_y_eq_z, r_word, *load_word, l_word);
      return TRUE;
    }

  word = New_Tagged_Fd_Variable;
  MATH_CSTR_3(x_plus_y_eq_z, r_word, *load_word, word);

  if (c_word > 0)
    MATH_CSTR_3(x_plus_c_eq_y, l_word, c_word, word);
  else
    MATH_CSTR_3(x_plus_c_eq_y, word, -c_word, l_word);

  return TRUE;
}




/*-------------------------------------------------------------------------*
 * COMPAR_MONOM                                                            *
 *                                                                         *
 * This function is called by qsort to order a polynomial term. It compares*
 * 2 monomial terms according to the following sequence:                   *
 *                                                                         *
 * positive coefficients (from greatest to smallest)                       *
 * negative coefficients (from smallest to greatest)                       *
 *                       (ie. from |greatest| to |smallest|)               *
 * null coefficients                                                       *
 *-------------------------------------------------------------------------*/
static int
Compar_Monom(Monom *m1, Monom *m2)
{
  int cmp;

  if (m1->a_word > 0)
    cmp = (m2->a_word > 0) ? m2->a_word - m1->a_word : -1;
  else
    cmp = (m2->a_word > 0) ? +1 : m1->a_word - m2->a_word;

  return cmp;
}




/*-------------------------------------------------------------------------*
 * PUSH_DELAYED_CSTR                                                       *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static WamWord
Push_Delayed_Cstr(int cstr, WamWord a1, WamWord a2, WamWord a3)
{
  WamWord res_word;

  res_word = Make_Self_Ref(H);
  Global_Push(res_word);

  if (delay_sp - delay_cstr_stack == DELAY_CSTR_STACK_SIZE)
    Pl_Err_Resource(resource_too_big_fd_constraint);

  delay_sp->cstr = cstr;
  delay_sp->a1 = a1;
  delay_sp->a2 = a2;
  delay_sp->a3 = a3;
  delay_sp->res = res_word;

  delay_sp++;

  return res_word;
}




/*-------------------------------------------------------------------------*
 * ADD_MONOM                                                               *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Add_Monom(Poly *p, int sign, WamWord a_word, WamWord x_word)
{
  int i;

  if (a_word == 0)
    return;

  if (sign < 0)
    a_word = -a_word;

  for (i = 0; i < p->nb_monom; i++)
    if (p->m[i].x_word == x_word)
      {
	p->m[i].a_word += a_word;
	return;
      }

  if (p->nb_monom >= MAX_MONOMS)
    Pl_Err_Resource(resource_too_big_fd_constraint);

  p->m[p->nb_monom].a_word = a_word;
  p->m[p->nb_monom].x_word = x_word;
  p->nb_monom++;
}




/*-------------------------------------------------------------------------*
 * ADD_MULTIPLY_MONOM                                                      *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Add_Multiply_Monom(Poly *p, int sign, Monom *m1, Monom *m2)
{
  WamWord a_word;
  WamWord x_word;

  a_word = Tag0_Mul(m1->a_word, m2->a_word);

  if (a_word == 0)
    return TRUE;

  x_word = (m1->x_word == m2->x_word)
    ? Push_Delayed_Cstr(DC_X2_EQ_Y, m1->x_word, 0, 0)
    : Push_Delayed_Cstr(DC_XY_EQ_Z, m1->x_word, m2->x_word, 0);

  Add_Monom(p, sign, a_word, x_word);
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * NORMALIZE                                                               *
 *                                                                         *
 * This functions normalizes a term.                                       *
 * Input:                                                                  *
 *    e_word: term to normalize                                            *
 *    sign  : current sign of the term (-1 or +1)                          *
 *                                                                         *
 * Output:                                                                 *
 *    p     : the associated polynomial term                               *
 *                                                                         *
 * Normalizes the term and loads it into p.                                *
 * Non-Linear operations are simplified and loaded into a stack to be      *
 * executed later.                                                         *
 *                                                                         *
 * T1*T2 : T1 and T2 are normalized to give the polynomials p1 and p2, with*
 *         p1 = c1 + a1X1 + a2X2 + ... + anXn                              *
 *         p2 = c2 + b1X1 + b2X2 + ... + bmXm                              *
 *         and replaced by c1*c2 +                                         *
 *                         a1X1 * c2 + a1X1 * b1X1 + ... + a1X1 * bmXm     *
 *                         ...                                             *
 *                         anX1 * c2 + anXn * b1X1 + ... + anXn * bmXm     *
 *                                                                         *
 * T1**T2: T1 and T2 are loaded into 2 new words word1 and word2 that can  *
 *         be integers or variables (tagged words). The code emitted       *
 *         depends on 3 possibilities (var**var is not allowed)            *
 *         (+ optim 1**T2, 0**T2, T1**0, T1**1), NB 0**0=1                 *
 *-------------------------------------------------------------------------*/
static Bool
Normalize(WamWord e_word, int sign, Poly *p)
{
  WamWord word, tag, *adr;
  WamWord *fdv_adr;
  WamWord word1, word2, word3;
  WamWord f_n, le_word, re_word;
  int i;
  int n1, n2, n3;


  Deref(e_word, word, tag, adr);
  switch (tag)
    {
    case REF:
      if (vars_sp - vars_tbl == VARS_STACK_SIZE)
	Pl_Err_Resource(resource_too_big_fd_constraint);

      *vars_sp++ = word;
      Add_Monom(p, sign, TAGGED_1, word);
      return TRUE;

    case FDV:
      fdv_adr = UnTag_FDV(word);
      Add_Monom(p, sign, TAGGED_1, Tag_Value(REF, fdv_adr));
      return TRUE;

    case INT:
      if (word > Tag_Value(INT, MAX_COEF_FOR_SORT))
	sort = TRUE;

      Add_Cst_To_Poly(p, sign, word);
      return TRUE;

    case ATM:
      word = Put_Structure(ATOM_CHAR('/'), 2);
      Unify_Value(e_word);
      Unify_Integer(0);
      goto type_error;

    case STC:
      break;

    default:
    type_error:
      Pl_Err_Type(type_fd_evaluable, word);
    }

  adr = UnTag_STC(word);

  f_n = Functor_And_Arity(adr);
  for (i = 0; i < NB_OF_OP; i++)
    if (arith_tbl[i] == f_n)
      break;

  le_word = Arg(adr, 0);
  re_word = Arg(adr, 1);

  switch (i)
    {
    case PLUS_1:
      return Normalize(le_word, sign, p);

    case PLUS_2:
      return Normalize(le_word, sign, p) && Normalize(re_word, sign, p);

    case MINUS_2:
      return Normalize(le_word, sign, p) && Normalize(re_word, -sign, p);

    case MINUS_1:
      return Normalize(le_word, -sign, p);

    case TIMES_2:
#if 1				/* optimize frequent use: INT*VAR */
      Deref(le_word, word, tag, adr);
      if (tag != INT)
	goto any;

      if (word > Tag_Value(INT, MAX_COEF_FOR_SORT))
	sort = TRUE;

      word1 = word;
      Deref(re_word, word, tag, adr);
      if (tag != REF)
	{
	  if (tag != FDV)
	    goto any;
	  else
	    {
	      fdv_adr = UnTag_FDV(word);
	      word = Tag_Value(REF, fdv_adr);
	    }
	}
      Add_Monom(p, sign, word1, word);
      return TRUE;
    any:
#endif
      {
	Poly p1, p2;
	int i1, i2;

	New_Poly(p1);
	New_Poly(p2);

	if (!Normalize(le_word, 1, &p1) || !Normalize(re_word, 1, &p2))
	  return FALSE;

	word = Tag0_Mul(p1.c_word, p2.c_word);
	Add_Cst_To_Poly(p, sign, word);

	for (i1 = 0; i1 < p1.nb_monom; i1++)
	  {
	    Add_Monom(p, sign, Tag0_Mul(p1.m[i1].a_word, p2.c_word),
		      p1.m[i1].x_word);
	    for (i2 = 0; i2 < p2.nb_monom; i2++)
	      if (!Add_Multiply_Monom(p, sign, p1.m + i1, p2.m + i2))
		return FALSE;
	  }
	for (i2 = 0; i2 < p2.nb_monom; i2++)
	  Add_Monom(p, sign, Tag0_Mul(p2.m[i2].a_word, p1.c_word),
		    p2.m[i2].x_word);
	return TRUE;
      }

    case POWER_2:
      if (!Load_Term_Into_Word(le_word, &word1) ||
	  !Load_Term_Into_Word(re_word, &word2))
	return FALSE;

      if (Tag_Of(word1) == INT)
	{
	  n1 = UnTag_INT(word1);
	  if (Tag_Of(word2) == INT)
	    {
	      n2 = UnTag_INT(word2);
	      if ((n1 = Power(n1, n2)) < 0)
		return FALSE;

	      word = Tag_Value(INT, n1);
	      Add_Cst_To_Poly(p, sign, word);
	      return TRUE;
	    }

	  if (n1 == 1)
	    {
	      Add_Cst_To_Poly(p, sign, TAGGED_1);
	      return TRUE;
	    }

	  word = (n1 == 0)
	    ? Push_Delayed_Cstr(DC_ZERO_POWER_N_EQ_Y, word2, 0, 0)
	    : Push_Delayed_Cstr(DC_A_POWER_N_EQ_Y, word1, word2, 0);
	  goto end_power;
	}

      if (Tag_Of(word2) != INT)
	Pl_Err_Instantiation();
      else
	{
	  n2 = UnTag_INT(word2);
	  if (n2 == 0)
	    {
	      Add_Cst_To_Poly(p, sign, TAGGED_1);
	      return TRUE;
	    }

	  word = (n2 == 1)
	    ? word1
	    : (n2 == 2)
	    ? Push_Delayed_Cstr(DC_X2_EQ_Y, word1, 0, 0)
	    : Push_Delayed_Cstr(DC_X_POWER_A_EQ_Y, word1, word2, 0);
	}
    end_power:
      Add_Monom(p, sign, TAGGED_1, word);
      return TRUE;

    case MIN_2:
      if (!Load_Term_Into_Word(le_word, &word1) ||
	  !Load_Term_Into_Word(re_word, &word2))
	return FALSE;

      if (Tag_Of(word1) == INT)
	{
	  n1 = UnTag_INT(word1);
	  if (Tag_Of(word2) == INT)
	    {
	      n2 = UnTag_INT(word2);
	      n1 = math_min(n1, n2);
	      word = Tag_Value(INT, n1);
	      Add_Cst_To_Poly(p, sign, word);
	      return TRUE;
	    }

	  word = Push_Delayed_Cstr(DC_MIN_X_A_EQ_Z, word2, word1, 0);
	  goto end_min;
	}

      if (Tag_Of(word2) == INT)
	word = Push_Delayed_Cstr(DC_MIN_X_A_EQ_Z, word1, word2, 0);
      else
	word = Push_Delayed_Cstr(DC_MIN_X_Y_EQ_Z, word1, word2, 0);

    end_min:
      Add_Monom(p, sign, TAGGED_1, word);
      return TRUE;

    case MAX_2:
      if (!Load_Term_Into_Word(le_word, &word1) ||
	  !Load_Term_Into_Word(re_word, &word2))
	return FALSE;

      if (Tag_Of(word1) == INT)
	{
	  n1 = UnTag_INT(word1);
	  if (Tag_Of(word2) == INT)
	    {
	      n2 = UnTag_INT(word2);
	      n1 = math_max(n1, n2);
	      word = Tag_Value(INT, n1);
	      Add_Cst_To_Poly(p, sign, word);
	      return TRUE;
	    }

	  word = Push_Delayed_Cstr(DC_MAX_X_A_EQ_Z, word2, word1, 0);
	  goto end_max;
	}

      if (Tag_Of(word2) == INT)
	word = Push_Delayed_Cstr(DC_MAX_X_A_EQ_Z, word1, word2, 0);
      else
	word = Push_Delayed_Cstr(DC_MAX_X_Y_EQ_Z, word1, word2, 0);

    end_max:
      Add_Monom(p, sign, TAGGED_1, word);
      return TRUE;

    case DIST_2:
      if (!Load_Term_Into_Word(le_word, &word1) ||
	  !Load_Term_Into_Word(re_word, &word2))
	return FALSE;

      if (Tag_Of(word1) == INT)
	{
	  n1 = UnTag_INT(word1);
	  if (Tag_Of(word2) == INT)
	    {
	      n2 = UnTag_INT(word2);
	      n1 = (n1 >= n2) ? n1 - n2 : n2 - n1;
	      word = Tag_Value(INT, n1);
	      Add_Cst_To_Poly(p, sign, word);
	      return TRUE;
	    }

	  word = Push_Delayed_Cstr(DC_ABS_X_MINUS_A_EQ_Z, word2, word1, 0);
	  goto end_dist;
	}

      if (Tag_Of(word2) == INT)
	word = Push_Delayed_Cstr(DC_ABS_X_MINUS_A_EQ_Z, word1, word2, 0);
      else
	word = Push_Delayed_Cstr(DC_ABS_X_MINUS_Y_EQ_Z, word1, word2, 0);

    end_dist:
      Add_Monom(p, sign, TAGGED_1, word);
      return TRUE;

    case QUOT_2:
      word3 = Make_Self_Ref(H);	/* word3 = remainder */
      Global_Push(word3);
      goto quot_rem;

    case REM_2:
      word3 = Make_Self_Ref(H);	/* word3 = remainder */
      Global_Push(word3);
      goto quot_rem;

    case QUOT_REM_3:
    quot_rem:
      if (!Load_Term_Into_Word(le_word, &word1) ||
	  !Load_Term_Into_Word(re_word, &word2) ||
	  (i == QUOT_REM_3 && !Load_Term_Into_Word(Arg(adr, 2), &word3)))
	return FALSE;

      if (Tag_Of(word1) == INT)
	{
	  n1 = UnTag_INT(word1);
	  if (Tag_Of(word2) == INT)
	    {
	      n2 = UnTag_INT(word2);
	      if (n2 == 0)
		return FALSE;
	      n3 = n1 % n2;
	      word = Tag_Value(INT, n3);

	      if (i == QUOT_2 || i == QUOT_REM_3)
		{
		  if (i == QUOT_REM_3)
		    PRIM_CSTR_2(x_eq_c, word3, word);
		  else
		    H--;	/* recover word3 space */
		  n1 = n1 / n2;
		  word = Tag_Value(INT, n1);
		}

	      Add_Cst_To_Poly(p, sign, word);
	      return TRUE;
	    }

	  word = Push_Delayed_Cstr(DC_QUOT_REM_A_Y_R_EQ_Z, word1, word2,
				   word3);
	  goto end_quot_rem;
	}

      if (Tag_Of(word2) == INT)
	word = Push_Delayed_Cstr(DC_QUOT_REM_X_A_R_EQ_Z, word1, word2,
				 word3);
      else
	word = Push_Delayed_Cstr(DC_QUOT_REM_X_Y_R_EQ_Z, word1, word2,
				 word3);

    end_quot_rem:
      Add_Monom(p, sign, TAGGED_1, (i == REM_2) ? word3 : word);
      return TRUE;

    case DIV_2:
      if (!Load_Term_Into_Word(le_word, &word1) ||
	  !Load_Term_Into_Word(re_word, &word2))
	return FALSE;

      if (Tag_Of(word1) == INT)
	{
	  n1 = UnTag_INT(word1);
	  if (Tag_Of(word2) == INT)
	    {
	      n2 = UnTag_INT(word2);
	      if (n2 == 0 || n1 % n2 != 0)
		return FALSE;
	      n1 /= n2;
	      word = Tag_Value(INT, n1);
	      Add_Cst_To_Poly(p, sign, word);
	      return TRUE;
	    }

	  word = Push_Delayed_Cstr(DC_DIV_A_Y_EQ_Z, word1, word2, 0);
	  goto end_div;
	}

      if (Tag_Of(word2) == INT)
	word = Push_Delayed_Cstr(DC_DIV_X_A_EQ_Z, word1, word2, 0);
      else
	word = Push_Delayed_Cstr(DC_DIV_X_Y_EQ_Z, word1, word2, 0);

    end_div:
      Add_Monom(p, sign, TAGGED_1, word);
      return TRUE;

    default:
      word = Put_Structure(ATOM_CHAR('/'), 2);
      Unify_Atom(Functor(adr));
      Unify_Integer(Arity(adr));
      goto type_error;
    }

  return TRUE;
}




/*-------------------------------------------------------------------------*
 * LOAD_POLY                                                               *
 *                                                                         *
 * This function loads a polynomial term (without constant) into a word.   *
 * Input:                                                                  *
 *    nb_monom      : nb of monomial terms (nb_monom > 0)                  *
 *    m             : array of monomial terms                              *
 *    pref_load_word: wanted load_word (or NOT_A_WAM_WORD)                 *
 *                                                                         *
 * Output:                                                                 *
 *   load_word      : the word containing the loading ie.: <REF,adr>       *
 *                                                                         *
 * This functions does not take into account constants.                    *
 *-------------------------------------------------------------------------*/
static Bool
Load_Poly(int nb_monom, Monom *m, WamWord pref_load_word,
	  WamWord *load_word)
{
  if (nb_monom == 1 && m[0].a_word == TAGGED_1)
    {
      if (pref_load_word != NOT_A_WAM_WORD)
	{
	  if (!Fd_Math_Unify_X_Y(m[0].x_word, pref_load_word))
	    return FALSE;
	  *load_word = pref_load_word;
	  return TRUE;
	}

      *load_word = m[0].x_word;
      return TRUE;
    }

  if (pref_load_word != NOT_A_WAM_WORD)
    *load_word = pref_load_word;
  else
    *load_word = New_Tagged_Fd_Variable;

  return Load_Poly_Rec(nb_monom, m, *load_word);
}




/*-------------------------------------------------------------------------*
 * LOAD_POLY_REC                                                           *
 *                                                                         *
 * This function recursively loads a polynomial term into a word.          *
 * Input:                                                                  *
 *    nb_monom      : nb of monomial terms (nb_monom > 0)                  *
 *    m             : array of monomial terms                              *
 *    load_word      : the word where the term must be loaded              *
 *                                                                         *
 * At the entry, if nb_monom==1 then the coefficient of the monomial term  *
 * is > 1 (see call from Load_Poly() and recursive call).                  *
 *-------------------------------------------------------------------------*/
static Bool
Load_Poly_Rec(int nb_monom, Monom *m, WamWord load_word)
{
  WamWord load_word1;

  if (nb_monom == 1)
    {				/* here m[0].a_word!=TAGGED_1 */
      MATH_CSTR_3(ax_eq_y, m[0].a_word, m[0].x_word, load_word);

      return TRUE;
    }


  if (nb_monom == 2)
    {
      if (m[0].a_word == TAGGED_1)
	{
	  if (m[1].a_word == TAGGED_1)
	    MATH_CSTR_3(x_plus_y_eq_z, m[0].x_word, m[1].x_word, load_word);
	  else
	    MATH_CSTR_4(ax_plus_y_eq_z, m[1].a_word, m[1].x_word,
			m[0].x_word, load_word);
	}
      else if (m[1].a_word == TAGGED_1)
	MATH_CSTR_4(ax_plus_y_eq_z, m[0].a_word, m[0].x_word, m[1].x_word,
		    load_word);
      else
	MATH_CSTR_5(ax_plus_by_eq_z, m[0].a_word, m[0].x_word,
		    m[1].a_word, m[1].x_word, load_word);

      return TRUE;
    }

  if (nb_monom == 3 && m[2].a_word == TAGGED_1)
    load_word1 = m[2].x_word;
  else
    load_word1 = New_Tagged_Fd_Variable;

  if (m[0].a_word == TAGGED_1)
    {
      if (m[1].a_word == TAGGED_1)
	MATH_CSTR_4(x_plus_y_plus_z_eq_t, m[0].x_word, m[1].x_word,
		    load_word1, load_word);
      else
	MATH_CSTR_5(ax_plus_y_plus_z_eq_t, m[1].a_word, m[1].x_word,
		    m[0].x_word, load_word1, load_word);
    }
  else if (m[1].a_word == TAGGED_1)
    MATH_CSTR_5(ax_plus_y_plus_z_eq_t, m[0].a_word, m[0].x_word,
		m[1].x_word, load_word1, load_word);
  else
    PRIM_CSTR_6(ax_plus_by_plus_z_eq_t, m[0].a_word, m[0].x_word,
		m[1].a_word, m[1].x_word, load_word1, load_word);

  if (!(nb_monom == 3 && m[2].a_word == TAGGED_1))
    return Load_Poly_Rec(nb_monom - 2, m + 2, load_word1);

  return TRUE;
}




/*-------------------------------------------------------------------------*
 * LOAD_DELAY_CSTR_PART                                                    *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Load_Delay_Cstr_Part(void)
{
  NonLin *i;

  for (i = delay_cstr_stack; i < delay_sp; i++)
    {
      switch (i->cstr)
	{
	case DC_X2_EQ_Y:
	  MATH_CSTR_2(x2_eq_y, i->a1, i->res);
	  break;

	case DC_XY_EQ_Z:
	  MATH_CSTR_3(xy_eq_z, i->a1, i->a2, i->res);
	  break;

	case DC_DIV_A_Y_EQ_Z:
	  PRIM_CSTR_2(x_gte_c, i->a2, TAGGED_1);
	  MATH_CSTR_3(xy_eq_z, i->res, i->a2, i->a1);
	  break;

	case DC_DIV_X_A_EQ_Z:
	  MATH_CSTR_3(ax_eq_y, i->a2, i->res, i->a1);
	  break;

	case DC_DIV_X_Y_EQ_Z:
	  PRIM_CSTR_2(x_gte_c, i->a2, TAGGED_1);
	  MATH_CSTR_3(xy_eq_z, i->res, i->a2, i->a1);
	  break;

	case DC_ZERO_POWER_N_EQ_Y:
	  PRIM_CSTR_2(zero_power_n_eq_y, i->a1, i->res);
	  break;

	case DC_A_POWER_N_EQ_Y:
	  MATH_CSTR_3(a_power_n_eq_y, i->a1, i->a2, i->res);
	  break;

	case DC_X_POWER_A_EQ_Y:
	  MATH_CSTR_3(x_power_a_eq_y, i->a1, i->a2, i->res);
	  break;

	case DC_MIN_X_A_EQ_Z:
	  MATH_CSTR_3(min_x_a_eq_z, i->a1, i->a2, i->res);
	  break;

	case DC_MIN_X_Y_EQ_Z:
	  MATH_CSTR_3(min_x_y_eq_z, i->a1, i->a2, i->res);
	  break;

	case DC_MAX_X_A_EQ_Z:
	  MATH_CSTR_3(max_x_a_eq_z, i->a1, i->a2, i->res);
	  break;

	case DC_MAX_X_Y_EQ_Z:
	  MATH_CSTR_3(max_x_y_eq_z, i->a1, i->a2, i->res);
	  break;

	case DC_ABS_X_MINUS_A_EQ_Z:
	  MATH_CSTR_3(abs_x_minus_a_eq_z, i->a1, i->a2, i->res);
	  break;

	case DC_ABS_X_MINUS_Y_EQ_Z:
	  MATH_CSTR_3(abs_x_minus_y_eq_z, i->a1, i->a2, i->res);
	  break;

	case DC_QUOT_REM_A_Y_R_EQ_Z:
	  MATH_CSTR_4(quot_rem_a_y_r_eq_z, i->a1, i->a2, i->a3, i->res);
	  break;

	case DC_QUOT_REM_X_A_R_EQ_Z:
	  MATH_CSTR_4(quot_rem_x_a_r_eq_z, i->a1, i->a2, i->a3, i->res);
	  break;

	case DC_QUOT_REM_X_Y_R_EQ_Z:
	  MATH_CSTR_4(quot_rem_x_y_r_eq_z, i->a1, i->a2, i->a3, i->res);
	  break;
	}
    }

  delay_sp = delay_cstr_stack;
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * FD_MATH_UNIFY_X_Y                                                       *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Fd_Math_Unify_X_Y(WamWord x, WamWord y)
{
  WamWord x_word, x_tag, *adr;
  WamWord y_word, y_tag;

  Deref(x, x_word, x_tag, adr);
  Deref(y, y_word, y_tag, adr);

  if (x_tag == FDV && y_tag == FDV)
    {
      MATH_CSTR_2(x_eq_y, x, y);
      return TRUE;
    }

#ifdef DEBUG
  DBGPRINTF("Prolog Unif: ");
  Write_1(x_word);
  DBGPRINTF(" = ");
  Write_1(y_word);
  DBGPRINTF("\n");
#endif

  return Unify(x_word, y_word);
}





/*-------------------------------------------------------------------------*
 * X_EQ_C                                                                  *
 *                                                                         *
 * Defined here instead in fd_math_fd.fd to avoid A frame creation.        *
 *-------------------------------------------------------------------------*/
Bool
x_eq_c(WamWord x, WamWord c)
{
  WamWord word, tag, *adr;

  Deref(c, word, tag, adr);
  return Get_Integer(UnTag_INT(word), x);
}





#ifdef DEBUG

/*-------------------------------------------------------------------------*
 * DEBUG_DISPLAY                                                           *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Debug_Display(char *fct, int n, ...)
{
  va_list arg_ptr;
  WamWord word;
  int i;
  char *s1[] = { "plus", "eq", "neq", "lte", "lt", "gte", "gt", NULL };
  char *s2[] = { "+", "=", "\\=", "<=", "<", ">=", ">" };
  char **s;
  char *p;

  va_start(arg_ptr, n);

  DBGPRINTF("'");

  for (p = fct; *p; p++)
    {
      if (*p == '_')
	{
	  for (s = s1; *s; s++)
	    {
	      i = strlen(*s);
	      if (strncmp(*s, p + 1, i) == 0)
		break;
	    }

	  if (*s && p[1 + i] == '_')
	    {
	      p += 1 + i;
	      DBGPRINTF(s2[s - s1]);
	      continue;
	    }
	}

      DBGPRINTF("%c", *p);
    }


  DBGPRINTF("'(");
  for (i = 0; i < n; i++)
    {
      word = va_arg(arg_ptr, WamWord);

      Write_1(word);
      DBGPRINTF("%c", (i < n - 1) ? ',' : ')');
    }

  va_end(arg_ptr);
  DBGPRINTF("\n");
}


#endif