// goo.c - a simple smartmonster to show how to configure the responses
// for emots, say's, and tell's.  This is concidered a basic monster
// configuration...
// Creator - Kinslayer@Emperis
// fixed a bad return type , Leto 94-11-11

#include <mudlib.h>
inherit "/std/smartmonster.c" ;
 
void create () {
 
        object ob1, ob2, ob3 ;
 
        ::create() ;
        seteuid(getuid()) ;
	ob_data["short"]="Blue-Goo";
ob_data["long"]=@TEXT
Blue Goo, is one of Kinslayer's smartmonsters.  Goo responds to a few soul 
commands and a few other things that you say or tell it.  tell goo 'help'
for a list of things it responds to. 
TEXT
;
        set ("id", ({ "goo", "blue"}) ) ;
        set ("capacity", 10000) ;
        set ("max_cap", 10000) ;
        set ("max_vol", 10000) ;
        set ("volume", 10000) ;
        set ("mass", 3000) ;
        set ("bulk", 250) ;
        set_name ("Blue-Goo") ;
        set ("gender", "nuter") ;
        enable_commands() ;
        set_living_name("goo") ;
        set("race", "blob");
        set ("hit_points", 10) ;
        set ("max_hp",10) ;
        set ("attack_strength", 1) ;
        set ("damage", ({ 1,4 }) ) ;
        set ("weapon_name", "goo") ;
        set_skill("attack", 3, "strength") ;
        set_skill("defense", 8, "dexterity") ;
 
// Smartmonster data:
 
// set_response_object() - Used to set the object whcich is used to parse
// the responses.
 
        set_response_object(this_object()); 
 
// add_responses() - Used mostly so that the monster can respond
// to emotes and soul commands and things of that nature.
// add_response("ACTION", "REPLY-MESSAGE/COMMAND/L_FUN", "[PERSONAL-REPLY]", [CHANCE]);
// use '!' if you want to tell the monster to preform a command as the response
// '*' is used if an l_fun is used to handle the response
 
        add_response("smiles happily at you", "!smile $OTHER");
        add_response("smiles.", "Blue-Goo looks at $OTHER\n", 90);
        add_response("hugs you", "!hug $OTHER");
        add_response("giggles", "Goo looks at $OTHER and smirks.\n");
        add_response("smirks", "Blue-Goo smirks too!\n", "Goo tells you: What are you smirking at?\n");
 
// add_say_response() - Used if the trigger is a say or tell. has the same
// arguments as does the add_response() an implimentation of the
// l_fun response is used here.
 
        add_say_response(({"goo", "Blue-Goo"}), "!say Yes! that's right I'm the new and improved Blue-Goo! I respond to you!");
        add_say_response("hello", "!say Hello $OTHER");
        add_say_response("help", "*handle_help");
}
 
int handle_help(string who_obj, string who_str, string phrase) {
 
	if (phrase != "goo")  return 0;
        tell_object(find_player(lower_case(who_obj)), sprintf("Hello %s here is  a list of the things I respond to:\n %O", who_str, this_object()->query_responses()));

        say(sprintf("%s, %s, %s", who_obj, who_str, phrase));
	return 1;
}
