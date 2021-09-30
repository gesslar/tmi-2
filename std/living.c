// living.c
// Part of the new bodies project, which was launched by Watcher@TMI-2 and
// Mobydick@TMI-2.
// Based on the login code written by Buddha@TMI-2 in January 1993.
// This file was started 3-11-93 by Mobydick. All of the code that it
// inherits was pre-existing, and the appropriate headers in those files
// have been retained.

#include <mudlib.h>
 
inherit BODY_BASE ;
 
inherit "/std/living/spells.c" ;
inherit "/std/living/skills.c" ;
inherit "/std/living/edit.c" ;
inherit "/std/object/contents" ;
inherit "/std/living/env.c";
inherit "/std/coinvalue.c" ;
inherit "/std/living/messages.c";

 
// This procedure returns the total number of coins the player is carrying.
// Mobydick, 10-13-92.

nomask int coins_carried() {

	mapping wealth ;
	string *types ;
	int i, coins ;

	coins = 0 ;
	wealth = query("wealth") ;
	if (!wealth || !mapp(wealth) || wealth == ([ ]))  return 0;
	types = keys(wealth) ;
	for (i=0;i<sizeof(types);i++) {
		coins = coins + wealth[types[i]] ;
	}
	return coins ;
}


// This function returns the player's total wealth in value units.
// Mobydick, 11-5-92.

nomask int total_wealth() {

        mapping wealth ;
        string *types ;
        int i, coins ;

        coins = 0 ;
        wealth = query("wealth") ;
        if (!wealth || wealth==([])) return 0 ;
        types = keys(wealth) ;
        for (i=0;i<sizeof(types);i++) {
		coins = coins + wealth[types[i]]*coinvalue(types[i]) ;
        }
        return coins ;
}

//	This function is used to check if the user is able to 
//	see anything around it.       Watcher  (01/29/93)
 
int query_vision() {
   mixed *contents;
   int loop;
 
   //	If there is no environment, you obviously can't see anything. :)
 
   if(!environment())  return 0;
 
   // 	Check to see if user has special vision capabilities,
   //	either artificial or permanent
 
   if(query("full_vision"))  return 1;
 
   //	Check to see if user is blind, or environment prevents sight
 
   if(query("blind") || (int)environment()->query("no_vision"))
	return 0;
 
   //	Check light level in the environment
 
   if((int)environment()->query("light") > 0)  return 1;
 
   contents = all_inventory(environment());
 
   //	Check to see if any of the environment's contents have
   //	a light source of any kind
 
   for(loop=0; loop<sizeof(contents); loop++)
   if(contents[loop]->check_light())  return 1;
 
return 0; }
 
// These functions provides a mechanism for a player to be credited and
// debited with money.  Calling set or add bypasses certain checks
// and does not account for encumbrance.

int credit(string type, int amount)
{
    if(!stringp(type) || !type || !intp(amount) || amount<=0)
	return 0;
    if(query("capacity") < amount)
	return 0;
    add("capacity",-amount);
    add("wealth/"+type,amount);
    return 1;
}

int debit(string type, int amount)
{
    if(!stringp(type) || !type || !intp(amount) || amount<=0)
	return 0;
    if(query("wealth/"+type) < amount)
	return 0;
    add("capacity",amount);
    add("wealth/"+type,-amount);
    return 1;
}
