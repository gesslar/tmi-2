// Note: work in progress

// Leto made the 2nd generation room maker and paintbrush tool. (Apr 1st, 95)
// The original idea by Tigycho, the room creator by Blue and the
// original painting routines by Mobydick. Leto changed the painting routines
// to use "item" : @Enditem instead, and added wipe and change options.
// Leto Apr 6th, 1995 - Changed header structure, added future support
//			for exits and objects
// Leto Apr 9th, 1995 - Added exit handling, started to reassign the
//			add_actions to be more flexible, bumped version
//			to 2.1
// Leto Apr 1-th, 1995 -Enhanced exit handling so it will allow exits names
//			that are relative, absolute or in the same dir
//			(eg: "/foo/bar","foobar" and "../../../foo/bar")
//			Also fixed writing files with wrong path, bumped
//			version to 2.2
// To do: changing of short and long, calling the editor for all edits,
//        finish the new make/change/remove syntax, add light on/off,
//
// Original credits follow
//
// Hacked roommaker for one purpose, Blue 95-03-19
//
//
// paintbrush.c
// Lets a wizard add item descriptions to his area interactively, instead
// of coding them.
// Warning - this object is a little dangerous because it writes files
// itself, and you want to be careful about what EUID it has. You want
// it to load with the EUID of the wizard that loaded it, generally, and
// you probably don't want players getting hold of it. You _definitely_
// don't want this thing floating around with admin EUID or, worse,
// Root. So be careful.
// The idea for this object is due to Tigycho@TMI-2.
// It was coded by Mobydick@TMI-2 on 4-30-94.
// The basic algorithm is to read the room file, find the line which
// begins setting item descriptions, and add the appropriate code to
// the file at that point. It is necessary to recognize the appropriate
// place to make the insertion and to have a consistent method of adding
// new descriptions. This particular version of the paintbrush looks to
// make insertions after a line that looks _exactly_ like this:
//	set ("item_desc", ([
// It must start with one tab, and the spacing must be identical to that
// above. It will add the new item description after that line in the
// following format:
//	set ("item_desc", ([
//		"frog" :
//"A big blue frog.\n"+          <--- [ Leto: this has changed ! ]
//"Kind of an ugly frog.\n",
//	<old item descs in the same format>
//	]) ) ;
// When you code your rooms, you can include the two lines:
//	set ("item_desc", ([
//	]) ) ;
// which will include no item descs but make it possible to paint in
// new ones later.
// The interested student should be able to convert the tool to other
// styles of adding item descriptions. Some formats are easier to
// support than others, so bear that in mind as you design.
//
// end of credits, lets get to it ;)

#include <mudlib.h>
#include <commands.h>
inherit OBJECT;

#define HEADER "// %s made by %s.\\n// %s\\n// \
Created with the 2nd generation Paintbrush which was brought to\\n// \
you by Blue, Mobydick and Leto\n\
\n#include <mudlib.h>\ninherit ROOM;\n\
void create() {\n    seteuid(getuid());\n    ::create();\n"
#define FOOTER "	set (\"light\", 1);\n\n\
	set (\"item_desc\", ([\n]) ) ;\n	set (\"objects\", ([\n]) ) ;\n}\n"

// These are strings we need to search for to add or remove stuff around it.
#define ITEM_STRING "	set (\"item_desc\", (["
#define EXIT_STRING "	set (\"exits\", (["
#define OBJECT_STRING "	set (\"object\", (["

int painting, insertpt, now ;
string itemname, itemfile, roomname, prev_room_name, opposite_dir;
string *roomdata, description, *firstdirs, *seconddirs;
mapping expanded_dirs;

int help (string str) ;
void repair_room() ;
string opposite_direction(string origdir);
void replace_desc(string answer, string olddesc, int wipeonly);
string expand_dir(string indir);
void back_dir(string bd, string dest);
int begin_painting (string str, int skip);
int remove_exit(string exit);
int write_exit(string filename, string exit, int exit_line);
int change_exit(string exit);
int make_exit(string exit);

void create() {
    seteuid(getuid());
    seteuid(geteuid(previous_object()));
    painting = 0;
    firstdirs = ({ "north","northeast","east","southeast","up","in","foo" });
    seconddirs = ({ "south","southwest","west","northwest","down","out","bar" });
    expanded_dirs = ([
        "e" : "east",
        "n" : "north",
        "s" : "south",
        "w" : "west",
        "nw" : "northwest",
        "se" : "southeast",
        "sw" : "southwest",
        "ne" : "northeast",
        "u" : "up",
        "d" : "down",
    ]);
    set("id", ({ "maker", "roommaker", "gogh","van gogh","paintbrush","brush" }) );
    set("short", "a Van Gogh paintbrush [version 2.2]");
    set("mass",100);
    set("bulk",10);
    set("long", @EndText
This is a combination of Blue's roommaker and Moby's brush. It seemed
natural to combine these too. Leto also added some more painting functions.
This should be a very effective tool in quick area generation which do have 
atmosphere. My condolences to the Diku mud player who told me 'the coconut 
wasn't an object in the room!'.

Type "help brush" for more info.
EndText
);

}

void init() {
    seteuid(geteuid(previous_object()));
    add_action("newroom", "build");
    add_action("make", "make");
    add_action("make_exit", "makeexit");
    add_action("change_exit", "changeexit");
    add_action("remove_exit", "removeexit");
    add_action("begin_painting", "paint") ;
    add_action("begin_painting", "makeitem") ;
    add_action("begin_painting", "changeitem") ;
    add_action("wipe_painting", "eraseitem") ;
    add_action("wipe_painting", "wipeitem") ;
    add_action("help", "help") ;
}

int make(string what) {

string type,name;

	if(!what || what =="") 
	  {
	   notify_fail("Usage: Make <type> <name>\n\nType can be item, exit or object\n");
	   return 0;
	  }
	else
	  {
	  // If no type is given, assume item_desc....
	  if (sscanf(what,"%s %s",type,name)!=2) return begin_painting(what,0);
	  if (type=="item")  return begin_painting(name,0);
	  if(type=="object") 
	    {
	     write("Objects are not yet implemented, sorry.\n");
	     return 1;
            }
          if(type=="exit") return make_exit(name);
          // Whatever you want, it's not here
	  write("I'm not familiar with that type.\n");
	  return 1;
	  }
}

int newroom(string dir) {
    string dest,tmp;
    object odest;
    int last_slash,end,parents;

// Leto's attempt at a little security
 if (getuid(this_player()) != geteuid(this_object()) )
{
  write("Not owner, please get your own copy at "+ base_name(this_object())+".\n");
    destruct(this_object());
}
    if (!dir) {
        notify_fail("A direction, please!\n");
        return 0;
    }
    dir = expand_dir(dir);
    dest = (string)environment(this_player())->query("exits/"+dir);

    if (!dest) {
        notify_fail("No exit in that direction, yet.\n");
        return 0;
    }

// if the exit is "/foo/bar" then it's ok, but if it's "bar" then we need
// to find the current directory of this room and use that as base for "bar"
// Also, "../../foo/bar.c needs to be found too. Note that if you use file
// names as "../foo/../bar.c" that you will screw up, but you shouldn't use
// those filenames anyway :)
// Thus being in the right directory is no longer needed.

// Check if the exit is absolute, and if not make it absolute

   if( (dest[0]!='/') && (dest[0..2]!="../") ){
	tmp = base_name(environment(this_player()));
	last_slash = strsrch(tmp,"/",-1);
	dest = tmp[0..last_slash]+dest;
}

// Check if path is relative (using "../"), if so make it absolute 

if(dest[0..2]=="../") {

	parents = 0; end = 0;
	tmp = base_name(environment(this_player()));
	while(!end) {
    	   if(dest[0..2]=="../") 
	     {
	      dest = dest[3..<1];
	      parents = parents + 1;
	     }
	   else end = 1;
	}	
	// We now know the number of occurances of "../" in the exit string.
	// Let's make our absolute path for write_file.
	for(parents;parents>=0;parents--) {
	tmp = tmp[0..strsrch(tmp,"/",-1)-1];
	}
	dest = tmp+"/"+dest;
}

    if(dest[<2..<1] != ".c") dest = dest + ".c";
    if (file_exists(dest)) {
        notify_fail("That room already exists.\n");
        return 0;
    }
    prev_room_name = file_name(environment(this_player()));
    opposite_dir = opposite_direction(dir);

    write_file(dest, sprintf(HEADER, dest, capitalize(geteuid())+"@"+ mud_name(), ctime(time())));
    write("Short description: ");
    input_to("get_short", 0, dest);
    return 1;
}

void get_short(string sh, string dest) {
    if (!sh) {
        write("Put in a short desc, please: ");
        input_to("get_short", 0, dest);
    }
    write_file(dest, sprintf("    set(\"short\", \"%s\");\n    set(\"long\", @EndText\n", sh));
    write("Now a long desc, ending with '.'.\n");
    input_to("get_long", 0, dest);
    return;
}

void get_long(string lo, string dest) {
    if (lo == ".") {
        write_file(dest, "EndText\n      );\n"+EXIT_STRING+"\n");
        write("Long description done.\n");
        back_dir(opposite_dir, dest);
        return;
    }
    else {
        if (!lo) lo = "";
        write_file(dest, sprintf("%s\n", lo));
        input_to("get_long", 0, dest);
        return;
    }
}

void back_dir(string bd, string dest) {
    if (!bd || (bd=="")) {
        write("Please enter direction for previous room: ");
        input_to("back_dir", 0, dest);
        return;
    }
    else {
        printf("Exit %s to %s.\n", bd, prev_room_name);
        write_file(dest, sprintf("		\"%s\" : \"%s\",\n", bd, prev_room_name));
        write("Please enter the name of any other exit directions ('.' for none): ");
        input_to("exit_dir", 0, dest);
        return;
    }
}

void exit_dir(string ex, string dest) {
    if (!ex || ex=="") {
        write("Enter the name of a new exit direction or '.' for none): ");
        input_to("exit_dir", 0, dest);
        return;
    }
    else if (ex==".") {
        write_file(dest, "      ]) );\n");
        write_file(dest, FOOTER);
        printf("File written on %s.\n", dest);
	CMD_UPDATE->do_update(dest); // in case of loaded, rm'ed file
        this_player()->move_player(dest);
        return;
    }
    else {
        write_file(dest, sprintf("		\"%s\" : ", ex));
        write("Destination filename for " +ex+ " please (relative to this room): ");
        input_to("get_exdest", 0, dest);
        return;
    }
}

void get_exdest(string xd, string dest) {
    if (!xd) {
        write("Destination filename (relative to this room): ");
        input_to("get_exdest", 0, dest);
        return;
    }
    else {
        write_file(dest, sprintf("\"%s/%s\",\n", this_player()->query("cwd"), xd));
        write("Exit direction: ");
        input_to("exit_dir", 0, dest);
        return;
    }
}


string opposite_direction(string origdir) {
    int i;
    if ((i = member_array(origdir, firstdirs)) == -1) {
        if ((i = member_array(origdir, seconddirs)) == -1) {
            return "";
	}
	else {
	    return firstdirs[i];
	}
    }
    return seconddirs[i];
}

string expand_dir(string indir) {
    if (!expanded_dirs[indir]) return indir;
    else return expanded_dirs[indir];
}

int query_auto_load() {

    return 1; 
}

// This is also security-related: if the item has an environment, then
// it cannot move to a new one. Prevents dropping, theft, or other Bad
// Things.
// Leto: I don't think it works though Moby ;)

int move (mixed dest) {
	if (environment(this_object())) return 1 ;
	return ::move(dest) ;
}

int wipe_painting (string str) {


	write("ENV:"+environment(this_player())->query("item_desc/"+str));
	if(environment(this_player())->query("item_desc/"+str))
		replace_desc("y",str,1); //no confirms for me ;)
	else {
		write("No such item description in this room, aborted\n");
		return 1;
		}
	return 1;
}

int begin_painting (string str, int skip) {

	object here ;
	string olddesc;

	if (!str || str == "") return help("paint") ;
	// I'm sure there are more ways to foobar a file,let's catch this one ;)
	write(sscanf(str,"%*s %*s"));
	if (sscanf(str,"%*s %*s")==2)
	  {
	   write("Item descriptions can not have spaces!\n");
	   return 1;
	  }
	if(!skip) {
	olddesc = environment(this_player())->query("item_desc/"+str);
	if(olddesc) {
			write("Changing old description [y/n] ?\n");
			input_to("replace_desc",0,str);
			return 1;
		    }
         }
/*
// This can never happen, right Moby ?
	if (painting) {
	     write ("The paintbrush is already in use: try again later.\n") ;
	    return 1 ;
	}
 */
	here = environment(this_player()) ;
	if (!here) {
	    notify_fail ("You cannot paint the void: there's nothing to put the paint on.\n") ;
	    return 0 ;
	}
	roomname = file_name(here) ;
	if (!roomname) {
	    write ("Fatal error: could not determine this room's file name.\n") ;
	    return 1 ;
	}
	roomname += ".c" ;
	if (!file_exists(roomname)) {
	    write ("You cannot paint virtual rooms.\n") ;
	    return 1 ;
	}
// Leto : this can prob go, we had set the proper euids, so people shouldn't
// be able to paint other things.

// This is security but it's flimsy and very easy to defeat. It is
// here to prevent randoms from doing damage and does not actually
// make things secure. Only rely on the proper EUID for real security.
	if (!"/adm/obj/master"->valid_write(roomname,this_player(),"paintbrush")) {
	    write ("You try to paint the room but you find you cannot.\n") ;
	    return 1 ;
	}
// Read in the room file and find the insertion point.
	roomdata = explode(read_file(roomname),"\n") ;
	insertpt = member_array(ITEM_STRING,roomdata,1) ;
	if (insertpt==-1) {
	    write ("Could not find the proper place to insert item descriptions in this room.\n") ;
	    return 1 ;
	}
// Save a copy of the the original file, in case Something Goes Wrong.
	now = time() ;
	cp(roomname,"/tmp/room."+now) ;
// Let the player enter the item description.
	write ("You begin painting "+article(str)+" "+str+".\nTell me what you want it to look like:\n") ;
	itemname = str ;
		itemfile = "/tmp/paint."+now ;
	rm(itemfile) ;
	this_player()->edit(itemfile,"finish_paint", this_object()) ;
	return 1 ;
}

// This gets called when the player finishes editing the file description.

void finish_paint() {

	int ret, i ;

	description=read_file(itemfile);
	if (!description) {
	    write ("Painting aborted.\n") ;
	    return ;
	}
// Write out the first part of the file, then the new item_desc,
// then the rest of the file.
	rm(roomname) ;
	for (i=0;i<=insertpt;i++) {
	    ret = write_file(roomname,roomdata[i]+"\n") ;
	    if (ret!=1) {
		write ("Failed to write the new file.\n") ;
		repair_room() ;
		return ;
	    }
	}
	write_file (roomname,"		\""+itemname+"\" : @"+"End"+itemname+"\n") ;
	write_file(roomname,description+"End"+itemname+",\n");
	
	for (i=insertpt+1;i<sizeof(roomdata);i++) {
	    ret = write_file(roomname,roomdata[i]+"\n") ;
            if (ret!=1) {
                write ("Failed to write the new file.\n") ;
                repair_room() ;
                return ;
            }
        }
	write ("Your painting seems to come to life!\n") ;
	CMD_UPDATE->do_update(roomname) ;
	// Clean up temp files
	rm("/tmp/paint."+now);
	rm("/tmp/room."+now);
	return ;
}

void repair_room() {
	cp ("/tmp/room."+now,roomname) ;
	write ("Restored the old room file.\n") ;
	return ;
}

void replace_desc(string answer, string olddesc, int wipeonly) {

string *New, beginhit, endhit;
int beginpt, endpt,i;

	if(answer!="y" && answer!="yes") { write("Aborted with:"+answer+"!\n"); return;}
	roomname = file_name(environment(this_player())) + ".c";
	write("\n"+roomname+","+olddesc+"\n");
	roomdata = explode( read_file(roomname),"\n") ;

	beginpt = member_array(
"		\""+olddesc+"\" : @"+"End"+olddesc, roomdata) ;
	if (beginpt==-1) {
	    write ("Could not find the start of the old item descriptions in this room.\n") ;
	    return  ;
	} 
	endpt = member_array("End"+olddesc+",",roomdata,beginpt+1) ;
	if (endpt==-1) {
	    	write ("Could not find the end of the old item descriptions in this room.\n") ;
	    	return  ;
	}  
//	write("begin at line:"+beginpt+"\n");
//	write("end at line:"+endpt+"\n");
	New = roomdata[0..beginpt-1] + roomdata[endpt+1..<1];
	cp(roomname,roomname[0..<3]+".old");
	rm(roomname);
	write_file(roomname,implode(New,"\n")+"\n") ;
	write("You have erased the poor "+olddesc+".\n");
	if(wipeonly) CMD_UPDATE->do_update(roomname) ;
	// From now on, it acts the same as a 'new' description if
	// not called as wipeonly.
	if(!wipeonly) begin_painting(olddesc,1);
	return;
}

int change_exit(string exit) {

	write("Removing existing exit...\n");
        remove_exit(exit);
	make_exit(exit);
	return 1;
}

int make_exit(string exit) {

int exit_line;
string *New;

	if(environment(this_player())->query("exits/"+exit)) 
		return change_exit(exit);
	else
        {
        roomname = file_name(environment(this_player())) + ".c";
        roomdata = explode( read_file(roomname),"\n") ;
        exit_line = member_array(EXIT_STRING,roomdata);
        if (exit_line==-1) {
            write ("Could not find the exits descriptions in this room.\n");
	    return 1;
	    } else {
	printf("Please enter a filename for the exit '%s' :",exit);
	input_to("write_exit",0,exit,exit_line);
	return 1;
		}        
	}
}

int write_exit(string filename, string exit, int exit_line) {

	// There wasn't such an exit, so it's safe to just add one.
		
	cp(roomname,roomname[0..<3]+".old");
	rm(roomname);
	write_file(roomname,implode(roomdata[0..exit_line],"\n")+"\n") ;
	write_file(roomname, "		\""+ exit +"\" : \""+ filename +"\",\n");
	write_file(roomname,implode(roomdata[exit_line+1..<1],"\n")+"\n") ;
	write("Exit added.\n");
	CMD_UPDATE->do_update(roomname) ;
}

int remove_exit(string exit) {

int exit_line,i;

	if(!environment(this_player())->query("exits/"+exit)) 
	   {
	    write("No such exit here.\n");
	    return 1;
	   }
	else
           {
           roomname = file_name(environment(this_player())) + ".c";
           roomdata = explode( read_file(roomname),"\n") ;
           exit_line = member_array(EXIT_STRING,roomdata);
           if (exit_line==-1) 
		{
                 write ("Could not find the exits descriptions in this room.\n");
	         return 1;
	        } else 
		   {
	                for ( i=exit_line+1;i<sizeof(roomdata);i++) 
			  {
			  if(strsrch(roomdata[i], "		\""+exit+"\" :")==0)
		 	    break ;
                          }
		        if(i) {
				cp(roomname,roomname[0..<3]+".old");
				rm(roomname);
				write_file(roomname,implode(roomdata[0..i-1],"\n")+"\n") ;
				write_file(roomname,implode(roomdata[i+1..<1],"\n")+"\n") ;
				write("Line "+i+" containing exit '"+exit+"' removed.\n");
				CMD_UPDATE->do_update(roomname) ;
				return 1;
			      }
			else  {
				write("Failed to remove exit, incompatible format\n");
				return 1;
			      }
		   }
	       }
}

int help (string str) {
	if (!str || !id(str)) return 0 ;
	write  ( @EndHelp
The 2nd generation paintbrush is the best tool for any wizard. It lets
you create new rooms, and add, change or remove item descriptions, objects
and exits.
The paintbrush is picky about the format used in the files, so it is
probably best to only paint in rooms created with the brush itself.
To use it make your first room, and create an exit to a file that doesn't
exist. Then type "build <exit>". the brush will ask all the information it
needs, and create the file.

Commands:

paint  <name>
build  <exit>
make   <type> <name>      
change <type> <name>     
remove <type> <name>

(I'm still revamping the file, so for now, some may use the old syntax
 of makeexit,removeexit, changeexit and other commands might be there
 -- Leto)

Type can be one of "item", "object" or "exit". If no type is supplied,
item is assumed.

The Paintbrush 2nd generation (Van Gogh model) was brought to you by Blue,
Leto and Mobydick.
EndHelp
) ;
	return 1 ;
}

