// server.c
// This file is inherited by servers (daemons).
// It insures that they have euid privileges and can't be shadowed.
// It also makes sure that certain daemons don't go away due to inactivity.
//
// This file is part of the TMI Mudlib distribution.
// Please include this header if you use this code.
// Based on "cmd_m.c" by Sulam, Buddha, and Shard.  Hacking by Robocoder.
// Comment added by Blue to test edit logging.

#include <config.h>

// This is necessary to maintain data consistency.
int clean_up() {
    return 0;
}

// This gets used a lot.
void create() {
    seteuid(getuid(this_object()));
}

// This is necessary to maintain security.
int query_prevent_shadow() {
    return 1;
}

/* EOF */
