#include <mudlib.h>
#include <config.h>
#include <uid.h>
 
inherit DAEMON ;

#define PURGED	"/purged_1008"

 
 
static int move_contents(string path, int flag);
 
int clean_dir(string dir, int flag) {
   string tmp;
 
   //	Check euid of initiator to confirm correct permissions
   if(geteuid(previous_object()) != ROOT_UID &&
      !adminp(geteuid(previous_object())))
	return 0;
 
   if(!dir || dir == "")  return 0;
 
   if(file_size(dir) != -2)  return 0;		//  Not a directory
 
 
return move_contents(dir + "/", flag); }
 
 
//  This function recursively cleans a specific directory path
//  If flag = 1, it will not protect PROTECTed files
static int move_contents(string path, int flag) {
   mixed *dir;
   string what;
   int loop;
 
   //	Get directory contents array
   dir = get_dir( path );
   if(!dir || !sizeof(dir))  return -1;
 
   //	Loop through and delete contents of dir array
   for(loop=0; loop<sizeof(dir); loop++) {
   what = path + dir[loop];

   //	If selection is a directory ... recursively clean and remove it
   if(file_size( what ) == -2) {
  what = replace_string( what, "//", "/" );
   mkdirs( PURGED + what + "/" );
   move_contents(what + "/", 1);
   rmdir(what);  
   }
 
   //	If file isn't protected ... remove it
  if( flag ) {
  what = replace_string( what, "//", "/" );
     rename( what, PURGED + what );
	rm(what);
   }
  }
 
return 1; }
 
