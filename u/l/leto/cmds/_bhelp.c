/*
 * _help.c
 * description: user help command
 * authors: wayfarer, mr.x
 * BUGS: change special case below to use mappings
 * Pallando made a few updates 930109
 * Watcher updated the help search pattern 930415
 */

// This thingy is from Blue@Tmi-2 although he didn't say so
// Major rewrite by him, see doc_d. Good job Blue ;)
// Added filtering of LPC help for mortals, probably not
// thought of, because Blue did this on Tmi-2 :)
// Todo: filter using levels. Eg. Help on a simul_efun
// only for ones who have [r] in admsimul_efun. Blue ? :)
// Leto: Aug 18th, 1995
// Leto: Sep 16th, 1995: fixed mortal-wizard behaviour
// Leto: Oct 7th,  1995: Changes to use with 'string help(){}' calls.
//                       Also changed all write() calls.
// Fixed numerous bugs which were not allowing wizards or players
// to see helps for commands which duplicated the names of other
// help documents.  Also improved the help banner to make it
// consistent with the format of other player commands.
// Emeradii@EarthMud 3Jan95

#include <uid.h>
#include <help.h>
#include <config.h>
#include <daemons.h>
#include <mudlib.h>

#define STD_HELP "/help/help_screen"
//#define PROPH_DIR       "/doc/properties/"

inherit DAEMON;


void which_doc(string instr, string *list);


int 
filter_mortal(string item)
{
  return ( item[0..4] == "/help" ) ;
}


string
get_banner(string group, string topic)
{
  string temp;
  string spaces = "                                            ";
  int num;
  
  temp = "Help for "+topic+" ("+group+")";
  
  num = 71-strlen(temp);
  
  temp = "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n" +
         "=-="+spaces[0..num/2]+temp+spaces[0..(num+1)/2]+"=-=\n" +
	 "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n";

  return temp;
}


void
write_banner(string group, string topic)
{
  string temp;
  string spaces = "                                            ";
  int num;
  
  temp = "Help for "+topic+" ("+group+")";
  
  num = 71-strlen(temp);
  
  message ("help","=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n",this_player());
  message ("help","=-="+spaces[0..num/2]+temp+spaces[0..(num+1)/2]+"=-=\n",this_player());
  message ("help","=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n",this_player());
}


string
check_cmd( string path )
{
  string group = "";
  
  switch( path )
  {
    case USER_CMDS     : group = "Player Command";
                         break;

    case WIZ_CMDS      : group = "Wizard Command";
                         break;

    case WIZF_CMDS     : group = "File Command";
                         break;

    case ADM_CMDS      : group = "Admin Command";
                         break;
  }

  return group;
}


int
call_help(string path, string topic)
{
  string group, file, temp;
  object ob;

  file = path+"/_"+topic;

  if ((ob = find_object(file)) || (file_size(file+".c") >= 0 ) || (file_size(path + "/" + topic) >=0))
  {
    switch(path)
    {
      case HELP_DIR    : group = "General Help";
	                 temp = get_banner(group, topic);
	                 this_player()->more(explode( (temp+read_file(path+"/"+topic)+"\n"), "\n" ), 0, 1);
                         return 1;

      case WIZH_DIR    : group = "Developer Help";
	                 temp = get_banner(group, topic);
	                 this_player()->more(explode( (temp+read_file(path+"/"+topic)+"\n"), "\n" ), 0, 1);
                         return 1;

      case PROPH_DIR   : group = "Properties Help";
	                 temp = get_banner(group, topic);
	                 this_player()->more(explode( (temp+read_file(path+"/"+topic)+"\n"), "\n" ), 0, 1);
                         return 1;

      case CONCEPT_DIR : group = "Concepts Help";
	                 temp = get_banner(group, topic);
	                 this_player()->more(explode( (temp+read_file(path+"/"+topic)+"\n"), "\n" ), 0, 1);
                         return 1;

      case LPCH_DIR    : group = "LPC Help";
	                 temp = get_banner(group, topic);
	                 this_player()->more(explode( (temp+read_file(path+"/"+topic)+"\n"), "\n" ), 0, 1);
                         return 1;

      case DAEMONS_DIR : group = "Daemons Help";
	                 temp = get_banner(group, topic);
	                 this_player()->more(explode( (temp+read_file(path+"/"+topic)+"\n"), "\n" ), 0, 1);
                         return 1;

      default            : group = check_cmd(path);
                           break;
    }
 
    if ( (group == "") && (wizardp(this_player()))) temp = get_banner( path, topic );
    else if (group != "") temp = get_banner( group, topic );
    else temp = "";

    if (!ob)
    {
      call_other( file, "???" );
      ob = find_object( file );
    }
 
    if (!ob) 
    {
      message("error", temp + "A command with that name exists, but it has an error in it.\n", this_player());
      return 1;
    }
 
    if (function_exists("help", ob))
    {
//      this_player()->more(explode( (temp + call_other(file, "help")), "\n" ),0,1);
   this_player()->cat(temp); //call_other(file,"help");
      call_other(file,"help");
      return 1;
    } else {
      message("error", temp + "A command with that name exists, but it has no help.\n", this_player() );
      return 1;
    }
  }
 
  return 0;
}


int
cmd_bhelp(string topic)
{
  string *search_path;
  string *list, *newlist;
  string arg, name;
  int i, ii, it;
  object ob, act_ob;
  string bin, bintopic ;
  
  act_ob = this_player();

  // We need to print out the help for THIS COMMAND!!!
  if (topic == "help")
  { 
    message("help", this_object()->help(), this_player());
    return 1;
  }

  if (!topic || topic == "")
  {
    message("help",read_file(STD_HELP),this_player());
    return 1;
  }
 
  topic = lower_case( topic );
    
  arg = replace_string( topic, "-d ", "");
  if ( (arg != topic) && wizardp(this_player()) )
  {
    topic = arg;
 
    switch(topic)
    {
      case "help"        : message( "help", "[Help document files]\n\n", this_player() );
      	                   this_player()->force_me("ls /help");
			   return 1;

      case "properties"  : message( "help", "[Property document files]\n\n", this_player() );
                           this_player()->force_me("ls " + PROPH_DIR);
                           return 1;

      case "efun"        :
      case "efuns"       : message( "help", "[Efun document files]\n\n", this_player() );
                           this_player()->force_me("ls " + EFUN_DIR);
                           return 1;

      case "lfun"        :
      case "lfuns"       : message( "help", "[Lfun document files]\n\n", this_player() );
                           this_player()->force_me("ls " + LFUN_DIR);
                           return 1; 

      case "wizhelp"     : message( "help", "[Wizhelp document files]\n\n", this_player() );
                           this_player()->force_me("ls " + WIZH_DIR);
                           return 1;

      case "simul_efuns" :
      case "simul_efun"  : message( "help", "[Simul_efun document files]\n\n", this_player() );
                           this_player()->force_me("ls " + SE_DIR);
                           return 1;

      case "applies"     : message( "help", "[Applies document files]\n\n", this_player() );
                           this_player()->force_me("ls " + APPLY_DIR);
                           return 1; 

      case "lpc"         : message( "help", "[LPC document files]\n\n", this_player() );
                           this_player()->force_me("ls " + LPCH_DIR);
                           return 1;

      case "concepts"    : message( "help", "[Concepts document files]\n\n", this_player() );
                           this_player()->force_me("ls " + CONCEPT_DIR);
                           return 1;
    }

    return notify_fail("Directory search for "+topic+" found nothing.\n");
  }
   
  if (sscanf(topic,"%s %s",bin,bintopic)==2)
  {
    switch (bin)
    {
      case "help"        : return call_help(HELP_DIR, bintopic) ;
      
      case "wizhelp"     : return call_help(WIZH_DIR, bintopic) ;

      case "lfun"        :
      case "lfuns"       : return (int)MAN_D -> man( bin, bintopic) ;

      case "efun"        :
      case "efuns"       : return (int)MAN_D -> man( bin, bintopic) ;

      case "simul_efuns" : return (int)MAN_D -> man( bin, bintopic) ;

      case "applies"     : return (int)MAN_D -> man( bin, bintopic) ;

      case "commands"    : search_path = explode((string)act_ob->query("PATH"), ":");
                           if (search_path)
			   {
                             for (i = 0; i < sizeof(search_path); i++)
                               if (call_help(search_path[i],bintopic)) return 1;
                           }
                           message("help","There is no help available on that topic.\n", this_player() );
                           return 1 ;

      default            : message ("error","Couldn't find help section: "+bin+".\n", this_player () ) ;
                           return 1 ;
     }
  }

  list = DOC_D->get_docs_on(topic);

  if (list)
  {

    // leto: filter the list and remove all wiz
    //  only topics from the list
    // Maybe this should be done by the DOC_D and a wizard
    // flag should be passed along the DOC_D call

    if (!wizardp(this_player()))
      list = filter_array(list,"filter_mortal",this_object() );

    // Emeradii: Let's see if there are commands matching the topic which have help() in them.
    search_path = explode((string)act_ob->query("PATH"), ":");
    if (search_path)
    {
      int flag;

      for (i = 0; i < sizeof(search_path); i++)
      {
      	flag = 0;
      	
        arg = search_path[i]+"/_"+topic;

	if ( ob = find_object(arg) ) flag = 1;
	else if ( file_size(arg + ".c") >= 0 )
	{
	  flag = 1;
	  arg += ".c";
	} else if ( file_size(search_path[i] + "/" + topic) >=0 )
	{
	  flag = 1;
	  arg = search_path[i] + "/" + topic;
	}
	
        if (!ob && flag)
        {
          call_other( arg, "???" );
          ob = find_object( arg );
        }

	if (ob && flag && function_exists("help", ob))
        {
            list += ({ "<help>\n" }) + ({ arg });
        }
      }
    }

    // Leto: check to see if we didn't weed out all items for mortals
    if (!sizeof(list))
    {
      notify_fail("There is no help available on that topic.\n");
      return 0;
    }

    // Leto: show one item right away.
    if (sizeof(list)==2)
    {
      which_doc("1",list);
      return 1;
    }

    for (ii = 0; ii < sizeof(list); ii+=2)
    {
      printf("[%d] %s: %s", ii/2+1, list[ii+1], list[ii]);
    }

    printf("Which doc [1-%d,q]? ", sizeof(list)/2);

    input_to("which_doc", 0, list);
    return 1;
  }
 
  if (call_help(HELP_DIR, topic)) return 1;
 
  if (wizardp(act_ob))
  {
    if (call_help(WIZH_DIR, topic)) return 1;
    if (call_help(PROPH_DIR, topic)) return 1;
    if (call_help(LPCH_DIR, topic)) return 1;
    if (call_help(CONCEPT_DIR, topic)) return 1;
    if (call_help(DAEMONS_DIR, topic)) return 1 ;
  }
  
  search_path = explode((string)act_ob->query("PATH"), ":");

  if (search_path)
  {
    for (i = 0; i<sizeof(search_path); i++) 
      if (call_help(search_path[i],topic)) return 1; 
  }

  if (wizardp(act_ob))
  {
    if ( sscanf( topic, "%s %s", arg, name ) < 2 ) arg = topic;
    if ((int)MAN_D -> man( arg, name ) == 1) return 1 ;
  }
  
  if ( (ob = present(topic,act_ob)) || (environment(act_ob) &&
       (ob = present(topic,environment(act_ob)))) )
  {
    if (function_exists("help",ob)) 
    {
    	  message ("help","---------------------------------------------------\n",this_player() );
          message ("help","Help for "+topic+" (Object Help)\n",this_player() );
          message ("help","---------------------------------------------------\n",this_player() );
          ob->help();
          return 1;
    }
  }

  message("help","There is no help available on that topic.\n", this_player() );
  return 1;
}


void
which_doc(string instr, string *list)
{
  string temp;
  int wd;

  if (!instr || !stringp(instr))
  {
    message("prompt", "Which doc [1-"+sizeof(list)/2+",q]? ", this_player() );
    input_to("which_doc", 0, list);
    return;
  }

  if ((instr[0] | 32) == 'q')
  {
    message("help"," Thank you for using Blue-Help(tm)!  Please come again.\n", this_player() );
    return;
  }

  if (((wd = to_int(instr))) < 1 || (wd > sizeof(list)/2))
  {
     message("prompt", "A number in the range 1 to "+sizeof(list)/2 +
             ", please.\nWhich doc [1-"+sizeof(list)/2 +",q]? " +
             (this_player()->query("protocol")=="amcp/1.1") ? "\n" : "" ,this_player() );
     input_to("which_doc", 0, list);
     return;
  }
 
  if (list[wd*2-2] == "<help>\n")
  {
    temp = check_cmd( "/"+implode((explode(list[wd*2-1],"/"))[0..<2],"/") );
    if (temp == "") this_player()->more(explode( call_other(list[wd*2-1],"help"), "\n" ), 0, 1); 
      else { this_player()->cat(get_banner(temp,((explode(list[wd*2-1],"/"))[<1])[1..<1]));
      call_other(list[wd*2-1],"help");
   }
//      explode( get_banner(temp,((explode(list[wd*2-1],"/"))[<1])[1..<1])+call_other(list[wd*2-1],"help"),"\n")
//      ,0,1);
  } else this_player()->more(list[wd*2-1],!(wizardp(this_player())),1);
}


string
help()
{
  return @HelpText
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
=-=                     Help for help (Player Command)                      =-=
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Usage:  help <-d> <area> <topic>

The help command can give you help on a number of things in the mudlib
Typing "help" will get you a general overview. "help start" will show
you some information for new players. "help topics" will give you list
of things you might want to read help on.  Use of the -d flag will
show all topics relating to an area, but can only be used by immortals
or higher.
You can specify a particular area in which to look for help commands. The
HelpText
  + ((wizardp(this_player())) ?
      "possible areas are: help, wizhelp, commands, efuns, lfuns, simul_efuns,\n"+
      "applies, concepts, and lpc.\n\n" :

      "possible areas are: help and commands.\n\n") ;
}
