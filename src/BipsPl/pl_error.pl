/*-------------------------------------------------------------------------*/
/* GNU Prolog                                                              */
/*                                                                         */
/* Part  : Prolog buit-in predicates                                       */
/* File  : pl_error.pl                                                     */
/* Descr.: Prolog error management                                         */
/* Author: Daniel Diaz                                                     */
/*                                                                         */
/* Copyright (C) 1999,2000 Daniel Diaz                                     */
/*                                                                         */
/* GNU Prolog is free software; you can redistribute it and/or modify it   */
/* under the terms of the GNU General Public License as published by the   */
/* Free Software Foundation; either version 2, or any later version.       */
/*                                                                         */
/* GNU Prolog is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of              */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        */
/* General Public License for more details.                                */
/*                                                                         */
/* You should have received a copy of the GNU General Public License along */
/* with this program; if not, write to the Free Software Foundation, Inc.  */
/* 59 Temple Place - Suite 330, Boston, MA 02111, USA.                     */
/*-------------------------------------------------------------------------*/

:- built_in.


set_bip_name(Name,Arity):-
	'$call_c'('Set_Bip_Name_2'(Name,Arity)).            % in error_supp.c

current_bip_name(Name,Arity):-
	'$call_c_test'('Current_Bip_Name_2'(Name,Arity)).   % in error_supp.c




'$pl_err_instantiation':-
	'$pl_error'(instantiation_error).

'$pl_err_type'(Type,T):-
	'$pl_error'(type_error(Type,T)).

'$pl_err_domain'(Dom,T):-
	'$pl_error'(domain_error(Dom,T)).

'$pl_err_existence'(Object,T):-
	'$pl_error'(existence_error(Object,T)).

'$pl_err_permission'(Oper,Perm,T):-
	'$pl_error'(permission_error(Oper,Perm,T)).

'$pl_err_representation'(Flag):-
	'$pl_error'(representation_error(Flag)).

'$pl_err_evaluation'(Error):-
	'$pl_error'(evaluation_error(Error)).

'$pl_err_resource'(Flag):-
	'$pl_error'(resource_error(Flag)).

'$pl_err_syntax'(T):-
	'$pl_error'(syntax_error(T)).

'$pl_err_system'(T):-
	'$pl_error'(system_error(T)).





'$pl_error'(Msg):-
	'$call_c'('Context_Error_1'(ContextAtom)),
        throw(error(Msg,ContextAtom)).




syntax_error_info(FileName,Line,Char,Msg):-
	set_bip_name(syntax_error_info,4),
	'$call_c_test'('Syntax_Error_Info_4'(FileName,Line,Char,Msg)).







