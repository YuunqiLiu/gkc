/*
*
* Copyright (c) Tanel Tammet 2004-2019
*
* Contact: tanel.tammet@gmail.com                 
*
* This file is part of GKC
*
* GKC is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* GKC is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU Affero General Public License
* along with GKC.  If not, see <http://www.gnu.org/licenses/>.
*
*/

 /** @file dbprolog.y
 *  Grammar rules for prolog parser
 *
 */


%{

#include <stdio.h>
#include "dbprologparse.h"

#define DBPARM ((void*)(parm->db))

%}


/*
** 'pure_parser' tells bison to use no global variables and create a
** reentrant parser.
** The newer version is
** %define api.pure
*/ 

%define api.pure
%parse-param {parse_parm *parm}
%parse-param {void *scanner}
%lex-param {yyscan_t *scanner}


%token IS
%token TATOM
%token TFLOAT
%token TINT
%token NOT
%token VAR
%token FAIL

%left NOT
/* TODO: make these work 
%left OR
%left AND
%nonassoc '<' '>' '=' LE GE NE
%left '+' '-'
%left '*' '/'
*/



%% /* Grammar rules and actions follow */


input: clauselist	{ $$ = MKWGPAIR(DBPARM,$1,MKWGNIL); }
     ;

clauselist: /* empty */		{ $$ = MKWGNIL; }
	  | clause clauselist	{ $$ = MKWGPAIR(DBPARM,$1, $2); }
	  ;

clause: fact { $$ = $1; }
      | rule { $$ = $1; }
      ;

fact: functionform '.' { $$ = MKWGPAIR(DBPARM,$1, MKWGNIL); }
    ;

functionform: TATOM '(' arguments ')'	{ $$ = MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,$1), $3); }
	    | NOT TATOM '(' arguments ')' { $$ = MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"not"), MKWGPAIR(DBPARM,MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,$2), $4), MKWGNIL)); }
	    | '~' TATOM '(' arguments ')' { $$ = MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"~"), MKWGPAIR(DBPARM,MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,$2), $4), MKWGNIL)); }
	    ;

arguments: argument			{ $$ = MKWGPAIR(DBPARM,$1, MKWGNIL); }
	 | argument ',' arguments	{ $$ = MKWGPAIR(DBPARM,$1, $3); }
	 ;

argument: functionform	{ $$ = $1; }
	| TATOM		{ $$ = MKWGSTRING(DBPARM,$1); }
	| TINT 		{ $$ = MKWGINT(DBPARM,$1); }
	| TFLOAT 	{ $$ = MKWGSTRING(DBPARM,$1); }
	| VAR		{ $$ = MKWGSTRING(DBPARM,$1); }
	;

rule: functionform IS body '.'	{ $$ = MKWGPAIR(DBPARM,$1, $3); }
    ;

body: functionform		{ $$ = MKWGPAIR(DBPARM,MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"not"), MKWGPAIR(DBPARM,$1, MKWGNIL)), MKWGNIL); }
    | functionform ',' body	{ $$ = MKWGPAIR(DBPARM,MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"not"), MKWGPAIR(DBPARM,$1, MKWGNIL)), $3); }
    | functionform ';' body	{ $$ = MKWGPAIR(DBPARM,MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"and"), MKWGPAIR(DBPARM,$1, $3)), MKWGNIL); }
    ;

/*
body: functionform	{ $$ = MKWGPAIR(DBPARM,$1, MKWGNIL); }
    | orlist		{ $$ = $1; }
    | andlist		{ $$ = $1; }
    ;

orlist: 			{ $$ = MKWGNIL; }
      | functionform ',' orlist		{ $$ = MKWGPAIR(DBPARM,$1, $3); }
      | functionform ',' andlist	{ $$ = MKWGPAIR(DBPARM,$1, $3); }
      ;

andlist: 			{ $$ = MKWGNIL; }
      | functionform ';' andlist	{ $$ = MKWGPAIR(DBPARM,$1, $3); }
      | functionform ';' orlist		{ $$ = MKWGPAIR(DBPARM,$1, $3); }
      ;
*/

fact: atomargs '.' { $$ = $1; }
    ;

atomargs: TATOM '(' arguments ')' { $$ = MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,$1), MKWGPAIR(DBPARM,$3, MKWGNIL)); }
	;

rule: TATOM '(' arguments ')' IS body	{$$ = MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,$1), MKWGPAIR(DBPARM,$3, MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,":-"), $6))); }
    ;

/*
arguments: argument			{ $$ = MKWGPAIR(DBPARM,$1, MKWGNIL); }
	 | argument ',' arguments	{ $$ = MKWGPAIR(DBPARM,$1, $3); }
	 ;
*/

body: terms '.'	{ $$ = $1; }
    ;

terms: term	{ $$ = $1; }
     | term ',' terms { $$ = MKWGPAIR(DBPARM,MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"and"), MKWGPAIR(DBPARM,$1, $3)), MKWGNIL); }
     | term ';' terms { $$ = MKWGPAIR(DBPARM,MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"or"), MKWGPAIR(DBPARM,$1, $3)), MKWGNIL); }
     ;

term: TATOM '(' term ')' { $$ = MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,$1), MKWGPAIR(DBPARM,$3, MKWGNIL)); }
    | NOT '(' term ')'	{ $$ = MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"not"), $3); }
    | atomargs	{ $$ = $1; }
    | '!'	{ $$ = MKWGPAIR(DBPARM,MKWGSTRING(DBPARM,"cut"), MKWGNIL); }
    ;

	
%%

