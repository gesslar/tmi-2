/*
// File     : master.c
//
// Purpose:
// This is the MudOS master object.
// This is the second object loaded after simul_efun.c.
// Everything written with 'write()' at startup will be printed on stdout.
// 1. create() will be called first.
// 2. flag() will be called once for every argument to the flag -f
//    supplied to 'parse'.
// 3. epilog() will be called.
// 4. The game will enter multiuser mode, and enable log in.
//
// Revision history:
// Time       Who and What
// (mudlib.n)
//          : Originally written by unknown authors, probably Lars
// The Mud Institute (TMI)
//          : Sulam and Buddha added or removed functions as they became
//            necessary or obsolete.
slk
// 91-12-** : Sulam mostly rewrote it.
dldlk
//            during the development of the new MudOS enhanced LPmud driver
ddk
// 92-03-12 : Buddha revised it again to support more than one mud.
// 92-04-11 : Sulam hacked and slashed some serious bugs in access and groups
//            necessitated inheriting those files.
// (TMI-2)
// 93-05-28 : Mobydick altered valid_write so that certain log_files could be
//            written by certain objects (/std/user and /cmds/file/_ed)
//            even when their EUID's didn't permit. This was necessary so that
//            the editing log and USAGE log could be maintained, since the
//            log_file calls for those logs come from objects that
//            don't have ROOT_UID.
// 93-08-28 : Pallando made various additions including valid_link()
// 94-03-14 : Atomic Archimedes added the call to reclaim_objects().
// 94-06-12 : Pallando did UIDs for /www
// 94-09-08 : Pallando fixed bugs in crash()
//
// 94-11-09 : Leto renamed the master applies that changed since 0.9.19.26
//            (destruct_environment_of -> destruct_env_of) 
//            (get_ed_buffer_save_file_name -> get_save_file_name ) 
//            Added valid_object(object ob)  
.ddsk;
//            Added Beek's error handling code
*/
// 94-12-23 : Leto added Sym's patch for valid_override (for v20.25a4+)
// 95-04-26 : Leto removed the .edrc file use. It's a property now
//            Also fixed some simul_efun calls, which were
//            preventing the simul_obj from updating (resursive loop)
// 96-03-03 : Leto fixed a hole in creator_file returning Root
//            or backbone in case u,d or student.

#pragma save_binary

#include <uid.h>
#include <config.h>
#include <daemons.h>
#include <net/daemons.h>
#include <commands.h>
#include <switches.h>
#include <domains.h>
#include <logs.h>
#include <driver/origin.h>

#include "/adm/obj/master/parse_com.c"
inherit "/adm/obj/master/access";
//inherit "/adm/obj/master/groups";  // Leto, done by access.c now

static int access_loaded = 0;

void preload( string file );

void create()
{
	if(origin() == ORIGIN_DRIVER)
		call_out("free_objects", 3600);
}

static void free_objects()
{
	call_out("free_objects", 3600);
	efun::reclaim_objects();
}

// To test a new function xx in object yy, do
// parse "-fcall yy xx arg" "-fshutdown"

static void flag( string str )
{
     string file;
     mixed arg;
    
    if( sscanf( str, "for %d", arg ) == 1 )
    {
        int i;
        // empty loop for speed test
        for( i = 0; i < arg; i++ ) { }
        return;
    }
    if( sscanf( str, "echo %s", arg ) == 1 )
    {
        write( arg + "\n" );
        return;
    }
    if( sscanf( str, "call %s %s", file, arg ) == 2 )
    {
        arg = (string)call_other( file, arg );
        write( "Got " + arg + " back.\n" );
        return;
    }
    write( "master: Unknown flag " + str + "\n" );
}

// This function is called every time a player connects.
// input_to() can't be called from here.

object connect()
{
    object login_ob;
    mixed err;
    
    err = catch( login_ob = clone_object( CONNECTION ) );
    if( err )
    {
        receive( "It looks like someone is working on the player object.\n" );
        receive( err );
        destruct( this_object() );
    }
    return login_ob;
}


// This is used for loading "virtual objects".
// Again, a separate program handles this task.

mixed compile_object( string file )
{
    return (mixed)VIRTUAL_D-> compile_object( file );
}

// This is called when there is a segmentation fault or a bus error,
// As it's static it can't be called by anything but the driver.

static void crash( string error, object command_giver, object current_object )
{
    log_file( "crashes", mud_name() + " CRASHED on: " + ctime( time() ) +
      " ERROR: " + error + "\n" );
    shout( "Game Driver shouts: Ack!!!!!!\n" ); /* polite for the dist copy */
    shout( "Someone tells you: I think the game is crashing.\n" );
    shout( "Game Driver forced you to: quit\n" );

    //  idea from MudOS's testsuite
    if( command_giver ) log_file( "crashes",
	"this_player: " + file_name( command_giver ) + "\n" );
    if( current_object )log_file( "crashes",
	"this_object: " + file_name( current_object ) + "\n" );

    //  Added by Inspiral with ideas from Wirehead..hopefully this will 
    //    prevent MudOS to save users if the filesystem is full.
    write_file( TMP_DIR "MudOS_crash", "abcdefg" );
    if( file_size( TMP_DIR "MudOS_crash" ) )
    {
	rm( TMP_DIR "MudOS_crash" );
	SHUTDOWN_D-> do_shutdown( 0 );
    }
}

// Write and Read stuff:
// valid_write: called with the file name, the object initiating the call,
//              and the function by which they called it.
// valid_read:  called exactly the same as valid_write()
//
// These now use a special feature that enables us to update the access
// of people without rebooting the game, an unfortunate side effect of
// the old system.
//
// Something to be careful of is commands in /bin that do file manipulation,
// as they have root access.

int valid_write( string file, mixed user, string func )
{
    int i;
    string tmp, eff_user;

//    return 1;
    if( !access_loaded )
    {
        access_loaded = 1;
        if( !load_groups() )
        {
            write( "*Error in loading group list\n" );
            shutdown();
        }
        if( !load_access() )
        {
            write( "*Error in loading access list\n" );
            shutdown();
        }
    }
// Leto debug
    if( user == master() && func == "log_file" ) return 1;
    if( !objectp( user ) )
        user = find_player( user );
    if( !user ) return 0;
    if( geteuid( user ) == ROOT_UID )/* Should this be allowed? */
	    return 1;
    
    if( ( func != "rm" ) && !( QUOTA-> quota_check( file ) ) )
    {
        write( "Warning: Directory quota violation.\n" );
        return 0;
    }
    
    i = check_access( file, user );
    if( i&2 )
        return( i&2 );
    // added by Tru for /std/save.c, extended by Buddha for /std/user/save.c
    else if( objectp( user ) )
        return( file == data_file( user ) || file == user_data_file( user )+__SAVE_EXTENSION__ );
    else return 0;
}

int valid_read( string file, mixed user, string func )
{
    int i;
    
   // return 1; commented out Leto Dec 27 19:51
    if( !access_loaded )
    {
        access_loaded = 1;
        if( !load_groups() )
        {
            write( "*Error in loading group list\n" );
            shutdown();
        }
        if( !load_access() )
        {
            write( "*Error in loading access list\n" );
            shutdown();
        }
    }
    if( func == "file_size" ) return 1; // make ls a bit faster
    if( !objectp( user ) )
        user = find_living( user );
    if( geteuid( user ) == ROOT_UID )
        return 1;
    i = check_access( file, user );
    if( i&1 )
        return( i&1 );
    // added by Tru for /std/save.c, extended by Buddha for /std/user/save.c
    else if( objectp( user ) )
        return( file == data_file( user ) || file == user_data_file( user )+__SAVE_EXTENSION__ );
    else return 0;
}

// valid_save_binary: determines whether an object will save a binary
// image of itself to the directory specified in the runtime config
// file. (TMI-2 uses /data/binaries)  See the driver's options.h for
// more information.
int valid_save_binary( string filename )
{
    string creator;
    creator = creator_file( filename );
    if( creator == ROOT_UID || creator == BACKBONE_UID ) return 1;
    return 0;
}

// valid_seteuid: determines whether an object ob can become euid str.
// This is very important because the euids still control most of the
// access permissions.
int valid_seteuid( object ob, string str )
{
    // ROOT_UID has priveleges...
    if( getuid( ob ) == ROOT_UID ) return 1;
    if( file_name( ob ) == SIMUL_EFUN_OB ) return 1;
    // TEMP, to provide an euid with 'basic' permissions, is able to write
    // to the datafiles of some bulletin boards.  This may no longer be needed.
    if( str == "TEMP" ) return 1;
    // The newsreader.  Needs special perms for news directories.
    if( str == "News" && getuid( ob ) == BACKBONE_UID ) return 1;
 // Prevent untraceable objects form doing seteuid, eg. /ftp/
// and /open. Leto 11/11/95
//  if( getuid(ob) == "Anonymous" ) return 0;

    // An object is lucky enough to be itself...
    if( getuid( ob ) == str ) return 1;
    // creator_file() is a simul_efun that determines who is 'responsible'
    // for an object.  It's another way of saying getuid(ob) in general, but
    // there are sometimes differences.
    if( creator_file( file_name( ob ) ) == str ) return 1;
    return 0; // Default case
}

// The master object is asked if it is ok to shadow object ob. Use
// previous_object() to find out who is asking.
//
// In this example, we allow shadowing as long as the victim object
// hasn't denied it with a query_prevent_shadow() returning 1.
nomask int valid_shadow( object ob )
{
    // dangerous to allow people to shadow things with global access
    if( creator_file( file_name( ob ) ) == ROOT_UID ) return 0;
    // best not to let them shadow the simul_efun ob either ;-)
    if( file_name( ob ) == SIMUL_EFUN_OB ) return 0;
 // Let's protect admins too. Sigh   Leto
  if(adminp(ob)) return 0;
    // the generic properties can be overloaded... but not shadowed.
    if( function_exists( "query", previous_object() ) ) return 0;
    // this function returns the "link" to the secure information.
    if( function_exists( "query_link", previous_object() ) ) return 0;
    // this gives an object the chance to stop the shadow also
    return !ob-> query_prevent_shadow( previous_object() );
}

// Function name:       epilog()
// Description:         Loads master data, including list of all domains and
//                      wizards. Then make a list of preload stuff
// Arguments:           load_empty: If true, epilog() does no preloading
// Return:              List of files to preload
string *epilog( int load_empty )
{
    string *items;
    
    items = update_file( CONFIG_DIR + "preload" );
    call_out( "socket_preload", 5 );
    return items;
}

// For very odd reasons, this can't be done at the normal preload time.
// while nobody has explained it to me yet, at least know that this works.
void socket_preload()
{
    string *items;
    int i;
    
    items = update_file( CONFIG_DIR + "socket_preload" );
    
    for( i = 0; i < sizeof( items ); i++ )
        if( items[i] && items[i] != "" )
            call_other( items[i], "???" );
}

// preload an object
void preload( string file )
{
    int t1;
    string err;
    
//  if (file_name(previous_object()) != SIMUL_EFUN_OB) return;
    if (origin() != ORIGIN_DRIVER)  return;
    write( "Preloading : " + file + "..." );
    err = catch( call_other( file, "??" ) );
    if( err != 0 )
    {
        write( "\nGot error " + err + " when loading " + file + "\n" );
    } else {
        write( "\n" );  // there should be some statistic given here...
    }
}

// Get the owner of a file.  Used by log_error().
// This one appears to need some explicit paths....  I'll see how this
// works. Note that the positioning of the home directory can't change
// yet... HOME_DIRS might be something like "/u", if my home directory
// is /u/b/buddha... This should work in most cases.
string get_wiz_name( string file )
{
    string name, rest, dir;
    
    if( sscanf( file, HOME_DIRS + "%s/%s/%s", dir, name, rest ) == 3 )
    {
        return name;
    }
    return 0;
}

// Write an error message into a log file. The error occured in the object
// <file>, giving the error message <message>.
void log_error( string file, string message ) {
    string name, home, dom;
    
    if (file[0] != '/')
        file = "/" + file;

    if (origin() == ORIGIN_DRIVER &&
         (file_name(previous_object()) == CMD_LOAD ||
          file_name(previous_object()) == CMD_CLONE ||
          file_name(previous_object()) == CMD_UPDATE))
        name = geteuid(previous_object(1));
    else
        name = get_wiz_name(file);

    // We can't have simul_efun calls, since this is also called
    // when updating the simul_efun
//  if (name)  home = user_path(name);
    if (name)
        home = sprintf("/u/%c/%s/", name[0], name);
    else if (sscanf(file, "/d/%s/%*s", dom))
        home = sprintf("/d/%s/", dom);
    else
        home = LOG_DIR;

    if (file_size(home) == -1)
        home = LOG_DIR;

    if (file_size(home + "log") == -2) {
        write_file(LOG_DIR "master_errors",
              sprintf("log_error: %slog is a directory\n", home));
        home = LOG_DIR;
    }
    write_file(home + "log", message);
}

// save_ed_setup and restore_ed_setup are called by the ed to maintain
// individual options settings. These functions are located in the master
// object so that the local gods can decide what strategy they want to use.
// This used to be saved as .edrc in someone's homedir, but has been
// changed to use a property instead.
int save_ed_setup( object who, int code )
{
    if( !intp( code ) )
        return 0;
      return (int) this_player()-> set("ed_settings", code);
}

// Retrive the ed setup. Used to be a file, it's a property now.
int retrieve_ed_setup( object who )
{
        return (int)this_player()-> query( "ed_settings" );
}

// If the user gets disconnected while in ed, save what they were editing
// in this file...
string get_ed_buffer_save_file_name( string file )
{
    string *path;
    
   if (!file) return file;
    path = explode( file, "/" );
    file = user_path( geteuid( this_player() ) );
   if (sizeof(path) < 1) return 0;
  if( file_size(file) == -2) 
        file += path[sizeof( path ) - 1] + "~";
    else
        file = "/tmp/" + geteuid( this_player() ) + ":" +
          path[sizeof( path ) - 1] + "~";
    return file;
}


// When an object is destructed, this function is called with every
// item in that room. We get the chance to save users from being destructed.
void destruct_env_of( object ob)
{
    if( !interactive( ob ) )
    {
        ob-> remove();
        if( ob ) destruct( ob );
        return;
    }
    tell_object( ob,
      "Everything you see dissolves. " +
      "Luckily, you are transported somewhere...\n" +
    "" );
    ob-> move( VOID );
}

// This is called by the game driver to resolve path names in ed.
string make_path_absolute( string file, object ob, int writeflg )
{
    string path;

    path = resolv_path( "cwd", file );
#ifdef ED_LOG
	if(ob)
    log_file(ED_LOG, wrap(capitalize(geteuid(ob)) + " : \"" + path +
	"\" [" + extract(ctime(time()), 4, 15) + "]"));
#endif
#ifdef FILE_LOG
    if (writeflg)
	FLOCK_D->log_change(path, "?");
#endif
    return path;
}

// This is called by the mail object.  Other things might like it too.
int player_exists( string who )
{
    if( file_size( user_data_file(
      find_object_or_load( CONNECTION ), who )+__SAVE_EXTENSION__ ) > 0 )
        return 1;
    return 0;
}

string get_root_uid() { return ROOT_UID;  }
string get_bb_uid() { return BACKBONE_UID;  }

string creator_file( string str )
{
    string *path;
    
    if( str[0] != '/' ) str = "/" + str;

    path = explode( str, "/" ) - ({ "" });
    if( !path ) return 0;
    if( path[0] == 0 )
    {
        log_file( "creator_file",
          "no first element of array, str = " + str + "\n" );
        path = path[1..<1];
    }
    // Here's where all the permissions are sorted into uid's.
    // This is very important.
    switch( path[0] )
    {
        case "adm":
            if( str == SIMUL_EFUN_OB ) return "NONAME";
            else return ROOT_UID;
            break;
        case "cmds": return ROOT_UID; break;
        case "www":
	    if( ( sizeof( path ) > 1 ) && path[1] == "gateways" )
                return "Anonymous";
	    else
		return "Anonymous";
            break;
        case "std":
        // later, put a dir of bodies that is all Backbone... for now:
        // if( base_name( str ) == USER_OB ) return BACKBONE_UID;
            return "NONAME"; break;
        case "obj": return BACKBONE_UID; break;
        case "u":
            if( (path[2]!=ROOT_UID)&&(path[2]!=BACKBONE_UID)) {
            if (sizeof(path) >= 4 && path[2] && path[3])
                return path[2]; 
            break; }
        case "student":
            if( (path[1]!=ROOT_UID)&&(path[2]!=BACKBONE_UID)) {
            if (sizeof(path) >= 3 && path[1] && path[2])
                return path[1]; 
            break; }
        case "d":
            if( (path[1]!=ROOT_UID)&&(path[2]!=BACKBONE_UID)) {
            if (sizeof(path) >= 3 && path[2])
//          if (sizeof(path[1]) >=2 && path[1])
                return capitalize(path[1]);
            break; }
        case "open": return "Anonymous"; break;
        default:
    return "Anonymous";
            break;
    }
    return 0;
}

// these are the defaults for author and domain scoring
string author_file( string filename )
{
    string *path;
    
    path = explode( filename, "/" );
    if( !path ) return "NONAME";
    if( path[0] == "u" ) return path[2];
    return 0;
}

string domain_file( string filename )
{
    string *path;
    
    path = explode( filename, "/" );
    if( !path ) return "NONAME";
    
    switch( path[0] )
    {
        case "adm": return ROOT_UID; break;
        case "cmds": return ROOT_UID; break;
        case "std": return "NONAME"; break;
        case "obj": return BACKBONE_UID; break;
        case "student":
        case "u": return "User"; break;
        case "open": return "Anonymous"; break;
	case "d":
	    if (sizeof(path) > 1 && path[1] != "*")
		return capitalize(path[1]);
	/* otherwise, fall through */
        default: return "NONAME"; break;
    }
}

// Check with the telnet daemon to see if the socket attempt
// is permitted or not.
int valid_socket( object calling_ob, string func, mixed *info )
{
    int ret = 0;
#ifdef INTERMUD
    ret = (int)TELNET_D-> telnet_permission( calling_ob, func, info );
#endif /* INTERMUD */
    return ret;
}

// this ought to check things against domains.h
int valid_domain( string dom )
{
    if( member_array( dom, DOMAIN_DIRS ) !=  - 1 )
        return 1;
    return 0;
}

int valid_override( string bfile, string name, string file )
{
    if (file[0] != '/') file = "/" + file;
   sscanf(file, "%s.c", file);
    if ( (file == SIMUL_EFUN_OB) || (file == "/adm/simul_efun/overrides") ) return 1;
// Changed by Blue, 94-01-14, after Artagel informed me of a bug
// fix in the latest driver (0.9.18.31) which will pass the
// simul_efun filename as an argument here instead of the file
// it includes.  With the ||, it should work with old drivers
// too.
    if( function_exists( name, find_object( SIMUL_EFUN_OB ) ) ) return 0;
    return 1;
}

int valid_hide( object who )
{
    string eff_user;
    eff_user = geteuid( who );
  if (adminp(eff_user) || eff_user == ROOT_UID)
        return 1;
    return 0;
}

// Called by the link() efun when trying to make the file path <reference>
// synonymous with the original file at filepath <original>
int valid_link( string original, string reference )
{
    return 0; // Link not allowed
}

// Used by /std/save.c (added by Tru)
void make_data_dir()
{
    string *parts, dir;
    string path;
    int j;
    
    path = data_dir( previous_object() );
    parts = explode( path, "/" );
    dir = "";
    for( j = 0; j < sizeof( parts ); j++ )
    {
        dir += parts[j];
        mkdir( dir );
        dir += "/";
    }
}


int different(string fn, string pr) {
    int tmp;

    sscanf(fn, "%s#%d", fn, tmp);
    fn += ".c";
    return (fn != pr) && (fn != ("/" + pr));
}

string trace_line(object obj, string prog, string file, int line) {
    string ret;
    string objfn = obj ? file_name(obj) : "<none>";

    ret = objfn;
    if (different(objfn, prog))
        ret += sprintf(" (%s)", prog);

    if (file != prog)
        ret += sprintf(" at %s:%d\n", file, line);
    else
        ret += sprintf(" at line %d\n", line);

    return ret;
}

varargs string standard_trace(mapping mp, int flag) {
    string obj, ret;
    mapping *trace;
    int i, n;

    ret = mp["error"] + "Object: " +
          trace_line(
                mp["object"], mp["program"], mp["file"], mp["line"]
          ) + "\n";

    trace = mp["trace"];

    for (i = 0, n = sizeof(trace); i < n; i++) {
        if (flag)
            ret += sprintf("#%d: ", i);

        ret += sprintf("'%s' at %s",
              trace[i]["function"],
              trace_line(trace[i]["object"],
                    trace[i]["program"],
                    trace[i]["file"],
                    trace[i]["line"])
              );
    }
    return ret;
}

string error_handler(mapping mp, int caught) {
    string ret;

    // try to distinguish between different types of errors
    ret = "---\n" + standard_trace(mp);
    write_file((caught ? "/log/catch" : "/log/runtime"), ret);

    // let the player know an error occurred
    if (this_player(1)) {
        message("error",
              sprintf("%sTrace written to %s\n",
              mp["error"], (caught ? "/log/catch" : "/log/runtime")),
              this_player(1));
        this_player(1)->set_error(mp);
    } else if (this_player()) {
        message("error",
              sprintf("%sTrace written to %s\n",
              mp["error"], (caught ? "/log/catch" : "/log/runtime")),
              this_player());
        this_player()->set_error(mp);
    }

    // whatever we return goes to the debug.log (in this case, nothing)
    return 0;
}

int valid_object(object ob) {

	string foo;
	int dummy;
	if (!sscanf(file_name(ob), "%s#%*d", foo)) foo = file_name(ob);
        
        // No cloning from /tmp. That way we can always get an uid
	if (foo[0..4] == "/tmp/")  return 0;
        // No cloning invisible files, nasty to look up irritating 'hidden'
        // objects. so "~/foo.c" is ok and "~/.foo.c" is not
	if (strsrch(foo,"/.") != -1 )  return 0;
        return 1;
}

int valid_bind(object binder, object old_owner, object new_ownwe) {

return 0; // For now, lets disable it, since Tmi-2 doesn't use it yet
}
/* Th'Th'Th'That's All Folks! */
