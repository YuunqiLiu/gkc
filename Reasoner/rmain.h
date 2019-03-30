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


 /** @file rmain.h
 *  Reasoner top level: initialisation and startup.
 *
 */


#ifndef DEFINED_RMAIN_H
#define DEFINED_RMAIN_H


#include "glb.h" 

int wg_run_reasoner(void *db, int argc, char **argv);
int wg_import_otter_file(void *db, char* filename, int iskb);
int wg_import_prolog_file(void *db, char* filename);
glb* wg_init_reasoner(void *db, int argc, char **argv);
int wr_init_active_passive_lists_from_all(glb* g);
int wr_init_active_passive_lists_from_one(glb* g, void* db, void* kb_db);
void wr_show_stats(glb* g, int show_local_complex);

void wr_set_detailed_printout(glb* g);

#endif
