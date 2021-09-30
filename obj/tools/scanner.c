/*
        By Drizzt@Darkpowers
        This little thingy-mo-bobber will scan domain directories for
        rooms, and print up some stats on themm..
        A typical datafile should look like
        /d/Fooland
        /d/Fooland/rooms
        Or something or a nothers.  Just so you get all the rooms ditectories into 
        A datafile
 
        -- Drizzt@Darkpowers added the -r and -f options
        -- Drizzt@Darkpowers added the -d option
	-- Leto@Earth fixed for use of MudOS v21, and prevented max_eval error
        -- Avatar@Eodon fixed for use of MudOS v21.b5
*/

#include <mudlib.h>
inherit OBJECT;
 
string *scandirs,dm;
mixed *fail;
int rc,mc,oc,dflag;
mapping err;
mapping misfil;
 
int startup(string foo);
void exit_consistancy(object ob);
int start_up();
int scan_dir(string path);
void write_scanfile(string check, int rflag, int fflag);
string chunk(string str);
 
string chunk(string str){
        if(!str) return 0;
        if( str[<2..<1] != ".c" )
           return str + ".c";
        else return str;
}
 
void write_scanfile(string check, int rflag, int fflag){
        string *files,file;
        int i;
        if(fflag){
                file = read_file(check);
                files = explode(file,"\n");
                i = sizeof(files);
                while(i--){
                        scandirs += ({ files[i] });
                }
                if(rflag){
                        i = sizeof(scandirs);
                        while(i--){
                                scan_dir(scandirs[i]+"/");
                        }
                }
        }
        if(!fflag){
                scandirs += ({ check });
                if(rflag){
                        i = sizeof(scandirs);
                        while(i--){
                                scan_dir(scandirs[i]+"/");
                        }
                }
        }
        return ;
}
 
int scan_dir(string path) {
        mixed *dir;
        string what;
        int loop;
 
        dir = get_dir( path );
        if(!dir || !sizeof(dir))  return -1;
        for(loop=0; loop<sizeof(dir); loop++) {
                what = path + dir[loop];
                if(file_size( what ) == -2) {
                        scan_dir(what + "/");
                        scandirs += ({ what });
                }
        }
        return 1; 
}
 
void create(){
        seteuid(getuid());
        scandirs = ({});
        set("id",({"scanner","ms","*mu*","clip","clipboard"}));
        set("short","A clip-board for scanning domains");
        set("long",
"A clip-board for scanning domains\n[1] setup <flags> <file>\n"
"[2] scan (Scans files setup within setup)\n[3] write - writes"
" file to home directory\n[4] info - tells same as write.\n");
        set("bulk",0);
        set("mass",0);
}
                      
void init(){
        add_action("startup","setup");
        add_action("info","info");
        add_action("writeit","write");
         add_action("start_up","Scan");
}
 
int startup(string foo){
        string *flags, foostr,file,fle,*doodle;
        int rflag, fflag,nflag,i;
 
        if(!foo || sscanf(foo,"%s %s",foostr,fle) != 2){
                write("MS: setup <flags> <filename>\n");
                write("VFlags: -r = Recurse Subdirectories\n");
                write("VFlags: -f = Use a pre-made file\n");
		write("VFlags: ~/ = Use no options\n");
                write("VFlags: -d = Scan a domain <filename = domain>\n");
                write("Example: setup -r /d/Fooland ( Will scan /d/Fooland and recurse )\n");
                write("Example: setup -rf /open/foo.data ( Will use /open/foo.data for scanning data\n");
                write("         And will recurse.\n");
                write("Example: setup -f /open/foo.data ( Will simply scan only using foo.data )\n");
                write("Example: setup -d Fooland ( Will scan ALL of the Fooland Domain.\n");
                return 1;
        }
        write("MS: Settings Reset.\n");
        dm = 0;
        dflag = 0;
        scandirs = ({});
        file = resolv_path((string)this_player()->query("cwd"),fle);
        flags = explode(lower_case(foostr),"");
        if(member_array("-", flags) != -1) flags -= ({ "-" });
        if(member_array("r", flags) != -1) rflag = 1;
        if(member_array("f", flags) != -1) fflag = 1;
        if(member_array("/", flags) != -1) nflag = 1;
        if(member_array("d", flags) != -1) dflag = 1;
 
        if(dflag && fflag || dflag && rflag || dflag && nflag){
                write("MS: You cannot specify the 'd' flag with any other flags.\n");
                return 1;
        }
        if(fflag && !file_exists(file) && !nflag){
                write("MS: No such file exists \""+file+"\".\n");
                return 1;
        }
        if(!fflag && !directory_exists(fle) && !nflag && !dflag){
                write("MS: No such directory exists \""+fle+"\".\n");
                return 1;
        }
        if(nflag && rflag || nflag && fflag){
                write("MS: Invalid use of flags.\n");
                return 1;
        }
        if(dflag){
                if(!directory_exists("/d/"+fle)){
                        write("MS: There is no such domain.\n");
                        write("MS: Valid Domain Directories include:\n");
                        doodle = get_dir("/d/");
                        i = sizeof(doodle);
                        while(i--){
                                if(file_size("/d/"+doodle[i]) == -2) write("MS: "+doodle[i]+"\n");
                        }
                        return 1;
                }
        }
        write("MS: Starting the scan: ");
        if(dflag) write("MS: Scanning Domain: "+fle+"\n");
        if(rflag) write("MS: Recursing Directories\n");
        if(fflag) write("MS: Using existing Configuration\n");
        write("\n");
        scandirs = ({});
        if(dflag) write_scanfile("/d/"+fle,1, 0);
        dm = fle;
        if(fflag) write_scanfile(file,rflag,fflag);
        else 
                  write_scanfile(fle,rflag,fflag);      
        scandirs = uniq_array(scandirs);
        write("MS: Scanning Complete - "+sizeof(scandirs)+" directories loaded.\n");
        write("MS: Now type <Scan> to start the scanner.\n");
        return 1;
}
 
int start_up(){
        int i,j,k;
        string *dir,val,foo;
        string *individ;
        object ms,rom, *obs;
        err = ([ ]);
        misfil = ([]);
        fail = ({});
        rc = 0;
        oc = 0;
        mc = 0;
        if(sizeof(scandirs) == 0){
                write("MS: No directories have been set for scanning.\n");
                return 1;
        }
        write("MS: Initiating Scan:\n");
        individ = scandirs;
        i = sizeof(individ);
        while(i--){
                reset_eval_cost(); // Leto
                write("MS: Scanning - "+individ[i]+" ");                
                dir = get_dir(individ[i]+"/");
                j = sizeof(dir);
                write(j+" files.\n");
                while(j--){
                        reset_eval_cost(); // Leto 
                        if(sscanf(individ[i]+"/"+dir[j],"%s.c",foo) == 1){                        
                                foo = individ[i]+"/"+dir[j];
                                rom = find_object(foo);
                                if(!rom) {
                                        val = catch(foo->XXX());
                                        if(val){
                                                fail += ({ chunk(foo) });
                                        } else {
                                                rom = find_object(foo);
                                        }
                                }
                                if(rom && rom->query("exits")){
                                        obs = all_inventory(rom);
                                        k = sizeof(obs);
                                        while(k--){
                                                if(obs[k]->query("npc")) mc++;
                                                if(!obs[k]->query("npc") && !userp(obs[k])) oc++;
                                        }
                                        rc++;
                                        exit_consistancy(rom);
                                        rom->remove();
                                }                                
                        }
                }
        }
        write("MS: Finished.  Use <info> to display the information\n");
        write("    And <write> to write to file.\n");
        fail = uniq_array(fail);
        return 1;
}                                 
 
void exit_consistancy(object ob){
        mapping exits, newexits;
        string *rooms, *dirs, *rooms2, file,foo,fl,val;
        object newb;
        int i,j;
        
        fl = base_name(ob);
        exits = ob->query("exits");
        dirs = keys(exits);
        rooms = values(exits);
        i = sizeof(dirs);
        while(i--){
                // First we need the file to be in file format not file.c
                file = rooms[i];
                file = chunk(file);
                if(file_exists(file)){
                        foo = file;
                        newb = find_object(foo);
                        if(!newb) {
                                val = catch(foo->XXX());
                                if(val){
                                        fail += ({ chunk(foo) });
                                } else {
                                        newb = find_object(foo);
                                }
                        }
                        if(newb){
                                newexits = newb->query("exits");
                                if(newexits){
                                        rooms2 = values(newexits);
                                        if(member_array(fl,rooms2) == -1 && member_array(fl +".c",rooms2) == -1){
                                                err += ([ chunk(base_name(ob)) : dirs[i] ]);
                                        } 
                                }        
                        } else {
                                fail += ({ chunk(file) });
                        }
                } else {
                        misfil += ([ chunk(fl) : chunk(file) ]); 
                }
        }
}
                                
 
int info(){
        int i;
        string *files, *dirs, stuff;
        
        stuff = stuff + "Total objects counted were : "+oc+"\n";
        stuff = stuff + "Total rooms counted were   : "+rc+"\n";
        stuff = stuff + "Total monsters counted were: "+mc+"\n";
        if(!err || sizeof(err) == 0){
                stuff = stuff + "Exit Consistancy: No errors found.\n";
        } else {
                files = keys(err);
                dirs = values(err);
                i = sizeof(dirs);
                stuff = stuff + "Exit Consistancy: "+i+" errors found.\n";
                while(i--){
                        stuff = stuff + "File: "+files[i]+" - Exit: "+dirs[i]+"\n";
                }
                stuff = stuff + "Missing Files: "+sizeof(misfil)+" were missing.\n";
                files = keys(misfil);
                dirs = values(misfil);
                i = sizeof(files);
                while(i--){
                        stuff = stuff + "File:"+files[i]+" couldn't access "+dirs[i];
                }
                i = sizeof(fail);
                stuff = stuff + "Non-Loading files:";
                if(i){
                        stuff = stuff + " "+i+" errors found.\n";
                        while(i--){
                                stuff = stuff + "File: "+dump_variable(fail[i])+" failed to load\n";
                        }
                } else {
                        stuff = stuff + " All files loaded\n";
                }
        } 
        this_player()->more(stuff);
        return 1;
}
 
int writeit(){
        string file,*dirs,*files;
        int i;
        if(!dflag){
                file = user_path(geteuid(this_player()))+"scanlog."+time();
                if(!master()->valid_write(file,this_player(),"ed"))
                        file = "/tmp/"+geteuid(this_player())+".scanlog";
                if(!master()->valid_write(file,this_player(),"ed")){
                        write("MS: Write failed.\n");
                        return 1;
                }
        } else {
                file = "/d/"+dm;
                if(!master()->valid_write(file,this_player(),"ed"))
                        file = user_path(geteuid(this_player()))+"scanlog."+time();
                if(!master()->valid_write(file,this_player(),"ed"))
                        file = "/open/"+geteuid(this_player())+".scanlog";
                if(!master()->valid_write(file,this_player(),"ed")){
                        write("MS: Write failed.\n");
                        return 1;
                }
        }
        write_file(file,"ScanLog report at "+ctime(time())+"\n");
        write_file(file,"***********************************\n");
        write_file(file,"   Total Rooms: "+rc+"\n");
        write_file(file,"Total Monsters: "+mc+"\n");
        write_file(file," Total Objects: "+oc+"\n");
        write_file(file,"***********************************\n");
        files = keys(err);
        dirs = values(err);
        i = sizeof(files);
        if(i){
                write_file(file,"Exit-Consistancy Check: "+i+" errors found.\n");
                write_file(file,"-------------------------------------------\n");
                while(i--){
                        write_file(file,"["+i+"] File: "+files[i]+" Dir: "+dirs[i]+"\n");
                }
                write_file(file,"***********************************\n");
        } else {
                write_file(file,"Exit-Consistancy Check: No errors found.\n");
                write_file(file,"***********************************\n");
        }
        files = keys(misfil);
        dirs = values(misfil);
        i = sizeof(dirs);
        if(i){
                write_file(file,"Missing Files Check: "+i+" errors found.\n");
                write_file(file,"----------------------------------------\n");
                while(i--){
                        write_file(file,"["+i+"] File: "+dirs[i]+" | accessed by "+files[i]);
                }
                write_file(file,"***********************************\n");
        } else {
                write_file(file,"Missing Files Check: No errors found.\n");
                write_file(file,"***********************************\n");
        }
        i = sizeof(fail);
        if(i){
                write_file(file,"Non-Loading Files Check: "+i+" errors found.\n");
                write_file(file,"----------------------------------------\n");
                while(i--){
                        write_file(file,"["+i+"] File: "+fail[i]+"\n");
                }
                write_file(file,"***********************************\n");
        } else {
                write_file(file,"Non-Loading Files Check: All files loaded.\n");
        }
        write("MS: File written to \""+file+"\"\n");
        return 1;
}
