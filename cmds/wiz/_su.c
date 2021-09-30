//	File	:  /cmds/std/_su.c
//	Creator	:  Sulam@TMI (1-21-92)
//	Updated	:  Huma@TMI (11-15-92) for conversion to mudlib 0.9
//	
//	Completely overhauled by Watcher@TMI (2-18-93) for new connection
//	login system, and to improve a few of the su processes.
//
//	Updated by Watcher@TMI (03/92) to accomodated body transfers
//	into monsters by users.
//
//	Updated by Watcher@TMI (04/04/93) stripping away multiple user
//	capabilities to avoid confusion and fix a few other things.
//
//	This command allows wizards to refresh their player object, or
//	change into another user, if the correct password is known.
//	Admins are not required to enter the password.

#include <uid.h>
#include <priv.h>
#include <config.h>
#include <logs.h>
#include <mudlib.h>
#include <daemons.h>
#include <channels.h>
#include <login.h>
#include <login_macros.h>

inherit DAEMON ;

// Local hack?
#undef CAP_NAME
#ifndef CAP_NAME_MASTER_ONLY
#define CAP_NAME(ob) capitalize((ob)->query("name"))
#else
#define CAP_NAME(ob) (ob)->query("name")
#endif

mapping active = ([]);
 
static int link_monster(string name);
static int get_password(string pass);
static void enter_world();
static int check_password(string pass);
static void try_again();
static void switch_player();
static void complete_entry(string str);

void create() {
	seteuid(getuid(this_object()));		//  Set daemons permissions
}

int cmd_su(string name) {
   object ob;
 
   //	Setup the euid of the command to ROOT
 
   seteuid( getuid(this_object()) );
 
   //	If no name is given assume user's real (non-monster) name
 
   if(!name)  name = (string)this_player()->link_data("name");
 
   //	Make sure user's name is lowercase
 
   name = lower_case(name);
 
   //	Check to see if anyone is using the su command.
 
   if(!undefinedp(active["new"])) {
   notify_fail("Su: The su command is presently in use. Please try again.\n");
   return 0; }
 
   //	Check to see if a monster matches the requested id

   if(wizardp(this_player()) && link_monster(name))  return 1;
 
   //	Create new connection object to link 
 
   ob = new(CONNECTION);
 
   //	Check to see if user actually exists in the database
 
   if(!file_exists(user_data_file(ob, name) + __SAVE_EXTENSION__)) {
 
      if(wizardp(this_player()))
	write("Su: No such user or monster exists.\n");
      else
	write("Su: No such user exists.\n");
 
   ob->remove();
   return 1; }
 
   // Add user and connection object to su storage array
 
   active["old"] = this_player();
   active["new"] = ob;
   active["who"] = name;
 
   //	Set and pass permissions
 
   seteuid( name );
   export_uid(ob);
   seteuid(getuid());
 
   //	Name connection object and attempt to restore user's data
 
   ob->SET_NAME(name);
 
   if(!ob->restore()) {
   write("Su: Could not restore requested user.\n");
   ob->remove();
   active = ([]);
   return 1; }
 
   if(!wizardp(this_player()) && !ob->query("wizard") &&
      name != (string)this_player()->link_data("name")) {
   write("Su: Players may only su to themselves or a wizard character.\n");
   ob->remove();
   active = ([]);
   return 1; }
 
   //	If user is an admin, and target isn't an admin
 
   if(adminp(this_player()->link_data("name")) &&
      !adminp(name)) {
   active["same"] = 0;
   switch_player();
   }
 
   //	Switching to a new user ... ask for password
 
   else if((string)this_player()->link_data("name") != name) {
   active["same"] = 0;
   input_to("get_password", 1);
   write("Password: ");
   }
 
   //	Same player ... so don't ask password
 
   else {
   active["same"] = 1;
   switch_player();
   }
 
return 1; }
 
static int get_password(string pass) {
 
   write("\n");
 
   //	Check to see if inputed password is correct
 
   if(!check_password(pass)) {
   write("Su: Incorrect password.\n");
   active = ([]);
   return 0; }
 
   //	If correct, change into the new shell
 
   switch_player();
 
return 1; }
 
static int check_password(string pass) {
   string password;
 
   //	Get stored character password from connection object
 
   password = (string)active["new"]->PASS;
 
   //	Compared inputed password with stored character password
 
   if(password != crypt(pass, password))  return 0;
 
return 1; }
 
static void switch_player() {
   int volume, capacity;
 
   //	Switch interactive between old and new shells
/*
  if(member_array("/std/body.c",deep_inherit_list(active["new"])) == -1){
	write("Security violation\n");
	return 1;
}
*/
   if( exec(active["new"], active["old"]) )
	enter_world();
 
   else {
   write("Su: Login attempt failed.\n");
   active["new"]->remove();
   active = ([]);
   return; }
 
}
 
static void enter_world() {
   object *inv;
    string *history;
    int cmd_num, ptr, max;
   int i, volume, capacity;
 
   //	Save present user configuration
 
   active["old"]->save_data();
 
   //	Setup new user object and move inventory to new host
 
   if(!active["new"]->restore_body() || !active["new"]->connect() ||
      !active["new"]->BODY_OB->query("name")) {
   write("Su: Could not restore new user body.\n");
   exec(active["old"], active["new"]);
   active["new"]->BODY_OB->remove();
   active["new"]->remove();
   active = ([]);
   return; }
 
   //	Get old user volume and capacity
 
   if(active["same"]) {
   volume = (int)active["old"]->query("volume");
   capacity = (int)active["old"]->query("capacity");
   }
 
    history = copy((string *)active["old"]->query_history());
    cmd_num = (int)active["old"]->query_cmd_num();
    max = (int)active["old"]->query_max();
    ptr = (int)active["old"]->query_ptr();
   //	Move the inventory..etc.. if old isn't a monster.
 
   if(!active["old"]->query("npc") && !active["new"]->query("npc")) {
 

   for(inv=all_inventory(active["old"]); i<sizeof(inv); i++) 
   if(!inv[i]->query_auto_load()) 
	inv[i]->move( active["new"]->BODY_OB );
 
   //	Reset previous volume and capacity in new user shell
   //   in users last, so the move always succeeds
    if(active["same"]) {
       active["new"]->set("volume", volume);
       active["new"]->set("capacity", capacity);
    }
 
   }
 
#ifdef SU_LOG
   log_file(SU_LOG, CAP_NAME(active["old"]) + ": sued into " +
           CAP_NAME(active["new"]) + " from " +
	   query_ip_name(this_player()) + " [" +
	   extract(ctime(time()), 4, 15) + "]\n");
#endif

   if(active["old"]->query("npc")) {
   write("Reappear in the same location? [y/n] ");
   input_to("complete_entry", 0);
   return; }
 
   complete_entry("yes");

return; }
 
static void complete_entry(string str) {
    string domain;
 
   if(!str || member_array(str, ({ "y", "yes", "n", "no" })) == -1) {
   write("Reappear in the same location? [y/n] ");
   input_to("complete_entry", 0);
   return; }
 
   if(str == "no" || str == "n")
   active["new"]->BODY_OB->move( START );

   else active["new"]->BODY_OB->move( environment(active["old"]) );
 
   if(active["old"]->query("npc")) {
	active["old"]->clear_monster();
 
         domain = master()->domain_file(file_name(active["old"]));
         switch (domain) {
             case ROOT_UID:
             case BACKBONE_UID:
             case "NONAME":
             case "User":
             case "Anonymous":
                 seteuid("Anonymous");
                 break;
             default:
                 seteuid(domain);
                 break;
         }
         export_uid(active["old"]);
         seteuid(getuid());
	active["old"]->reset_monster();
   }
 
   if(active["same"]) {
   write("Transfer complete.\n");
   if(!active["old"]->query("npc"))
        message("prompt", this_player()->write_prompt(1), this_player());
    active["new"]->BODY_OB->init_setup();
   }

   else {
   write("You polymorph into " + active["new"]->query("cap_name") + ".\n");
   say(active["old"]->query("cap_name") + " polymorphed into " +
       active["new"]->query("cap_name") + ".\n",
       ({ active["old"], active["new"] }));
   active["new"]->BODY_OB->init_setup();
   }
 
   //	Announce the departure of the old char if different from new
 
   if(!active["same"] && !active["old"]->query("npc"))
	ANNOUNCE->announce_user( active["old"], 1 );
 
   if(!active["old"]->query("npc")) {
   active["old"]->save_data();
   active["old"]->remove();
   }
 
   CHANNELS_D -> initialize_user();
   active = ([]);
}
 
//	This function handles the user transfers into monster shells

static int link_monster(string name) {
   object ob, old;
   int ret;
 
   old = this_player();
 
   ob = get_object( name );
 
   //   Check to see if a monster matches the requested id

   if(!ob || !ob->query("npc") || !environment(ob) || this_player() == ob)
	return 0;

   if(interactive(ob)) {
   write("Su: That monster body is presently inhabited.\n");
   return 1; }

   this_player()->query_link()->set("tmp_body", ob);
   ret = this_player()->query_link()->switch_body();

   if(!ret) {
   write("Su: Could not transfer into " + name + ".\n");
   return 1; }
 
   ob->clear_monster();
   seteuid( (string)this_player()->link_data("name") );
   export_uid(ob);
   seteuid(getuid());

   ob->init_setup();
   old->set_link(0);
   old->quit();

   write("Transfer complete.\n> ");

return 1; }
 
mapping query_active() {  return active;  }
 
int help() {
 
   write("Usage: su [user]\n\n" +
    "The su command allows a user to change from their present character\n" +
    "to another, either a fresh version of the present character or a new\n" +
    "version of another character (provided you know the password). It is\n" +
    "equivalent to logging out and back in again, but quicker. With an\n" +
    "argument, su prompts you for the requested user's password, and if\n" +
    "answered correctly, you will be polymorphed into that user. Without\n" +
    "an argument, or if you pass your own character's name as an argument,\n"+
    "you will be transfered into a new copy of your present character.\n");
 
return 1; }
 
