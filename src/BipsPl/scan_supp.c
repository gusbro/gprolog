/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : Prolog buit-in predicates                                       *
 * File  : scan_supp.c                                                     *
 * Descr.: scanner support                                                 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>

#define SCAN_SUPP_FILE

#include "engine_pl.h"
#include "bips_pl.h"




/*---------------------------------*
 * Constants                       *
 *---------------------------------*/

/*---------------------------------*
 * Type Definitions                *
 *---------------------------------*/

/*---------------------------------*
 * Global Variables                *
 *---------------------------------*/

static int c_orig, c;		/* for read */
static int c_type;

static char *err_msg;



						       /* parser variables */
/*---------------------------------*
 * Function Prototypes             *
 *---------------------------------*/

static int Read_Next_Char(StmInf *pstm, Bool convert);

static void Scan_Number(StmInf *pstm, Bool integer_only);

static void Scan_Quoted(StmInf *pstm);

static int Scan_Quoted_Char(StmInf *pstm, Bool convert, int c0);



#define   Unget_Last_Char       Stream_Ungetc(c_orig,pstm)




/*-------------------------------------------------------------------------*
 * SCAN_PEEK_CHAR                                                          *
 *                                                                         *
 *-------------------------------------------------------------------------*/
int
Scan_Peek_Char(StmInf *pstm, Bool convert)
{
  int c_look;

  c_look = Stream_Peekc(pstm);

  if (convert)
    c_look = Char_Conversion(c_look);

  return c_look;
}




/*-------------------------------------------------------------------------*
 * READ_NEXT_CHAR                                                          *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static int
Read_Next_Char(StmInf *pstm, Bool convert)
{
  c_orig = c = Stream_Getc(pstm);

  if (c == EOF)
    c_type = 0;
  else
    {
      if (convert)
	c = Char_Conversion(c);

      c_type = char_type[c];
    }

  return c;
}




/*-------------------------------------------------------------------------*
 * SCAN_TOKEN                                                              *
 *                                                                         *
 * Scan the next token. The flag comma_is_punct specifies if ',' must be   *
 * considered as a punctuation (e.g. separator of args of compound term or *
 * of a list) or as an atom.                                               *
 * The scanner only consumes the needed characters of the token, calling   *
 * Unget_Last_Char if necessary (see Scan_Number). Thus after a token has  *
 * been read Stream_Peekc() will return the character directly following   *
 * this token.                                                             *
 *-------------------------------------------------------------------------*/
char *
Scan_Token(StmInf *pstm, Bool comma_is_punct)
{
  int c0;
  char *s;
  Bool layout_before = FALSE;


  err_msg = NULL;

start_scan:

  for (;;)
    {
      Read_Next_Char(pstm, TRUE);
      if (c_type != LA)		/* layout character */
	break;
      layout_before = TRUE;
    }


  token.line = pstm->line_count + 1;
  token.col = pstm->line_pos;


  if (c == EOF)
    {
      token.type = TOKEN_END_OF_FILE;
      return err_msg;
    }


  switch (c_type)
    {
    case SL:			/* small letter */
    case UL:			/* underline */
    case CL:			/* capital letter */
      token.type = (c_type == SL) ? TOKEN_NAME : TOKEN_VARIABLE;
      s = token.name;
      do
	{
	  *s++ = c;
	  Read_Next_Char(pstm, TRUE);
	}
      while (c_type & (UL | CL | SL | DI));
      *s = '\0';
      Unget_Last_Char;
      break;

    case DI:			/* digit */
      Scan_Number(pstm, FALSE);
      break;

    case QT:			/* quote */
    case DQ:			/* double quote */
    case BQ:			/* back quote */
      Scan_Quoted(pstm);
      break;

    case GR:			/* graphic */
      c0 = c;
      Read_Next_Char(pstm, TRUE);
      if (c0 == '.' && (c == EOF || (c_type & (LA | CM))))
	{
	  if (c_type == CM)
	    Unget_Last_Char;

	  token.type = TOKEN_FULL_STOP;
	  break;
	}

      if (c0 == '/' && c == '*')	/* comment */
	{
	  Read_Next_Char(pstm, TRUE);
	  if (c != EOF)
	    do
	      {
		c0 = c;
		Read_Next_Char(pstm, TRUE);
	      }
	    while (c != EOF && (c0 != '*' || c != '/'));

	  if (c == EOF)
	    {
	      token.type = TOKEN_END_OF_FILE;
	      token.line = pstm->line_count + 1;
	      token.col = pstm->line_pos;
	      err_msg = "*/ expected here for /*...*/ comment";
	      break;
	    }

	  layout_before = TRUE;
	  goto start_scan;
	}


      token.type = TOKEN_NAME;
      s = token.name;
      *s++ = c0;
      while (c_type == GR)
	{
	  *s++ = c;
	  Read_Next_Char(pstm, TRUE);
	}
      *s = '\0';
      Unget_Last_Char;
      break;

    case CM:			/* comment character */
      do
	Read_Next_Char(pstm, TRUE);
      while (c != '\n' && c != EOF);

      if (c == EOF)
	{
	  token.type = TOKEN_END_OF_FILE;
	  token.line = pstm->line_count + 1;
	  token.col = pstm->line_pos;
	  err_msg = "new-line expected here for %%... comment";
	  break;
	}

      layout_before = TRUE;
      goto start_scan;

    case PC:			/* punctuation character */
      if (c == '(' && !layout_before)
	{
	  token.type = TOKEN_IMMEDIAT_OPEN;
	  break;
	}

      token.type = TOKEN_PUNCTUATION;
      token.punct = c;
      break;

    case SC:			/* solo character */
      if (c == ',' && comma_is_punct)
	{
	  token.type = TOKEN_PUNCTUATION;
	  token.punct = c;
	  break;
	}

      token.type = TOKEN_NAME;
      token.name[0] = c;
      token.name[1] = '\0';
      break;

    case EX:			/* extended character */
      token.type = TOKEN_EXTENDED;
      token.name[0] = c;
      token.name[1] = '\0';
      break;
    }

  return err_msg;
}




/*-------------------------------------------------------------------------*
 * SCAN_NUMBER                                                             *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Scan_Number(StmInf *pstm, Bool integer_only)
{
  int lg;
  int radix, i;
  char *p, *f;
  int c_orig0;

  p = token.name;
  do
    {
      *p++ = c;
      Read_Next_Char(pstm, TRUE);
    }
  while (c_type == DI);
  lg = p - token.name;

  if (!integer_only &&		/* float if . and digit */
      c == '.' && isdigit(Scan_Peek_Char(pstm, TRUE)))
    goto is_a_float;
  /* integer number */
  token.type = TOKEN_INTEGER;
  *p++ = '\0';
  sscanf(token.name, "%ld", &token.int_num);
  if (lg != 1 || token.int_num != 0 || strchr("'bBoOxX", c) == NULL)
    goto push_back;

  if (c == '\'')		/* 0'<character> */
    {
      c = Scan_Quoted_Char(pstm, TRUE, '\'');
      if (c == -1)		/* <character> is ' */
	{
	  token.line = pstm->line_count + 1;
	  token.col = pstm->line_pos + 1;
	  err_msg = "quote character expected here";
	}

      if (c == -2 || c == -3)
	{
	  Unget_Last_Char;

	  token.line = pstm->line_count + 1;
	  token.col = pstm->line_pos + 1;
	  err_msg = "character expected here";
	}

      token.int_num = c;
      return;
    }

  radix = (c == 'b' || c == 'B') ? (f = "01", 2) :
    (c == 'o' || c == 'O') ? (f = "01234567", 8)
    : (f = "0123456789abcdefABCDEF", 16);
  p = token.name;
  Read_Next_Char(pstm, TRUE);
  token.int_num = 0;
  while ((p = strchr(f, c)) != NULL)
    {
      i = p - f;
      if (i >= 16)
	i -= 6;
      token.int_num = token.int_num * radix + i;
      Read_Next_Char(pstm, TRUE);
    }
  goto push_back;


is_a_float:			/* float number */

  token.type = TOKEN_FLOAT;
  *p++ = '.';
  Read_Next_Char(pstm, TRUE);
  while (c_type == DI)
    {
      *p++ = c;
      Read_Next_Char(pstm, TRUE);
    }

  if (c == 'e' || c == 'E')
    {
      c_orig0 = c_orig;
      Read_Next_Char(pstm, TRUE);
      if (!(c_type == DI || ((c == '+' || c == '-') &&
			     isdigit(Scan_Peek_Char(pstm, TRUE)))))
	{
	  Unget_Last_Char;
	  c_orig = c_orig0;
	  goto end_float;
	}

      *p++ = 'e';
      *p++ = c;

      Read_Next_Char(pstm, TRUE);
      while (c_type == DI)
	{
	  *p++ = c;
	  Read_Next_Char(pstm, TRUE);
	}
    }


end_float:
  *p = '\0';
  sscanf(token.name, "%lf", &token.float_num);

push_back:
  Unget_Last_Char;
}




/*-------------------------------------------------------------------------*
 * SCAN_QUOTED                                                             *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Scan_Quoted(StmInf *pstm)
{
  int c0;
  char *s;
  Bool convert = (c_orig != '\'');

  token.type = (c_type == QT) ? TOKEN_NAME
    : (c_type == DQ) ? TOKEN_STRING : TOKEN_BACK_QUOTED;
  s = token.name;
  c0 = c;

  for (;;)
    {
      c = Scan_Quoted_Char(pstm, convert, c0);
      if (c == -1)
	{
	  *s = '\0';
	  return;
	}

      if (c == -2)		/* EOF or \n */
	break;

      if (c == -3)		/* \ followed by \n */
	continue;

      *s++ = c;
    }
  /* error */
  *s = '\0';

  if (err_msg)
    return;

  Unget_Last_Char;

  token.line = pstm->line_count + 1;
  token.col = pstm->line_pos + 1;

  switch (token.type)
    {
    case TOKEN_NAME:
      err_msg = "quote character expected here";
      break;

    case TOKEN_BACK_QUOTED:
      err_msg = "back quote character expected here";
      break;

    case TOKEN_STRING:
      err_msg = "double quote character expected here";
      break;

    default:			/* to avoid compiler warning */
      ;
    }
}




/*-------------------------------------------------------------------------*
 * SCAN_QUOTED_CHAR                                                        *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static int
Scan_Quoted_Char(StmInf *pstm, Bool convert, int c0)
{
  int radix;
  char *p, *f;
  int x, i;

  Read_Next_Char(pstm, convert);
  if (c == c0)
    {
      if (Scan_Peek_Char(pstm, convert) != c0)	/* '' or "" or `` */
	return -1;

      Read_Next_Char(pstm, convert);
      return c;
    }

  if (c == EOF || c == '\n')
    return -2;

  if (c != '\\')
    return c;
  /* escape sequence */
  Read_Next_Char(pstm, convert);
  if (c == '\n')		/* \ followed by \n */
    return -3;

  if (strchr("\\'\"`", c))	/* \\ or \' or \" or \`  " */
    return c;

  if ((p = (char *) strchr(escape_symbol, c)))	/* \a \b \f \n \r \t \v */
    return escape_char[p - escape_symbol];

  if (c == 'x' || ('0' <= c && c <= '7'))	/* \xnn\ \nn\ */
    {
      if (c == 'x')
	{
	  radix = 16;
	  f = "0123456789abcdefABCDEF";
	  x = 0;
	}
      else
	{
	  radix = 8;
	  f = "01234567";
	  x = c - '0';
	}

      Read_Next_Char(pstm, convert);
      while ((p = strchr(f, c)) != NULL)
	{
	  i = p - f;
	  if (i >= 16)
	    i -= 6;
	  x = x * radix + i;
	  Read_Next_Char(pstm, convert);
	}

      if (!Is_Valid_Code(x) && err_msg == NULL)
	{
	  token.line = pstm->line_count + 1;
	  token.col = pstm->line_pos;
	  err_msg = "invalid character code in \\constant\\ sequence";
	}
      if (c != '\\')
	{
	  if (err_msg == NULL)
	    {
	      token.line = pstm->line_count + 1;
	      token.col = pstm->line_pos;
	      err_msg = "\\ expected in \\constant\\ sequence";
	    }

	  Unget_Last_Char;
	}

      return (int) (unsigned char) x;
    }

  if (err_msg == NULL)
    {
      token.line = pstm->line_count + 1;
      token.col = pstm->line_pos;
      err_msg = "unknown escape sequence";
    }

  return 0;
}




/*-------------------------------------------------------------------------*
 * RECOVER_AFTER_ERROR                                                     *
 *                                                                         *
 * Finds the next full stop (to restart after an error)                    *
 *-------------------------------------------------------------------------*/
void
Recover_After_Error(StmInf *pstm)
#define Next_Char   Read_Next_Char(pstm,convert); if (c==EOF) return
{
  int c0;
  Bool convert;

  if (pstm->eof_reached)
    return;

  for (;;)
    {
    loop:
      convert = FALSE;
      Next_Char;

      if (c == '.')
	{
	  Next_Char;
	  if (c_type & (LA | CM))
	    return;		/* full stop found */
	}

      if ((c_type & (QT | DQ | BQ)) == 0)
	continue;

      /* quoted token */
      c0 = c;
      convert = (c_orig != '\'');

      for (;;)
	{
	  Next_Char;
	  if (c == c0)		/* detect end of token - also for  '' or "" or `` */
	    break;

	  if (c != '\\')
	    continue;
	  /* escape sequence */
	  Next_Char;
	  if (c == '\n')	/* \ followed by \n */
	    continue;

	  if (strchr("\\'\"`", c))	/* \\ or \' or \" or \`  " */
	    continue;

	  if (strchr(escape_symbol, c))	/* \a \b \f \n \r \t \v */
	    continue;

	  if (c != 'x' && (c < '0' || c > '7'))
	    continue;

	  for (;;)		/* \xnn\ \nn\ */
	    {			/* simply find terminal \ */
	      Next_Char;
	      if (c == c0)
		goto loop;

	      if (c == '\\' || !isxdigit(c))
		break;
	    }
	}
    }
}




	  /* Other Scanner facilities */




/*-------------------------------------------------------------------------*
 * SCAN_NEXT_ATOM                                                          *
 *                                                                         *
 * Scan the next atom.                                                     *
 *-------------------------------------------------------------------------*/
char *
Scan_Next_Atom(StmInf *pstm)
{
  char *s;

  for (;;)
    {
      Read_Next_Char(pstm, TRUE);
      if (c_type != LA)		/* layout character */
	break;
    }


  token.line = pstm->line_count + 1;
  token.col = pstm->line_pos;

  if (c_type == DQ
      && Flag_Value(FLAG_DOUBLE_QUOTES) != FLAG_DOUBLE_QUOTES_ATOM)
    c_type = 0;

  switch (c_type)
    {
    case SL:			/* small letter */
      s = token.name;
      do
	{
	  *s++ = c;
	  Read_Next_Char(pstm, TRUE);
	}
      while (c_type & (UL | CL | SL | DI));
      *s = '\0';
      Unget_Last_Char;
      break;

    case QT:			/* quote */
    case DQ:			/* double quote */
    case BQ:			/* back quote */
      err_msg = NULL;
      Scan_Quoted(pstm);
      if (err_msg)
	return err_msg;
      break;

    case GR:			/* graphic */
      s = token.name;
      while (c_type == GR)
	{
	  *s++ = c;
	  Read_Next_Char(pstm, TRUE);
	}
      *s = '\0';
      Unget_Last_Char;
      break;

    case SC:			/* solo character */
      token.name[0] = c;
      token.name[1] = '\0';
      break;

    default:
      Unget_Last_Char;
      return "cannot start an atom (use quotes ?)";
    }

  token.type = TOKEN_NAME;
  return NULL;
}




/*-------------------------------------------------------------------------*
 * SCAN_NEXT_NUMBER                                                        *
 *                                                                         *
 * Scan the next number (integer if integer_only is TRUE).                 *
 *-------------------------------------------------------------------------*/
char *
Scan_Next_Number(StmInf *pstm, Bool integer_only)
{
  Bool minus_op = FALSE;

  for (;;)
    {
      Read_Next_Char(pstm, TRUE);
      if (c_type != LA)		/* layout character */
	break;
    }


  token.line = pstm->line_count + 1;
  token.col = pstm->line_pos;


  if (c == '-' && isdigit(Scan_Peek_Char(pstm, FALSE)))	/* negative number */
    {
      Read_Next_Char(pstm, TRUE);
      minus_op = TRUE;
    }

  if (c_type != DI)
    {
      Unget_Last_Char;
      return "cannot start a number";
    }


  Scan_Number(pstm, integer_only);

  if (minus_op)
    {
      if (token.type == TOKEN_INTEGER)
	token.int_num = -token.int_num;
      else
	token.float_num = -token.float_num;
    }

  return NULL;
}