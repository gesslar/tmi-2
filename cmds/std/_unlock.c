 
//	File	:  /cmds/std/_unlock.c
//	Creator	:  Mobydick@TMI (probably)
//	Rewrite	:  Watcher@TMI  (4/13/93)
//
//	This is the standard open command for doors. The object open
//	command is handled by the objects themselves.
 
#include <mudlib.h>

inherit DAEMON ;

#define SYNTAX	"Syntax: unlock [object] with [optional desc] key\n"
 
int cmd_unlock (string str) {
   mapping doors;
   mixed *all_keys;
   object key, env;
   string *tmp, dir, key_name;
 
   notify_fail( SYNTAX );
 
   if(!str || str == "") {  notify_fail("Unlock what?\n");  return 0; }
 
   if(sscanf(str, "%s door with %s", dir, key_name) != 2 &&
      sscanf(str, "%s door with key", dir) != 1 &&
      sscanf(str, "door with %s", key_name) != 1 &&
      sscanf(str, "%s door", dir) != 1 &&
      str != "door" && str != "door with key")  return 0;
 
   env = environment(this_player());
 
   if(!env) {  
   notify_fail("The void has no doors to unlock, my friend.\n");
   return 0; }
 
   //	Get the door mapping from the environment.
 
   doors = env->query("doors");
 
   if(!doors) {
   notify_fail("There is no door here to unlock.\n");
   return 0; }
 
   //	Check to see if we have the key.
 
   if(!key_name) {

	all_keys = filter_array(all_inventory(this_player()), "find_keys",
				this_object());
	if(all_keys && sizeof(all_keys) > 1) {
	notify_fail("Which key do you wish to unlock the door with?\n");
	return 0; }

   }
 
   else {
	key = present(key_name, this_player());
	if(!key) key = present(key_name + " key", this_player());
   }
   
   if(!key)  key = present("key", this_player());
 
   //	Sorry, you don't have that particular key.
 
   if(!key) {
   notify_fail("You do not presently have" + (key_name ? " such" : "") + 
	       " a key.\n");
   return 0; }
 
   //	Get array of existing door directions.
 
   tmp = keys( doors );
 
   //  Check to see if the user can actually see.

   if(!this_player()->query("vision")) {
   write("You feel around in the dark for the door lock.\n");
   dir = tmp[ random(sizeof(tmp)) ];
   }
 
   if(!dir) {

	if(sizeof(tmp) > 1) {
	notify_fail("Which door did you wish to unlock?\n");
	return 0; }

   dir = tmp[0];
   }
 
   if(!doors[dir]) {
   notify_fail("There is no " + dir + " door.\n");
   return 0; }
 
   //	Check to see if the door actually has a unlock.
 
   if(!doors[dir]["lock"] || doors[dir]["lock"] == "none") {
   notify_fail("The " + dir + " door does not have a lock.\n");
   return 0; }
 
   //	Now check and see if the door is actually locked.
 
   if(doors[dir]["status"] == "open" || doors[dir]["status"] == "closed") {
   notify_fail("The " + dir + " door is already unlocked.\n");
   return 0; }
 
   //	See if the key fits, or is a skeleton key.
 
   if((string)key->query("to_lock") != doors[dir]["lock"] &&
      (string)key->query("to_lock") != "skeleton") {
   notify_fail("The key does not turn the lock.\n");
   return 0; }
 
   //	Okay...unlock the door, and update its linked mirror lock.
 
   env->set_status(dir, "closed");
   env->update_link(dir);
 
   write("You unlock the " + dir + " door.\n");
   say((string)this_player()->query("cap_name") + " unlocks the " + dir + 
       " door.\n", ({ this_player() }));
 
return 1; }
 
protected int find_keys(object obj) {  return (int)obj->id("key");  }

int help() {
 
   write( SYNTAX + "\n" +
     "This command will attempt to unlock the [object] with a specific key\n" +
     "within your possession. Remember...not all keys turn all locks.\n");
 
return 1; }
 
