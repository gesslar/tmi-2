/*
 * Robocoder's Scrollback Shell
 *
 * Copyright 1995 by Anthon Pang, Omni Communications
 *                                PO Box 18121, RPO Kerrisdale
 *                                2225 W 41st Avenue
 *                                Vancouver, BC
 *                                Canada  V6M 4L3
 *
 * Permission is granted to TMI-2 and Earth to distribute this with their
 * respective libs.
 *
 *      Email: apang@mindlink.net
 *
 * Beware young adventurer...you are about to visit a realm not entirely
 * unlike your own.  But many a skilled adventurer has walked this path.
 * It is long and winding, and danger lurks in every shadow.  Be afraid,
 * for fear will guide you.
 *
 * Your adventure...begins now.
 */

#include <mudlib.h>
#include <driver/origin.h>

/*
 * simul_efun overrides
 */
#include "/adm/simul_efun/say.c"
#include "/adm/simul_efun/tell_object.c"

inherit OBJECT;

/*
 * defaults to using LINES setting for size of scrollback buffer...but
 * you can override this with SCROLLSIZE env var
 */
#define DEFAULT_SCROLLBACK_SIZE 20
#define DEFAULT_LINES 20
int SCROLLSIZE;     /* size */

/*
 * rely on mudlib to provide useful message classes...user must
 * have set SCROLLMASK env var to enable the filter;
 *   example:  setenv SCROLLMASK snoop|shout|gwiz|gtell
 */
#define DEFAULT_SCROLL_MASK "prompt|snoop"
string *SCROLLMASK; /* mask */

object OWNER;       /* authorized owner of this shell */
mapping SBUFS;      /* scroll back buffers...a mapping of arrays */

void pager(string s, string *arr, int i);

nomask static void initialize() {
    string tmp;

    OWNER = this_player();
    SBUFS = ([ ]);

    SCROLLSIZE = OWNER->query_env("SCROLLSIZE");
    if (stringp(SCROLLSIZE))  SCROLLSIZE = atoi(SCROLLSIZE);
    if (!SCROLLSIZE) {
        SCROLLSIZE = OWNER->query_env("LINES");
        if (stringp(SCROLLSIZE))  SCROLLSIZE = atoi(SCROLLSIZE);
        if (!SCROLLSIZE) {
            SCROLLSIZE = DEFAULT_SCROLLBACK_SIZE;
        }
    }

    tmp = OWNER->query_env("SCROLLMASK");
    if (!tmp || tmp == "")
        tmp = DEFAULT_SCROLL_MASK;
    tmp = replace_string(tmp, " ", "");     /* no spaces */
    tmp = replace_string(tmp, ",", "|");    /* change commas to bar delims */
    SCROLLMASK = explode(tmp, "|");         /* array of classes to ignore */
}

void create() {
    if (origin() == ORIGIN_CALL_OTHER)
        return;

    set("id", ({ "shell", "sb", "scrollback" }));
    set("short", "A scrollback buffer");
    set("long", "Robocoder's Scrollback Buffer\n");
    set("prevent_drop", 1);

    initialize();
}

mixed query_auto_load() {
    return 1;
}

int clean_up() { return 1; }

/*
 * Personally, I don't mind add_action()'s...in this case, it means
 * I don't have to write separate command files
 */
void init() {
    if (environment() && this_player() == environment() &&
          interactive(environment())) {
        add_action("sb", "sb");
        add_action("help", "help");
    } else {
        remove();
    }
}

nomask int sb(string arg) {
    string *mapkeys;
    mixed *arr;
    int i;

/* This seems silly, since it's called from this_player()
 * and this_player != this_object()   --- Leto
    if (previous_object() != this_object()) {
        return 0;
    }
*/

    if (!arg || arg == "") {
        /* display list of buffers */
        mapkeys = keys(SBUFS);
        if (i = sizeof(mapkeys)) {
            tell_object(OWNER, "Scrollback Buffers:\n");
            while (i--) {
                arr = SBUFS[mapkeys[i]];
                tell_object(OWNER, sprintf("\t%s [%d]\n",
                      mapkeys[i],
                      arr[1] ? arr[0] - 2 : sizeof(arr) - 2));
            }
        } else {
            tell_object(OWNER, "No scrollback buffers available.\n");
        }
    } else if (arg == "-reset") {
        /* reset...using current env vars */
        initialize();
        tell_object(OWNER, "Scrollback Reset Complete.\n");
    } else {
        /* display named buffer */
        if (undefinedp(SBUFS[arg])) {
            write(sprintf("Buffer '%s' is empty.\n", arg));
        } else {
            arr = SBUFS[arg];
	    if (arr[1])
                pager("", arr[2..arr[0]-1], 0);
            else
                pager("", arr[arr[0]..<1] + arr[2..arr[0]-1], 0);
        }
    }

    return 1;
}

/*
 * use our own pager to prevent scrollback dump from being logged =)
 */
void pager(string s, string *arr, int i) {
    int l;

    if (s != "" && s[0] == '?') {
        tell_object(OWNER, @ENDHELP
*** More (Jr.) ***
?       - for help
<       - to go to beginning
>       - to go to end
b       - to go back to previous page
y, CR   - to display next page
n, q, x - to quit
ENDHELP );
    } else {
        l = OWNER->query_env("LINES");
        if (stringp(l))  l = atoi(l);
        if (!l)  l = DEFAULT_LINES;

        if (s == "" || s[0] == 'y') {
            if (sizeof(arr) - i < l)
                l = sizeof(arr) - i;
            while (l--)
                tell_object(OWNER, arr[i++]);
        } else if (s[0] == '<') {
            i = 0;
            if (sizeof(arr) - i < l)
                l = sizeof(arr) - i;
            while (l--)
                tell_object(OWNER, arr[i++]);
        } else if (s[0] == 'b') {
            i -= l;
            if (i < 0)
                i = 0;
            if (sizeof(arr) - i < l)
                l = sizeof(arr) - i;
            while (l--)
                tell_object(OWNER, arr[i++]);
        } else if (s[0] == '>') {
            i = sizeof(arr) - l + 1;
            if (sizeof(arr) - i < l)
                l = sizeof(arr) - i;
            while (--l)
                tell_object(OWNER, arr[i++]);
            tell_object(OWNER, "(More) * End of File *\n");
            input_to("pager", arr, i);
            return;
        } else if (s[0] == 'n' || s[0] == 'q' || s[0] == 'x') {
            return;
        } else {
            tell_object(OWNER, "(More) unrecognized command\n");
            input_to("pager", arr, i);
            return;
        }
    }

    if (i != sizeof(arr)) {
        tell_object(OWNER, "(More): \n");
        input_to("pager", arr, i);
    }
}

int receive_message(mixed Class, string msg) {
    mixed *arr;

    if (intp(Class))
        Class = "" + Class;

    if (member_array(Class, SCROLLMASK) == -1) {
        /*
         * if not in filter list, save to buffer;
         * allocate buffer, if necessary...
         */
        if (undefinedp(SBUFS[Class])) {
            SBUFS[Class] = arr = allocate(2 + SCROLLSIZE);
            arr[0] = 2;
            arr[1] = 1; /* not full */
        } else {
            arr = SBUFS[Class];
        }
        arr[arr[0]] = "> " + msg;

        /*
         * index manipulation
         */
        if ((++arr[0]) == SCROLLSIZE + 2) {
            arr[0] = 2;
            arr[1] = 0; /* full */
        }
    }

    tell_object(OWNER, msg);
}

int help(string s) {
    if (s == "sb" || s == "scrollback") {
        write( @ENDTEXT
Robocoder's Scrollback Shell v1.0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Syntax:  sb [<Class> | -reset]

Description:
    The scrollback shell allows the user to maintain a history of received
    messages.  It isn't a true scrollback buffer, because the history is
    simply piped back to the user via `more'.

    Arguments:
        -reset  - reset shell

        <Class> - name or id of a message Class

    Environment variables:
        SCROLLSIZE - the maximum number of messages to keep in a buffer
                    (this applies to all scrollback buffers)
                    Example: setenv SCROLLSIZE 20

        SCROLLMASK - a mask of buffers to filter out
                    Example: setenv SCROLLMASK prompt|snoop|gwiz|gtell
ENDTEXT );
        return 1;
    }
    return 0;
}
