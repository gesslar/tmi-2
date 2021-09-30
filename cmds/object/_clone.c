 
/*
// This file is part of the TMI distribution mudlib.
// Please include this header if you use this code.
// Written by Sulam(12-19-91)
// Help() added 1/27/92 Brian
// Qixx@Hero Added "cwf" stuff.
// Watcher@TMI (3/24/93) Cleaned up and file existence check.
// Watcher@TMI (4/3/93) Improved the message handling.
*/

#include <mudlib.h>
#include <move.h>
#include <uid.h>

inherit DAEMON ;

#define SYNTAX	"Syntax: clone [filename]\n"
#define GUEST_NO_CLONE 0
 
int cmd_clone(string str) {
	object ob;
	mixed res;
 
	notify_fail( SYNTAX );
 
	if(!str)  str = (string)this_player()->query("cwf");
	if(!str || str == "")  return 0;

#ifdef GUEST_NO_CLONE
	if (getuid(previous_object())=="guest") {
	    write ("Guest is not allowed to clone items.\n") ;
	    return 1 ;
	}
#endif
 
	if( getuid(this_object()) != ROOT_UID) {
	  notify_fail("Resetting uid of clone command.\n");
	  this_player()->force_me("clone " + str );
	  destruct(this_object());
	return 0; }
 
	seteuid( getuid(previous_object()) );
 
	str = resolv_path("cwd", str);
 
	if(extract(str, strlen(str)-2, strlen(str)-1) != ".c")
	  str += ".c";
 
	if(!file_exists(str)) {
	write("Clone: " + str + " does not exist.\n");
	return 1; }
 
	write("Cloning: " + str );
 
	if (res=catch(ob=new(str)))
	   write("\nError: " + res + "\n");
 
	if( !ob ) return 1;
 
	if(visible(this_player()))
	say (wrap((string)this_player()->query_mclone(ob)+"\n")) ;
 
	if( ob->move(this_player())!=MOVE_OK || (int)ob->get() ) {
	  if( !ob ) return notify_fail( "Object destructed on move.\n" );
  	  ob->move(environment(this_player()));
	  write(" to " + file_name(environment(this_player())) + ".\n");
	}

	else write(" to " + file_name(this_player()) + ".\n");
 
	this_player()->set("cwf",str);
 
return 1; }

int help() {
 
   write( SYNTAX + "\n" +
        "This command creates an object from the file which\n"+
        "is passed to the command.  The object is cloned into\n"+
        "your environment if it cannot be carried, otherwise\n"+
        "it is cloned into your inventory.\n"+
	"If the object is not loaded, this command will force it to load.\n"+
	"The object's create() function will be called in the clone. It is\n"+
	"almost always better to clone an object than to use the master\n"+
	"copy of it. Rooms are the major exception to this rule.\n") ;
  return 1;
}
 
/* EOF */
 
