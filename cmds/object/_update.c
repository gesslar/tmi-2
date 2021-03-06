/*
// This file is part of the TMI mudlib distribution.
// Please include this header if you use this code.
// Written by Sulam(12-19-91)

// Modified 3/11/92 By Huthar to allow updating of master.c
// It is important to put the file_size() after the destruction
// of the object as file_size does an apply_master() which will
// cause the master object to load itself should it not exist.

// Modifed 92/04/25 by Truilkan to do The Right Thing (TM) -- i.e. avoid
// leaving people in the void whenever possible (even when updating a remote
// room).

// Hacked up 9/26/92 by Guile to use inherit_list() and deep_inherit_list()
// for a "deep-update".  Very useful for updating files like std/user.c

// Mobydick added the help file 11-13-92, using the existing update doc file.
// Protected against cutting connection on players in void 11-15-92, Brian

// Fixed 11-16-92 by Mobydick to do The Really Right Thing (TM) - which
// is, only save the room's _players_. Tru had it saving monsters as
// well, which meant that any monster cloned in the room's
// create procedure got duplicated.

// Modified to print out the lines in your log file generated
// by compile errors or whatever by Zak, 921125.

// Modified by Qixx@Hero 12/1/92 to pull in "cwf" when no args
// are specified.

// Zak, 921228: Will dump lines in log/log file if error messages
// went there. Cleaned up a bit too.

// Watcher changed TEMP_ROOM to using VOID designated in config.h (1-14-93)

// Zak, 930208: -r & -R update the object as well (after it's inherited
// items are updated. The help implied this final step, but it never got done
// (So you used to have to do update -R blah, update (the 2nd to update
// blah itself)

/// Beek, 930919: Made it so virtual objects don't get dested if you try
/// to update them
///
/// Leto, 941111: Made /std/user#3434 not updateable, it dested users.
///  added the same for connection a bit later ;)
*/

#include <config.h>
#include <mudlib.h>

inherit DAEMON;


void do_update(string file);
void display_errs(int siz, string nam);

int cmd_update(string str)
{
    object	ob;
    string	file, res, temp, *obs, logf;
    int		i, deep_up, syslog_siz, mylog_siz;

    if (!str)
	str = (string)this_player()->query("cwf");
    if (!str) {
        notify_fail("You have no current working file set.\n");
        return 0;
    }
  
if ( (strsrch(str,"/std/user#") != -1 ) || ( strsrch(str,
     "/std/connection#") != -1 ) )
  {
    write("Updating a user or connection object isn't allowed!\n");
    return 1;
  }
    if (str)		// update the specified file
    {
	if(sscanf(str, "-r %s", temp))
	{
	    file = temp;
	    deep_up = 1;
	}
	else if(sscanf(str, "-R %s", temp))
	{
	    file = temp;
	    deep_up = 2;
	}
	else if(str == "-r")
	{
	    file = 0;
	    deep_up = 1;
	}
	else if(str == "-R")
	{
	    file = 0;
	    deep_up = 2;
	}
	else
	     file = str;
    } // if(str)
    if(!file)
    {
	if(!environment(this_player()))
	{
	    notify_fail("Update: You don't have an environment.\n");
	    return 0;
	}
	file = file_name(environment(this_player()));
    }
    else 
	file = resolv_path("cwd", file);
 
    //	Make sure that a ".c" is appended on the filename

    if(extract(file, strlen(file)-2, strlen(file)-1) != ".c")
	file += ".c";
 
    this_player()->set("cwf",file);

    logf = (string) this_player()->query("name");
    logf=user_path(logf) + "log";	// assume if can use update, has a name
    mylog_siz = file_size(logf);
    syslog_siz = file_size(LOG_DIR + "log");

    seteuid(geteuid(previous_object()));

/* Beek 091993 - doesn't dest virtual objects now */
    if (find_object(file) && virtualp(find_object(file))) {
      notify_fail("That object is virtual and cannot be updated.\n");
      return 0;
    }
    if(!file_exists(file)) {
	if(find_object(file)) {
	destruct(find_object(file));
	notify_fail("Update: " + file + " does not exist.\n\tLoaded " +
		    "copy of file removed from memory.\n"); }
    	else notify_fail("Update: " + file + " does not exist.\n");
    return 0; }

    ob = find_object(file);
    if (!ob)
    {
	res = catch(file->apply_load());
	if(res)
	{
	    display_errs(mylog_siz, logf);
	    display_errs(syslog_siz, LOG_DIR + "log");
	    notify_fail("Failure to load object.\n");
	    return 0;
	}
	ob = find_object(file);
    }
    if(deep_up == 1)
	obs = inherit_list(ob);
    else if (deep_up ==2)
	obs = deep_inherit_list(ob);
    i = sizeof(obs);
    if (deep_up)
	while (i--)
	    do_update(obs[i]);
    do_update(file_name(ob));
    display_errs(mylog_siz, logf);
    display_errs(syslog_siz, LOG_DIR + "log");
    return 1;
} /* cmd_update */


void display_errs(int origsiz, string logf)
{
    int siz;

    siz = file_size(logf);
    if (origsiz == -1)
	origsiz = 0;
    if (siz == -1)
	siz = 0;
    if (siz > origsiz)
    {
	string errs;
	errs = read_bytes(logf, origsiz, siz - origsiz);
	printf("Errors written to %s:\n%s", logf, errs);
    }
} /* display_errs */


protected void do_update(string file)
{
    mixed	res;
    object	ob, *obs;
    int		i, update;

    write(file + ": ");
    ob = find_object(file);
    if(ob)
    {
	obs = all_inventory(ob);
	obs = filter_array(obs, "filter_player", this_object());
	if (sizeof(obs) && (file == VOID))
	{
	    write("Error: Cannot update void while occupied.\n");
	    return;
	}
	obs->move(VOID);
	if(function_exists("remove", ob) && res = catch(ob->remove()))
	{
	    write("Error in remove().\nError: " + res + "\n");
	    return;
	}
	if(ob)
	    destruct(ob);
	if(ob)
	    return;
	file_size("???");
	update = 1;
    } // if (ob)
    if(res = catch(file->apply_load()))
    {
	write("Error in loading object.\n");
	write("Error: " + res + "\n");
	return;
    }
    if(obs)  obs->move(find_object(file));
    write((update ? "Updated and loaded." : "Loaded.") + "\n");
    return;
} /* do_update */


int filter_player(object ob)
{
    return userp(ob);
} /* filter_player */


int help()
{
    write ("Usage: update [-r/R] <filename>\n\
The update command loads the specified file. If no argument is given, then\n\
the room the wizard is in is updated.\n\
Updating a file only affects the master copy of the object. Clones of the\n\
object that were cloned before the update will NOT be affected. You will\n\
need to destroy those objects and re-clone them for any changes to take\n\
effect.\n\
Update takes optional flags: -r and -R, for versions of update which will\n\
also update files inherited by this object. update -r will update this\n\
object and the files that it inherits. update -R will update this object,\n\
the files that it inherits, any files that _those_ files inherit, and so\n\
on, recursively until all files in the entire chain are updated.\n\
");
    return 1;
} /* help */
