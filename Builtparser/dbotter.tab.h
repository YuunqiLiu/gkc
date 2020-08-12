/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_WG_YYOTTER_PARSER_DBOTTER_TAB_H_INCLUDED
# define YY_WG_YYOTTER_PARSER_DBOTTER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int wg_yyotterdebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    VARIABLE = 258,
    FILEEND = 259,
    TINT = 260,
    TFLOAT = 261,
    DATE = 262,
    TIME = 263,
    STRING = 264,
    ID = 265,
    URI = 266,
    TCONST = 267,
    VAR = 268,
    NEQ = 269,
    EQ = 270,
    IMP = 271,
    AND = 272,
    EQV = 273,
    NEQV = 274,
    RIMP = 275,
    NOR = 276,
    NAND = 277,
    AT = 278,
    EMPTYSTRING = 279
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int wg_yyotterparse (parse_parm *parm, void *scanner);

#endif /* !YY_WG_YYOTTER_PARSER_DBOTTER_TAB_H_INCLUDED  */
