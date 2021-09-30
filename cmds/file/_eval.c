/*
// File:       /cmds/xtra/_eval.c
// Purpose:    Evaluates a string as LPC commands
// Credits:
// 92-12-07    Pallando (aka Douglas Reay) created this command.
//             This file is a part of the TMI distribution mudlib.
//             Please retain this header if you use this code.
// 92-12-08    Pallando changed it not to use /tmp on idea from Watcher
// 92-12-12    Pallando changed it to use the identify() simul_efun
// 93-04-15    Pallando added final desting of object on idea from Robocoder
// 93-05-28    Artagel added strict typing
// 93-06-24    Pallando changed it to use /open on idea from Mobydick
// 93-07-16    Robocoder added some evil admin logging
// 93-10-01     Rust added doith() to make debugging easier.
// 93-10-23   Mobydick added the ADMIN_ONLY define.
// 94-04-27    Robocoder added (liberally hacked in) Grendel's set euid code
// 94-06-29   Yavie added ansi.h on a suggestion from Crunch.
// 95-01-23    Fixed a type check for when we hit 20.26   Leto
*/

#include <config.h>
#include <uid.h>
#include <mudlib.h>
#include <commands.h>
#include <logs.h>
#include <ansi.h>

// #define ADMIN_ONLY 1

inherit DAEMON;

#define SYNTAX "Syntax: \"eval [-uid] <lpc commands>\".\n"

#define LOGS ({ \
  user_path( geteuid( this_player() ) ) + "log", \
  LOG_DIR "log", \
  LOG_DIR "debug.log", \
  })

//prototype
string doith(string arg);

// If your mud allows objects in /tmp to load, use that, otherwise use /open
// If you have no loadable public directories, undefine TMP_FILE
// #define TMP_FILE temp_file( "eval", this_player() ) + ".c"
#define TMP_FILE OPEN_DIR + "eval." + geteuid( this_player() ) + ".c"
#define TMP_EUID_FILE SECURE_DIR + "tmp/eval." + geteuid( this_player() ) + ".c"

// Comment out the next line if you want failed evals to be deleted.
// #define KEEP_FAILED_EVALS

int cmd_eval( string a )
{
  string file, filename;
  mixed err, ret;
  mapping logs;
  string *names;
  int loop, i;
  string euid;

  if( !a ) { notify_fail( SYNTAX ); return 0; }

/* Note that this only protects you against stupid wizzes if the wiz can
 * write and load an object in some dir in the mudlib.
 */
#ifdef ADMIN_ONLY
  if (!adminp(geteuid(this_player()))) {
    write ("Sorry, this command can only be used by admins.\n") ;
    return 1 ;
  }
#endif

  if(a[0] == '-')
  {
    if(a[1] == '-')
      a = a[2..<1];
    else if (geteuid(previous_object())
	     && adminp(geteuid(previous_object()))
	     && previous_object() == this_player()
	     && this_player() == this_player(1))
    {
      i = strsrch(a, ' ');
      if(i != -1)
      {
        euid = a[1..(i-1)];
	a = a[(i+1)..<1];
      } else {
	notify_fail("call: seteuid: bad euid\n");
	return 0;
      }
    } else {
      notify_fail("call: seteuid: permission denied\n");
      return 0;
    }
  }

  // The includes in the file aren't necessary (though inheriting OBJECT
  // allows remove() to work).  You should change them as necessary for
  // your own mud.  They do allow shorter eval commands though.  Eg:
  //     eval return children( USER )
  file = @EndText
#include <config.h>
#include <daemons.h>
#include <net/daemons.h>
#include <commands.h>
#include <uid.h>
#include <mudlib.h>
#include <driver/runtime_config.h>
#include <net/i3.h>
inherit OBJECT;
EndText
    ;

    // It is wise to prevent shadowing of this object, since its UID/EUID
    // is changed.
    if (euid) {
        file += @EndEuid
void create() { }
int query_prevent_shadow() { return 1; }
mixed eval()
{
    if (file_name(previous_object()) != CMD_EVAL) {
        seteuid("Anonymous");
        return;
    }
    seteuid(
EndEuid
        "\"" + euid + "\");";
    } else {
        file += @EndNoEuid
void create() { seteuid( getuid() ); }
mixed eval()
{
EndNoEuid
        ;
    }

     file += (string) doith(a) + ";\n}\n";

#ifdef EVAL_CALL_LOG
    seteuid(ROOT_UID);
  if (!adminp(geteuid(this_player())))
    log_file(EVAL_CALL_LOG, "************\n" + extract(ctime(time()), 4, 15) +
      " [" + geteuid(this_player()) + "] eval'd: " + a + "\n");
#endif

  seteuid( geteuid( previous_object() ) );

  if (euid)
    filename = TMP_EUID_FILE;
  else {
  // Use a wizard's directory if it exists, a public directory if not.
    filename = user_path( geteuid( this_player() ) );
    if( file_size( filename ) == -2 )
    {
      filename += "CMD_EVAL_TMP_FILE.c";
    } else {
#ifdef TMP_FILE
      filename = TMP_FILE;
#else
      notify_fail( "You need a home directory to use the eval command.\n" );
      return 0;
#endif
    }
  }

  // If anything has been left from a previous eval, remove it
  rm( filename );
  if( ret = find_object( filename ) ) ret-> remove();

  write_file( filename, file );

  logs = ([]);
  loop = sizeof( LOGS );
  while( loop-- )
    logs[LOGS[loop]] = file_size( LOGS[loop] );

  if( err = catch( ret = (mixed)call_other( filename, "eval" ) ) )
  {
    write( "Error = " + err );
#ifdef KEEP_FAILED_EVALS
    write( "The file is in " + filename + "\n" );
#else
    rm( filename );
#endif
    loop = sizeof( names = keys( logs ) );
    while( loop-- )
      CMD_UPDATE-> display_errs( logs[names[loop]], names[loop] );
  } else {
    // If you don't have the identify() simul_efun, use dump_variable()
    write( wrap( "Result = " + identify( ret ) ) );
    rm( filename );
    if( ret = find_object( filename ) ) ret-> remove();
  }
  return 1;
}

string help()
{
  return( SYNTAX + @Endtext
Effect: calls a function containing <lpc commands>
Example: if you type:
    eval return function_exists("set",this_player())
this creates a temporary file in your home dir containing the line:
    eval() { return function_exists( "set", this_player() ); }
and then does call_other on the files's eval() and displays the returned value
Endtext
  );
}

/* alog to take a string of LPC and explode it (safely) on its ;'s. */

string doith(string arg) {
  int i;    // pointer to string within array containing an exploded bit 
  int j;    // pointer to character within a given string
  int cnt;  // current count of (+1) ('s and (-1) )'s
  int poq;  // (waiting on) parethesisisis or quote; dual state
  string *inp;

  inp = explode(arg, ";");
  if(arg[<1..<1] == ";")
  inp[sizeof(inp)-1] += ";";
  for(i=0;i<sizeof(inp); i++) {
    for(j=0;j<strlen(inp[i]); j++) {
      if(!poq && inp[i][j] == ')') cnt --;
      if(!poq && inp[i][j] == '(') cnt ++;
      if(inp[i][j] == 92 && inp[i][j+1] == 34) j+=2;
      if(inp[i][j] == 34) poq = (!poq);  // toggle poq state 
      if(strlen(inp[i])==j+1) {
        if(sizeof(inp) == i+1) {
          if (cnt>0)   write("eval: You have "+cnt+" too many left parenthesis. (().\n");
          else  if(cnt<0) write("eval: You have "+(-cnt)+" too many right parenthesis. ()).\n");
          else if(poq) write("eval: CR in middle of stinking string.\n");
          // error or no error we return here at the end
          return implode(inp, ";\n");
        }
        else if (poq || cnt)
          inp[i] = inp[i] + ";"+ inp[i+1], inp -= ({inp[i+1]});
      }
    }
  }
}

