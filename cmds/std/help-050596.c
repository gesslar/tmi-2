/*
 * _help.c
 * description: user help command
 * authors: wayfarer, mr.x
 * last modified: 2/14/92
 * BUGS: change special case below to use mappings
 * Pallando made a few updates 930109
 * Watcher updated the help search pattern 930415
 * Pretty much rewritten by Blue from October 94 onwards.
 */

// If this command gives unexpected problems, use ohelp instead
#include <uid.h>
#include <help.h>
#include <config.h>
#include <daemons.h>
#include <mudlib.h>

#define STD_HELP "/doc/help/help_screen"

inherit DAEMON;


void display_doc(string docfile);


void write_banner(string group, string topic) {
    printf("---------------------------------------------------\nHelp for %s (%s)\n----------------------------------------------------\n", topic, group);
//  write ("---------------------------------------------------\n");
//  write ("Help for "+topic+" ("+group+")\n");
//  write ("---------------------------------------------------\n");
}


int cmd_help(string topic) {
  string *search_path;
  string *list;
  string arg,name ;
  int i, ii;
  object ob, act_ob;
  string bin, bintopic ;
  
  act_ob = this_player();
  
  if(!topic || topic == "") {
      this_player()->more(STD_HELP, 0 ,1);
      return 1;
    }
 
  topic = lower_case (topic);
 
  switch(topic) {

	case "properties" :	write("[Property document files]\n\n");
				this_player()->force_me("ls " + PROPH_DIR);
				return 1;
				break;
	case "efun"	:
	case "efuns"	:	write("[Efun document files]\n\n");
				this_player()->force_me("ls " + EFUN_DIR);
				return 1;
				break;
	case "lfun"	:
	case "lfuns"	: 	write("[Lfun document files]\n\n");
				this_player()->force_me("ls " + LFUN_DIR);
				return 1; 
				break;
	case "wizhelp"	:	write("[Wizhelp document files]\n\n");
				this_player()->force_me("ls " + WIZH_DIR);
				return 1;
				break;
	case "simul_efuns" :	
	case "simul_efun"  :	write("[Simul_efun document files]\n\n");
				this_player()->force_me("ls " + SE_DIR);
				return 1;
				break;
	case "applies"	:	write("[Applies document files]\n\n");
				this_player()->force_me("ls " + APPLY_DIR);
				return 1; 
				break;
	case "lpc" 	:	write("[LPC document files]\n\n");
				this_player()->force_me("ls " + LPCH_DIR);
				return 1;
				break;
	case "concepts"	:	write("[Concepts document files]\n\n");
				this_player()->force_me("ls " + CONCEPT_DIR);
				return 1;
				break;

   }


    list = DOC_D->get_docs_on(topic, explode((string)this_player()->query("PATH"), ":"));
    if (sizeof(list)) {
        if (sizeof(list)==2) {
            printf("%s: %s\n", list[1], list[0]);
            display_doc(list[1]);
            return 1;
        }
        for (ii=0;ii<sizeof(list);ii+=2) {
            printf("[%d] %s: %s", ii/2+1, list[ii+1], list[ii]);
        }
        printf("Which doc [1-%d,q]? ", sizeof(list)/2);
        input_to("which_doc", 0, list);
        return 1;
    }
  
  if ((ob = present(topic,act_ob)) ||
      (environment(act_ob) && (ob = present(topic,environment(act_ob)))))
    {
      if(function_exists("help",ob))
	{
	  write ("---------------------------------------------------\n");
	  write ("Help for "+topic+" (Object Help)\n");
	  write ("---------------------------------------------------\n");
	  ob->help();
	  return 1;
	}
    }
  write("There is no help available on that topic.\n");
  return 1;
}

void which_doc(string instr, string *list) {
int wd;

    if (!instr || !stringp(instr)) {
        printf("Which doc [1-%d,q]? ", sizeof(list)/2);
        input_to("which_doc", 0, list);
        return;
    }
    if ((instr[0] | 32) == 'q') {
        printf("Thank you for using Blue-Help(tm)!  Please come again.\n");
        return;
    }
    if (((wd = to_int(instr))) < 1 || (wd > sizeof(list)/2)) {
        printf("A number in the range 1 to %d, please.\nWhich doc [1-%d,q]? ", sizeof(list)/2, sizeof(list)/2);
        input_to("which_doc", 0, list);
        return;
    }
    display_doc(list[wd*2-1]);
}


void display_doc(string docfile) {
    object tmp_ob;

    if (file_exists(sprintf("%s.c", docfile))) {
        if ((tmp_ob = find_object(docfile)) || (tmp_ob = load_object(docfile))) {
            if (function_exists("help", tmp_ob)) {
                tmp_ob->help();
            }
            else {
                printf("A command %s exists, but it has no help.\n", docfile);
            }
        }
    }
    this_player()->more(docfile);
}


void see_alsos(string docfile) {
    string contents, tmp;
    string *see_alsos;

    contents = read_file(docfile);
    if (!sscanf(contents, "%*sSEE ALSO%s", tmp)) {
        return;
    }
    sscanf(tmp, "%s\n\n%*s", tmp);
    printf("See also: %s\n", tmp);
}


int help() {
	write ("Usage: help topic\n\n"+
"The help command can give you help on a number of things in the mudlib\n"+
"Typing \"help\" will get you a general overview. \"help start\" will show\n"+
"you some information for new players. \"help topics\" will give you list\n"+
"of things you might want to read help on.\n"
"If there is more than one help topic matching the one\n"
"you choose, you will be offered a menu to pick from.\n");
	return 1 ;
}
