// recursive update with call other, for hacking ;)
// leto
// Avatar of Eodon, 02-11-95: Added help function to it
// Avatar of Eodon, 02-11-95: Bug removed: not trailing '/' necessary
// Also building a 'matching' function, making it possible to use
// something like: udr p*, or /cmds/wiz/_u??????

#include <commands.h>

inherit DAEMON;
/* Avatar: EodonLib uses some very nice defines! */
#define TP this_player()

create() 
{ 
  seteuid(getuid());
}

int cmd_udr(string str)
{
  string path;
  mixed *files;
  int i;
  
  if (!str || str=="")
    path = TP->query("cwd")+"/";
  else
    path = resolv_path(TP->query("cwd"),str);

  if (file_size(path)==-2 && path!="/")
    files = get_dir(path+"/*.c");
  else
  {
    files = get_dir(path+"*.c");
    if (path!="/")
      path = "/"+implode(explode(path,"/")[0..<2],"/")+"/";
  } 

  if (sizeof(files)>1)
    // Remove directories (current and parent)
    files -= ({".",".."});
  if (sizeof(files)>0)
    // but not itself :) 
    files -= ({"_udr.c"});

  write("Objects found: "+sizeof(files)+"\n");
  for (i=0; i<sizeof(files); i++)
  {
    set_eval_limit(-1);
    // Compose path
    str = path+(string)files[i];
    // if it *is* a file then update it...
    if (file_size(str)!=-2) 
      CMD_UPDATE->cmd_update(str);
  }
  return 1;
}

#define SYNTAX	"Usage: udr <path>\n"
string query_syntax()
{
  return "[<directory>[/filespec]]";
}

string query_desc()
{
  return "This command, which stands for update directory, "
         "will update and load all objects (*.c) found in that specified "
         "directory. If no argument is given, the current directory "+
         "is used.\nExamples:\n"
         "udr a*\n"
         "udr ../temp\n"
         "udr ../test/q??ck";
}
