// command to update all the files in a directory
// Coded by Skylight on 18-5-95

#include <mudlib.h>
#include <commands.h>
inherit DAEMON;

int
cmd_dupdate(string str) {
  string *files, dir;
  int len, i;
  object ob;
  str = resolv_path(this_player()->query("cwd"), str);
  if (file_size(str) == -2) str += "/*.c";
  len = strlen(str);
  if (len > 1 && str[len-2..len-1] != ".c")
    str += ".c";
  files = get_dir(str);
  if (!sizeof(files)) return notify_fail("No matching .c files found.\n");
  len = strsrch(str, "/", -1);
  dir = str[0..len];
  while (i < sizeof(files)) {
    CMD_UPDATE->cmd_update(dir+files[i]);
/*
    write(files[i]+": ");
    ob = find_object(dir+files[i]);
    if (ob) {
      catch(ob -> remove());
      if (ob) destruct(ob);
      write("destructed, ");
    }
    if (catch(call_other(dir+files[i], "???")))
      write("failed to load\n");
    else
      write("loaded\n");
*/
    ++i;
  }
  return 1;
}

