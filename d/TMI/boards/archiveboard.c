inherit "/std/board/archive.c";

void create()
{
	::create();
	set_attic_dir ("/d/TMI/data/attic") ;
	set_save_dir("/d/TMI/data") ;
	set_save_file("foo");
		set_location("/d/TMI/rooms/archiveroom");
	set ("short.text","The Tmi-2 mudlib archive board");
}
