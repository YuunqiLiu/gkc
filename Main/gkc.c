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

 /** @file gkc.c
 *  GKC reasoner: the main command line tool
 */

/* ====== Includes =============== */



#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#endif

#ifdef _WIN32
#include <conio.h> // for _getch
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include "../config-w32.h"
#else
#include "../config.h"
#endif
#include "../Db/dballoc.h"
#include "../Db/dbmem.h"
#include "../Db/dbdata.h"
#include "../Db/dbdump.h"
#include "../Db/dblog.h"
#include "../Db/dbquery.h"
#include "../Db/dbutil.h"
#include "../Db/dblock.h"
#include "../Db/dbjson.h"
#include "../Db/dbschema.h"
#ifdef USE_REASONER
#include "../Builtparser/dbparse.h"
#include "../Reasoner/rmain.h"
#include "../Reasoner/init.h"
//#include "../Db/dbapi.h"
#endif



/* ====== Private defs =========== */

#ifdef _WIN32
#define sscanf sscanf_s  /* XXX: This will break for string parameters */
#endif

#define FLAGS_FORCE 0x1
#define FLAGS_LOGGING 0x2

/* Helper macros for database lock management */

#define RLOCK(d,i) i = wg_start_read(d); \
    if(!i) { \
        fprintf(stderr, "Failed to get database lock\n"); \
        break; \
    }

#define WLOCK(d,i) i = wg_start_write(d); \
    if(!i) { \
        fprintf(stderr, "Failed to get database lock\n"); \
        break; \
    }

#define RULOCK(d,i) if(i) { \
        wg_end_read(d,i); \
        i = 0; \
    }
#define WULOCK(d,i)  if(i) { \
        wg_end_write(d,i); \
        i = 0; \
    }

/* === this should come from dbapi.h ========== */

//void* wg_attach_local_database_with_kb(wg_int size, void* kb);

/* ======= Private protos ================ */

gint parse_shmsize(char *arg);
gint parse_flag(char *arg);
int parse_memmode(char *arg);
wg_query_arg *make_arglist(void *db, char **argv, int argc, int *sz);
void free_arglist(void *db, wg_query_arg *arglist, int sz);
void query(void *db, char **argv, int argc);
void del(void *db, char **argv, int argc);
void selectdata(void *db, int howmany, int startingat);
int add_row(void *db, char **argv, int argc);
wg_json_query_arg *make_json_arglist(void *db, char *json, int *sz,
 void **doc);
void findjson(void *db, char *json);
void print_indexes(void *db, FILE *f);

void wg_set_kb_db(void* db, void* kb);
void segment_stats(void *db);
void wg_show_strhash(void* db);

void wg_show_database(void* db);
void gkc_show_cur_time(void);

#define SHOW_CONTENTS 1
//#undef SHOW_CONTENTS

/* ====== Functions ============== */


/*
how to set 500 meg of shared memory:

su
echo 500000000  > /proc/sys/kernel/shmmax
*/

/** usage: display command line help.
*
*/

void usage(char *prog) {
  printf("usage: gkc <problem file>\n"\
         "or   : gkc -prove <problem file> <strategy file>\n");
/*  
#ifdef USE_REASONER
    printf("    importotter <filename> - import facts/rules from "\
    "otter syntax file.\n"\
    "    importprolog <filename> - import facts/rules from "\
    "prolog syntax file.\n"\
    "    runreasoner - run the reasoner on facts/rules in the database.\n");
#endif
#ifdef HAVE_RAPTOR
  printf("    exportrdf <col> <filename> - export data to a RDF/XML file.\n"\
    "    importrdf <pref> <suff> <filename> - import data from a RDF file.\n");
#endif
#ifdef USE_DBLOG
  printf("    replay <filename> - replay a journal file.\n");
#endif
  printf("    info - print information about the memory database.\n"\
    "    add <value1> .. - store data row (only int or str recognized)\n"\
    "    select <number of rows> [start from] - print db contents.\n"\
    "    query <col> \"<cond>\" <value> .. - basic query.\n"\
    "    del <col> \"<cond>\" <value> .. - like query. Matching rows "\
    "are deleted from database.\n"\
    "    addjson [filename] - store a json document.\n"\
    "    findjson <json> - find documents with matching keys/values.\n"\
    "    createindex <column> - create ttree index\n" \
    "    createhash <columns> - create hash index (JSON support)\n" \
    "    dropindex <index id> - delete an index\n" \
    "    listindex - list all indexes in database\n");
#ifdef _WIN32
  printf("    server [-l] [size] - provide persistent shared memory for "\
    "other processes (-l: enable logging in the database). Will allocate "\
    "requested amount of memory and sleep; "\
    "Ctrl+C aborts and releases the memory.\n");
#else
  printf("    create [-l] [size [mode]] - create empty db of given size "\
    "(-l: enable logging in the database, mode: segment permissions "\
    "(octal)).\n");
#endif
  printf("\nCommands may have variable number of arguments. "\
    "Commands that take values as arguments have limited support "\
    "for parsing various data types (see manual for details). Size "\
    "may be given as bytes or in larger units by appending k, M or G "\
    "to the size argument.\n");
*/    
}

/** Handle the user-supplied database size (or pick a reasonable
*   substitute). Parses up to 32-bit values, but the user may
*   append up to 'G' for larger bases on 64-bit systems.
*/
gint parse_shmsize(char *arg) {
  char *trailing = NULL;
  long maxv = LONG_MAX, mult = 1, val = strtol(arg, &trailing, 10);

  if((val == LONG_MAX || val == LONG_MIN) && errno==ERANGE) {
    fprintf(stderr, "Numeric value out of range (try k, M, G?)\n");
  } else if(trailing) {
    switch(trailing[0]) {
      case 'k':
      case 'K':
        mult = 1000;
        break;
      case 'm':
      case 'M':
        mult = 1000000;
        break;
      case 'g':
      case 'G':
        mult = 1000000000;
        break;
      default:
        break;
    }
  }

#ifndef HAVE_64BIT_GINT
  maxv /= mult;
#endif
  if(val > maxv) {
    fprintf(stderr, "Requested segment size not supported (using %ld)\n",
      mult * maxv);
    val = maxv;
  }

  return (gint) val * (gint) mult;
}

/** Handle a command-line flag
*
*/
gint parse_flag(char *arg) {
  while(arg[0] == '-')
    arg++;
  switch(arg[0]) {
    case 'f':
      return FLAGS_FORCE;
    case 'l':
      return FLAGS_LOGGING;
    default:
      fprintf(stderr, "Unrecognized option: `%c'\n", arg[0]);
      break;
  }
  return 0;
}

/** Parse the mode bits given in octal (textual representation)
 *
 */
int parse_memmode(char *arg) {
  char *trailing = NULL;
  long parsed = strtol(arg, &trailing, 8);
  if(errno == 0 && (!trailing || strlen(trailing) == 0) &&
      parsed <= 0777 && parsed > 0) {
    return (int) parsed;
  }
  return 0;
}

/** top level for the database command line tool
*
*
*/

int main(int argc, char **argv) {

  char *shmname = NULL;
  void *shmptr = NULL;
  void *shmptrlocal = NULL;
  int i, scan_to;
  gint shmsize, shmsize2;
  wg_int rlock = 0;
  wg_int wlock = 0;
  int islocaldb=0; // lreasoner sets to 1 to avoid detaching db at the end

  /* look for commands in argv[1] or argv[2] */
  
  if(argc < 3) scan_to = argc;
  else scan_to = 3;
  shmsize = 0; /* 0 size causes default size to be used */

  /* 1st loop through, shmname is NULL for default. If
   * the first argument is not a recognizable command, it
   * is assumed to be the shmname and the next argument
   * is checked against known commands.
   */
#ifdef _WIN32  
  shmname="8000";
#endif  

  //printf("\nargc %d argv[0] %s scan_to %d\n",argc,argv[0],scan_to);
  for(i=1; i<scan_to; i++) {
    if (!strcmp(argv[i],"help") || !strcmp(argv[i],"-h")) {
      usage(argv[0]);
      exit(0);
    }
    if (!strcmp(argv[i],"version") || !strcmp(argv[i],"-v")) {
      wg_print_code_version();
      exit(0);
    }
    if (!strcmp(argv[i],"free")) {
      /* free shared memory */
      wg_delete_database(shmname);
      exit(0);
    }
    if(argc>(i+1) && !strcmp(argv[i],"import")){
      wg_int err, minsize, maxsize;
      int flags = 0;

      if(argv[i+1][0] == '-') {
        flags = parse_flag(argv[++i]);
        if(argc<=(i+1)) {
          /* Filename argument missing */
          usage(argv[0]);
          exit(1);
        }
      }

      err = wg_check_dump(NULL, argv[i+1], &minsize, &maxsize);
      if(err) {
        fprintf(stderr, "Import failed.\n");
        break;
      }

      shmptr=wg_attach_memsegment(shmname, minsize, maxsize, 1,
        (flags & FLAGS_LOGGING), 0);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }

      /* Locking is handled internally by the dbdump.c functions */
      err = wg_import_dump(shmptr,argv[i+1]);
      if(!err)
        printf("Database imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error in wg_import_dump, db may have"\
          " become corrupt\n");
      else
        fprintf(stderr, "Import failed.\n");
      break;
    }
    else if(argc>(i+1) && !strcmp(argv[i],"export")){
      wg_int err;
      int flags = 0;

      if(argv[i+1][0] == '-') {
        flags = parse_flag(argv[++i]);
        if(argc<=(i+1)) {
          /* Filename argument missing */
          usage(argv[0]);
          exit(1);
        }
      }

      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }

      /* Locking is handled internally by the dbdump.c functions */
      if(flags & FLAGS_FORCE)
        err = wg_dump_internal(shmptr,argv[i+1], 0);
      else
        err = wg_dump(shmptr,argv[i+1]);

      if(err<-1)
        fprintf(stderr, "Fatal error in wg_dump, db may have"\
          " become corrupt\n");
      else if(err)
        fprintf(stderr, "Export failed.\n");
      break;
    }
#ifdef USE_DBLOG
    else if(argc>(i+1) && !strcmp(argv[i],"replay")){
      wg_int err;

      shmptr=wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }

      WLOCK(shmptr, wlock);
      err = wg_replay_log(shmptr,argv[i+1]);
      WULOCK(shmptr, wlock);
      if(!err)
        printf("Log suggessfully imported from file.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing, database may have "\
          "become corrupt\n");
      else
        fprintf(stderr, "Failed to import log (database unmodified).\n");
      break;
    }
#endif
    else if(argc>(i+1) && !strcmp(argv[i],"exportcsv")){
      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }

      RLOCK(shmptr, rlock);
      wg_export_db_csv(shmptr,argv[i+1]);
      RULOCK(shmptr, rlock);
      break;
    }
    else if(argc>(i+1) && !strcmp(argv[i],"importcsv")){
      wg_int err;

      shmptr=wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }

      WLOCK(shmptr, wlock);
      err = wg_import_db_csv(shmptr,argv[i+1]);
      WULOCK(shmptr, wlock);
      if(!err)
        printf("Data imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");
      break;
    }

#ifdef USE_REASONER
    else if(argc>(i+1) && !strcmp(argv[i],"importprolog")){
      wg_int err;
       
      shmptr=wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      err = wg_import_prolog_file(shmptr,argv[i+1]);
      if(!err)
        printf("Data imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");
      break;
    }
    else if(argc>(i+1) && !strcmp(argv[i],"importotter")){
      wg_int err;
#ifdef _WIN32
      shmsize=1000000000; // 2000 meg
#else
      shmsize=2000000000; 
#endif      
      printf("\nto import otter\n");
      gkc_show_cur_time();
      shmptr=wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      printf("\nto wg_import_otter_file\n");
      gkc_show_cur_time();
      err = wg_import_otter_file(shmptr,argv[i+1],1);
      printf("\nexited wg_import_otter_file\n");
      gkc_show_cur_time();
      if(!err)
        printf("Data imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing otter file, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");


      wg_show_database(shmptr);

      break;

    }
     else if(argc>(i+1) && !strcmp(argv[i],"-readkb")){
      wg_int err;
#ifdef _WIN32
      shmsize=1000000000; // 2000 meg
#else
      shmsize=2000000000; 
#endif      
      printf("\nto -readkb");
      gkc_show_cur_time();

      wg_delete_database(shmname);
      printf("\nprevious memory database deleted\n");
      gkc_show_cur_time();

      shmptr=wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      printf("\nto wg_import_otter_file %s\n",argv[i+1]);
      gkc_show_cur_time();
      err = wg_import_otter_file(shmptr,argv[i+1],1);
      printf("\nexited wg_import_otter_file\n");
      gkc_show_cur_time();
      if(!err)
        printf("Data imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing otter file, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");

      //wg_show_database(shmptr);

      printf("\nstarting to init_shared_database\n");      
      gkc_show_cur_time();

      init_shared_database(shmptr);

      printf("\nexited init_shared_database\n");      
      gkc_show_cur_time();

      //wg_show_database(shmptr);

      //(((glb*)(offsettoptr(shmptr,(dbmemsegh(shmptr)->rglb))))->stat_built_cl)=10;
      (db_rglb(shmptr)->stat_built_cl)=25;
      //wr_show_stats(offsettoptr(shmptr,(dbmemsegh(shmptr)->rglb)));
      //wr_show_stats(db_rglb(shmptr),1);

      wr_show_database_details(NULL,shmptr,"shared db");

      break;


    } else if(argc>i && (!strcmp(argv[i],"runreasoner"))) {
      //wg_int err;

      printf("\nto wg_attach_existing_database\n");
      gkc_show_cur_time(); 
      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      //printf("about to call wg_run_reasoner\n");
      wg_show_database(shmptr);
      printf("\nto wg_run_reasoner\n");
      gkc_show_cur_time();
      //err = 
      wg_run_reasoner(shmptr,argc,argv);
      //if(!err);
        //printf("wg_run_reasoner finished ok.\n");
      //else
        //fprintf(stderr, "wg_run_reasoner finished with an error %d.\n",err);
      //break;
      wg_show_database(shmptr);
      break;

    } else if(argc>i && !strcmp(argv[i],"qrun")){
      wg_int err;

      printf("\nqrun starts\n");
      gkc_show_cur_time();
      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      printf("\ndb attached, showing attached shared memory db\n");
      //printf("about to call wg_run_reasoner\n");
      wg_show_database(shmptr);

      // --- create a new temporary local db ---
      shmsize2=100000000;
      printf("\nto wg_attach_local_database_with_kb\n");
      gkc_show_cur_time();
      shmptrlocal=(void*)(wg_attach_local_database_with_kb(shmsize2,(void*)shmptr));
      if(!shmptrlocal) {
        fprintf(stderr, "Failed to attach local database.\n");
        exit(1);
      }
      //wg_set_kb_db(shmptrlocal,shmptr); // set the kb field of local db to shared db    
      islocaldb=1;
      err=0;
      printf("\nto wg_import_otter_file\n");
      gkc_show_cur_time();
      err = wg_import_otter_file(shmptrlocal,argv[i+1],0);
      if(!err)
        printf("Data imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing otter file, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");      
      printf("\nshowing local db\n");  
      wg_show_database(shmptrlocal);
      // ---- local db created ------

      printf("\nto call wg_run_reasoner\n");
      gkc_show_cur_time();
      err = wg_run_reasoner(shmptr,argc,argv);
      //if(!err);
        //printf("wg_run_reasoner finished ok.\n");
      //else
        //fprintf(stderr, "wg_run_reasoner finished with an error %d.\n",err);
      //break;
      printf("\nwg_run_reasoner returned\n");
      gkc_show_cur_time();
      printf("\nshowing shared memory db\n"); 
      wg_show_database(shmptr);
      printf("\nqrun exits\n");
      break;

    } else if(argc>i && !strcmp(argv[i],"-querykb")){
      wg_int err;

      printf("\n-querykb starts with external shmname %s\n",shmname);
      gkc_show_cur_time();
      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      /*
      printf("\ndb attached, showing attached shared memory db shmptr %ld\n",
        (unsigned long int)((gint)shmptr));
      gkc_show_cur_time();
      //printf("about to call wg_run_reasoner\n");
      wr_show_database_details(NULL,shmptr,"shmptr");
      */ 
      //exit(0); 
      // --- create a new temporary local db ---
      shmsize2=100000000;     
      printf("\nto wg_attach_local_database_with_kb with shmptr %ld\n",
        (unsigned long int)((gint)shmptr));
      gkc_show_cur_time();
      shmptrlocal=wg_attach_local_database_with_kb(shmsize2,(void*)shmptr);
 
      /*
      printf("\nto show headers right after return from attach:\n");
      printf("\nshmptrlocal is %lx and gint %ld\n",
        (unsigned long int)shmptrlocal,(gint)shmptrlocal);
      wr_show_database_headers(shmptrlocal);
      */

      //shmptrlocal=wg_attach_local_database(shmsize2);
      printf("\nshmptrlocal is %lx and gint %ld\n",
        (unsigned long int)shmptrlocal,(gint)shmptrlocal);
      gkc_show_cur_time();
      //shmptrlocal=wg_attach_local_database(shmsize2);
      if(!shmptrlocal) {
        fprintf(stderr, "Failed to attach local database.\n");
        exit(1);
      }
      //wg_set_kb_db(shmptrlocal,shmptr); // set the kb field of local db to shared db    
      islocaldb=1;
      err=0;
      //wr_show_database_headers(shmptrlocal);

      //wr_show_database_details(NULL,shmptrlocal,"shmptrlocal");
      // exit(0);

      printf("\nqrun1 to wg_import_otter_file from argv[i+1] %s\n",argv[i+1]);
      gkc_show_cur_time();
      err = wg_import_otter_file(shmptrlocal,argv[i+1],0);
      if(!err)
        printf("Data imported from %s.\n",argv[i+1]);
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing otter file, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");      
      //printf("\nshowing local db\n");  
      //gkc_show_cur_time();
      
      //wr_show_database_headers(shmptrlocal);
      //wr_show_database_details(NULL,shmptrlocal,"shmptrlocal");

      
      // ---- local db created ------

      printf("\nto call wg_run_reasoner\n");
      gkc_show_cur_time();
      err = wg_run_reasoner(shmptrlocal,argc,argv);
      //if(!err);
        //printf("wg_run_reasoner finished ok.\n");
      //else
        //fprintf(stderr, "wg_run_reasoner finished with an error %d.\n",err);
      //break;
      printf("\nwg_run_reasoner returned\n");
      gkc_show_cur_time();


      //printf("\nshowing shared memory db\n"); 
      //wr_show_database_details(NULL,shmptr,"shmptr");
      printf("\n-querykb exits\n");
      break;  

    } else if(argc>i && 
                  (!strcmp(argv[i],"lrunreasoner") ||
                   !strcmp(argv[i],"-prove")))  {
      wg_int err;
#ifdef _WIN32
      shmsize=1000000000; // 2000 meg
#else
      shmsize=2000000000; 
#endif
      //shmsize=20000000;
      shmptr=wg_attach_local_database(shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach local database.\n");
        exit(1);
      }
      islocaldb=1;
      err = wg_import_otter_file(shmptr,argv[i+1],0);
      if(!err)
        printf("Data imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing otter file, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");      
      //wg_show_database(shmptr);
      //printf("about to call wg_run_reasoner\n");
      err = wg_run_reasoner(shmptr,argc,argv);
      //wg_show_database(shmptr);
      //if(!err);
        //printf("wg_run_reasoner finished ok.\n");
      //else
        //fprintf(stderr, "wg_run_reasoner finished with an error %d.\n",err);
      //break;
      break;
    } else if(i==1 && argc==2) {
      wg_int err;
#ifdef _WIN32
      shmsize=1000000000; // 2000 meg
#else
      shmsize=2000000000; 
#endif
      //shmsize=20000000;
      shmptr=wg_attach_local_database(shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach local database.\n");
        exit(1);
      }
      islocaldb=1;
      err = wg_import_otter_file(shmptr,argv[1],0);
      if(!err)
        printf("Data imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing otter file, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");      
      //wg_show_database(shmptr);
      //printf("about to call wg_run_reasoner\n");
      err = wg_run_reasoner(shmptr,argc,argv);
      //wg_show_database(shmptr);
      //if(!err);
        //printf("wg_run_reasoner finished ok.\n");
      //else
        //fprintf(stderr, "wg_run_reasoner finished with an error %d.\n",err);
      //break;
      break;
    }

#endif

#ifdef HAVE_RAPTOR
    else if(argc>(i+2) && !strcmp(argv[i],"exportrdf")){
      wg_int err;
      int pref_fields = atol(argv[i+1]);

      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }

      printf("Exporting with %d prefix fields.\n", pref_fields);
      RLOCK(shmptr, rlock);
      err = wg_export_raptor_rdfxml_file(shmptr, pref_fields, argv[i+2]);
      RULOCK(shmptr, rlock);
      if(err)
        fprintf(stderr, "Export failed.\n");
      break;
    }
    else if(argc>(i+3) && !strcmp(argv[i],"importrdf")){
      wg_int err;
      int pref_fields = atol(argv[i+1]);
      int suff_fields = atol(argv[i+2]);

      shmptr=wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }

      printf("Importing with %d prefix fields, %d suffix fields.\n,",
        pref_fields, suff_fields);
      WLOCK(shmptr, wlock);
      err = wg_import_raptor_file(shmptr, pref_fields, suff_fields,
        wg_rdfparse_default_callback, argv[i+3]);
      WULOCK(shmptr, wlock);
      if(!err)
        printf("Data imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");
      break;
    }
#endif
    else if(!strcmp(argv[i], "info")) {
      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      RLOCK(shmptr, rlock);
      segment_stats(shmptr);
      RULOCK(shmptr, rlock);
      break;
    }
#ifdef _WIN32
    else if(!strcmp(argv[i],"server")) {
      int flags = 0;
      if(argc>(i+1) && argv[i+1][0] == '-') {
        flags = parse_flag(argv[++i]);
      }

      if(argc>(i+1)) {
        shmsize = parse_shmsize(argv[i+1]);
        if(!shmsize)
          fprintf(stderr, "Failed to parse memory size, using default.\n");
      }
      shmptr=wg_attach_memsegment(shmname, shmsize, shmsize, 1,
        (flags & FLAGS_LOGGING), 0);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      printf("Press Ctrl-C to end and release the memory.\n");
      while(_getch() != 3);
      break;
    }
#else
    else if(!strcmp(argv[i],"create")) {
      int flags = 0, mode = 0;
      if(argc>(i+1) && argv[i+1][0] == '-') {
        flags = parse_flag(argv[++i]);
      }

      if(argc>(i+1)) {
        shmsize = parse_shmsize(argv[i+1]);
        if(!shmsize)
          fprintf(stderr, "Failed to parse memory size, using default.\n");
      }
      if(argc>(i+2)) {
        mode = parse_memmode(argv[i+2]);
        if(mode == 0)
          fprintf(stderr, "Invalid permission mode, using default.\n");
      }
      shmptr=wg_attach_memsegment(shmname, shmsize, shmsize, 1,
        (flags & FLAGS_LOGGING), mode);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      break;
    }
#endif
    else if(argc>(i+1) && !strcmp(argv[i],"select")) {
      int rows = atol(argv[i+1]);
      int from = 0;

      if(!rows) {
        fprintf(stderr, "Invalid number of rows.\n");
        exit(1);
      }
      if(argc > (i+2))
        from = atol(argv[i+2]);

      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      RLOCK(shmptr, rlock);
      selectdata(shmptr, rows, from);
      RULOCK(shmptr, rlock);
      break;
    }
    else if(argc>(i+1) && !strcmp(argv[i],"add")) {
      int err;
      shmptr=wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      WLOCK(shmptr, wlock);
      err = add_row(shmptr, argv+i+1, argc-i-1);
      WULOCK(shmptr, wlock);
      if(!err)
        printf("Row added.\n");
      break;
    }
    else if(argc>(i+2) && !strcmp(argv[i],"del")) {
      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      /* Delete works like query(), except deletes the matching rows */
      del(shmptr, argv+i+1, argc-i-1);
      break;
      break;
    }
    else if(argc>(i+3) && !strcmp(argv[i],"query")) {
      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      /* Query handles it's own locking */
      query(shmptr, argv+i+1, argc-i-1);
      break;
    }
    else if(argc>i && !strcmp(argv[i],"addjson")){
      wg_int err;

      shmptr=wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }

      WLOCK(shmptr, wlock);
      /* the filename parameter is optional */
      err = wg_parse_json_file(shmptr, (argc>(i+1) ? argv[i+1] : NULL));
      WULOCK(shmptr, wlock);
      if(!err)
        printf("JSON document imported.\n");
      else if(err<-1)
        fprintf(stderr, "Fatal error when importing, data may be partially"\
          " imported\n");
      else
        fprintf(stderr, "Import failed.\n");
      break;
    }
    else if(argc>(i+1) && !strcmp(argv[i],"findjson")) {
      shmptr=wg_attach_existing_database(shmname);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      WLOCK(shmptr, wlock);
      findjson(shmptr, argv[i+1]);
      WULOCK(shmptr, wlock);
      break;
    }
    else if(argc>(i+1) && !strcmp(argv[i], "createindex")) {
      int col;
      shmptr = (void *) wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      sscanf(argv[i+1], "%d", &col);
      WLOCK(shmptr, wlock);
      wg_create_index(shmptr, col, WG_INDEX_TYPE_TTREE, NULL, 0);
      WULOCK(shmptr, wlock);
      break;
    }
    else if(argc>(i+1) && !strcmp(argv[i], "createhash")) {
      gint cols[MAX_INDEX_FIELDS], col_count, j;
      shmptr = (void *) wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      col_count = 0;
      for(j = i+1; j<argc; j++) {
        int col;
        sscanf(argv[j], "%d", &col);
        cols[col_count++] = col;
      }
      WLOCK(shmptr, wlock);
      wg_create_multi_index(shmptr, cols, col_count,
        WG_INDEX_TYPE_HASH_JSON, NULL, 0);
      WULOCK(shmptr, wlock);
      break;
    }
    else if(argc>(i+1) && !strcmp(argv[i], "dropindex")) {
      int index_id;
      shmptr = (void *) wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      sscanf(argv[i+1], "%d", &index_id);
      WLOCK(shmptr, wlock);
      if(wg_drop_index(shmptr, index_id))
        fprintf(stderr, "Failed to drop index.\n");
      else
        printf("Index dropped.\n");
      WULOCK(shmptr, wlock);
      break;
    }
    else if(!strcmp(argv[i], "listindex")) {
      shmptr = (void *) wg_attach_database(shmname, shmsize);
      if(!shmptr) {
        fprintf(stderr, "Failed to attach to database.\n");
        exit(1);
      }
      RLOCK(shmptr, rlock);
      print_indexes(shmptr, stdout);
      RULOCK(shmptr, rlock);
      break;
    }

    shmname = argv[1]; /* no match, assume shmname was given */
  }

  if(i==scan_to) {
    /* loop completed normally ==> no commands found */
    usage(argv[0]);
  }
  if(shmptr && !islocaldb) {
    RULOCK(shmptr, rlock);
    WULOCK(shmptr, wlock);
    wg_detach_database(shmptr);
  }
  exit(0);
}

/** Parse row matching parameters from the command line
 *
 *  argv should point to the part in argument list where the
 *  parameters start.
 *
 *  If the parsing is successful, *sz holds the size of the argument list.
 *  Otherwise that value should be ignored; the return value of the
 *  function should be used to check for success.
 */
wg_query_arg *make_arglist(void *db, char **argv, int argc, int *sz) {
  int c, i, j, qargc;
  char cond[80];
  wg_query_arg *arglist;
  gint encoded;

  qargc = argc / 3;
  *sz = qargc;
  arglist = (wg_query_arg *) malloc(qargc * sizeof(wg_query_arg));
  if(!arglist)
    return NULL;

  for(i=0,j=0; i<qargc; i++) {
    arglist[i].value = WG_ILLEGAL;
  }

  for(i=0,j=0; i<qargc; i++) {
    int cnt = 0;
    cnt += sscanf(argv[j++], "%d", &c);
    cnt += sscanf(argv[j++], "%79s", cond);
    encoded = wg_parse_and_encode_param(db, argv[j++]);

    if(cnt!=2 || encoded==WG_ILLEGAL) {
      fprintf(stderr, "failed to parse query parameters\n");
      free_arglist(db, arglist, qargc);
      return NULL;
    }

    arglist[i].column = c;
    arglist[i].value = encoded;
    if(!strncmp(cond, "=", 1))
        arglist[i].cond = WG_COND_EQUAL;
    else if(!strncmp(cond, "!=", 2))
        arglist[i].cond = WG_COND_NOT_EQUAL;
    else if(!strncmp(cond, "<=", 2))
        arglist[i].cond = WG_COND_LTEQUAL;
    else if(!strncmp(cond, ">=", 2))
        arglist[i].cond = WG_COND_GTEQUAL;
    else if(!strncmp(cond, "<", 1))
        arglist[i].cond = WG_COND_LESSTHAN;
    else if(!strncmp(cond, ">", 1))
        arglist[i].cond = WG_COND_GREATER;
    else {
      fprintf(stderr, "invalid condition %s\n", cond);
      free_arglist(db, arglist, qargc);
      return NULL;
    }
  }

  return arglist;
}

/** Free the argument list created by make_arglist()
 *
 */
void free_arglist(void *db, wg_query_arg *arglist, int sz) {
  if(arglist) {
    int i;
    for(i=0; i<sz; i++) {
      if(arglist[i].value != WG_ILLEGAL) {
        wg_free_query_param(db, arglist[i].value);
      }
    }
    free(arglist);
  }
}

/** Basic query functionality
 */
void query(void *db, char **argv, int argc) {
  int qargc;
  void *rec = NULL;
  wg_query *q;
  wg_query_arg *arglist;
  gint lock_id;

  arglist = make_arglist(db, argv, argc, &qargc);
  if(!arglist)
    return;

  if(!(lock_id = wg_start_read(db))) {
    fprintf(stderr, "failed to get lock on database\n");
    goto abrt1;
  }

  q = wg_make_query(db, NULL, 0, arglist, qargc);
  if(!q)
    goto abrt2;

/*  printf("query col: %d type: %d\n", q->column, q->qtype); */
  rec = wg_fetch(db, q);
  while(rec) {
    wg_print_record(db, (gint *) rec);
    printf("\n");
    rec = wg_fetch(db, q);
  }

  wg_free_query(db, q);
abrt2:
  wg_end_read(db, lock_id);
abrt1:
  free_arglist(db, arglist, qargc);
}

/** Delete rows
 *  Like query(), except the selected rows are deleted.
 */
void del(void *db, char **argv, int argc) {
  int qargc;
  void *rec = NULL;
  wg_query *q;
  wg_query_arg *arglist;
  gint lock_id;

  arglist = make_arglist(db, argv, argc, &qargc);
  if(!arglist)
    return;

  /* Use maximum isolation */
  if(!(lock_id = wg_start_write(db))) {
    fprintf(stderr, "failed to get lock on database\n");
    goto abrt1;
  }

  q = wg_make_query(db, NULL, 0, arglist, qargc);
  if(!q)
    goto abrt2;

  if(q->res_count > 0) {
    printf("Deleting %d rows...", (int) q->res_count);
    rec = wg_fetch(db, q);
    while(rec) {
      wg_delete_record(db, (gint *) rec);
      rec = wg_fetch(db, q);
    }
    printf(" done\n");
  }

  wg_free_query(db, q);
abrt2:
  wg_end_write(db, lock_id);
abrt1:
  free_arglist(db, arglist, qargc);
}

/** Print rows from database
 *
 */
void selectdata(void *db, int howmany, int startingat) {

  void *rec = wg_get_first_record(db);
  int i, count;

  for(i=0;i<startingat;i++){
    if(rec == NULL) return;
    rec=wg_get_next_record(db,rec);
  }

  count=0;
  while(rec != NULL) {
    wg_print_record(db, (gint *) rec);
    printf("\n");
    count++;
    if(count == howmany) break;
    rec=wg_get_next_record(db,rec);
  }

  return;
}

/** Add one row of data in database.
 *
 */
int add_row(void *db, char **argv, int argc) {
  int i;
  void *rec;
  gint encoded;

  rec = wg_create_record(db, argc);
  if (rec==NULL) {
    fprintf(stderr, "Record creation error\n");
    return -1;
  }
  for(i=0; i<argc; i++) {
    encoded = wg_parse_and_encode(db, argv[i]);
    if(encoded == WG_ILLEGAL) {
      fprintf(stderr, "Parsing or encoding error\n");
      return -1;
    }
    wg_set_field(db, rec, i, encoded);
  }

  return 0;
}

/** Parse a JSON argument from command line.
 *  *sz contains the arglist size
 *  *doc contains the parsed JSON document, to be deleted later
 */
wg_json_query_arg *make_json_arglist(void *db, char *json, int *sz,
 void **doc) {
  wg_json_query_arg *arglist;
  void *document;
  gint i, reclen;

  if(wg_check_json(db, json) || wg_parse_json_param(db, json, &document))
    return NULL;

  if(!is_schema_object(document)) {
    fprintf(stderr, "Invalid input JSON (must be an object)\n");
    wg_delete_document(db, document);
    return NULL;
  }

  reclen = wg_get_record_len(db, document);
  arglist = malloc(sizeof(wg_json_query_arg) * reclen);
  if(!arglist) {
    fprintf(stderr, "Failed to allocate memory\n");
    wg_delete_document(db, document);
    return NULL;
  }

  for(i=0; i<reclen; i++) {
    void *rec = wg_decode_record(db, wg_get_field(db, document, i));
    gint key = wg_get_field(db, rec, WG_SCHEMA_KEY_OFFSET);
    gint value = wg_get_field(db, rec, WG_SCHEMA_VALUE_OFFSET);
    if(key == WG_ILLEGAL || value == WG_ILLEGAL) {
      free(arglist);
      wg_delete_document(db, document);
      return NULL;
    }
    arglist[i].key = key;
    arglist[i].value = value;
  }

  *sz = reclen;
  *doc = document;
  return arglist;
}

/** JSON query
 */
void findjson(void *db, char *json) {
  int qargc;
  void *rec = NULL, *document = NULL;
  wg_query *q;
  wg_json_query_arg *arglist;

  arglist = make_json_arglist(db, json, &qargc, &document);
  if(!arglist)
    return;

  q = wg_make_json_query(db, arglist, qargc);
  if(!q)
    goto abort;

  rec = wg_fetch(db, q);
  while(rec) {
    wg_print_json_document(db, NULL, NULL, rec);
    printf("\n");
    rec = wg_fetch(db, q);
  }

  wg_free_query(db, q);
abort:
  free(arglist);
  if(document) {
    wg_delete_document(db, document);
  }
}

/** Print information about the memory database.
 */
void segment_stats(void *db) {
  char buf1[200], buf2[40];
#ifndef _WIN32
  //struct passwd *pwd;
  //struct group *grp;
#endif
  db_memsegment_header *dbh = dbmemsegh(db);

  printf("database key: %d\n", (int) dbh->key);
  printf("database version: ");
  wg_print_header_version(dbh, 0);
  wg_pretty_print_memsize(dbh->size, buf1, 40);
  wg_pretty_print_memsize(dbh->size - dbh->free, buf2, 40);
  printf("free space: %s (of %s)\n", buf2, buf1);
#ifndef _WIN32
  /*
  pwd = getpwuid(wg_memowner(db));
  if(pwd) {
    printf("owner: %s\n", pwd->pw_name);
  }
  grp = getgrgid(wg_memgroup(db));
  if(grp) {
    printf("group: %s\n", grp->gr_name);
  }
  printf("permissions: %o\n", wg_memmode(db));
  */
#endif
#ifdef USE_DBLOG
  if(dbh->logging.active) {
    wg_journal_filename(db, buf1, 200);
    printf("logging is active, journal file: %s\n", buf1);
  } else {
    printf("logging is not active\n");
  }
#endif
  printf("database has ");
  switch(dbh->index_control_area_header.number_of_indexes) {
    case 0:
      printf("no indexes\n");
      break;
    case 1:
      printf("1 index\n");
      break;
    default:
      printf("%d indexes\n",
        (int) dbh->index_control_area_header.number_of_indexes);
      break;
  }
}

void print_indexes(void *db, FILE *f) {
  int column;
  db_memsegment_header* dbh = dbmemsegh(db);
  gint *ilist;

  if(!dbh->index_control_area_header.number_of_indexes) {
    fprintf(f, "No indexes in the database.\n");
    return;
  }
  else {
    fprintf(f, "col\ttype\tmulti\tid\tmask\n");
  }

  for(column=0; column<=MAX_INDEXED_FIELDNR; column++) {
    ilist = &dbh->index_control_area_header.index_table[column];
    while(*ilist) {
      gcell *ilistelem = (gcell *) offsettoptr(db, *ilist);
      if(ilistelem->car) {
        char typestr[3];
        wg_index_header *hdr = \
          (wg_index_header *) offsettoptr(db, ilistelem->car);
        typestr[2] = '\0';
        switch(hdr->type) {
          case WG_INDEX_TYPE_TTREE:
            typestr[0] = 'T';
            typestr[1] = '\0';
            break;
          case WG_INDEX_TYPE_TTREE_JSON:
            typestr[0] = 'T';
            typestr[1] = 'J';
            break;
          case WG_INDEX_TYPE_HASH:
            typestr[0] = '#';
            typestr[1] = '\0';
            break;
          case WG_INDEX_TYPE_HASH_JSON:
            typestr[0] = '#';
            typestr[1] = 'J';
            break;
          default:
            break;
        }
        fprintf(f, "%d\t%s\t%d\t%d\t%s\n",
          column,
          typestr,
          (int) hdr->fields,
          (int) ilistelem->car,
#ifndef USE_INDEX_TEMPLATE
          "-");
#else
          (hdr->template_offset ? "Y" : "N"));
#endif
      }
      ilist = &ilistelem->cdr;
    }
  }
}

/*



void wg_set_kb_db(void* db, void* kb) {
  db_memsegment_header* dbh = dbmemsegh(db);
  dbh->kb_db=kb;
}
*/
// ============ showing database contents for development ============

void wg_show_database(void* db) {
  printf("\nwg_show_database called\n");

  segment_stats(db);
  //wg_show_strhash(db);

  printf("\nwg_show_database exited\n");
}


void wg_show_strhash(void* db) {
  db_memsegment_header* dbh = dbmemsegh(db);
  gint i;
  gint hashchain;
  // lasthashchain;
  gint type;
  //gint offset;
  //gint refc;
  //int encoffset;

  printf("\nshowing strhash table and buckets\n");
  printf("-----------------------------------\n");
  printf("configured strhash size %d (%% of db size)\n",STRHASH_SIZE);
  printf("size %d\n", (int) (dbh->strhash_area_header).size);
  printf("offset %d\n", (int) (dbh->strhash_area_header).offset);
  printf("arraystart %d\n", (int) (dbh->strhash_area_header).arraystart);
  printf("arraylength %d\n", (int) (dbh->strhash_area_header).arraylength);
  printf("nonempty hash buckets:\n");
  for(i=0;i<(dbh->strhash_area_header).arraylength;i++) {
    hashchain=dbfetch(db,(dbh->strhash_area_header).arraystart+(sizeof(gint)*i));
    //lasthashchain=hashchain;   
    if (hashchain!=0) {
      printf("%d: contains %d encoded offset to chain\n",
        (int) i, (int) hashchain);
      for(;hashchain!=0;
          hashchain=dbfetch(db,decode_longstr_offset(hashchain)+LONGSTR_HASHCHAIN_POS*sizeof(gint))) {
#ifdef SHOW_CONTENTS             
          printf("hashchain %ld decode_longstr_offset(hashchain) %ld fulladr %ld contents %ld\n",
                 hashchain,
                 decode_longstr_offset(hashchain),
                 (decode_longstr_offset(hashchain)+LONGSTR_HASHCHAIN_POS*sizeof(gint)),
                 dbfetch(db,decode_longstr_offset(hashchain)+LONGSTR_HASHCHAIN_POS*sizeof(gint)));
#endif          
          type=wg_get_encoded_type(db,hashchain);
          printf("  ");
#ifdef SHOW_CONTENTS                    
          //wg_debug_print_value(db,hashchain);
          printf("\n");
          printf("  type %s",wg_get_type_name(db,type));
#endif          
          if (type==WG_BLOBTYPE) {
            //printf(" len %d\n",wg_decode_str_len(db,hashchain));
          } else if (type==WG_STRTYPE || type==WG_XMLLITERALTYPE ||
                     type==WG_URITYPE || type== WG_ANONCONSTTYPE) {
          } else {
            printf("ERROR: wrong type in strhash bucket\n");
            exit(0);
          }
          //lasthashchain=hashchain;
      }
    }
  }
}


void gkc_show_cur_time(void)  {
  time_t rawtime;
  clock_t curclock;
  struct tm * timeinfo;
  float run_seconds;

  time (&rawtime);
  timeinfo = localtime ( &rawtime );
  curclock=clock();
  run_seconds = (float)(curclock) / CLOCKS_PER_SEC;
  printf("run_seconds %f time %s\n", run_seconds,asctime (timeinfo));
}



#ifdef __cplusplus
}
#endif
