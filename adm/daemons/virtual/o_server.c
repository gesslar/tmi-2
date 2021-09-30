// persistent virtual object compiler

#include <mudlib.h>

inherit DAEMON;

#define DEBUG 1

#ifdef DEBUG
#define BUG(x) log_file("o_server", x)
#else
#define BUG(x)
#endif



object compile_object(string s) {
	string filename, uid, *tmp;
	object ob;

	write("trying to compile " + s + "\n");
  
	if (file_size(s + SAVE_EXTENSION)==-1)
		return 0;
	filename = read_file(s + SAVE_EXTENSION, 1, 1);
	if (!sscanf(filename, "#%s.c", filename)) {
		write("sscanf failed.\n");
		return 0;
	}
	write("got: " + filename + "\n");
	ob = new("/" + filename);
	write("cloned it...\n");
	tmp = explode(s, "/");
	uid = tmp[sizeof(tmp)-1];
	write("got uid: " + uid + "\n");
	seteuid(uid);
	export_uid(ob);
	seteuid(getuid());
	return ob;
}


