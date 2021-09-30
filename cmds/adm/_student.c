 
// 	File	:  /cmds/adm/_student.c
//	Creator	:  Watcher@TMI-2  (5/93)
//
//	This command allows Admins to add a wizard to the Institute
//	domain as a student (access to the /student directory)
 
#include <uid.h>
#include <mudlib.h>
#include <config.h>
#include <daemons.h>

inherit DAEMON ;
 
#define SYNTAX 	"Syntax: student [-r] [user]\n"
 
int cmd_student(string str) {
   mapping domain;
   object act_ob, link, player;
   int i, remove;
  string  sponsor;

   act_ob = previous_object();
 
   seteuid(getuid(this_object()));

   notify_fail( SYNTAX );
   if(!str || str == "")  return 0;

   //	Check for admin or teacher permissions in user
 
   if(!adminp(geteuid(this_player(1))) &&
	!member_group(geteuid(this_player(1)), "teachers")) {
   write("Student: Only teachers may give student status to a user.\n");
   return 1; }

   //	Check for student removal flag in command line

   if(sscanf(str, "-r %s", str) == 1)  remove = 1;
  if(adminp(geteuid(this_player(1)))) 
	sscanf( str, "%s %s", str, sponsor );
  sscanf( str, "%s %s",str, sponsor );
 
   //	Check to see if there are already a maximum number of students.

   if(!remove && sizeof(get_dir(STUDENT_DIR)) > MAX_STUDENT_NUM) {
   write("Student: There are no more student positions available.\n");
   return 1; }
 
   str = lower_case(str);
 
   //	Check to see if the target user actually exists
 
   if(!user_exists(str)) {
   write("Student: No such user exists.\n");
   return 1; }
 
   //	Check to see if the user is presently online
 
   player = find_player(str) ;
 
   if(player)  link = player->query_link();
 
   //	Otherwise create the user's connection object 
 
   if(!link) {
   link = new(CONNECTION);
   seteuid(str);
   export_uid(link);
   seteuid(ROOT_UID);
   link->set("name", str);
   link->restore();
   }

   //	Get the user's domain mapping.
 
   domain = link->query("domains");
 
   if(!domain)  domain = ([]);
 
 
   //	Is a student priviledge removal or addition requested...

   if(!remove) {
 
   //	Check to see if the user is already a student
 
   if(domain["institute"]) {
   write("Student: " + capitalize((string)link->query("name")) +
	 " is already a student.\n");
   return 1; }
   
   //	Set the domain flag
 
   domain["institute"] = ([ "level" : "student", "sponsor" : 
sponsor ? sponsor : (string)this_player()->query("name") ]);
   domain["primary"] = "institute";
   }

   else {

   if(!domain["institute"]) {
   write("Student: " + capitalize((string)link->query("name")) +
	 " is not presently a student.\n");
   return 1; }
 
   //	Remove the domain flag

   if(domain["primary"] && domain["primary"] == "institute")
	domain["primary"] = 0;

   map_delete(domain, "institute");
   }
 
   link->set("domains", domain);
 
   //	Save the connection data
 
   link->save_data();
 
   //	If the player isn't online ... toast the connection object.
 
   if(!player)  destruct(link);
 
 
   //	Give correct action responses to the admin and user if online.
 
   if(!remove) {
 
   write("You have given student status to " + capitalize(str) + ".\n");
 
   if(player) 
   tell_object(player, 
	"You have been given student status by " +
	(string)previous_object()->query("cap_name") + ".\n");
 

   //   Make the student directory...if it doesn't already exist.

   if(!directory_exists(STUDENT_DIR + str)) {
	write(STUDENT_DIR + str + "/ and " + STUDENT_DIR + str +
	      "/workroom.c created.\n");
	
	if(player)
	tell_object(player, STUDENT_DIR + str + "/ and " + STUDENT_DIR +
		    str + "/workroom.c created.\n");
 
        mkdir(STUDENT_DIR + str);
        write_file(STUDENT_DIR + str + "/workroom.c",
                   read_file("/obj/workroom.c"));
   }
   }

   else {

   write("You remove " + capitalize(str) + "'s student priviledges.\n");

   if(player)
   tell_object(player, "Your student priviledges have been removed by " +
	(string)this_player()->query("cap_name") + ".\n");
   }
 
   //	Update the student status in the student daemon.
 
   if(!remove)  STUDENT_D->add_student( str, sponsor );
   else STUDENT_D->remove_student( str );
 
return 1; }

string help() {
 
   return( SYNTAX + "\n" +
     "This command allows an Admin to give student status in the institute\n"+
     "domain to a selected user. This will set the user's domain settings,\n"+
     "as well as create a working directory within the student directory\n"+
     "tree for the user. There are a maximum of " + MAX_STUDENT_NUM + 
     " positions available.\nA user's student priviledges may be removed " +
     "with the \"r\" flag. Note\nthat this will not remove the user's " +
     "student directory.\n");
 
}
 
