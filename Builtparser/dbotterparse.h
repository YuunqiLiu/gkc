/*
* $Id:  $
* $Version: $
*
* Copyright (c) Tanel Tammet 2004,2005,2006,2007,2008,2009,2010
*
* Contact: tanel.tammet@gmail.com                 
*
* This file is part of WhiteDB
*
* WhiteDB is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* WhiteDB is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with WhiteDB.  If not, see <http://www.gnu.org/licenses/>.
*
*/

 /** @file dbotterparse.h
 *  Special defs and headers for otter parser
 *
 */

#ifndef DEFINED_DBOTTERPARSE_H
#define DEFINED_DBOTTERPARSE_H

#include "dbgenparse.h"

int     wg_yyotterlex(YYSTYPE *, void *);
int     wg_yyotterlex_init(void **);
int     wg_yyotterlex_destroy(void *);
void    wg_yyotterset_extra(YY_EXTRA_TYPE, void *);
int     wg_yyotterparse(parse_parm *, void *);   

void wg_yyottererror (parse_parm* parm, void* scanner, const char* msg);

#endif
