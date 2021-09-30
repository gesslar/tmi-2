// Meeting Room C
// Part of the TMI-2 conference center.
// Mobydick@TMI-2, 2-1-93.

inherit "/d/Conf/conf_room" ;

void create() {
	::create() ;
	set ("light", 1) ;
	set ("short", "Meeting Room C") ;
	set ("long",
"This is meeting room C, in which structured discussion may be held.\n"+
"There is a long table with a large number of chairs around which the\n"+
"discussants may sit. At the head of the table is a large chair which is\n"+
"labeled \"Moderator\". A large sign on the wall shows the current status\n"+
"of the meeting room.\n"+
"You can reach the conference center by typing 'out'.\n"
	) ;
	set ("exits", ([
		"out" : "/d/Conf/room/centre"
	]) ) ;
	set_echo_room ("/d/Conf/room/spec_roomC") ;
	logfile = "room.C.log" ;
	votefile = "votes.C.log" ;
	call_other ("/d/Conf/boards/conf_C", "frog") ;
        call_other ("/d/Conf/obj/vendor_C", "frog") ;
}
