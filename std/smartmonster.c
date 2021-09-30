// /u/k/kinslayer/sr.c (smartresponce.c) smart responce inspired by
// Padron McCarthy at NannyMud.
//
// inherit this file to add smart responces to monsters, note this
// file inherits monster.c so it must be configured like a monster..
// 
// Kinslayer@Empris (13MAY94) Version 1.0.0

#pragma strict_types
#pragma save_binary

#include <mudlib.h>

inherit MONSTER;

/*-----------------------------------------------------------------*/
// Debugging stuff, to activate set_log_file() must be set to
// a dir that the UID has write access to, for most cases this should
// be the domain dir for the object, or the user dir.

string log_file_name;

void set_log_file(string str) {
  log_file_name = str;
}

string query_log_file() {
  return log_file_name;
}

void say_confused(string str) {
string junk;
 
    if (this_player() != this_object() && !this_player()->query("npc")) {
        junk = sprintf("%s (%O) tells you: I am confused: %s\n", this_object()->query("cap_name"), file_name(this_object()), str);
        tell_object(this_player(), junk);
    }
 
    else if (previous_object() != this_object() && living(previous_object()) &&!previous_object()->query("npc")) {
        junk = sprintf("%s (%O) tells you: I am confused: %s\n", this_object()->query("cap_name"), file_name(this_object()), str);
        tell_object(previous_object(), junk);
    } else
        say(sprintf("%s seems to be confused: %s\n", this_object()->query("cap_name"), str));
        junk = sprintf("%O : %s (%O) was confused: %s\n", ctime(time()), this_object()->query("cap_name"), file_name(this_object()), str);
        if(log_file_name != 0)
          write_file(log_file_name, junk);
} /* say_confused */
 

void say_error(string str) {
    string tell_message, log_message;
 
    tell_message = sprintf("%s (%O) tells you: ERROR! %s\n", this_object()->query("cap_name"), file_name(this_object()), str);
    log_message = sprintf("%O: %s (%O, creator: %O)   ERROR: %s\n", ctime(time()), this_object()->query("cap_name"), file_name(this_object()),creator_file(file_name(this_object())),str);
 
    if (this_player() != this_object())
        log_message += sprintf("this_player = %s (%O)\n", this_player()->query("cap_name"), file_name(this_player()));
 
    if (previous_object() != this_object())
        log_message += sprintf("previous_object() = %s\n",file_name(previous_object()));
 
    if (this_player() != this_object() && !this_player()->query("npc"))
        tell_object(this_player(), tell_message);
    else if (previous_object() != this_object() && living(previous_object()) && !previous_object()->query("npc"))
        tell_object(previous_object(), tell_message);
    else {
        say(sprintf("%s looks very confused: %s.\n", this_object()->query("cap_name"), str));
        if(log_file_name != 0)
        write_file(log_file_name, log_message);
   }
} /* say_error */
 

void debug_log(string str, int junk) {
string file_format;
  if(log_file_name != 0 && junk == 0) {
    file_format = sprintf(@FILEFORMAT

******************************************************************************
File_name: %O Cap_name: %s
Time: %O Location: %O

Debug message:
%s
FILEFORMAT
, file_name(this_object()), this_object()->query("name"), ctime(time()), environment(this_object()), str);

    write_file(log_file_name, file_format);
  } else if(log_file_name != 0 && junk != 0)
    write_file(log_file_name, str);

}

#define DEBUG_LOG(str, junk)       debug_log(str, junk)
#define SAY_CONFUSED(str)        say_confused(str)
#define SAY_ERROR(str)           say_error(str)

int handle_caught_texts;

void smartwrite(mixed something);


/*---------------------------------------------------------------------------*/

mixed response_object;  // The object in which to call the functions 
mixed *response_data;
 
//  response_data is an array of multiples of four elements, each of which is
//    the action the monster will respond to (a string),
//    the message to print in response to this action (a string),
//    an (optional) special messages to the "opponent" (also a string),
//    the chance for each response, if it matched (an integer 0..100)
//    If the "message to print" starts with '*' or '!',
//    it is used as a function to call or a command to give.
//    It could also be an array of actions to perform.
 
 
varargs void add_response(string act, mixed response, mixed arg3, mixed arg4) {
    string opponent_msg;
    int chance;
 
//      Usage:
//           add_response  ACTION  "REPLY-MESSAGE"  [ "PERSONAL-REPLY-MESSAGE" ]  [ CHANCE ]
//           add_response  ACTION  "*FUNCTION-NAME"  [ CHANCE ]
//           add_response  ACTION  "!COMMAND"  [ CHANCE ]
//           add_response  ACTION  ARRAY-OF-RESPONSES  [ CHANCE ]
     
 
    if (pointerp(response) || response[0] == '*' || response[0] == '!') {
        if (!intp(arg3) || arg4 != 0) {
            SAY_ERROR("Bad arguments (arg 3 or 4) to add_response.");
            return;
        }
        chance = arg3;
    }
    else if (!stringp(response)) {
        SAY_ERROR("Bad argument (response) to add_response.");
        return;
    }   
    else if ((stringp(arg3) || arg3 == 0) && intp(arg4)) {
        opponent_msg = arg3;
        chance = arg4;
    }
    else if (intp(arg3) && arg4 == 0) {
        chance = arg3;
    }
    else {
        SAY_ERROR("Bad arguments to add_response.");
        return;
    }
 
    if (chance == 0)
        chance = 100;
 
    if (response_data == 0)
        response_data = ({ });
    response_data += ({ act, response, opponent_msg, chance });
 
    handle_caught_texts = 1;
} /* add_response */
 
void set_responses(mixed *all_responses) {
    if (all_responses != 0 && !pointerp(all_responses)) {
        SAY_ERROR("Bad argument to set_responses.");
        return;
    }
    response_data = all_responses;
    if (response_data)
        handle_caught_texts = 1;
} /* set_responses */
 
mixed *query_responses() {
    return response_data;
} /* query_responses */
 
void set_response_object(mixed obj) {
    response_object = obj;
}
 
/*---------------------------------------------------------------------------*/
 
mixed *say_response_data;
 
//  say_response_data is an array of multiples of four elements, each of which is
//    the word or word the monster will respond to
//      (a string, or an array of strings or arrays of strings)
//    the message to print in response to this action (a string),
//    an (optional) special messages to the "opponent" (also a string),
//    the chance for each response, if it matched (an integer 0..100)
//    If the "message to print" starts with '*' or '!',
//    it is used as a function to call or a command to give.
//    It could also be an array of actions to perform.
 
 
varargs void add_say_response(mixed word, mixed response, mixed arg3, mixed arg4) {
    string opponent_msg;
    int chance;
 
//      Usage:
//          add_say_response  WORD  "REPLY-MESSAGE"  [ "PERSONAL-REPLY-MESSAGE" ]  [ CHANCE ]
//          add_say_response  WORD  "*FUNCTION-NAME"  [ CHANCE ]
//          add_say_response  WORD  "!COMMAND"  [ CHANCE ]
//          add_say_response  WORD  ARRAY-OF-RESPONSES  [ CHANCE ]
//          WORD can be either a string, or an array of strings or arrays of strings.
 
 
    if (pointerp(response) || response[0] == '*' || response[0] == '!') {
        if (!intp(arg3) || arg4 != 0) {
             SAY_ERROR("Bad arguments (arg 3 or 4) to add_say_response.");
            return;
        }
        chance = arg3;
    }
    else if (!stringp(response)) {
        SAY_ERROR("Bad argument (response) to add_say_response.");
        return;
    }   
    else if ((stringp(arg3) || arg3 == 0) && intp(arg4)) {
        opponent_msg = arg3;
        chance = arg4;
    }
    else if (intp(arg3) && arg4 == 0) {
        chance = arg3;
    }
    else {
        SAY_ERROR("Bad arguments to add_say_response.");
        return;
    }
 
    if (chance == 0)
        chance = 100;
 
    if (say_response_data == 0)
        say_response_data = ({ });
    say_response_data += ({ word, response, opponent_msg, chance });
 
    handle_caught_texts = 1;
} /* add_say_response */
 
void set_say_responses(mixed *all_say_responses) {
    if (all_say_responses != 0 && !pointerp(all_say_responses)) {
        SAY_ERROR("Bad argument to set_say_responses.");
        return;
    }
    say_response_data = all_say_responses;
    if (say_response_data)
        handle_caught_texts = 1;
} /* set_say_responses */
 
mixed *query_say_responses() {
    return say_response_data;
} /* query_say_responses */
/* Substitute "$OTHER" with the opponent's name */
string substitute_other(string str, string opponents_name) {
    string part1, part2;
 
    while (sscanf(str, "%s$OTHER%s", part1, part2) == 2)
        str = part1 + opponents_name + part2;
    while (sscanf(str, "%s$LOWOTHER%s", part1, part2) == 2)
        str = part1 + lower_case(opponents_name) + part2;
    return str;
} /* substitute_other */
 
/*  This function is called when the response is to be performed
 *  - i. e. it matched, and the dice rolled our way!
 */
void perform_response(string opponents_name, string what, string how,
                      mixed response, string opponent_msg)
{
 
    DEBUG_LOG("perform_response('" + opponents_name + "', '" + what + "', '" + how + "', ...)", 1);
 
    if (pointerp(response)) {
        /* This "response" is really an array of responses */
        if (opponent_msg) {
            SAY_ERROR("Bad argument (opponent_msg) to perform_response.");
            return;
        }
        else {
            int i, n;
 
            n = sizeof(response);
            for (i = 0; i < n; ++i)
                perform_response(opponents_name, what, how, response[i], 0);
        }
    }
    else if (!stringp(response)) {
        SAY_ERROR("Bad argument (response) to perform_response.");
        return;
    }
    else if (response[0] == '!') {
        string cmd;
        sscanf(response, "!%s", cmd);
        command(substitute_other(cmd, opponents_name));
    }
    else if (response[0] == '*') {
        string fun;
 
        sscanf(response, "*%s", fun);
        fun = substitute_other(fun, opponents_name);
 
        DEBUG_LOG("Calling " + fun + "(\"" + opponents_name +
                  "\", \"" + what + "\", \"" + how + "\")", 1);
 
        call_other(response_object, fun, opponents_name, what, how);
    }
    else {
        object opponent_obj;
 
        if (opponent_msg && opponents_name)
            opponent_obj = present(opponents_name);
 
        if (opponent_obj) {
            tell_object(opponent_obj, substitute_other(opponent_msg, opponents_name));
            say(substitute_other(response, opponents_name), opponent_obj);
        }
        else
            say(substitute_other(response, opponents_name));
    }
} /* perform_response */
 
/*---------------------------------------------------------------------------*/
 
int match_responses(string str) {
    string who, what, how, msg, opponent_msg, junk;
    int i, n;
    object opponent;
 
#ifdef MONSTER_COMPAT
    if (talk_ob)
        test_match(str);
#endif
 
    if (response_data == 0)
        return 0;
    n = sizeof(response_data);
    /* Backwards. New responses that are added should be tested before old ones. */
    for (i = n - 4; i >= 0; i -= 4) {
        what = response_data[i];
        /* how = ""; */
 
        /*  As it is now, the action "smiles" matches all of "X smiles\n",
         *  "X smiles.\n", "X smiles like a surgeon.\n" and "X smilesiglurps.\n".
         *  Maybe it should match "X smiles\n", "X smiles.\n"
         *  and "X smiles like a surgeon.\n" but not "X smilesiglurps.\n"?
         */
        if (   sscanf(str, "%s " + what + " %s\n", who, how) == 2
            || sscanf(str, "%s " + what, who) == 1) {
 
            if (random(100) < response_data[i + 3]) {
                msg = response_data[i + 1];
                opponent_msg = response_data[i + 2];
 
                /* The "who" from the last sscanf could be too long! */
                /* sscanf(who, "%s %s\n", who, junk); */
 
                if (how == 0)
                    how = "";
 
                perform_response(who, what, how, msg, opponent_msg);
                return 1;
            }
        } /* if this response matched */
    } /* for all stored responses */
    return 0;
} /* match_responses */
 
/*---------------------------------------------------------------------------*/
 
string *split_into_words(string str) {
    int wordstart, pos, afterpos;
    int c, nl;
    string *result;
 
    afterpos = strlen(str);
    result = ({ });
    pos = 0;
    nl = "\n"[0];  /* Grr! '\n' doesn't work! */
    while (pos < afterpos) {
        /* First, skip leading blanks and interpunctation. */
        while (   pos < afterpos
               && (((c = str[pos]) == ' ') || c == nl || c == '.'
                   || c == '?' || c == '!' || c == ',' || c == ':'))
            ++pos;
        wordstart = pos;
        /* If we haven't reached the end of the string,
         * take the word that starts here.
         */
        if (pos < afterpos) {
            while (   pos < afterpos
                   && ((c = str[pos]) != ' ') && c != nl && c != '.'
                   && c != '?' && c != '!' && c != ',' && c != ':')
                ++pos;
            result += ({ str[wordstart..pos-1] });
        }
    }
    return result;
} /* split_into_words */
 
/*---------------------------------------------------------------------------*/
 
int match_say_responses(object opponent, string who, string phrase) {
    mixed datawords;
    string msg, opponent_msg;
    int i, n, word_nr, nr_datawords;
    string *inputwords;
 
    if (say_response_data == 0)
        return 0;
    inputwords = split_into_words(lower_case(phrase));
    n = sizeof(say_response_data);
    /* Backwards. New responses that are added should be tested before old ones. */
    for (i = n - 4; i >= 0; i -= 4) {
        datawords = say_response_data[i];
        if (pointerp(datawords)) {
            /* "datawords" was an array of words. Match against each of them! */
            nr_datawords = sizeof(datawords);
            for (word_nr = 0; word_nr < nr_datawords; ++word_nr) {
                if (member_array(datawords[word_nr], inputwords) != -1) {
                    msg = say_response_data[i + 1];
                    opponent_msg = say_response_data[i + 2];
                    perform_response(who, "(says)", phrase, msg, opponent_msg);         
                    return 1;
                }
            } /* for each word in the array */
        }
        else {
            /* "datawords" was just a single word. Match against it! */
            if (member_array(datawords, inputwords) != -1) {
                msg = say_response_data[i + 1];
                opponent_msg = say_response_data[i + 2];
                perform_response(who, "(says)", phrase, msg, opponent_msg);             
                return 1;
            }
        }
    } /* for all stored say_responses */
    return 0;
} /* match_say_responses */
 
/*---------------------------------------------------------------------------*/
 
mixed say_handler_obj;  /* The object in which to call... */
string say_handler_fun; /* ...this function when the monster hears a "say" */
 
void set_say_handler(mixed obj, string fun) {
    if (obj)
        say_handler_obj = obj;
    else
        say_handler_obj = previous_object();
    if (fun)
        say_handler_fun = fun;
    else
        say_handler_fun = "handle_say";
    handle_caught_texts = 1;
} /* set_say_handler */
 
/*---------------------------------------------------------------------------*/
 
mixed tell_handler_obj;     /* The object in which to call... */
string tell_handler_fun;    /* ...this function when the monster hears a "tell" */
 
void set_tell_handler(mixed obj, string fun) {
    if (obj)
        tell_handler_obj = obj;
    else
        tell_handler_obj = previous_object();
    if (fun)
        tell_handler_fun = fun;
    else
        tell_handler_fun = "handle_tell";
    handle_caught_texts = 1;
} /* set_tell_handler */
 
/*---------------------------------------------------------------------------*/
 
mixed give_handler_obj;  /* The object in which to call... */
string give_handler_fun; /* ...this function when the monster is given an object */
 
void set_give_handler(mixed obj, string fun) {
    if (obj)
        give_handler_obj = obj;
    else
        give_handler_obj = previous_object();
    if (fun)
        give_handler_fun = fun;
    else
        give_handler_fun = "handle_give";
    handle_caught_texts = 1;
} /* set_give_handler */
 
/*---------------------------------------------------------------------------*/
 
mixed give_money_handler_obj;  /* The object in which to call... */
string give_money_handler_fun; /* ...this function when the monster is given some money */
 
void set_give_money_handler(mixed obj, string fun) {
    if (obj)
        give_money_handler_obj = obj;
    else
        give_money_handler_obj = previous_object();
    if (fun)
        give_money_handler_fun = fun;
    else
        give_money_handler_fun = "handle_give_money";
    handle_caught_texts = 1;
} /* set_give_money_handler */
 
/*---------------------------------------------------------------------------*/
 
mixed arrive_handler_obj;       /* The object in which to call... */
string arrive_handler_fun;      /* ...this function when someone arrives */
 
void set_arrive_handler(mixed obj, string fun) {
    if (obj)
        arrive_handler_obj = obj;
    else
        arrive_handler_obj = previous_object();
    if (fun)
        arrive_handler_fun = fun;
    else
        arrive_handler_fun = "handle_arrive";
    handle_caught_texts = 1;
} /* set_arrive_handler */
 
/*---------------------------------------------------------------------------*/
 
mixed leave_handler_obj;        /* The object in which to call... */
string leave_handler_fun;       /* ...this function when someone leaves */
 
void set_leave_handler(mixed obj, string fun) {
    if (obj)
        leave_handler_obj = obj;
    else
        leave_handler_obj = previous_object();
    if (fun)
        leave_handler_fun = fun;
    else
        leave_handler_fun = "handle_leave";
    handle_caught_texts = 1;
} /* set_leave_handler */
 
/*---------------------------------------------------------------------------*/
 
/* Maybe we should put "smartpresent" in 'obj/simul_efun.c' or somewhere? */
 
static object smartpresent2(mixed what, mixed where) {
    object obj, foundobj, *all_inv;
    string lwhat, lwhat2, junk;
    int i, n, the_number;
 
    if (what == 0)
        return 0;
    obj = present(what, where);
    if (obj || !stringp(what))
        return obj;
    lwhat = lower_case(what);
    obj = present(lwhat, where);
    if (obj)
        return obj;
    while (   sscanf(lwhat, "the %s", lwhat) == 1
           || sscanf(lwhat, "a %s", lwhat) == 1
           || sscanf(lwhat, "an %s", lwhat) == 1
           || sscanf(lwhat, "%s.", lwhat) == 1) {
        obj = present(lwhat, where);
        if (obj)
            return obj;
    }
    while (sscanf(lwhat, "%s %d", lwhat2, the_number) == 2) {
        if (where == 0)
            where = environment(this_object());
        all_inv = all_inventory(where);
        if (pointerp(all_inv)) {
            n = sizeof(all_inv);
            foundobj = 0;
            for (i = 0; i < n && the_number > 0; ++i) {
                if (all_inv[i]->id(lwhat2)) {
                    if (--the_number == 0)
                        return obj;
                    else if (!foundobj)
                        foundobj = all_inv[i];
                }
            }
            if (foundobj)
                return foundobj;
        }
        lwhat = lwhat2;
    }
    while (   sscanf(lwhat, "%s, %s", lwhat2, junk) == 2
           || sscanf(lwhat, "%s - %s", lwhat2, junk) == 2
           || sscanf(lwhat, "%s %s", lwhat2, junk) == 2) {
        obj = present(lwhat2, where);
        if (obj)
            return obj;
        lwhat = lwhat2;
    }
    return 0;
} /* smartpresent2 */
 
object smartpresent(mixed what, mixed where) {
    object foundobj;
 
    if (where == 0) {
        foundobj = smartpresent2(what, environment(this_object()));
        if (foundobj)
            return foundobj;
        foundobj = smartpresent2(what, this_object());
        return foundobj;
    }
    else
        return smartpresent2(what, where);
} /* smartpresent */
 
/*-----------------------------------------------------------------*/
int do_matching(string str) {
    string who, phrase, what, whom, how;
    object who_obj, what_obj;
    int the_number;
 
    DEBUG_LOG("do_matching(\"" + str + "\")", 1);
 
    /* Was this something that someone said? */
    if (   (say_handler_obj || say_response_data)
        && (   (sscanf(str, "%s says: %s\n", who, phrase) == 2)
            || (sscanf(str, "%s says \"%s\"\n", who, phrase) == 2))) {
        who_obj = smartpresent(who, environment(this_object()));
        if (who_obj) {
            if (say_response_data && match_say_responses(who_obj, who, str)) {
 
            }
            else if (say_handler_obj)
                call_other(say_handler_obj, say_handler_fun, who_obj, who, phrase);
        }
        else {
            SAY_CONFUSED(who + " said something, but doesn't seem to be here now.");
        }
    } /* if (say_handler_obj) */
 
    /* Was this something that someone told this monster? */
    if (   (tell_handler_obj || say_response_data)
        && (sscanf(str, "%s tells you: %s\n", who, phrase) == 2)) {
        who_obj = smartpresent(who, environment(this_object()));
        if (!who_obj)
            who_obj = find_living(lower_case(who));
        if (who_obj) {
            if (say_response_data && match_say_responses(who_obj, who, str)) {
 
            }
            else if (tell_handler_obj)
                call_other(tell_handler_obj, tell_handler_fun, who_obj, who, phrase);
        }
        else {
            SAY_CONFUSED(who + " told me something, but can't be found.");
        }
    } /* if (tell_handler_obj) */
 
    /* Did someone just give this monster some money? some editing may be needed here!*/
    if (   give_money_handler_obj
        && (sscanf(str, "%s gives you %d gold coins.\n", who, what) == 2)) {
        /* Ok, someone gave this monster some gold coins! Now: who? */
        who_obj = smartpresent(who, environment(this_object()));
        if (!who_obj) {
            SAY_CONFUSED(who + " gave me some gold, but doesn't seem to be here now.");
        }
        else
            call_other(give_money_handler_obj, give_money_handler_fun, who_obj, who, what);
    } /* if (give_money_handler_obj && ...) */
 
    /* Did someone just give this monster something? */
    if (   give_handler_obj
        && (sscanf(str, "%s gives %s to %s.\n", who, what, whom) == 3)
        && (   id(whom)
            || id(lower_case(whom))
            || (   (sscanf(whom, "%s %d", whom, the_number) == 2)
                && (id(whom) || id(lower_case(whom)))))) {
        /* Ok, someone gave this monster something! Now: who and what? */
        who_obj = smartpresent(who, environment(this_object()));
        if (!who_obj) {
            SAY_CONFUSED(who + " gave me something, but doesn't seem to be here now.");
        }
        else {
            /* Ok, a known someone gave this monster something! Now: what? */
            what_obj = smartpresent(what, this_object());
            if (what_obj)
                call_other(give_handler_obj, give_handler_fun, who_obj, who, what_obj, what);
            else {
                SAY_CONFUSED(who + " gave me '" + what + "', but I don't seem to have it now.");
            }
        }
    } /* if (give_handler_obj && ...) */
 
    /* Was this a message about someone arriving? */
    if (   arrive_handler_obj
        && (sscanf(str, "%s arrives%s.\n", who, how) == 2)) {
        who_obj = smartpresent(who, environment(this_object()));
        if (who_obj)
            call_other(arrive_handler_obj, arrive_handler_fun, who_obj, who, how);
        else {
            SAY_CONFUSED(who + " arrived, but doesn't seem to be here now.");
        }
    } /* if (arrive_handler_obj) */
 
    /* Was this a message about someone leaving? */
    if (   leave_handler_obj
        && (sscanf(str, "%s leaves %s.\n", who, how) == 2)) {
        call_other(leave_handler_obj, leave_handler_fun, 0, who, how);
    } /* if (leave_handler_obj) */
 
    if (response_data && match_responses(str))
        return 1;
    return 0;
} /* do_matching */
/*---------------------------------------------------------------------------*/
 
object fight_beat_obj;  /* The object in which to call... */
string fight_beat_fun;  /* ...this function each heartbeat when fighting */
 
void set_fight_beat(object obj, string fun) {
    if (obj)
        fight_beat_obj = obj;
    else
        fight_beat_obj = previous_object();
    if (fun)
        fight_beat_fun = fun;
    else
        fight_beat_fun = "fight_beat";
} /* set_fight_beat */
 
/*---------------------------------------------------------------------------*/
 
object peace_beat_obj;  /* The object in which to call... */
string peace_beat_fun;  /* ...this function each heartbeat when NOT fighting */
 
void set_peace_beat(object obj, string fun) {
    if (obj)
        peace_beat_obj = obj;
    else
        peace_beat_obj = previous_object();
    if (fun)
        peace_beat_fun = fun;
    else
        peace_beat_fun = "peace_beat";
} /* set_peace_beat */
 
/*-----------------------------------------------------------------*/

// queue_text() - here all texts that are received via receive_message()
// will be queued on the variable, "waiting_texts".

mixed waiting_texts;
int latest_receive_message_time;

void queue_text(string str) {
  if (waiting_texts == 0)
    waiting_texts = str;
  else if (stringp(waiting_texts))
    waiting_texts = ({ waiting_texts, str });
  else  waiting_texts += ({ str });
}

string get_queued_text() {
  string temp;

  if(stringp(waiting_texts)) {
    temp = waiting_texts;
    waiting_texts = 0;
  }
  else if (sizeof(waiting_texts) == 2) {
    temp = waiting_texts[0];
    waiting_texts = waiting_texts[1];
  }
  else { temp = waiting_texts[0];
    waiting_texts = waiting_texts[1..sizeof(waiting_texts)-1];
  }
  return temp;
}

int nr_waiting_texts() {
  if(waiting_texts == 0)
    return 0;
  else if (stringp(waiting_texts))
    return 1;
  else return sizeof(waiting_texts);
}

/*-----------------------------------------------------------------*/

// receive_message() - waiting_texts are queued here then on the next
// heart_beat the next waiting_text is processed.  If the newest addition
// to the queue is more than ten seconds old then the queue is dumped

void receive_message(string Class, string str) {
string junk;

  junk = sprintf("receive_message(Class: %s, string: %s);", Class, str);

  debug_log(junk, 0);
  if(previous_object() != this_object()) {

// Padron made a small error here in smartmonster ver 0.6.2 He did
// not put '()' around time() - latest_receive_message_time > 10
// so this condition was never used in his smartmonster version
// it was testing for && time() which is always true unless the date
// is Jan. 1, 1970 -Kinslayer

    if(waiting_texts && (time() - latest_receive_message_time > 10))
      waiting_texts = 0;
    latest_receive_message_time = time();

    queue_text(str);
  }
}
/*---------------------------------------------------------------------------*/
 
/*  Handle the texts on the queue.
 *  DON'T handle additional texts that may arrive now.
 */
void match_the_queue() {
    int i, n;
 
    n = nr_waiting_texts();
    for (i = 0; i < n; ++i)
        do_matching(get_queued_text());
} /* match_the_queue */
 
/*---------------------------------------------------------------------------*/
 
// this heart_beat() replaces the one in monster.c
 
void heart_beat() {
 
// If we're chasing someone, better go after him.
 
   if (query("in_pursuit")) {
      command("go "+query("in_pursuit")) ;
      if (present((object)query("pursued"),environment(this_object()))) {
         tell_object((object)query("pursued"),this_object()->query("cap_name")+" attacks you!\n") ;
         query("pursued")->kill_ob(this_object(),1) ;
      }
      delete ("in_pursuit") ;
      delete ("pursued") ;
   }
 
   if (query("moving")==1) move_around() ;
   if(random(100)+1 < query("chat_chance"))  monster_chat();
   if(waiting_texts) match_the_queue();
 
   heal_up();
 
   //  If no-one is around and fully healed, shut down the heartbeat
 
   if(!environment_check() && query("hit_points") == query("max_hp")) {
 
   if(query("max_sp") && query("max_sp") != query("spell_points"))
        return;
 
// If we're whanging on someone, best not to shut it down... :)
 
        if (sizeof(attackers)) return ;
 
   set_heart_beat(0);  hb_status = 0;  
   }
 
}
 

