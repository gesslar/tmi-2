/*
 * File: /std/user.c
 * The user body is from the TMI-2 lib. Part of the bodies project and
 * implemented by Watcher@TMI-2 and Mobydick@TMI-2. The code contained
 * in this object is heavily based on code found in the original user.c
 *
 * The relevant code headers follow:
 * Original Authors: Sulam and Buddha @ The Mud Institute
 * Many other people have added to this as well.
 * This file has a long revision history dating back to the hideous
 * mudlib.n and is now probably not at all the same.
 * This file is part of the TMI distribution mudlib.
 * Please keep this header with any code within.
 *
 * 94-11-09 Leto added Beek's error handler code
 * 94-11-28 Leto added Beek's move_or code
 * 94-11-30 Leto added receive_snoop
 * 95-01-11 Blue modified receive_snoop slightly.
 */

#define DEBUG

#include <config.h>
#include <login.h>
#include <commands.h>
#include <daemons.h>
#include <net/daemons.h>
#include <mudlib.h>
#include <flock.h>
#include <move.h>
#include <money.h>
#include <priv.h>
#include <driver/origin.h>
#include <uid.h>
#include <switches.h>
#include <messages.h>

/*
 * Some files are inherited by including user.h.
 */
#include <user.h>
#include <user2.h>
#include <logs.h>
#include <body.h>
#include <login_macros.h>

/*
 * This is only needed for consistency_check.
 */
#include <channels.h>
#include <domains.h>

/*
 * For receive_snoop
 */
#include <ansi.h>

inherit LIVING ;

/*
 * This is probably only a local hack.
 */
#ifndef CAP_NAME_MASTER_ONLY
#  undef CAP_NAME
#  undef LINK_CAP_NAME
#  define CAP_NAME capitalize(query("name"))
#  define LINK_CAP_NAME capitalize(link_data("name"))
#endif

#define HALL "/d/Fooland/hall"

/*
 * prototypes for local functions
 */
protected varargs void complete_setup (string str);
protected void die();
protected int create_ghost();
int coins_carried();
void init_setup();
void destroy_autoload_obj();
varargs void execute_attack(int hit_mod, int dam_mod);
void call_user_dump(string type);

/*
 * Setup basic and command catch systems
 */
void basic_commands() {
    add_action("quit", "quit");
}

protected void init_commands() {
    string path;

    add_action( "cmd_hook", "", 1 );

    path = query("PATH");

    if (!path) {
        if ( wizardp(this_object()) )
            path = NEW_WIZ_PATH;
        else
            path = USER_CMDS;

        set("PATH", path, MASTER_ONLY);
    }
}

/*
 * Setup standard user command hook system.  This system interfaces
 * with the cmd bin system, the environment's exits, and feeling entries.
 */
nomask protected int cmd_hook(string cmd) {
    string file;
    string verb;
    int foo;
    mapping before, after;

    verb = query_verb();

    if (environment() && environment()->valid_exit(verb)) {
        verb = "go";
        cmd = query_verb();
    }

    file = (string)CMD_D->find_cmd(verb, explode(query("PATH"), ":"));

    if (file && file != "") {
#ifdef PROFILING
        before = rusage();
#endif
        foo = (int)call_other(file, "cmd_" + verb, cmd);

#ifdef PROFILING
        after = rusage();
              "/adm/daemons/profile"->log_cmd(verb,before,after);
#endif

        return foo;
    }

    if (environment() && environment()->query("quiet"))
        return 0;

#ifdef PROFILING
    before = rusage();
#endif

    foo = (int)EMOTE_D->parse(verb, cmd);

    if (foo) {
#ifdef PROFILING
        after = rusage();
        "/adm/daemons/profile"->log_cmd(verb,before,after);
#endif
        return foo;
    }

#ifdef PROFILING
    before = rusage();
#endif

#ifndef INTERMUD
    if (verb == "gwiz" || verb == "interwiz") {
        printf("Sorry, %s does not support intermud.\n",capitalize(mud_name()));
        return 1;
    }
#endif /* INTERMUD */

    foo = (int) CHANNELS_D -> parse_channel( verb, cmd );

#ifdef PROFILING
    if ( foo ) {
        after = rsuage();
        "/adm/daemons/profile" -> log_cmd( verb, before, after );
    }
#endif

    return foo;
}


/*
 * This function protects the object from being shadowed for
 * certain secure functions.
 */
nomask int query_prevent_shadow(object ob) {
    mixed *protect;
    int loop;

    protect = ({ "catch_tell", "receive_message", "do_alias", "do_xverb",
                 "do_substitution", "process_input", "tsh",
                 "do_nickname", "receive_snoop" });

    for (loop = sizeof(protect); loop--; )
        if (function_exists(protect[loop], ob))  return 1;

    return 0;
}

/*
 * Move the player to another room. Give the appropriate
 * message to on-lookers.
 * The optional message describes the message to give when the player
 * leaves.
 */
varargs int move_player(mixed dest, string message, string dir) {
    object prev;
    int res;

    prev = environment( this_object() );
    if ( res = move(dest) != MOVE_OK ) {
        message(MSG_INFO, "You remain where you are.\n", this_player());
        return res;
    }

    if (environment() && wizardp(this_object()) && query("display_path"))
        message(MSG_LOOK, sprintf("[%s]\n", file_name(environment())),
              this_object());

    if (message == "SNEAK") {
        set_temp("force_to_look", 1);
        command("look");
        set_temp("force_to_look", 0);
        return 0;
    }

    if (!query("invisible")) {

        if (message == 0 || message == "") {

            if (dir && dir != "") {
                message(MSG_MOVEMENT, (string)query_mout(dir) + "\n",
                      prev, this_object());
                message(MSG_MOVEMENT, (string)query_min() + "\n",
                      environment(), this_object());
            } else {
                message(MSG_MOVEMENT, (string)query_mmout() + "\n",
                      prev, this_object());
                message(MSG_MOVEMENT, (string)query_mmin() + "\n",
                      environment(), this_object());
            }
        } else {
            message(MSG_MOVEMENT, message + "\n", prev, this_object());
            message(MSG_MOVEMENT, (string)query_min() + "\n", environment(),
                  this_object());
        }
    }

    set_temp ("force_to_look", 1);
    command("look");
    set_temp("force_to_look", 0);


    /*
     * Follow/track mudlib support
     */
    if (!query("no_follow") && environment() != prev && prev)
        all_inventory(prev)->follow_other(this_object(), environment(), dir);

    return 0;
}

private int clean_up_attackers() {
    mixed *attackers_tmp;
    int i;

    attackers_tmp = ({ });

    for (i = sizeof(attackers); i--; ) {
        /*
         * If he's dead, then forget about him entirely.
         */
        if (attackers[i] == 0 || !living(attackers[i]))
            continue;

        /*
         * If he's not here, then forget about him.
         */
        if (environment(attackers[i]) != environment(this_player()))
            continue;

        /*
         * If he's a ghost, we've done enough to him already :)
         */
        if ((int)attackers[i]->query_ghost() == 1) continue;

        /*
         * If we get this far, then we still want to be attacking him
         */
        attackers_tmp += ({ attackers[i] });
    }

    /*
     * Copy the tmp list over to the attackers list.
     */
    attackers = attackers_tmp;

    if (sizeof(attackers_tmp) == 0)
        any_attack = 0;

    return any_attack;
}

/*
 * Continue_attack is called from heart_beat..
 * here is where we can try to see if we're dead or in combat.
 */
void continue_attack() {
    /*
     * Check if this object has just died. if so, do the death stuff.
     */
    if (query("hit_points") < 0) {
        die();
        return;
    }

    /*
     * If there's no one to attack, then we are finished.
     */
    if (!any_attack)  return;

    /*
     * Call the clean_up_attackers function to see who's left. If it
     * returns 0, then there's no one left.
     */
    if (clean_up_attackers() == 0) {
        /*
         * No attackers in the room
         */
        message(MSG_COMBAT, "The combat is over.\n", this_player());
        return;
    }

    /*
     * Check to see if the player is doing something that prevents
     * him from making an attack.
     */
    if (query("stop_attack")>0) {
        message(MSG_COMBAT, "You are too busy to make an attack!\n",
              this_player());
        return;
    }

    /*
     * Check to see if we're under wimpy, and if so, run away.
     */
    if (query("hit_points")*100/query("max_hp") < this_player()->query("wimpy")) {
        run_away();
        return;
    }

    execute_attack();
}

protected int loop, noise;

/*
 * This is the big, ugly CPU hogging function where the combat actually
 * takes place.
 */
varargs void execute_attack (int hit_mod, int dam_mod) {
    int att_sk, def_sk, str, dex, ac, wc, hit_chance;
    string name, verb1, verb2, vname, damstr, damstr2, wepstr, *verbs;
    mixed *contents;
    object *prots;
    object weapon;
    int old_inv, s;
    string victim, posgender;
    int *damrange;

    /*
     * Check to see if primary target is dead, if so move to the next attack
     * in the attackers queue. If the attackers queue is empty, stop attack call.
     */
    if (attackers[0]->query("hit_points") < 0) {
        attackers -= ({ attackers[0] });
        if (!attackers || !sizeof(attackers))  return;
    }

    /*
     * Is the target being protected? If so, find the alternate target.
     */
    prots = attackers[0]->query("protectors");
    if (prots && sizeof(prots) > 0) {
        /*
         * Get rid of all ineligible protectors: dead or not present.
         */
        prots = filter_array(prots,"valid_protect", this_object());

        /*
         * If there are eligible protectors, then move the protector to the top
         * of the list, adding him if needed.
         */
        if (s = sizeof(prots)) {

            /*
             * Ok, I tried fixing it, but it is rather complex. In any case,
             * I do not think it is doing what it is supposed to do.
             * Someone mixed strings and objects all the way through... Leto
             */
//          victim = prots[random(s)];
            weapon = prots[random(s)];

            /*
             * re-using a variable to save memory - sorry bout that :(
             */
//          weapon = find_player(victim); //Leto
            ac = member_array(weapon,attackers);
            if (ac>-1) {
                attackers[ac]=attackers[0];
                attackers[0]=weapon;
            } else {
                attackers = ({ weapon }) + attackers;
                weapon -> kill_ob(this_object());
            }
        }
    }

    /*
     * hit_mod and dam_mod are modifiers that can be passed to the attack.
     * The heartbeat doesn't add them but you can make special attacks by
     * calling execute_attack directly. Be careful if you do so, you'll want
     * to also call kill_ob to make sure a fight starts...
     */
//  if (!hit_mod) hit_mod=0;
//  if (!dam_mod) dam_mod=0;

    /*
     * Collect the various statistics needed to get the hit chance and damage.
     */
    str = query("stat/strength");
    dex = attackers[0]->query("stat/dexterity");
    ac = attackers[0]->query("armor_class");
    weapon = query("weapon1");

    /*
     * If they don't have a weapon, they get their intrinsic combat skills.
     */
    if (!weapon) {
        /*
         * These are the numbers/strings for a unarmed user attack. They're
         * hard-coded but you could equally well set them as properties in the
         * user object.
         */
        wc = 2;
        dmin = 1;
        dmax = 3;
        verb1 = "swing at";
        verb2 = "swings at";
        wepstr = "fists";
        weapontype = "Blunt weapons";
    } else {
        /*
         * If we get here, then the player has a weapon, and we query the
         * weapon for its attack properties.
         * Does he have a second weapon? If so, use it on 20% of attacks.
         */
        if (query("weapon2") && random(5)==0) {
            weapon = query("weapon2");
        }
        wc = query("attack_strength");
        damrange = weapon->query("damage");
        dmin = damrange[0];
        dmax = damrange[1];
        verbs = allocate(2);
        verbs = weapon->get_verb();
        verb1 = verbs[0];
        verb2 = verbs[1];
        wepstr = (string)weapon->query("name");
        weapontype = capitalize(weapon->query("type")+" weapons");
    }

    name = query("cap_name");
    vname = attackers[0]->query("cap_name");
    posgender = possessive(query("gender"));

    /*
     * Check the attacker's attack skill and the defenders skill(s).
     *
     * If a player, check his weapons skills. Otherwise, use the attack skill
     * for a monster.
     */
    att_sk = query_skill(weapontype);

    /*
     * Ditto for the defender.
     */
    if (!(int)attackers[0]->query_monster()==1) {
        if (attackers[0]->query("armor/shield")) {
            def_sk = attackers[0]->query_skill("Shield defense");
        } else {
            def_sk = attackers[0]->query_skill("Parrying defense");
        }
    } else {
        def_sk = attackers[0]->query_skill("defense");
    }

    /*
     * This is the combat formula.
     * If you are using drunkenness, and want it to affect combat, then
     * call query("drunk"), which goes 1-25, and subtract it from the
     * hit chance.
     */
    hit_chance = 30 + str + att_sk + 3*wc - dex - def_sk - 3*ac + hit_mod;

    /*
     * Attacking invisible creatures is really rather difficult.
     */
    if ((int)attackers[0]->query("invisible")>0)
        hit_chance /= 5;

    /*
     * The hit chance is constrained to be between 2 and 98 percent.
     */
    if (hit_chance<2)  hit_chance = 2;
    if (hit_chance>98)  hit_chance = 98;

    /*
     * Improve the skills of the attacker and defender.
     *
     * The probability of the skill improving depends on the hit chance. If the
     * hit chance is 0 or 100, the skill does not improve. If the hit chance is
     * 50%, then the skill improves automatically. The closer the hit chance is
     * to 50%, the more likely the skill is to improve. This rewards players for
     * taking on monsters roughly equal in skill to themselves.
     */
    skill_improve_prob = hit_chance * (100-hit_chance) / 25;
    if (random(100)<skill_improve_prob) {
        improve_skill(weapontype,1);
    }
    if (random(100)<skill_improve_prob) {
        if (!attackers[0]->query_monster()) {
            if (attackers[0]->query("armor/shield")) {
                attackers[0]->improve_skill("Shield defense",1);
            } else {
                attackers[0]->improve_skill("Parrying defense",1);
            }
        }
    }

    /*
     * Get a list of all in the room who are listening to the battle.
     */
    contents = all_inventory(environment(this_object()));
    contents = filter_array(contents, "filter_listening", this_object());

    /*
     * This is the damage formula.
     * We have to calculate this first because we don't want to print messages
     * of the form "You hit for zero damage." If the damage is less than zero,
     * we print a "You miss" message regardless of the hit_chance roll.
     */
    damage = random(dmax-dmin+1)+dmin+str/8-1+att_sk/10-def_sk/5+dam_mod;

    /*
     * Before we print any messages, we need to make ourselves visible.
     * Otherwise the person we're attacking doesn't get the message. We
     * become invisible again after the messages are printed.
     * This is klugey but real real easy.
     */
    old_inv = query("invisible");
    set ("invisible", 0);

    /*
     * If positive damage, and the hit lands, then we do damage and print
     * the appropriate damage messages.
     */
    if (damage>0 && random(100)<hit_chance) {

        str = attackers[0]->query("hit_points");
        if (damage)  attackers[0]->receive_damage(damage);
        qs = objective((string)attackers[0]->query("gender"));

        /*
         * We print different damage messages based on how much damage was
         * done. You might want to make this system a little more interesting.
         * Go nuts.
         */
        switch (damage) {
            case 1:
                damstr = "scratch "+qs+".";
                damstr2 = "scratches "+qs+".";
                break;
            case 2..3 :
                damstr = "do light damage.";
                damstr2 = "does light damage.";
                break;
            case 4..6 :
                damstr = "hit.";
                damstr2 = "hits.";
                break;
            case 7..9 :
                damstr = "deliver a solid blow.";
                damstr2 = "delivers a solid blow.";
                break;
            case 10..15 :
                damstr = "hit hard!";
                damstr2 = "hits hard!";
                break;
            case 16..20 :
                damstr = "inflict massive damage!";
                damstr2 = "inflicts massive damage!";
                break;
            default :
                /*
                 * Mobydick just ran out of ideas at this point.  :)
                 */
                damstr = "whomp "+qs+"!";
                damstr2 = "whomps "+qs+"!";
        }


        /*
         * The routines below check to see if all the listeners really
         * want to hear how the battle is going (Watcher, 4/27/93).
         */
        if (!(query("noise_level") && damage < 2))
            message(MSG_COMBAT,
                  sprintf("You %s %s with your %s and %s\n",
                        verb1, vname, wepstr, damstr),
                  this_player());

        for (loop = sizeof(contents); loop--; ) {
            if (contents[loop])
                noise = (int)contents[loop]->query("noise_level");

            if (noise && (noise > 1 || (noise == 1 && damage < 2)))
                continue;

            message(MSG_COMBAT,
                  sprintf("%s %s %s with %s %s and %s\n",
                        name, verb2, vname, posgender,
                        wepstr, damstr2), contents[loop]);
        }

        if (attackers[0] &&
              !(attackers[0]->query("noise_level") && damage < 2))
            message(MSG_COMBAT,
                  sprintf("%s %s you with %s %s and %s\n",
                        name, verb2, posgender,
                        wepstr, damstr), attackers[0]);
    } else {
        /*
         * If we got here, it means we missed the hit roll, or did zero damage.
         */
        if (!query("noise_level"))
            message(MSG_COMBAT,
                  sprintf("You %s %s with your %s but you miss.\n",
                        verb1, vname, wepstr),
                  this_player());

        for (loop = sizeof(contents); loop--; )
            if (contents[loop] && !contents[loop]->query("noise_level"))
                message(MSG_COMBAT,
                      sprintf("%s %s %s with %s %s but misses.\n",
                            name, verb2, vname, posgender, wepstr),
                      contents[loop]);

        if (attackers[0] && !attackers[0]->query("noise_level"))
            message(MSG_COMBAT,
                  sprintf("%s %s you with %s %s but misses you.\n",
                        name, verb2, posgender, wepstr),
                  attackers[0]);
    }

    /*
     * Restore the old invisibility setting.
     */
    if (old_inv)
        set ("invisible", old_inv);

    return;
}

/*
 * This function filters out the living objects who are listening
 * to the present battle.
 */
int filter_listening(object obj) {
    if (obj == this_object() || obj == attackers[0])  return 0;
    return living(obj);
}

/*
 * This one filters dead or absent people from an array.
 */
int valid_protect (string str) {

    object foo;

    foo = find_player(str);
    if (!foo)  return 0;
    if (environment(foo) != environment(this_object())) {
        return 0;
    }
    if ((int)foo->query("hit_points")<0)  return 0;
    return 1;
}


void create() {

    if (user_exists(getuid()))  return;

    /*
     * Until the user's name and id is set ... give it a temporary one.
     */
    set("name", "noname", MASTER_ONLY);
    set("id", ({ "noname" }));

    /*
     * We set EUID of 0 so that the login daemon can export the proper
     * UID onto the player. If you are running without AUTO_SETEUID, then
     * this has no effect, but under auto-EUID-setting it's important.
     * Also it makes it mildly harder for people to get themselves into
     * trouble by cloning user.c.
     */
    seteuid(0);


    /*
     * there's some standard properties that need to be locked so that
     * Joe Random Wizard can't break security by setting them on people
     * who haven't used them yet.
     */
    set("npc", 0, LOCKED);
    set("snoopable", 0, MASTER_ONLY);
    set("invisible", 0, MASTER_ONLY);
    set("short", "@@query_short");
    set("cap_name", capitalize(query("name")), MASTER_ONLY);
    set("title", "@@query_title", MASTER_ONLY);
    set("linkdead", "@@query_linkdead");
    set("age", 0, MASTER_ONLY);
    set("ghost", 0, MASTER_ONLY);
    set("shell", "none", MASTER_ONLY);
    set("user", 1, MASTER_ONLY);
    set("vision", "@@query_vision");
    set("harass", 0, OWNER_ONLY);
#ifdef NO_PKILL
    set("no_attack", 1, OWNER_ONLY);
#else
    set("no_attack",0, OWNER_ONLY);
#endif
    alias = ([ ]);

    /*
     * Complete the standard user attribute settings.
     */
    set("volume", MAX_VOLUME);
    set("capacity", MAX_CAPACITY);
    set("mass", USER_MASS);
    set("bulk", USER_BULK);
    set ("time_to_heal", 10);
    set("short", "@@query_short");
    set("channels", "", MASTER_ONLY);
    enable_commands();
}

void remove() {
    string euid;
    mixed *inv;
    int loop;
    object prev;

    if (prev = previous_object()) {
        euid = geteuid(previous_object());
        if ( (euid != ROOT_UID) && 
              base_name(prev) != "/std/connection" &&
              (euid != geteuid(this_object())) &&
              !adminp(euid) ) {
            message(MSG_SYSTEM, "You may not remove other players.\n",
                  this_player());
            return;
        }
    }
    seteuid(euid);
    free_locks(this_object());

    save_data();
    destroy_autoload_obj();

    inv = all_inventory(this_object());
    for (loop = sizeof(inv); loop--; )
        if (inv[loop]->query("prevent_drop"))
            inv[loop]->remove();

//  CMWHO_D->remove_user(this_object());
    if (link)  link->remove();
    living::remove();
}

nosave int in_de_quit_script;

varargs int quit(string str) {
    object *stuff, *inv;
    int i, j;

    if (!(origin() == ORIGIN_LOCAL || origin() == ORIGIN_DRIVER)
              && geteuid(previous_object()) != ROOT_UID)
        return 0;

    if (str) {
        notify_fail("Quit what?\n");
        return 0;
    }

    /*
     * If the #define is on, then save their location for starting next time.
     */
#ifdef REAPPEAR_AT_QUIT
    if (environment(this_player())) {
        set ("start_location", file_name(environment(this_player())));
    } else {
        set ("start_location", START);
    }
#endif

    /*
     * Free any outstanding file locks.
     */
    free_locks(this_object());

    // Deregister with channel daemon
#ifdef INTERMUD
    CHANNELS_D->cleanup_user(this_object());

#endif

    /*
     * Get rid of any party memberships.
     */
//  check_team();
    PARTY_D->check_party(this_object());

    if ( wizardp( this_object() ) )   {
        string quit_script; // Pallando 93-02-11

        quit_script = user_path( query( "name" ) ) + ".quit";
        if ( file_size( quit_script ) > 0 ) {
            if (in_de_quit_script++)
                message(MSG_SYSTEM,
                      "Oi, stupid! Don't put `quit' in your ~/.quit file\n",
                      this_object());
            else
                call_other( this_object(), "tsh", quit_script );
            in_de_quit_script = 0;
        }
    }

    /*
     * If the "harass" logging is still on, then turn it off.
     */
    if (query("harass") > 0 ) {
        set("harass", 0);
        message(MSG_INFO, "Harass Log turned off.\n", this_object());
    }


    /*
     * If this is an invisible wizard, we let the invisibility stay on: but
     * if it's a player who cast the invisibility spell, then we want him
     * to be visible when he reappears.
     */
    if (!wizardp(this_object()))  set("invis", 0);

    if (link) {
//      link->set("last_on", time());
        link->set("ip", query_ip_name(this_object()));
    }

#ifdef QUIT_LOG
    if (link)
        log_file(QUIT_LOG, CAP_NAME + ": quit from " +
              query_ip_name(this_object()) + " [" +
              extract(ctime(time()), 4, 15) + "]\n");
    else
        log_file(QUIT_LOG, CAP_NAME +
              ": swept [" + extract(ctime(time()), 4, 15) + "]\n");
#endif

    /*
     * First save the user's personal data.
     */
    save_data();

    /*
     * Then destroy autoloading inventory.
     */
    destroy_autoload_obj();

    /*
     * Now drop everything droppable on the user.
     */
    inv = all_inventory( this_object() );
    inv = filter_array(inv, "inventory_check", this_object());

    if (inv && sizeof(inv))  command("drop all");

    /*
     * Announce the departure of the user.
     */
    if (this_object() && visible(this_object()))
        message(MSG_MOVEMENT, query("cap_name") + " left the game.\n",
              environment(), this_player());

    if (this_object() && interactive(this_object()))
        ANNOUNCE->announce_user(this_object(), 1);

#ifdef LOGOUT_MSG
    if (previous_object() == this_player() && this_player())
        message(MSG_INFO, LOGOUT_MSG, this_object());
#endif

    /*
     * Clean up a few loose ends before shutting down the user.
     */
//  CMWHO_D->remove_user(this_object());
    if (link)  link->remove();
    living::remove();

    return 1;
}


/*
 * This function determines if the user has anything droppable
 * when they quit the mud.
 */
protected int inventory_check(object obj) {
    if (obj->query_auto_load())  return 0;
    if (!obj->query("short") || obj->query("prevent_drop"))  return 0;
    return 1;
}


/*
 * This procedure is called from the setup() function below.  It is
 * basically here to check that existing users get whatever new settings
 * they need to function in today's changing mudlib.
 */
void consistency_check() {
    int i,j;
    mapping doms, doms2;
    string *domlist;

    /*
     * if you think everyone has been "fixed" then what you put here should
     * moved to create() and taken out.
     * right now it's empty because hopefully everyone has been updated.
     */
//  set("no_attack",0,READ_ONLY);

    /*
     * I don't know why this was done.  I'm changing back to the
     * original _stopping_ player killing.  Blue.
     */
#ifdef NO_PKILL
    set("no_attack", 1, OWNER_ONLY);
#endif

    /*
     * Added by Leto to fix people with stupid vol/cap, causing
     * the quad bug to appear more often.
     */
    if (query("volume") < 0)  {
        message(MSG_SYSTEM,
              "\nVolume too small, fixed by consistency check.\n\n",
              this_object());
        set("volume",500);
        log_file("consistency_check",
              "Fixed negative volume of "+query("cap_name") + "\n");
    } else if (query("volume") > 10000) {
        message(MSG_SYSTEM,
              "\nVolume too large, fixed by consistency check.\n\n",
              this_object());
        set("volume",500);
        log_file("consistency_check",
              "Fixed too large volume of "+query("cap_name") + "\n");
    }
    if (query("capacity") < 0) {
        message(MSG_SYSTEM,
              "\nCapacity too small, fixed by consistency check.\n\n",
              this_object());
        set("capacity",5000);
        log_file("consistency_check",
              "Fixed negative capacity  of "+query("cap_name") + "\n");
    } else if (query("capacity") > 10000) {
        message(MSG_SYSTEM,
              "\nCapacity too large, fixed by consistency check.\n\n",
              this_object());
        set("capacity",5000);
        log_file("consistency_check",
              "Fixed too large capacity of "+query("cap_name") + "\n");
    }
    if (query("bulk") < 0) {
        message(MSG_SYSTEM,
              "\nBulk too small, fixed by consistency check.\n\n",
              this_object());
        set("bulk",1000);
        log_file("consistency_check",
              "Fixed negative bulk of "+query("cap_name") + "\n");
    } else if (query("bulk") > 10000) {
        message(MSG_SYSTEM,
              "\nBulk too large, fixed by consistency check.\n\n",
              this_object());
        set("bulk",1000);
        log_file("consistency_check",
              "Fixed too large bulk of "+query("cap_name") + "\n");
    }
    if (query("mass") < 0) {
        message(MSG_SYSTEM,
              "\nMass too small, fixed by consistency check.\n\n",
              this_object());
        set("mass",7500);
        log_file("consistency_check",
              "Fixed negative mass of "+query("cap_name") + "\n");
    } else if (query("mass") > 75000) {
        message(MSG_SYSTEM,
              "\nMass too large, fixed by consistency check.\n\n",
              this_object());
        set("mass",7500);
        log_file("consistency_check",
              "Fixed too large mass of "+query("cap_name") + "\n");
    }

    /*
     * we don't want folks to be snoopable when they first log in.
     */
    set("snoopable", 0, MASTER_ONLY);

// Leto added a check for 'leader'
// I'd really like to see this done at logoff time.....
if(query("leader")) delete("leader");
    /*
     * Added demotion from domains that no longer exist.  Blue, 950330.
     */
    if (query_link() && query_link()->query("last_on") < 796700000) {
/* Commented out by Leto after Lucas reported it didn't work.
 * I'll debug it when i have the time
        doms = query_link()->query("domains");
        if (!doms)
            doms = ([ ]);
        doms2 = ([ ]);
        domlist = DOMAIN_LIST + ({ "primary" });
        for (i = 0, j = sizeof(domlist); i<j; i++) {
            if (doms[domlist[i]])
            doms2 += ([ domlist[i] : doms[domlist[i]] ]);
        }
        query_link()->set("domains", doms2);
   */ }

}

/*
 * This function is called when the player enters the game. It handles
 * news displays, player positioning, and other initial user setups.
 */
void setup() {
    string *news;
    int i, s;

    /*
     * Check to see if the user body has a "name"
     */
    if (!query("name"))  return;
    seteuid(getuid());

    /*
     * Initiate user shell setup protocal
     */
    init_setup();

    /*
     * Display last logon and logon site
     */
    if (link_data("last_on"))
        message(MSG_INFO,
              sprintf("\nLast logon:  %s from %s.\n\n",
                    ctime(link_data("last_on")), link_data("ip")),
              this_object());

    /*
     * Set termtype, info is in connection.c
     */
    if(query_link())
    set("termtype",query_link()->query("termtype"));

    debug("Setup: Running the consistency check.\n");

    consistency_check();  // A catch-all to upgrade old users

    debug("Setup: Setting special flags and reading news.\n");

    if (query("inactive"))   delete("inactive");

    CHANNELS_D -> initialize_user();

    if ( !query("cwd") ) set("cwd", "/doc");

    /*
     * Get the news from the news daemon and put it out line by line
     * to avoid overloading one write output.
     */
    news = explode( (string)MSG_D->display_news(), "\n");

    for (i = 0, s = sizeof(news); i < s; i++)
        message(MSG_INFO, news[i] + "\n", this_object());

    if (query("hushlogin")) { complete_setup(); return; }
#ifdef NO_LOGIN_PAUSE
    complete_setup();
    return;
#endif


    if (query("busy"))
        message(MSG_INFO, "\nYour busy flag is still on!\n", this_object());
    if (query("hide"))
        message(MSG_INFO, "You are still hidden. Not announced!\n",
              this_object());
    message(MSG_INFO,
          "Terminal type is "+query("termtype")+".\n\n"
          "[Press ENTER to continue]  ",
          this_object());

    input_to("complete_setup",2);

    return;
}

/*
 * Complete remainder of character setup after NEWS
 * has been read by the entering player
 */
protected varargs void complete_setup (string str) {
    object ghost;
    string temp;
    mixed student_time;

    /*
     * This is here to permit an admin to shut down the game from the
     * "Press ENTER to continue" prompt. Sometimes this is helpful if there
     * is an object that is interfering with commands or otherwise wedging
     * the game, but will go away if the game is reset. It's not a security
     * problem because only admins can use it, and they could just log in
     * and use the shutdown command anyway...;)
     */
#ifdef SAFETY_SHUTDOWN
    if (adminp(getuid(this_object()))) {
        if (str=="shutdown") {
            CMD_SHUTDOWN->cmd_shutdown("0 because safety shutdown invoked.");
        }
    }
#endif

    message(MSG_INFO, "\n", this_object());

    if(link){
    link->set("last_on", time());
    link->set("ip", query_ip_name(this_object()));
    }

    set("reply", 0);
    set("wreply", 0);

    debug("Complete_setup: Moving to the start location.\n");

    temp = getenv("START");
    if (!(temp && stringp(temp) && move(temp) == MOVE_OK)) {
        temp = query("start_location");
#ifdef REAPPEAR_AT_QUIT
        if (!(temp && stringp(temp) && move(temp) == MOVE_OK))
            move(START);
#else
            move (START);
#endif
    }


    call_out("save_me", 1);

    ANNOUNCE->announce_user(this_object(), 0);

    /*
     * This is commented out because of a problem with socket handling
     * by some versions of UNIX. Basically, if you make the call to USERID_D,
     * then the driver will call back to the user's host machine and ask for
     * the user's account name. Under some OS's (Ultrix and HP_UX to name two,
     * but there may be more) if the query returns "Host is unreachable", eg if
     * there is a firewall machine between the driver and the user's machine,
     * then the driver will break the user's connection, and anyone from that machine
     * will be unable to play the MUD.
     * You can undefine this at your own risk, but you'll be cutting off anyone
     * from a protected site, which means most .com addresses and a fair
     * smattering of other hosts, if your OS behaves this way. TMI-2's host
     * runs Ultrix, so we leave it commented out. A good 90% of hosts don't
     * support the user name query protocol anyway, so we're not losing that
     * much. It's your decision if you want to get the names of the other 10%
     * of users, or leave it commented out...
     */
//  USERID_D->query_userid();

    debug("Complete_setup: Checking to see if user is dead.\n");

    if (link_data("dead")) {
        ghost = create_ghost();
        move( VOID );
        set_temp("force_to_look", 1);
        ghost->force_me("look");
        set_temp("force_to_look", 0);
        message(MSG_INFO, "\nYou suddenly realize that you are still a ghost.\n",
              ghost);
        message(MSG_MOVEMENT, sprintf("%s enters the game.\n", query("cap_name")),
              environment(ghost), ({ ghost }));
        remove();
        return;
    }

    if (visible(this_object()))
        message(MSG_MOVEMENT, query("cap_name") + " enters the game.\n",
              environment(), this_player());

#ifdef LOGIN_LOG
    log_file(LOGIN_LOG, CAP_NAME + ": logged in from " +
          query_ip_name(this_object()) + " [" +
          extract(ctime(time()), 4, 15) + "]\n");
#endif

    set_temp("force_to_look", 1);
    command("look");
    set_temp("force_to_look", 0);

    student_time = STUDENT_D->query_time_left(query("name"));

    if (student_time != -1) {
        if (student_time<0)
            message(MSG_SYSTEM,
                  sprintf("\n%s\n\n",
                        blink(" [WARNING:  Your time period as a student has"
                              " ended.  You should copy any files\n"
                              "  you want to keep using ftpd.  "
                              "Type \"help ftpd\" if you need to.]")),
                  this_object());
        else
            message(MSG_SYSTEM,
                  "\n [You have " + bold(format_time(student_time, 1)) +
                  " left as a student]\n\n",
                  this_object());
    }
    debug("Complete_setup: Setup complete.\n");
}

/*
 * This function is called cyclically to save the user data
 * periodically, if AUTOSAVE is defined.
 */
protected void autosave_user() {

    remove_call_out("autosave_user");
    call_out("autosave_user", AUTOSAVE);

    if (!wizardp(this_object()))
        message(MSG_INFO, "Autosave.\n", this_object());

    save_me();
}

void heart_beat() {
    int age;

    continue_attack();
    unblock_attack();
    heal_up();

#ifdef IDLE_DUMP
    if (this_object() && interactive(this_object()) &&
          !wizardp(this_object()) &&
          query_idle(this_object()) > IDLE_DUMP)
        call_user_dump("idle");
#endif

    /*
     * Add to the user's online age total.
     */
    age = query("age");

    if (!query_temp("last_age_set"))
        set_temp("last_age_set", time());

    age += (time() - query_temp("last_age_set"));
    set_temp("last_age_set", time());
    ob_data["age"] = age;
}


/*
 * This function returns whether the user is linkdead or not.
 */
nomask int query_linkdead() {  return !interactive(this_object());  }

protected
void net_dead() {

    save_data();
    message(MSG_MOVEMENT,(string)query("cap_name") + " has gone net-dead.\n",
          environment(), this_object());
//  check_team();
    PARTY_D->check_party(this_object());
    ANNOUNCE->announce_user(this_object(), 3);
//  CMWHO_D->remove_user(this_object());
    set_heart_beat(0);

#ifdef LINKDEAD_DUMP
    call_out("call_user_dump", LINKDEAD_DUMP, "linkdead");
#endif

#ifdef NETDEAD_LOG
    log_file(NETDEAD_LOG, LINK_CAP_NAME + ": net-dead from " +
          query_ip_name(this_object()) + " [" +
          extract(ctime(time()), 4, 15) + "]\n");
#endif

    link->remove();
}

void restart_heart() {
    message(MSG_MOVEMENT, query("cap_name")+" has reconnected.\n",
          environment(), this_object());
    message(MSG_MOVEMENT, "Reconnected.\n", this_object());
    ANNOUNCE->announce_user(this_object(), 2);
    USERID_D->query_userid();
    set_heart_beat(1);
    set("inactive", 0);
    remove_call_out("call_user_dump");

#ifdef RECONNECT_LOG
    log_file(RECONNECT_LOG, CAP_NAME + ": reconnected from " +
          query_ip_name(this_object()) + " [ " +
          extract(ctime(time()), 4, 15) + "]\n");
#endif
}

void call_user_dump(string type) {
    if (this_player() && this_player() != this_object())  return;
    message(MSG_SYSTEM, "\nSorry, you have idled too long.\n", this_object());

    if (environment()) {
        if (type == "linkdead")
            message(MSG_MOVEMENT,
                  sprintf(
                        "A janitor suddendly appears and sweeps "
                        "%s into a nearby vortex.\n", query("cap_name")),
                  environment(), this_object());
        else
            message(MSG_MOVEMENT,
                  sprintf("%s has idled too long\n", query("cap_name")),
                  environment(), this_object());
    }

    quit();
}

protected void die() {
    object killer, ghost, corpse, coins, *stuff;
    mapping wealth;
    string *names, name;
    int i, res, totcoins;

    /*
     * Set the user's killer variable
     */
    killer = query("last_attacker");
    if (!killer)  killer = previous_object();

    /*
     * If the wizard has themself set to "immortal", then
     * they cannot die ... stop death call.
     */
    if (wizardp(this_object()) && query("immortal")) {
        message(MSG_COMBAT, "Your immortality protects you from certain death.\n",
              this_object());
        return;
    }

    /*
     * If the user is already dead ... stop death call.
     */
    if (link_data("dead"))  return;

    /*
     * Bail out of any parties they may be involved in.
     */
//  check_team();
    PARTY_D->check_party(this_object());

    /*
     * Announce the user's death
     */
    message(MSG_INFO, "You have died.\n", this_object());
    message(MSG_INFO, query("cap_name") + " has died.\n", environment(),
          this_player());

    init_attack();

    /*
     * Setup corpse with user's specifics
     */
    corpse = clone_object("/obj/corpse");

    corpse->set_name(query("cap_name"));

    i = query("mass");
    if (i>0) corpse->set("mass", i);

    i = query("bulk");
    if (i>0) corpse ->set("bulk", i);

    i = query("capacity");
    if (i>0) corpse ->set ("capacity", i);

    i = query("volume");
    if (i>0) corpse ->set("volume", i);

    corpse->move(environment(this_object()));

    stuff = all_inventory(this_object());

    for (i = sizeof(stuff); i--; )
        if (!stuff[i]->query("prevent_drop") && !stuff[i]->query_auto_load())
            stuff[i]->move(corpse);

    wealth = query("wealth");
    if (wealth) {
        names=keys(wealth);
        for (i = sizeof(wealth); i--; ) {
            coins = clone_object(COINS);
            coins->set_type(names[i]);
            coins->set_number(wealth[names[i]]);
            totcoins = totcoins + wealth[names[i]];
            if (coins)
                coins->move(corpse);
        }
    }
    set ("wealth", ([ ]));

    res = query("capacity");
    set ("capacity", res + totcoins);

    if (killer)  name = (string)killer->query("cap_name");

#ifdef KILLS
    if (killer)
        log_file(KILLS, CAP_NAME + " was killed by " +
              (name ? name + " " : "") + "(" + file_name(killer) +
              ") [" + extract(ctime(time()), 4, 15) + "]\n");
    else
        log_file(KILLS, CAP_NAME + " was killed by something [" +
              extract(ctime(time()), 4, 15) + "]\n");
#endif

    /*
     * Switch user to ghost body
     */
    ghost = create_ghost();

    if (!ghost)  return;

    message(MSG_INFO,
          "\nYou have a strange feeling.\n"
          "You can see your own lifeless body from above.\n\n",
          ghost);

    if (killer) {
        ghost->set("killer_ob", killer);
        ghost->set("killer_name", (string)killer->query("name"));
    }

    /*
     * Use a call_out to make sure all the above calls have
     * completed their required processing (so we don't lose
     * this_object() before everything is done).
     */
    call_out("remove", 0);
}

string query_short() {
    if (query("name") == "noname")  return "Noname";

    if (!interactive(this_object()))
        return (query("title") + " [disconnected]");

    if (query("inactive"))
        return query("cap_name") + " is presently inactive";
    else if (this_player() && attackers && sizeof(attackers) &&
          environment(this_player()) == environment(this_object()))
        return query("cap_name") + " is attacking " +
              capitalize((string)attackers[0]->query("name")) + ".";
    else if (query_idle(this_object())>300) return query("title")+" (idle)";
        return query("title");
}

string query_title() {
    string str;

    if (!(str = getenv( "TITLE" )))
        str = query("cap_name");
    else if ( !sscanf(str, "%*s$N%*s") )
        str = query("cap_name") + " " + str;
    else
        str = replace_string( str, "$N", ""+ query("cap_name") );
    return str;
}

/*
 * not in use anymore?
 */
nomask void catch_msg(object source, string *msg) {
    int i, s;

    for (i = 0, s = sizeof(msg); i < s; i++)
        receive(msg[i]);
}

void hide(int i) {
    set_hide(i);
}

/*
 * support for debugging an error that a user encounters during this login
 */
private nosave mapping last_error;

void set_error(mapping m) {
    if (previous_object() != master())
        return;
    last_error = m;
}

mapping query_error() {
    if (file_name(previous_object()) == CMD_DBXFRAME ||
          file_name(previous_object()) == CMD_DBXWHERE)
        return last_error;
    else
        message(MSG_SYSTEM, "No permission to query error!", this_player());
}

/*
 * this driver apply is called when the user's environment is being
 * destructed; move the player to a safe place, or end up in limbo
 */
int move_or_destruct(object ob) {
    object old_env = environment();


    if (origin() != ORIGIN_DRIVER)  return 0; // I guess 0 Leto
    if (!ob && old_env != find_object(VOID))
        ob = find_object(VOID);
    if (!ob && old_env != find_object(HALL))
        ob = find_object_or_load(HALL);
    if (!ob) {
        /*
         * This is bad.  Try to save them anyway.
         */
        ob = clone_object("/std/room");
        if (!ob) {
            /*
             * we did our best ...
             */
            return 1;
        }
        ob->set_short("A Temporary room");
        ob->set_long("Something really nasty happened.\n");
        ob->set("light",1); // Nice to actually see the short and long;)
    }
    move(ob);
    if (environment() == old_env) {
        /*
         * we HAVE to move or we get dested.  Override weight checks etc
         */
        move_object(ob); // Leto (no longer need to pass this_object as arg)
    }
    return 0;
}

/* EOF */
