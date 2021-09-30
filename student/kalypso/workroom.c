/* file "/u/k/kalypso/workroom.c"
 *code stollen from "~leto/workroom.c"
 *original header follows:
 */


/*
 *      leto's workroom.
 *      (first thing he ever coded on a mud, so kind of historical)
 *      (And it euh, got sort of updated a few times, so sort of
 *      not so historical anymore ;)
 */


//
//created stardate 960319.1032


#include <mudlib.h>
#include <config.h>
#include <uid.h>

inherit ROOM ;

#define FRIENDS ({"quozl","kalypso","anakin","kitten","kalypso"})
private int shield;
private string *allowed;

void create() {
        :: create() ;
        seteuid(getuid());

    allowed = FRIENDS ;
    set ("short", "kalypso's workroom");
    set ("long", wrap(
"	You find yourself in a room filled with codebooks, printers and\n" 
"computer parts. There are files scattered all over the place. stuff\n"
"like config.h mudlib.h and even a player.o or two.\n"
"There are Ozzy posters all over the walls. Guns ~n~ Roses are jamming\n"
"on the cd player	\n"
));
 	set ("exits", ([ 
                   "start" : START,
                   "void" : VOID,
                   "punkette" : "/u/p/punkette/workroom",
                   ]) ) ;
    set ("light", 1);
 
shield = 1;
}
void init()
{
if(getuid(previous_object()) == "kalypso")  
    {
      add_action("lock","lock");
      add_action("unlock","unlock");
      add_action("eject","eject");
    }
 if(member_array(getuid(previous_object()),allowed)!=-1) return;

else {
     if(shield)
        {
     if( !(member_array(getuid(this_player()), allowed)!=-1) )

                 {
                  write(wrap("Sorry, kalypso does not want to be disturbed right "
                             "now. He is probably teaching or having a private"
                             " conversation. Either try and ask him or try "
                             "again later...Thanks.\n"));
           tell_object(find_player("kalypso"),(this_player()->query("name")+
        " tried to barge in here.\n"));
           this_player()->move(this_player()->query("last_location"));

                 return;
     }
  }
}
}

int
lock()
{
  if(getuid(this_player()) != "kalypso")
    return 0;
        shield =1;
  write("Room is now locked.\n");
  return 1;
}

int
unlock()
{
  if(getuid(this_player()) != "kalypso")
    return 0;
  shield=0;
  write("Room is now unlocked.\n");
  return 1;
}

int
eject(string arg)
{
  object ob;
  object last;
  string name;

  if(!arg || arg=="")
    return 0;

  if(getuid(this_player()) != "kalypso")
    return 0;

  if(!(ob=present(arg,this_object())))
    {
      notify_fail(arg+" is not here\n");
      return 0;
    }

  if(!(last=(object)ob->query("last_location")))
  last=load_object(VOID);
  name=(string)ob->query("cap_name");

  ob->move_player(last,name + " is ejected from the room","");
  return 1;
}
