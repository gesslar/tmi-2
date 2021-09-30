// _term.c
// Creator: Leto@EarthMud (04/29/94)
// Description: The start of an advanced customizable term setting
//              Maybe someday it will use TERMCAP, if someone tells
//              me how it works
//
//  You might need to change some query(foo)'s to link_data(foo)'s 
//  if you use tmi 1.1 or newer
//
// If you wonder why i dont use setenv/getenv for this, it's because
// it messes up your screen after typing 'set'
// I think it's not worth it to fix that, so i use the set/get

#include <mudlib.h>
#include <commands.h>
#include <ansi.h>

/* 
 * Colours that we support, and their ansi values. Will be replaced by
 * Pinkfish keywords asap. right now, it's hardcoded ansi
 */
#define VALID_COLOURS ({ "bold" ,"normal" ,"red" ,"green" ,"yellow" ,"blue" ,"magenta" , "cyan" ,"white" ,"hred"  ,"hgreen", "hyellow" , "hblue" , "hmagenta" , "hcyan" , "hwhite" })
#define VALID_COLOURS_ANSI ({ BOLD ,NOR , RED , GRN  , YEL , BLU , MAG , CYN , WHT , HIR, HIG , HIY , HIB , HIM  , HIC , HIW  })
/* 
 * Should prob. be moved to messages.h or something. It's a 2d array (mortals,
 * wizards) holding a 2d array (class,default_colour)
 */
#define MES_CLASSES ({ ({ ({"tell","hgreen"}) , ({"channels","hblue"}), ({"say","hcyan"}),({"announce","hyellow"}),({"combat","normal"}),({"spell","red"}),({"system","hred"}),({"error","normal"}) }), ({ ({"tell","hgreen"}) , ({"channels","hblue"}), ({"say","hcyan"}),({"announce","hyellow"}),({"combat","normal"}),({"spell","red"}),({"system","hred"}),({"error","normal"}),({"developer","normal"}),({"debug","white"}),({"wizcall","hred"}) }) })

void configure(int count);
int help();
void colour(string col, int count);

inherit DAEMON;


int cmd_term(string str) {

	int x,y;
    string sts;
    object shell;

     if (!str)
       { 
	write(help());
        return 1;
       }
   if(sscanf(str,"%s %d %d",str,x,y)!=3)
    sscanf(str,"%s %dx%d",str,x,y);

  switch(str) {
    case "on" :
  {
   write("ANSI / VT100 enabled.\n");
   (CMD_CHSH->cmd_chsh("/obj/shells/term.c")) ;
    CMD_SHELL->cmd_shell("");
    return 1;
  }
    case "off" :
  {
   write("ANSI disabled.\n");
   shell = present("shell", this_player());
   if (shell) shell->remove();
   this_player()->delete("shell");
   return 1;
  }

   case "config" :
   case "configure" : { configure(-1); return 1 ; }
   case "auto" :
{
   write("Setting window size on auto detect. You might need to relogin for it to take effect.\n");
 this_player()->delete("terminal_override");
  return 1;
}
   case "screen":
{
   if( (x>0) && (y>0) )
   {
  write(sprintf("Setting terminal window size override to:%d,%d\n",x,y));
 this_player()->set("terminal_override",({ x,y }) );
  this_player()->setenv("LINES",(int)y);
 this_player()->setenv("COLUMNS",(int)x);
  return 1;
 }
  return notify_fail("Usage: term screen <columns> <lines>\n");
 }
   case "test" :
 {
  int i;
  write("Testing ANSI colour capability.\n");
  for(i=0;i<sizeof(VALID_COLOURS);i++) write("Testing "+VALID_COLOURS_ANSI[i]+
  VALID_COLOURS[i]+NOR+".\n");
  return;
  }
     default :
 {
  if (present("shell"))  sts = "on" ;
   else sts = "off";
  notify_fail("Use <term on> , <term off> or <term config>\n"+
              "Your current setting is: "+sts+".\n");
  return 0;
}
}
}

int help(){
write( @EndHelp
The term command allows you to customize your settings, sometimes
overriding settings that are determined upon login.

Usage:

term configure   : configures your terminal in repect with colours
term on          : enables the special term colour settings.
term off         : disables the special term colour settings.
term screen x y  : overrides the determined screen size (stays after quit!)
term auto        : disables the screen size override.
term test        : test ANSI colours.
EndHelp
);}

void configure(int count) {
/*
 * We use VALID_COLOURS,MES_CLASSES (A 2d array for: class,default)
 */
	if(count==-1) {
	write(@EndBan
Okay, lets get some nice settings for your terms. All these settings will 
be saved. If you want the [default] colour, you can just press return.
Valid VT100 modes are     : bold, normal
Addition for ANSI we have : red,green,yellow,blue,magenta,cyan,white 
or for high intensity     : hred,hgreen,hyellow,hblue,hmagenta,hcyan,hwhite

EndBan);
	return configure(0);
	}

	if(count<sizeof( MES_CLASSES[wizardp(this_player())] )) {
	write(sprintf("Which colour would you like for %s messages [%s] ?",
	MES_CLASSES[wizardp(this_player())][count][0], MES_CLASSES[wizardp(this_player())][count][1]));
	input_to("colour",0,count);
	return;
	}

	write("Configuration finished.\n");
	return ;
}

void colour(string col, int count) {

	if(col!="" && member_array(col,VALID_COLOURS)==-1) {
	write(sprintf("Invalid colours, valid colours are:%s.\n",
	implode(VALID_COLOURS,",")));
	return configure(count);
	}

	if(!col||col=="") col = MES_CLASSES[wizardp(this_player())][count][1];
	this_player()->set("term/"+MES_CLASSES[wizardp(this_player())][count][0],
	VALID_COLOURS_ANSI[member_array(col,VALID_COLOURS)]    );
	count++;
	return configure(count);
}
