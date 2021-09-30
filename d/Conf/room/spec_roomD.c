// spec_roomD.c
// You can stand in this room and listen to the proceedings in the
// conference room, which echoes things here.
// Written by Mobydick@TMI-2, 2-1-93.
// Wasn't real hard.

#include <mudlib.h>

inherit ROOM ;

void create() {
	::create() ;
	seteuid(getuid()) ;
	set ("light", 1) ;
	set ("short", "Spectator Room D") ;
	set ("long",
"This is the observation room for meeting room D. There is a large window\n"+
"which looks out into the meeting room. Anything said or emoted there\n"+
"which be echoed here (preceeded by \"Conf: \") unless the moderator turns\n"+
"the echoing off. You may \"look at window\" to see who is in the meeting\n"+
"room, and you may \"read sign\" to learn the status of the meeting room.\n"+
"") ;
	set ("exits", ([ "out" : "/d/Conf/room/gallery" ]) ) ;
	set ("item_func", ([
                "window" : "look_at_room"
        ]) ) ;
}

void init() {
	add_action("read", "read") ;
}

int read (string str) {

	object ob ;

	if (!str || str!="sign") {
		return 0 ;
	}
	ob = find_object_or_load("/d/Conf/room/room_D") ;
	ob->read_sign("sign") ;
	return 1 ;
}

int look_at_room() {

	object env, ob ;

	ob = find_object_or_load ("/cmds/std/_look.c") ;
	env = find_object_or_load("/d/Conf/room/room_D.c") ;
        write ((string) ob->look_in_room(env)) ;
	return 1 ;
}

int query_vision() {
	return 1 ;
}

// clean_up() added to check for viewed room before cleaning
// up.  Causes errors if cleaned during a conference.
// Blue 950423.

int clean_up() {
    object watchroom;
    if (!(watchroom=find_object("/d/Conf/room/room_D.c"))) {
        return ::clean_up();
    }
    if (!(watchroom->query_echoing())) {
        return ::clean_up();
    }
    return 1;
}

