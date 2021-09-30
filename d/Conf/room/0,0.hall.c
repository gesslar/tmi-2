// 0,0.hall.c
// The first room of the infinite hallway in the conference center.
// Mobydick, 2-1-93.
// See /adm/daemons/virtual/hall_server.c for more details.

#include <mudlib.h> 

inherit ROOM ;
inherit DOORS;

void create() {
	::create() ;
	set ("light", 1) ;
	set ("short", "The start of a very long hallway") ;
	set ("long", 
"This is the start of a hallway that seems to stretch almost to infinity.\n"+
"On either side of the hallway are many doors leading to small rooms where\n"+
"people may converse in privacy. Feel free to use one.\n") ;
	set ("exits", ([
	"north" : "/d/Conf/room/1,0.hall.c",
	"south" : "/d/Conf/room/bar.c",
	"east" : "/d/Conf/room/0,1.hall.c",
	"west" : "/d/Conf/room/0,-1.hall.c"
	]) ) ;
	create_door ("east", "west", "A wooden door", "open") ;
	create_door ("west", "east", "A wooden door", "open") ;
}
