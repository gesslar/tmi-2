#include <config.h>
#include <mudlib.h>

inherit ROOM;

void create()
{
  ::create();
  seteuid( 0 );
  set( "light", 1 );
	set ("short" , "A living room");
  set( "long", @EndText
	It appears you have interupted a superbowl party.
This group of drunken fans appear to be a bit upset that you are here.
The T.V. is blasting the superbowl and the smell of beer and popcorn,
is coming from the Kitchen.
EndText
  );
  set( "exits", ([
    "start" : START,
	"kitchen" : "/student/kalypso/rooms/kitchen.c",
  ]) );
}
