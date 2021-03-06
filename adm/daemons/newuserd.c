// File : /adm/daemons/newuserd.c
//
// This is the character creation module.
// This daemon is called from logind.c if a player enters and requests an
// unused name. It then sets up the new user as per the user's preferences.
// Begun by Buddha, sometime in September 1992.
//
// Mobydick added race, skills, and stats, 10-11-92.
// Extended by Buddha in December 1992 and January 1993
//
// Psyche@TMI-2 (93/01/27) added acceptance of first-letter input
//	when choosing gender and race.
//
// Streamlined and option addition added by Watcher@TMI (2/23/93)
//
// Watcher@TMI (4/15/93) added AUTO_WIZHOOD option.
//
// Languages added by Megadeath@TMI-2
//
// Mail registration added by Karathan (8/12/93)
//
// 94-11-09 : Leto fixed languages to mapping instead of string *
// 95-04-30 : Blue made RACES a #define in <user2.h>.  Will do
//            genders sometime too.
// 96-01-14 : Dm made GENDERS a #define in <user2.h>. Modifed code
//            to same functionality as its RACES predecessor.

 
#include <logs.h>
#include <uid.h>
#include <priv.h>
#include <mudlib.h>
#include <config.h>
#include <login.h>
#include <login_macros.h>
#include <language.h>
#include <daemons.h>
#include <user2.h>
// Who knows why races are defined in newuserd, but hey.

protected void set_skills(object player);
protected void set_stats(object player);


void create()
{
    seteuid(ROOT_UID);
}


void create_new_user(object user, string pass)
{
    object body;

    if (geteuid(previous_object()) != ROOT_UID) return;
    user->SET_BODY(USER_OB); // let's assume a standard login
    body = new(USER_OB);

    body->set("name", (string)user->NAME, MASTER_ONLY);
    body->set ("cap_name", capitalize((string)user->NAME), MASTER_ONLY) ;
    body->set("snoopable", 1, MASTER_ONLY);

    user->SET_BODY_OB(body);

    cat(NPLAYER_INTRO);

//  If EMAIL_REGISTRATION is defined in /include/login.h then check to see if
//  this is a registered name with a predefined password.
#ifdef EMAIL_REGISTRATION
    if (pass && stringp(pass) && pass != "")
      { write("Please enter the password sent to you with your\n" +
	     "registration acceptance: ");
	input_to("get_pass", 3, pass, user, 0);
	return; }
#endif  /* EMAIL_REGISTRATION */

    write("Please enter a unique password for your character. Do not use\n"+
   "the password you use on your computer account, and please do not use\n"+
   "any word that appears in the dictionary.\n") ;
    input_to("new_pass", 3, user, 0);
}


protected void new_pass(string pass, object user, int count)
{
    if (strlen(pass) < 5)
      { write("\nI'm sorry, your password must be at least 5 characters.\n");
	if (count > 2)
	  { write("\nYou have taken too many tries.\n");
	    user->remove_user();
	    return; }
	write("Please change your password: ");
	input_to("new_pass", 3, user, count + 1);
	return; }
    write("\nPlease reenter your password to confirm: ");
    input_to("new_pass2", 3, pass, user, count);
}


protected void new_pass2(string pass2, string pass, object user, int count)
{
    if (pass == pass2)
      { user->SET_PASS(crypt(pass2, 0));
	write("\n\nYour gender can be male, female, neuter, or hermaphrodite." +
	      "\nPlease enter your gender: ");
	input_to("new_gender", 2, user, (object)user->BODY_OB, 0);
	return; }
    write("\nSorry, the passwords have to match.\n");
    if (count > 2)
      { write("\nYou have taken too many tries.\n");
	user->remove_user();
	return; }
    write("Please enter your character's password: ");
    input_to("new_pass", 3, user, count + 1);
}


protected void get_pass(string pass, string prev, object user, int count)
{
    if (crypt(pass, prev) != prev)
      { write("Sorry, that password is incorrect.\n");
	if (count > 2)
	  { write("\nYou have taken too many tries.\n");
	    user->remove_user();
	    return; }
	write("Please reenter your password: ");
	input_to("get_pass", 3, prev, user, count + 1);
	return; }
    user->SET_PASS(prev);
//    write("\n\nYour gender can be male, female, neuter, or hermaphrodite." +
//	  "\nPlease enter your gender: ");
    printf("\nYour gender can be:\n   %-=60s\n", implode(GENDERS, ", "));
    input_to("new_gender", 2, user, (object)user->BODY_OB, 0);
}


protected void new_gender(string g, object user, object body, int count) {
    string *rs;
    if (!g || sizeof(rs = regexp(GENDERS, "^"+g)) != 1) {
//    if (!g || member_array(g, ({"male", "female", "neuter", "hermaphrodite",
//    				"m", "f", "n", "h" })) == -1)
//      { write("\nYou can only be male, female, neuter, or hermaphrodite.\n");
      printf("\nYour gender can be:\n   %-=60s\n", implode(GENDERS, ", "));
	if (count > 2)
	  { write("\nYou have taken too many tries.\n");
	    user->remove_user();
	    return; }
	write("Please enter your gender: ");
	input_to("new_gender", user, body, count + 1);
	return; }

// Psyche@TMI-2 (93/27/01): translates one-letter input to
//			    corresponding full word
// I have NEVER seen this date convention before.  Please
// never use it again.  Blue.  950430, by the way.

//    switch(g)
//      { case "m": g = "male";
//		  break;
//	case "f": g = "female";
//		  break;
//	case "n": g = "neuter";
//		  break;
//	case "h": g = "hermaphrodite";
//		  break; }
    body->set("gender", g, READ_ONLY);
    printf("\nYour race can be one of:\n   %-=60s\n", implode(RACES, ", "));
//  write ("\nYour race can be human, elf, dwarf, gnome, or orc.\n") ;
    write ("Please enter your race: ") ;
    input_to("new_race", user, body, 0);
    return ;
}


protected void new_race(string r, object user, object body, int count)
{
    string *rs;
    if (!r || sizeof(rs = regexp(RACES, "^"+r)) != 1) {
//  if (!r || member_array(r, ({"elf", "dwarf", "gnome", "human", "orc",
// 				"e", "d", "g", "h", "o" })) == -1)
//    { write("\nYou must choose human, elf, dwarf, gnome, or orc.\n");
    printf("\nYour race can be one of:\n   %-=60s\n", implode(RACES, ", "));
	if (count > 2)
	  { write("\nYou have taken too many tries.\n");
	    user->remove_user();
	    return; }
	write ("Please enter your race: ") ;
	input_to("new_race", user, body, count + 1);
	return; }
//  switch(r)
//    { case "h": r = "human";
//		//break;
//	case "e": r = "elf";
//		//break;
//	case "d": r = "dwarf";
//		//break;
//	case "g": r = "gnome";
//		//break;
//	case "o": r = "orc";
//		//break; }
    body->set("race",r);
    write("\nPlease enter your email address (user@host): ");
    input_to("new_email", user, body, 0);
}


protected void new_email(string e, object user, object body, int count)
{
    string id, host;

    if (sscanf(e, "%s@%s", id, host) != 2 || id == "" || host == "")
      { write("Mail address must be in the form user@host\n");
	if (count > 2)
	  { write("\nYou have taken too many tries.\n");
	    user->remove_user();
	    return; }
	write("Please reenter your email address: ");
	input_to("new_email", user, body, count + 1);
	return; }
	if(e=="user@host") write("How original...ah well.\n");
    user->SET_EMAIL(e);
    write("Please enter your real name: ");
    input_to("get_real_name", user, body);
}


protected void get_real_name(string rn, object user, object body)
{
    if (!rn || rn == "")  rn = "???";
    user->SET_RNAME(rn);
 
    // Ok, that's all we need to know from them... let's get them connected.
    // the stats could be rolled, etc, but that's not my job to code -- buddha
 
    set_stats(body);
    set_skills(body);
 
    seteuid(geteuid(user));
    export_uid(body);
    seteuid(getuid());
    user->connect();
 
    cat(NPLAYER_NEWS);

//  If NEW_USER is defined in /include/logs.h then log the creation time.
#ifdef NEW_USER
    log_file(NEW_USER, capitalize((string)user->NAME) + " was created on " +
	     extract(ctime(time()), 4, 15) + " from " + query_ip_name() + ".\n");
#endif /* NEW_USER */
 
//  If AUTO_WIZHOOD is defined in /include/config.h then any newly created
//  users will automatically be granted wizard status, and given the PATH
//  given by the define. This would be handy on places like TMI where
//  wizard bits are freely given.
#ifdef AUTO_WIZHOOD
    user->set("wizard", 1);
    body->set("PATH", AUTO_WIZHOOD);
    write("\t[You have been granted automatic wizard status]\n");
#endif /* AUTO_WIZHOOD */
 
    body->setup();
    user->save_data();
    body->save_data();

//  If EMAIL_REGISTRATION is defined in /include/login.h then check to see if
//  this is a registered name that now can be removed from the file.
#ifdef EMAIL_REGISTRATION
    (void)BANISH_D->remove_mailreg_name(user->NAME);
#endif /* EMAIL_REGISTRATION */
}


int clean_up()
{
    destruct(this_object()); 
    return 1;
}


protected void set_stats(object player) {

	int strength, intelligence, dexterity, constitution ;
	int hp, sp, total ;
	//string *languages ;
	mapping languages; // Leto
	mapping stat ;
	//languages = ({ }) ;
	languages = ([]); // Leto
	stat = allocate_mapping(4);

	total = 0 ;
	while (total<50 || total > 70) {
		strength = 9+random(7)+random(7) ;
		intelligence = 9+random(7)+random(7) ;
		dexterity = 9+random(7)+random(7) ;
		constitution = 9+random(7)+random(7) ;
		total = strength + intelligence + dexterity + constitution ;
	}
	switch (player->query("race")) {
		case "human" : {
			languages = ([ "human" : 100 ]) ;
			break ;
		}
		case "elf" : {
			intelligence = intelligence + 3 ;
			constitution = constitution - 2 ;
			dexterity = dexterity + 1 ;
			strength = strength - 2 ;
			languages = ([ "elvish" : 100 ]) ;
			break ;
		}
		case "dwarf" : {
			intelligence = intelligence - 1 ;
			dexterity = dexterity - 2 ;
			constitution = constitution + 2 ;
			strength = strength + 1 ;
			languages = ([ "dwarvish" : 100 ]) ;
			break ;
		}
		case "gnome" : {
			strength = strength - 3 ;
			dexterity = dexterity + 3 ;
			constitution = constitution - 1 ;
			intelligence = intelligence + 1 ;
			languages = ([ "gnomish" : 100 ]) ;
			break ;
		}
		case "orc" : {
			strength = strength + 3 ;
			constitution = constitution + 1 ;
			dexterity = dexterity - 1 ;
			intelligence = intelligence - 3 ;
			languages = ([ "orcish" : 100 ]) ;
		}
	}
	stat["strength"] = strength ;
	stat["intelligence"] = intelligence ;
	stat["dexterity"] = dexterity ;
	stat["constitution"] = constitution ;
	player->set("stat", stat, LOCKED) ;

#ifdef LANGUAGES
	languages += ([ "common" : 100 ]) ;
	player->set("languages", languages);
#endif
	hp = 25 + constitution + random(21) ;
	sp = 45 + intelligence + random(21) ;
	player->set("hit_points", hp) ;
	player->set("max_hp", hp, MASTER_ONLY);
	player->set("spell_points", sp) ;
	player->set("max_sp", sp, LOCKED);
	return ;
}

protected void set_skills(object player) {
	player->wipe_skills() ;
 player->set_skill("Thrusting weapons",0,"strength") ;
 player->set_skill("Cutting weapons",0,"strength") ;
 player->set_skill("Blunt weapons",0,"strength") ;
 player->set_skill("Parrying defense",0,"dexterity") ;
 player->set_skill("Shield defense",0,"dexterity") ;
 player->set_skill("Combat spells",0,"intelligence") ;
 player->set_skill("Healing spells",0,"intelligence") ;
 player->set_skill("Divinations spells",0,"intelligence") ;
	player->set_skill("Wilderness",0,"dexterity") ;
 player->set_skill("First aid",0,"dexterity") ;
 player->set_skill("Theft",0,"dexterity") ;
 player->set_skill("Stealth",0,"dexterity") ;
	return ;
}
