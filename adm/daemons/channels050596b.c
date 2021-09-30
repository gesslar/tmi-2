//      File:                   channels.c  (A daemon)
//      Author:                 Inspiral@Tabor, et al
//      Created:                93-05-10
//      Purpose:                To efficiently handle player communication.
//
// REVISON HISTORY:
// 930725  - Fixed problem with creating spurious channels!    -- Inspiral
// 930801  - Added scan_config()                               -- Inspiral
// 930803  - Reformatted data structure to use
//           more arrays than mappings                         -- Inspiral
// 930811  - Removed all reliances on query() and set() lfuns. -- Inspiral
// 930818  - Added log(), tell_user(), secure()                -- Inspiral
// 930826  - fixed bug in del user                             -- Pallando
// 931004  - added emotes                                      -- Rust
// 940322  - Rewritten to be faster, and to index users by
//           object                                            -- Inspiral
// 940911  - Added missing prototpye                           -- Leto
// 950312  - cleaned up format & changed channels variable set
//           in the player to a mapping type instead of an
//           array since Earthmud uses the mapping version     -- Zifi@earthmud
// 951001  - Added parse_amcp_channel                          -- Leto@Earthmud
// 960325  - Added backlog for each channel                    -- Perrin@Earthmud
// 960429  - Fixed channel emotes and formatting problems      -- Emeradii@Earthmud

#include <channels.h>
#include <commands.h>
#include <emoted.h>
#include <daemons.h>

varargs mixed display_channel(string chan, object viewer);
int register_channel(string chan, object user, int state);
int log(string log_mesg);
void add_chan_backlog(string chan,string mesg);

mapping channels;  // contains all our channel stats
mapping alias;     // a rude alias mechanism

#define BACKLOG_MAX 15

int
tell_user(object user, string mesg)
{
    tell_object(user, wrap(sprintf("CHANNELS:\t%s\n", mesg)));
    return 1;
}

#define NO_NEW_CHANNELS

static varargs int
create_channel(string chan)
{
    if (channels[chan])
        return 1;

    channels[chan] = ([ ]);
    channels[chan]["vis"] = ({  });
    channels[chan]["backlog"] = ({ });

    return 1;
}


static int
kill_user(object user, string chan)
{
    if (undefinedp(channels[chan]))
        return 0;

    channels[chan]["vis"] -= ({user});

    return 1;
}


static varargs int
delete_channel(string chan)
{
    if (undefinedp(channels[chan]))
        return 0;

    map_delete(channels, chan);

    return 1;
}


static varargs int
add_user(string chan, object user)
{
    string *toggle_list,priv;

    if (!chan || !user)
        return 0;

    if (!pointerp(toggle_list = (string *) user->query("channels/toggle")))
    {
	user->set("channels/toggle",({  }));
	toggle_list = ({  });
    }

    // chan doesn't exist...do we have permission to create it?
    if (undefinedp(channels[chan]))
    {
#ifdef NO_NEW_CHANNELS
	return 0;
#else
	create_channel(chan, user);
#endif
    }

    if (!undefinedp(priv = channels[chan]["priv"]) && !member_group(geteuid(user), priv))
        return 0;

    if (member_array(chan,toggle_list) == -1)
	user->add("channels/toggle",({chan}));

    channels[chan]["vis"] = uniq_array(channels[chan]["vis"] + ({user}));

    return 1;
}


static int
kill_users(mixed chan)
{
    int i;

    if (!chan) chan = keys(channels);
    else chan = ({chan});

    i = sizeof(chan);
    while(i--)
	channels[chan[i]]["vis"] -= ({ 0 });
    
    return 1;
}


varargs int
initialize_user(object whom)
{
    int i, j;
    object *list;
    string priv;
    string *current_list;

    if (!whom || !living(whom)) list = users();
    else list = ({whom});  // We have one user.

    i = sizeof(list);
    while(i--)
    {
	current_list = (string *) list[i]->query("channels/toggle");

	if (!pointerp(current_list))
	{
	    list[i]->set("channels/toggle",({  }));
	    current_list = ({  });
	}

	j = sizeof(current_list);
	while(j--)
	    register_channel(current_list[j], list[i], ADD);
    }

    return 1;
}


varargs int
parse_channel(string chan, string mesg, string name, int emote)
{
    int i;
    string *ignore;
    object *list;        // Holds the list of all players currently on chan
    string ext_ob, err;
    string plural;       // Holds the plural form of the channel name, chan
    string verb, cmd;

    if (alias[chan]) chan = alias[chan];

    if (undefinedp(channels[chan]))
        return 0;

    kill_users(chan);

    if (undefinedp((list = channels[chan]["vis"])))
        return 0;

    i = sizeof(list);  // Optimize is.

    // Channel format is wrong in the daemon..Attempt a fix, but log.
    if (!pointerp(list))
    {
	channels[chan]["vis"] = ({  });
	initialize_user();  // Shall we reconfigure?
	return log(sprintf("parse_channel: channel %s is not a pointer!  Fixing.", identify(chan)));
    }

    // Is the name flag not given? or, is name not even in the list?
    if (this_player() && member_array(this_player(),list) == -1) 
        return 0;

    if (this_player() && !name)
	name = (string) this_player()->query("cap_name");

    if (!mesg || mesg == "")
    {
	CMD_SHOW->cmd_show(chan);
	return 1;
    }

    // Output backlog stuff here
    if (mesg[0..4]=="/last")
    {
        if (!sizeof(channels[chan]["backlog"]))
    	    message("channels","No backlog yet,sorry.\n",this_player());
        else
    	    message("channels",sprintf("Backlog of channel %s:\n%s",chan,
    	            implode(channels[chan]["backlog"],"")+"\n"),this_player());
        return 1;
    }

    if (undefinedp((plural = channels[chan]["plural"])))
	plural = pluralize_verb(chan);

    if (mesg[0] == ';')
    {
        if (!sscanf(mesg[1..],"%s %s",verb,cmd))
	    verb = mesg[1..];

	mesg = EMOTE_D->return_parse(verb,cmd);

	if (!mesg)
        {
            if (!cmd) cmd = "";
            mesg = name + " " + verb + " " + cmd;
        }

	emote = 1;
    }
    else if (mesg[0..5] == ":emote")
    {
    	if (!sscanf(mesg[7..],"%s %s",verb,cmd))
	    verb = mesg[7..];
	
	mesg = EMOTE_D->return_parse(verb,cmd);

	if (!mesg)
        {
            if (!cmd) cmd = "";
            mesg = name + " " + verb + " " + cmd;
        }

	emote = 1;
    }
    else if (emote)
    {
        mesg = name + " " + mesg;
    }

    while(i--)
    {
        if (!objectp(list[i]))
	    continue;

	if (list[i] == this_player())
	{
	    if (emote)
	    {
		message("channels",iwrap(capitalize(chan)+": "+mesg),list[i]);
		add_chan_backlog(chan,iwrap(capitalize(chan)+": "+mesg));
	    }
	    else
	    {
		message("channels",iwrap("You "+chan+": "+mesg),list[i]);
		add_chan_backlog(chan,iwrap(name+" "+plural+": "+mesg));
	    }
	}
	else
	{
	    ignore = (string *) list[i]->query("ignore");

	    if (!pointerp(ignore))
	        ignore = ({ });

	    if (member_array(lower_case(name), ignore) != -1)
	        continue;

	    if (list[i]->getenv("add_newline"))
		tell_object(list[i], "\n");

	    if (emote) message("channels",iwrap(capitalize(chan)+": "+mesg),list[i]);
	    else message("channels",iwrap(name+" "+plural+": "+mesg),list[i]);

	    continue;
	}

    } // while()

    // Do we have an apply to an external object?
    chan = lower_case(chan);
    if (!undefinedp((ext_ob = channels[chan]["object"])))
    {
	// format of object flag:  alias#object
	sscanf(ext_ob,"%s#%s", chan, ext_ob);

	// Attempt the apply, log all errors
	if (err = catch(ext_ob->daemon_apply(this_player(), chan, mesg, emote)))
	    log(sprintf("parse_channel:\tError in %s:\n%s", ext_ob, err));
    }
    return 1;
}


varargs mixed display_channel(string chan, object viewer) {
    mixed list;

    if(!chan || chan == "") list = keys(channels);
    else {
	if(undefinedp(list = channels[chan]))
	    return sprintf("There is no such channel:  %s.",identify(chan));
	list = channels[chan]["vis"];
    }
    if(!pointerp(list) || !sizeof(list)) {
	channels[chan]["vis"] = ({  });
	return sprintf("CHANNELS:\tNo Users Subscribed to %s", identify(chan));
    }
    if(chan && chan != "")
	list = filter_array(list, "flush_list", this_object(), viewer);
    list = map_array(list, "get_cap", this_object());
    list = (implode(list,",")); // Leto.. +"   <End>");
    return list;
}


static int scan_config() {
    string *list, *split_line, line;
    int i, j;

    list = update_file(CHANNELS_CONFIG);
    i = sizeof(list);

    // Once through the i for each non-"#" line in config file
    while(i--) {
	line = list[i];
	if(line) {
	    split_line = explode(line, ":");
	    j = sizeof(split_line);
	    channels[split_line[0]] = allocate_mapping(6);
	    channels[split_line[0]]["vis"] = ({  });
	    while(j--) {
		if((split_line[j] != "X") && (CHANNELS_INFO[j] != ""))
		    channels[split_line[0]][CHANNELS_INFO[j]] = split_line[j];
	    } // while(j)
	}  // IF (LINE)
    } // while(i)
    return 1;
}


int log(string log_mesg) {
    if(!log_mesg || log_mesg == "") return 0;
    log_file(CHANNELS_LOG, sprintf("%s %O - %s\n", intl_date_stamp(1),
	previous_object(), log_mesg));
    return 0;
}


mapping q() {
    return copy(channels);
}


mapping a() {
    return copy(alias);
}


void
process_alias() {
    int i;
    string *list, temp1;

    list = keys(channels);
    i = sizeof(list);
    while(i--) {
	if(!undefinedp(temp1 = channels[list[i]]["object"]))
	    if(sscanf(temp1, "%s#%*s", temp1) == 2)
		alias += ([temp1 : list[i]]);
    }
}


string get_cap(object ob) {
    return (string) ob->query("cap_name");
}


int flush_list(object user, object viewer) {
    return (objectp(user) && visible(user, viewer) && interactive(user));
}


void create() {
    channels = allocate_mapping(20);
    alias = allocate_mapping(20);
    seteuid(getuid());
    scan_config();
    process_alias();
    call_out("initialize_user", 2);
}


varargs int
register_channel(string chan, object user, int state)
{
    int ret;

    if (nullp(state))
	return notify_fail("register_channel:  no state given!");

    switch(state)
    {
        case DEL: ret = delete_channel(chan, user);
	          break;
        case CRE: ret = create_channel(chan, user, state);
	          break;
        case ADD: ret = add_user(chan, user);
	          break;
        case KIL: ret = kill_user(user, chan);
	          break;
        default : ret = 0;
	          break;
    }

    return ret;
}


// If all parsing of <foo>bar fails, we arrive at this function to see
// if the request might be a valid message class. We return 0 if it
// wasn't, and 1 if it was. We're assuming all strings are valid.

int parse_amcp_channel(string command, string mesg) {

    string *channels, control, channelname;

    channels = previous_object()->query("channels/toggle");
    command = lower_case(command);

    if(member_array(command,channels)!=-1)
    {
	if (!mesg) this_player()->receive_message(command,display_channel(
		command,this_player() ));
	return parse_channel(command,mesg); // channel message sent.
    }
    // is it a control message then ?
    if(sscanf(command,"channel&%s",control)!=1) return 0; // We don't get it.
    //write("CHAN:"+control);
    if(control=="list")
    {
	this_player()->dump_amcp_channels();
	return 1;
    }
    if(sscanf(control,"%s&%s",control,channelname)!=2)
    {
	this_player()->receive_message("error","Unknown channel control message.\n");
	return 1;
    }
    if(control=="subscribe") return CMD_TUNE->cmd_tune(channelname+" in");
    if(control=="unsubscribe") return CMD_TUNE->cmd_tune(channelname+" out");
    // Guess it's an unknown control message.
    this_player()->receive_message("error","Unknown channel control message.\n");
    return 1;

}

void add_chan_backlog(string chan,string mesg)
{
    string *backlog = channels[chan]["backlog"];
    int count = sizeof(channels[chan]["backlog"]);

    // if(undefinedp(channels[chan]["backlog"]))
    if(!count)
	channels[chan]["backlog"] = ({});

    if(count >= BACKLOG_MAX) backlog = backlog[1..];
    if (!backlog)
	backlog = ({ mesg });
    else
	backlog = backlog + ({ mesg });

    channels[chan]["backlog"] = backlog;
}
