 
//	File	:  /adm/daemons/purge.c
//	Creator	:  Watcher@TMI-2  (5/93)
//
//	This daemon allows admins to delete user characters that are
//	older than the inputed integer time value. Wizards can be 
//	deleted by defining flag = 1, and both the wizard and its
//	directory by defining flag = 2. If test if true, the directories
//	and user files will not be deleted, only flagged.

#include <uid.h>
#include <config.h>
#include <daemons.h>
#include <mudlib.h>

// Purges work in the following way. Every subdirectory of P_DATA_DIR is
// checked in succession. Every file in those subdirectories is checked
// to see if it's a valid .o file, and if it is, when the user last
// logged on. If it's NG, the user is removed.
// This is a lot of file access, so it's all done by call_outs. There are
// two parameters you can define: the time between each remove call, and
// the time between each check of a directory. The default values are
// 5 seconds per user remove and 300 seconds between directory sweeps:
// this means that directory sweeps will not run simultanously unless
// more than 60 users need to be purged from any one directory. It also
// means a full sweep will take 26*300 = 7800 seconds, about two hours.

#define USER_CALL_TIME 5
#define SWEEP_CALL_TIME 300
#define MAX_ERRORS 4
 
inherit DAEMON;
 
static int sweep_dir(string dir, int when, int verbose, int flag, int test);
static int fail_test(string dri, string what, int when, int flag);
static int remove_user(string where, int verbose, int flag, int test);
 
object link;
int total, busy;
int errors;
string err;

// Don't want this cleaning up with callouts pending.

int clean_up() {
	if (find_call_out("sweep_dir")>-1 || find_call_out("remove_user")>-1) {
		return 1 ;
	}
	::clean_up() ;
}
 
int process_users(int when, int verbose, int flag, int test) {
  string *main;
  int loop;

   //	Check time length and caller's permissions.
 
   if(!when || !intp(when) || (geteuid(previous_object()) != ROOT_UID &&
      !adminp(geteuid(previous_object()))))
	return 0;
 
   seteuid(getuid(this_object()));
 
   //	Check to see if the daemon is presently in use ... If so
   //	tell the caller to try again later (shouldn't really have
   //	repeated calls anyways).
 
   if(busy)  return 0;
 
   busy = 1;
 
   //	If there isn't a this_player(), then any write()s will get 
   //	dumped to the syslog ... would be messy ... so block it. :)

   if(!this_player())  verbose = 0;
 
   //	Check to make sure the flags are acceptable.
 
   if(flag < 0)  flag = 0;
   if(flag > 2)  flag = 2;
 
   //	Create temporary link connection object for restoring data.
 
   link = new(CONNECTION);
   total = 0;
 
   //	Get subdirectory listing from user save directory.

   main = get_dir( PDATA_DIR + "/" );
// For a test ;
 
   if(!main || !sizeof(main))  return 0;
 
   if(verbose)
	write(underscore("Initiating user character " +
	      (test ? "test " : "") + "purge") + "\n" +
	      "Last login prior to " + ctime(time()) + ".\n\n");
 
   //	Loop through directory contents using a catch() to catch 
   //	the "Too large evaluation" errors, then backup and do the
   //	failed directory again until you finish it.
 
   for(loop=0; loop<sizeof(main); loop++) 
      call_out( "sweep_dir", loop * SWEEP_CALL_TIME, 
	PDATA_DIR + "/" + main[loop] + "/", when, verbose, flag, test );

/*
   if(err = catch(sweep_dir(PDATA_DIR + "/" + main[loop] + "/", when, verbose,
      flag, test))) {
    errors++;
    if( errors >= MAX_ERRORS ) {
        error( "Purge giving up.\n" );
        return 0;
    }

   if(verbose)  write("Error occurred: " + err + "\nRestarting subdir sweep.\n");
   loop--;
   }
    errors = 0;
    }
*/
 
     call_out( "alert", (loop+3)*SWEEP_CALL_TIME, verbose, test) ;
return 1; }

void alert( int verbose, int test ) {
   //	We're all done ... destruct the working link.
   destruct(link);
 
 
   busy = 0;
}
 
 
 
//  This function sweeps through each of the user save subdirectories
//  and check each user to see if they match the purge criteria.
 
static int sweep_dir(string dir, int when, int verbose, int flag, int test) {
   string *contents;
   int loop, dinged;
 
   if(!directory_exists(dir))  return 0;
 
   if(verbose)  write(bold("Sweeping: " + dir) + "\n");
 
   dinged = 0 ;
   contents = get_dir(dir);

   if(!contents || !sizeof(contents))  return 0;

   //	Loop through contents of the subdirectory check for compliance.
 
    for(loop=0; loop<sizeof(contents); loop++) {
	if(fail_test(dir, contents[loop], when, flag)) {
	    dinged ++ ;
        call_out( "remove_user", dinged * USER_CALL_TIME, contents[loop],
					verbose, flag, test );
	   }
	}
return 1; }
 
static int fail_test(string dir, string what, int when, int flag) {
   string name;
 
   //	Parse off the user's name ...

   if(!what || !file_exists(dir + what) || 
      sscanf(what, "%s" + __SAVE_EXTENSION__, name) != 1)  return 0;
 
   //	Reset the link's properties for the next user.
 
   reload_object(link);
 
   seteuid(name);
   export_uid(link);
   seteuid(ROOT_UID);
 
   link->set("name", name);
 
   if(!link->restore())  return 0;
 
   if(!flag && (int)link->query("wizard"))  return 0;

   if(to_int((int)link->query("last_on")) < when)  return 1;

return 0; }

static int remove_user(string where, int verbose, int flag, int test) {
   string name, wiz_dir, tmp;
 
   sscanf(where, "%s" + __SAVE_EXTENSION__, name);
   wiz_dir = HOME_DIRS + extract(name, 0, 0) + "/" + name + "/";
   total++;

// Don't want to purge admins  -- Rust
  if( adminp(name) ) return 0;
   tmp = capitalize(name);
 
   if(flag == 2 && directory_exists(wiz_dir)) {
/*
      if(!test)  CLEAN_D->clean_dir(wiz_dir);
*/
// Rather than delete the dirs, move them to /purged/name/blah
// inspiral.
   if( !test ) "/adm/daemons/move_dir" -> clean_dir( wiz_dir );

      tmp += " and " + wiz_dir + " " + (test ? "flagged" : "deleted") + ".\n";
   }
   else tmp += " " + (test ? "flagged" : "deleted") + ".\n";
 
   if(!test) {
     rm(DATA_DIR + "/std/user/" + name[0..0] + "/" + name + __SAVE_EXTENSION__);
     rm(PDATA_DIR + "/" + name[0..0] + "/" + name + __SAVE_EXTENSION__);
   }
 
   if(verbose)  write(tmp);
 
return 1; }
 
int query_busy() {  return busy;  }
 
