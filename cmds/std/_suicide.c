 
//	File	:  /cmds/std/_suicide.c
//	Creator	:  Watcher@TMI	(02/08/93)
//
//	This command allows a player to commit suicide and have
//	the character file deleted from the database.
 
#include <config.h>
#include <mudlib.h>
#include <logs.h>
 
inherit DAEMON ;
 
//  If you do not wish suicided files to be saved in the
//  archive directory, simply #undef ARCHIVE_DIR
 
int busy;
 
int cmd_suicide() {
 
   //	Block any attempts by wizards to suicide
 
   if(wizardp(this_player())) {
   write("Sorry, only players may suicide their characters.\n");
   return 1; }

   // 	Prevent someone from suiciding the Guest character
 
   if((string)this_player()->query("name") == "guest") {
   write("Sorry, a Guest character may not commit suicide. Too messy.\n");
   return 1; }
 
   //	Check to see if the command's busy flag is set
 
   if(busy) {
   write("Sorry, the suicide command is presently busy. Please try again.\n");
   return 1; }
 
   busy = 1;				//  Set busy flag on
 
   write("Your character will be completely destroyed.\n" +
	 "Are you ABSOLUTELY sure you wish to commit suicide? [y/n] ");
   input_to("confirm_suicide");
 
return 1; }
 
static int confirm_suicide(string str) {
 
   if(!str || member_array(lower_case(str), ({ "yes", "y" })) == -1) {
   write("Suicide attempt aborted.\n");  busy = 1;
   return 1; }
 
   write("\nVery well. Input your password to confirm: ");
   input_to("pass_check", 1);
 
return 1; }
 
static int pass_check(string str) {
   string password, name;
 
   busy = 0;				// Reset busy flag
 
   //	Get player's name for backup purposes
 
   name = (string)this_player()->query("name");
 
   //	Get the user's password from the linked connection object
 
   password = ((object)this_player()->query_link())->query("password");
 
   //	Check to see the inputed password matches the actual password
 
   if(password != crypt(str, password)) {
   write("\nInvalid password. Suicide attempt aborted.\n");
   return 1; }
 
   write("\n\nAs you wish. Fare thee well, adventurer.\n" +
      "A bolt of lightning descends from the heavens, and all goes dark.\n\n");
 
   tell_room(environment(this_player()),
      "A bolt of lightning suddenly descends from the heavens. It strikes\n" +
      capitalize(name) + " in the chest, leaving only a pile of ashes where " +
      subjective((string)this_player()->query("gender")) + " once stood.\n",
      ({ this_player() }));
 
   //	Save the players attributes before file transfer 
 
   this_player()->save_data();
 
   //	If SUICIDE defined, write all suicides to a log file
 
#ifdef SUICIDE
   log_file(SUICIDE, capitalize(name) + " committed suicide from " +
	    query_ip_name(this_player()) + " [" +
	    extract(ctime(time()), 4, 15) + "]\n");
#endif
 
   //	Either move data files to ARCHIVE_DIR dir, or completely delete
 
   if(ARCHIVE_DIR) {
   rename(user_data_file(this_player()) + __SAVE_EXTENSION__, ARCHIVE_DIR + 
	  "user/" + name + __SAVE_EXTENSION__);
   rename(PDATA_DIR + name[0..0] + "/" + name + __SAVE_EXTENSION__, 
	  ARCHIVE_DIR + "connection/" + name + __SAVE_EXTENSION__);
   }
 
   else {
   rm( user_data_file(this_player()) + __SAVE_EXTENSION__ );
   rm( PDATA_DIR + name[0..0] + "/" + name + __SAVE_EXTENSION__ );
   }
 
   //	Remove the user object and connection from the game
 
   destruct( this_player()->query_link() );
   destruct( this_player() );
 
return 1; }
 
int help() {
 
   write("Syntax: suicide\n\n" +
     "This command can be used by players to completely delete their\n" +
     "character if they no longer wish to use in the game.\n");
 
return 1; }
 
