/*
 * alias user command
 * this is from portals
 * One of them wrote it, again.
 * In the course of time it grew into part of the TMI-2 mudlib, via the
 * old TMI mudlib. In any event, you should leave this header on. Moby
 * added it in 4-9-93.
 */

// Leto added a feature that will add $* at the end, if it wasn't already
// defined before. 05-04-95 (or was it 04-05-95 ? :)
// Changed $* to look for $, since you can have $1,$2 etc. Leto 05-06

#include <daemons.h>
#include <mudlib.h>

inherit DAEMON ;

void print_aliases(mapping n, mapping x)
{
   int i;
   string *tmp;

   tmp = keys(n);
   if(sizeof(tmp)) {
      write("Normal Aliases:\n");
      tmp = sort_array(tmp, "sort_keys", this_object());
 
      for(i = 0; i < sizeof(tmp); i++)
         printf("   %-15s %s\n",tmp[i],n[tmp[i]]);
      }
 
   tmp = keys(x);
   if(sizeof(tmp)) {
      write("Xverb Aliases:\n");
      tmp = sort_array(tmp, "sort_keys", this_object());
 
      for(i = 0; i < sizeof(tmp); i++)
         printf("   %-15s %s\n",tmp[i],x[tmp[i]]);
   } else if(!sizeof(keys(n)))  
      write("   No aliases defined.\n");
}

int cmd_alias(string str) {
  int i, sl;
  int index;
  string verb,cmd,*elements;
  mapping alias;
  object act_ob;
  
  act_ob = previous_object();
  
  if(str == "-global")
  {
    print_aliases((mapping)GA_SERVER_D->query_global_aliases(),
      (mapping)GA_SERVER_D->query_global_xaliases());
    return 1;
  }
  if(str == "-clear")
    {
      act_ob->clear_aliases();
      return 1;
    }

  alias = (mapping) act_ob->query_aliases();
  if (!alias) alias = ([]);
  if(!str)
    {
      elements = keys (alias);
      if(!elements || !sizeof(elements))
	{
	  write("No aliases defined.\n");
	  return 1;
	}
      print_aliases((mapping)act_ob->query_nalias(),
         (mapping)act_ob->query_xalias());
      return 1;
    }
  
 
  //  Strip out the leading spaces in the command request

  while(str[0] == ' ')   str = str[1..<1];
 

  if(sscanf(str,"%s %s",verb,cmd) == 2)
    {
      if (!verb || verb == "" || verb == " ") {
	  notify_fail("Sorry, you must specify a verb.\n");
      return 0; }
 
      if(verb=="alias")
	{
	  notify_fail ("Sorry, you can't alias 'alias'.\n");
	  return 0;
	}

      if(verb=="unalias") {
	notify_fail("Sorry, you cannot alias 'unalias'.\n") ;
	return 0 ;
      }

if(strsrch(cmd,"$")==-1) cmd = cmd + " $*";
      if (environment(this_player())) // it's probably not a new player
	if (!alias[verb])
	  write("Alias: "+verb+" ("+cmd+") added.\n");
	else
	  write("Alias: "+verb+" ("+cmd+") altered.\n");
      act_ob->add_alias(verb,cmd);
      return 1;
    }
  if(!alias[str])
    {
      write("The alias "+str+" wasn't found.\n");
      return 1;
    }
  printf("%-15s%s\n",str,alias[str]);
  return 1;
}

static int sort_keys(string a, string b) {  return strcmp(a, b);  }


string help()
{
  return(@HELP
Flags accepted:
	-clear		Clears all aliases.
	-global		Show the global alises.
  
alias			View current aliases.
alias <alias> <command>	Set the verb alias to execute command.
alias <alias>		Check the value of <alias>
unalias <alias>		Remove <alias> from the alias list.
  
Substitution variable that exist are:
     $# - Where # is the number of the word after the verb to substitute.
     $* - Will be substituted with everything after the verb.

Prefixing the alias' verb with a $ allows you to set up a verb that does not.
require a space after it. i.e. 'alias $' sat $' sat $*' will allow you to do says as:
'Hey! This is most Stufly!

Look at default aliases for examples.
Note: $* is added to all aliases, unless it was part of it.
HELP
  );
}

