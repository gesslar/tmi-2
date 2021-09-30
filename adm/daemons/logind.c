// File   :  /adm/daemons/logind.c
// Creator :  Sulam@TMI  (12-13-91)
// Updated :  Sulam@TMI  (3-29-92)  Added real_name prompting
//     Sulam@TMI  (4-10-92)  Added game locking
//     Buddha@TMI  (9-4-92)  Added news system and
//      expanded the login process
//
// Overhauled by Buddha@TMI (12-92) to support a separate creation
// module and to serve as a login daemon
//
// Updated :  Watcher@TMI  (2-9-93)  Moved news display to nes
//      news daemon, and inserted check for users stuck
//      in login limbo
//     Watcher@TMI  (2-22-93)  Added an optional userlist
//      display during the welcome screen
//     Watcher@TMI  (2-23-93)  Streamlined Guest login process,
//      added banish name system, and upgraded a few other things.
//     Watcher@TMI  (3-9-93)  Added login failure notification.
//     Watcher@TMI  (4-7-93)  Added max players, WIZ_LOCK,
//      ADMIN_LOCK, and shutdown entry check.
//     Watcher@TMI  (4/15/93)  Added NO_REMOTE_LOGIN check option.
//     Watcher@TMI  (4/29/93)  Added hibernation checks.
//     Karathan  (7/13/93)  Tidied and moved updated banish code
//      to the banish daemon.
//     Karathan  (8/12/93)  Added email-registration command handler.
//     Rust@TMI-2 (11/12/93)  Fixed WIZ_LOCK bugs.
//     Rust@TMI-2 (11/24/93) Added option to disable new character creation.
//     Mobydick@TMI-2 (4/8/94) Added logging of failed login attempts.
//			Original idea from Inspiral@Tabor
//     Inspiral@TMI-2 (04/18/94) Shortened log_file(BAD_LOGIN) message.
//    Beek@TMI-2 (7/31/94) hushlogin support
//	Leto@Tmi-2 (6/24/95) fix for SAVE_EXTENSION/__SAVE_EXTENSION__
 
// Much of the login process can be customized to suit your 
// specific variety of mud and its atmosphere.  These defines
// can be found, for the most part, in /include/login.h
 
#include <uid.h>
#include <priv.h>
#include <config.h>
#include <mailer.h>
#include <mudlib.h>
#include <login.h>
#include <daemons.h>
#include <login_macros.h>
#include <net/daemons.h>
#include <logs.h>
 
 
void logon(object ob);
string active_users();
static void get_name(string str, object ob);
static void get_password(string pass, object ob, int count);
static int check_password(string pass, object ob);
static void choice(string choice, object ob, string name);
static void register_site();
static void exec_old_copy(string s, object ob);
static void login_new_copy(object ob);
static void enter_world(object ob);
static void check_email(object user);
 
 
void create()
{
    seteuid(ROOT_UID);
}
 
// This is called when a new user logs in.
 
void logon(object ob)
{
    string local_host, pattern;
    int a1, a2, a3, a4;
	string s1,s2 ;
 
    if (base_name(previous_object()) != CONNECTION)
	return;
 
//  Enable NO_REMOTE_LOGINS in /include/login.h if only local site logins
//  to the mud are being permitted (ie the first two segments of the ip
//  number of the login match those of the mud host machine).  
#ifdef NO_REMOTE_LOGINS
    local_host = (string)DNS_MASTER->get_host_name(mud_name());
    if (!local_host || local_host == "" ||
	sscanf(local_host, "%d.%d.%d.%d", a1, a2, a3, a4) != 4)
      { write("\n  Could not resolve local network ip number from mud name " +
	      "server.\n  An accessible ip number must be available for user " +
	      "validation check.\n  Shutting down.\n\n");
	ob->remove_user();
	return; }
    else
	pattern = a1 + "." + a2 + ".%d.%d";
    if (!query_ip_number(ob) ||
	sscanf(query_ip_number(ob), pattern, a1, a2) != 2)
      { write("\n\t" + capitalize(mud_name()) + " is presently only " +
	      "accepting local logins.\n\tEnquires can be directed to " +
	      ADMIN_EMAIL + ".\n\n");
	ob->remove_user();
	return; }
#endif /* NO_REMOTE_LOGINS */
 
 
    write(LOGIN_MSG);
 
//  Enable USER_LIST in /include/login.h to display current users of the
//  mud at this stage of the login.
#ifdef USER_LIST
    write(wrap("Current users: " + active_users()) + "\n");
#endif /* USER_LIST */
 
    write(LOGIN_PROMPT);
    input_to("get_name", 2, ob);
    return;
}
 
 
static void get_name(string str, object ob)
{
    string tmp, tmp1, tmp2;
    int bad_name, loop, i, sd_time;
 
// Did they supply anything?
    if (!str || str=="") {
	write ("\nSorry, you must supply a name for your character.\n\n") ;
	write (LOGIN_PROMPT) ;
	input_to("get_name",2,ob) ;
	return ;
    }
// Do they want to quit at this point?
	if (str == "q" || str == "quit")
	{
	    write ("See you again soon!\n") ;
	    ob->remove_user() ;
	    return ;
	}
//  Enable ADMIN_LOCK in /include/login.h with a reason for lockout
//  if you wish to allow only Admin onto the mud.
#ifdef ADMIN_LOCK
    if (!adminp(str))
      { write("\n\n\n" +"\t\t" + mud_name() +
	      " Is Temporarily Closed for Repairs.\n\n" +
	      "\t\tReason for Closure:\n" +
	      "\t\t" + ADMIN_LOCK + "\n\n" +
	      "\t\tSorry for the inconvenience,\n" +
	      "\t\t\t-The Management\n\n\n");
	ob->remove_user();
	return; }
#endif /* ADMIN_LOCK */
 
 
//  If a maximum number of users is defined in /include/login.h, check
//  to make sure we haven't exceeded that number.  If so, and the user
//  is not an Admin, tell them to try again later.
#ifdef MAX_USERS
    if (!adminp(str) && sizeof(users()) > MAX_USERS)
      { write("\n\n  Sorry, " + capitalize(mud_name()) + " is full " +
	      "presently ... Please try again shortly.\n\n");
	ob->remove_user();
	return; }
#endif /* MAX_USERS */
 
 
//  If NO_SHUTDOWN_LOGIN is defined in /include/login.h and a non-admin
//  tries to login  during an ongoing shutdown, they will be informed of
//  the shutdown and when the mud is likely to be back up.
#ifdef NO_SHUTDOWN_LOGIN
    sd_time = (int)SHUTDOWN_D->query_shutdown();
    if (sd_time)
	sd_time = (sd_time - time()) / 60;
    if (sd_time && sd_time < 5 && !adminp(str))
      { write("\n\n  Sorry, a mud shutdown is currently in progress.\n  The " +
	      "mud should be back up in " + (sd_time + 3) + " minutes.\n" +
	      "  Hope to see you then!!\n\n");
	ob->remove_user();
	return; }
#endif /* NO_SHUTDOWN_LOGIN */
 
    if (str == "")
      { ob->remove_user();
	return; }
 
    if (strlen(str) > 11)
      { write("Sorry, your name can't have more than 11 characters.\n");
	write(LOGIN_PROMPT);
	input_to("get_name", 2, ob);
	return; }
 
    str = lower_case(str);
    for (i = 0; i < strlen(str); i++)
      { if (str[i] < 'a' || str[i] > 'z')
	  { write("Sorry, your name can only have letters. (a-z)\n" +
		  "Please enter a new name: ");
	    input_to("get_name", 2, ob);
	    return; } }
 
 
    //  Does the user even currently exist? If not, check for correct name.
    if (!file_exists(user_data_file(ob, str) + __SAVE_EXTENSION__))
      {
//  Enable NO_NEW_USERS to disable new character creation.
#ifdef NO_NEW_USERS
  write("\n\n\n\tSorry, "+capitalize(mud_name())+" is not open to new character creation.\n\n");
 if(NO_NEW_USERS)
write(NO_NEW_USERS);
 write("\n\n");
  ob->remove_user();
  return;
#endif /* NO_NEW_USERS */
 
//  Enable WIZ_lOCK in /include/login.h if you wish to only allow
//  wizards to login to the mud.  If you define a reason in WIZ_LOCK
//  it will be displayed before the user is dumped.
#ifdef WIZ_LOCK
  write("\n\n\n\t"+capitalize(mud_name())+" is presently closed to players.\n\n");
  if(WIZ_LOCK)
    write(WIZ_LOCK);
  write("\n\n");
  ob -> remove_user();
  return;
#endif /*WIZ_LOCK*/
 write("\n\"" + capitalize(str) + "\" is a new character.\n" +
	      "Is this really the name you wish to use? (y(es) or n(o)): ");
	input_to("choice", 2, ob, str);
	return; }
 
    seteuid(str);
    export_uid(ob);
    seteuid(getuid());
    ob->SET_NAME(str);
    if (!ob->restore())
      { write("Failed to restore login object.\n");
	ob->remove();
	return ; }
 
#ifdef WIZ_LOCK
    if (!ob->query("wizard"))
      { write("\n\n\n" + "\t" + capitalize(mud_name()) + " is presently " +
	      "closed to players.\n\n");
	if (WIZ_LOCK)
	    write( WIZ_LOCK );
	write("\n\n");
	ob->remove_user();
	return; }
#endif /* WIZ_LOCK */
 
    // If it is the Guest character, no password is asked for.
    if (str == "guest")
      { get_password("", ob, 1);
	return; } 
    input_to("get_password", 3, ob, 1);
    write(PASSWORD_PROMPT);
    return;
}
 
 
static void remove_copy(object ob)
{
    if (ob)
	ob->remove();
}
 
 
static void get_password(string pass, object ob, int count)
{
    object body;
    int hibernate;
 
    write("\n");
    if (!check_password(pass, ob) && (string)ob->query("name") != "guest")
      { write("Sorry, that password is incorrect.\n");
	if (count > 2)
	  { write("\nYou have taken too many tries.\n");
	    ob->set("passwd_fail", ({ query_ip_name(ob), time() }) );
#ifdef BAD_LOGIN
	    log_file(BAD_LOGIN,
ctime(time())[4..15] + " " + (string) ob -> query( "name" ) + " : " +
query_ip_name( ob ) + "\n" );
#endif
	    ob->remove_user();
	    return; }
	write("Please reenter your password: ");
	input_to("get_password", 3, ob, count + 1);
	return; }
 
    // This code checks to see if the user is in hibernation.
    hibernate = ob->query("hibernate");
    if (hibernate && time() < hibernate)
      { write("\nYour character is in hibernation until " + ctime(hibernate) +
       ".\nYou will be allowed to login again after that date. See you" +
       " then!\n");
	ob->remove_user();
	return; }
    else if (hibernate)
	ob->set("hibernate", 0);
    body = find_player((string) ob->NAME);
 
    // If user is stuck in login limbo ... dest old copy 
    if (body && !environment(body))
	body->remove();
 
    if (body)
      { ob->SET_BODY_OB(body);
	if (interactive(body))
 
//  If ONE_GUEST is defined in /include/login.h, only permit one guest login.
#ifdef ONE_GUEST
	  { if ((string)ob->query("name") == "guest")
	      { write("Sorry, there is already a Guest character active.\n" +
		      "Please try again shortly, or login as a new " +
		      "character.\n\n");
		call_out("remove_copy",1, this_player());
		return; }
#else /* ONE_GUEST */
	  { if ((string)ob->query("name") == "guest")
	      { exec_old_copy("n", ob);
		return; }
#endif /* ONE_GUEST */
	    write("\nYour other copy is still interactive.\n");
	    write("Do you want to throw it out? (y/n): ");
	    input_to("exec_old_copy", 2, ob);
	    return; }
 
//  If EXEC_COPY is defined in /include/login.h, log the exec copy call.
#ifdef EXEC_COPY
	log_file(EXEC_COPY, capitalize((string)ob->NAME) +
		 " :\texec copy from " + query_ip_address(ob) + " [" +
		 extract(ctime(time()),4,15) + "]\n");
#endif /* EXEC_COPY */
 
	if (ob->connect())
	    body->restart_heart();
	else
	  { write("Failed to reconnect...\n");
	    ob->remove(); }
	return; }
    login_new_copy(ob);
}
 
 
static
void login_new_copy(object ob)
{
    if (!ob->restore_body())
      { write("For some reason, your body could not be restored.\n" +
	      "You might want to send email to " + ADMIN_EMAIL +
	      " about this\nor log in with another name to alert the " +
	      "admins.\n");
	ob->remove();
	return; }
    if (ob->connect())
	enter_world(ob);
    else
      { write("Login failed... Sorry... \n");
	ob->remove(); }
}
 
 
static
void exec_old_copy(string s, object user)
{
    object tmp, link;
    string old_ip;
 
    if (member_array(s, ({ "y", "Y", "Yes", "yes" }) ) == -1)
      { if (!wizardp((object)user->BODY_OB) && 
	    (string)user->query("name") != "guest")
	  { write("Then come back another time.\n");
	    user->remove();
	    return; }
 
//  If NEW_COPY is defined in /include/login.h, log the new copy entry.
#ifdef NEW_COPY
	log_file(NEW_COPY, capitalize((string)user->NAME) +
		 " :\tNew copy from " + query_ip_name(user) + " [" +
		 extract(ctime(time()),4,15) + "]\n");
#endif /* NEW_COPY */
 
	login_new_copy(user);
	return; }
    //  Old connection object
    link = user->BODY_OB->query_link();
 
//  If FORCE_EXEC is defined in /include/login.h, log the new copy force.
#ifdef FORCE_EXEC
    log_file(FORCE_EXEC, capitalize((string)user->NAME) + " :\tForce exec " +
	     "from " + query_ip_name(user) + " [ " +
	     extract(ctime(time()),4,15) + "]\n");
#endif /* FORCE_EXEC */
 
    tell_object(user->BODY_OB,
		"\nYour character has been displaced by someone from " +
		query_ip_name(user) + ".\n");
    old_ip = user->BODY_OB->query("ip");

    tmp = new(CONNECTION);
    //  Exec them into any old object
    exec(tmp, user->BODY_OB);
    if (old_ip != query_ip_name(user) && old_ip != query_ip_number(user)) {
        user->BODY_OB->setup();
    }

    tmp->remove();
    if (user->connect())
	write("Reconnected.\n");
    else
	write("Reconnection failed.\n");
    link->remove();
    return;
}
 
 
static void enter_world(object user)
{
    mixed *bad_pass;
 
    seteuid(geteuid(user));
    export_uid(user->BODY_OB);
    seteuid(getuid());
    bad_pass = (mixed *)user->query("passwd_fail");
    if (bad_pass)
      { tell_object(this_player(), "\nWARNING: Login failure " +
		    ctime(bad_pass[1]) + " from " + bad_pass[0] + ".\n\n");
	user->set("passwd_fail", 0); }
    check_email(user);
    user->BODY_OB->setup();
}
 
 
static int check_password(string pass, object ob)
{
    string password;
 
    password = ob->PASS;
    if (password == crypt(pass, password))
	return 1;
    return 0;
}
 
 
static void choice(string choice, object user, string name)
{
    int i;
    string pass;
 
    write("\n");
    choice = lower_case(choice);
    if (member_array(choice, ({ "n", "no" }) ) >= 0)
      { write("Ok, please reenter it then: ");
	input_to("get_name", 2, user);
	return; }
    else if (member_array(choice, ({ "y", "yes" }) ) == -1)
      { write("Bad choice, please type y(es) or n(o): ");
	input_to("choice", user, name);
	return; }
 
//  If EMAIL_REGISTRATION has been defined in /include/login.h, check with
//  the banish daemon for a pre-registered name and password.
#ifdef EMAIL_REGISTRATION
    pass = BANISH_D->check_mailreg_name(name);
#else
    pass = "";
#endif
 
//  If REGISTER_MSG has been defined in /include/login.h, no new logins are
//  possible at this stage. REGISTER_MSG should be defined as the printed
//  message explaining why no unregistered users are being permitted.
#ifdef REGISTER_MSG
    if (!pass || !stringp(pass) || pass == "")
      { write(REGISTER_MSG);
	call_out("remove_copy", 1, user);
	return; }
#endif /* REGISTER_MSG */
 
//  If BANISHED_SITES has been defined in /include/login.h, check with
//  the banish daemon for use of a banished site.
#ifdef BANISHED_SITES
    //  Site banning code is from Dainia@DreamShadow
    //  Updated 7/13/93 by Karathan
    if (!pass || !stringp(pass) || pass == "")
      { i = BANISH_D->check_banned_site(query_ip_number());
	if (i < 0)
	  { write("\n  Could not resolve your machine's network ip number.\n" +
		  "  An accessible ip number must be available for user " +
		  "validation check.\n  Shutting down.\n\n");
 	   user->remove_user();
 	   return; }
	if (i)
	  { write("\n\n\tYour site has been placed on a registration by the\n" +
		  "\tadmins of " + mud_name() + ".  This means that you must " +
		  "mail\n\t\t" + ADMIN_EMAIL + "\n\tand request a character." +
		  "\n\n\t\t\t--The Management\n");
	    user->remove_user();
	    return; } }
#endif /* BANISHED_SITES */
 
 
//  If BANISHED_NAMES has been defined in /include/login.h, check with
//  the banish daemon for use of a banished name.
#ifdef BANISHED_NAMES
    if (BANISH_D->check_banned_name(name))
      { write("Sorry, that character name is restricted.\n\n" +
	      LOGIN_PROMPT);
	input_to("get_name", 2, user);
	return; }
#endif /* BANISHED_NAMES */
 
    seteuid(name);
    export_uid(user);
    seteuid(getuid());
    user->SET_NAME(name);
    NEWUSER_D->create_new_user(user, pass);
    return;
}
 
 
//  Check the user's email for new entries.
void check_email(object user)
{
    mapping mail_stat;
    int toread;
 
  if( file_exists( user_mbox_file( user->NAME ) + __SAVE_EXTENSION__ ) ) {
    mail_stat = (mapping) MAILBOX_D->mail_status(user->NAME);
    toread = mail_stat["unread"];
    if (toread)
	printf("\nYou have %d new piece%s of mail!\n\n",
 	toread, (toread == 1 ? "" : "s"));
  }
}
 
 
//  If online user display is selected, this function displays
//  all online, visible users to those in the login process.
string active_users()
{
    mixed *who;
    string output;
    int loop;
 
    who = users() ;
    who = filter_array(who, "filter_invis", this_object());
    if (!who || !sizeof(who))
	return "None.";
    who = map_array(who, "switch_name", this_object());
    who = uniq_array(who) ;
    if (sizeof(who) == 1)
	output = who[0] + ".";
    else
	output = implode(who[0..sizeof(who)-2], ", ") + ", and " +
		 who[sizeof(who)-1] + ".";
    return output;
}
 
 
static int filter_invis(object who)
{
    if (!who || !environment(who))
	// No logon's in display
	return 0;
    if ((int)who->query("npc"))
	// No monsters in display
	return 0;
#ifdef SUPPRESS_ADMIN_LOGIN
    if (adminp(geteuid(who)))
        return 0;
#endif
    return visible(who);
}
 
 
//  Switch user object pointer for name of user 
static string switch_name(object who)
{
    return capitalize((string)who->query("name")) ;
}

