/*
// tsh.c: TMI-shell or Tru-shell -- written by Truilkan@TMI 92/02/05 
//        meant to be inherited by player.c
//
// Brutally hacked up and destroyed by Buddha to install something "better"
// on 3-7-92
//
// 1992/03/08 - hacked again by Truilkan@TMI to fix pushd and popd
//
// 1993/02/06 - Multiple command repeat capability by Watcher@TMI
//
// 1993/02/22 - Allowing silent queries to write_prompt() by Watcher@TMI
//
// 1993/03/30 -	Uses STACK not CSTACK (makes `dirs' cmd easier)
//		Uses tilde_path in prompt.
//		Cleaned up a bit. (Zak@tmi-2)
// 1993/08/05 Grendel@tmi-2 - Added test for files in ~/tsh/
// 94-11-09   - Leto added an int typecast
*/

#include <adt_defs.h>
#include <commands.h>
#include <tsh.h>
#include <uid.h>

#define DEFAULT_PROMPT "> "
#define MAX_HIST_SIZE  50
#define MIN_HIST_SIZE  20
#define MIN_PUSHD_SIZE 5
#define MAX_PUSHD_SIZE 50

/*
    STACK_ADT should be inherited from here so that this object can have
    its own private stack
*/
private inherit STACK_ADT;       /* for pushd and popd */

private nosave string tsh_prompt;
private nosave int cur, hist_size, pushd_size, custom_prompt;

string do_nicknames(string arg);
string do_alias(string arg);
string do_substitution(string arg) ;
string handle_history(string arg);
nomask mixed query( string label );

/* do_new: called by the "new" command */

int do_new()
{
    string d1, d2;

    tsh_prompt = (string)this_object()->getenv("prompt");
    tsh_prompt = !tsh_prompt ? DEFAULT_PROMPT : tsh_prompt + " ";
    custom_prompt = (tsh_prompt != DEFAULT_PROMPT);

    d1 = (string)this_object()->getenv("pushd");
    pushd_size = 0;
    if (d1)
	sscanf(d1,"%d",pushd_size);
    if (pushd_size > MAX_PUSHD_SIZE)
	pushd_size = MAX_PUSHD_SIZE;
    if (!pushd_size)
	pushd_size = MIN_PUSHD_SIZE;

    d1 = (string)this_object()->getenv("history");
    hist_size = 0;
    if (stringp(d1))
	sscanf(d1,"%d",hist_size);
    else if (intp(d1))
        hist_size = (int)d1; // Leto 
    if (hist_size > MAX_HIST_SIZE)
	hist_size = MAX_HIST_SIZE;
    if (!intp(hist_size) || hist_size <= 0)
	hist_size = MIN_HIST_SIZE;
    return 1;
} // do_new


/* push current directory onto the stack and cd to dir named "arg" */
int pushd(string arg)
{
    string path;

    if (!arg && (string)this_object()->getenv("pushdtohome"))
	arg = "~";
    path = (string)this_object()->query("cwd");
    if (((int)CMD_CD->cmd_cd(arg)))
    {
	if (push(path) == -1)
	    write("dirstack full - try popd-ing some elements first\n");
    }
    return 1;
} // pushd


int popd()
{
    mixed dir;

    dir = pop();
    if ((int)dir == -1)
	write("Directory stack is empty.\n");
    else
	CMD_CD->cmd_cd((string)dir);
    return 1;
} // popd


mixed *dirs()
{
    return query_stack();
}


void initialize_tsh()
{
    string rcfile;

    do_new();
    if (pushd_size)
	alloc(pushd_size);
// According to Beek, the next 2 lines shoulf go, but that
// kills history completely..... Leto 942711
    if (hist_size)
	history::hist_alloc(hist_size);
    rcfile = user_path((string)this_object()->query("name")) + ".login";
    if( file_size(rcfile) > 0 )
	this_object()->tsh(rcfile);
} // initialize_tsh


string write_prompt(int silent)
{
    string path, prompt, tmp;

    if (custom_prompt)
    {
	prompt = tsh_prompt;
	prompt = replace_string(prompt,"$D",
			tilde_path((string)this_object()->query("cwd"), 0));
	prompt = replace_string(prompt,"\\n","\n");
	prompt = replace_string(prompt,"$N",lower_case(mud_name()));
	prompt = replace_string(prompt,"$T",ctime(time())) ;
	prompt = replace_string(prompt,"$C",""+(query_cmd_num() + 1));
    }
    else
	prompt = DEFAULT_PROMPT;
    if(!silent)
        message("prompt", prompt, this_player());
    return prompt;
} // write_prompt


protected nomask string process_input(string arg)
{
    int loop, num, macro;

    if (arg && arg != "")
    {
	if(sscanf(arg, "%d %s", num, arg) == 2)
	    macro = 1;

	arg = do_substitution(arg) ;
	arg = do_nicknames(arg);
	arg = do_alias(arg);
	arg = handle_history(arg);

    //	Allow multiple repeat actions like '5 north' as well as
    //	alias commands.   - Watcher  (02/05/93)

	if(macro && num && arg && arg != "" && !previous_object())
	{
		if(num > 10) {
		write("Command macro increment too high.\n");
		return ""; }
 
	    for(loop=0; this_player() && loop < num; loop++)
		if(!command(arg))
		{
		    write("Could not complete action. Command macro halted.\n");
		    break;
		}
	    return "";
	}
    }
   if( strlen( arg ) > 5 && arg[0..5] == "passwd" ) {
      CMD_PASSWD -> cmd_passwd( (strlen( arg ) > 6 ? arg[7..<1] : 0) );
      return "";
   }
    return arg;
} // process_input


nomask int tsh(string file)
{
    string contents, euid, *lines;
    int j, len;

    euid = geteuid( previous_object() );
	// 93-02-12 pallando (_tsh wasn't working)
    if (  ( euid != geteuid( this_object() ) )
       && ( euid != ROOT_UID )
       && !adminp(euid) )
	return 0;
    if (!file)
    {
	notify_fail("usage: tsh filename\n");
	return 0;
    }
    contents = read_file( resolv_path( "cwd", file ) );
    if (!contents) {
	contents = read_file( resolv_path( "cwd", "~/tsh/"+file ) );
	if (!contents)
	{
	    notify_fail("tsh: couldn't read " + file + "\n");
	    return 0;
	}
    }
    lines = explode(contents,"\n");
    len = sizeof(lines);
    for (j = 0; this_player() && j < len; j++)
    {
	if (lines[j][0] != '#' && !command(lines[j]))
	{
	    write(file + ": terminated abnormally on line #" + (j+1) + "\n");
	    write("while doing: " + lines[j] + "\n");
	    break;
	}
    }
    return 1;
} // tsh

// This function allows for ^foo^bar substitutions.

nomask string do_substitution (string arg) {

	string pre,post,old,New,lastcom ;

        if (arg == "^^") {
		write ((lastcom = query_last_command()) + "\n");
		return lastcom;
	}

	if (sscanf(arg,"^%s^%s",old,New)!=2) {
		return arg ;
	}

	lastcom = query_last_command() ;
	if (!lastcom || lastcom=="") {
		write ("No previous command.\n") ;
		return "" ;
	}

        if (old == "") {
		// allow you to prepend last string
		write (New + lastcom + "\n");
		return New + lastcom ;
	}

	if (sscanf(lastcom, "%s" + old + "%s", pre,post) != 2) {
		write ("Failed substitution.\n") ;
		return "" ;
	}
	write (pre+New+post+"\n") ;
	return pre+New+post ;
}
