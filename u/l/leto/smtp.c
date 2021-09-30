// This is an smtp client for handling incoming mail. If put on port 25,
// it should work with real internet hosts too. If you use it as a 'real'
// internet mailer, make sure you have a valid hostname and domainname,
// and make sure your Nameserver's MX record for your mudsite is not your
// regular mail.your.domain, or else you'll never receive email :)
// To make it work for Intermud3, add an OOB service called smtp with the
// correct portnumber. Remember ports < 1024 require root access, or other
// tricks.
// It is based on the responses I got from smail (A replacement for sendmail)
// since i didn't have any RFC's at hand :)
//
// Made by Leto@Earth Feb 2, 1996 (paul@via.nl)
// Very heavily based on Truilkan's smtp daemon. It uses some Tmi-2 simuls!
//
// Todo: Time outs on sockets to clean up mess. Should response with:
// 421 mudname.mudos.org SMTP command timeout, closing channel
//
// Original credits follow.
// rcsid = "$Header: /weaver2/archive/mudlib/single/net/smtp.c,v 1.12 1995/05/20 12:20:36 garnett Exp $"

// 1995/04/28
//
// An SMTP (World Wide Web) server written in LPC for the Interstice mudlib
// and MudOS v21 driver (using socket efuns).
//
// - this code is copyright 1995 by John Garnett (Truilkan)
//   and Dwayne Fontenot (Jacques).

// - A few bits of code (two or three lines) from the old TMI-2 smtp.c
//   by George Reese (Descartes) may still be present in this code.
//
// Permission is granted to use this code for any legal purpose but only if 
// these credits are kept with the code.  No warranty is expressed or implied.

// This SMTP server implements some part of SMTP/1.0.  Its not really
// complete but its more complete than the old TMI-2 SMTP server.

// Improvements over the old TMI-2 http.c:
//
// 1) outputs logging information in the NCSA smtpd format so all of
//    standard smtpd log processing tools should work.
// 2) pays attention to the return code of the socket_write() efun
//    and attempts to do the right thing.
// 3) is able to process large WWW client queries such as those generated by
//    MacWeb (which cause the read_callback to be called multiple times
//    for one query).
// 4) handles hostname resolution more efficiently
// 5) handles the POST method (in addition to GET).

// Change no_one to your name to get debug information to your
// character.
#define DEBUGGER "leto"

#define OB_RESOLVER "/adm/daemons/network/resolver.c"

#include <mudlib.h>
#include <config.h>
#include <mailer.h>
#include <net/config.h>
#include <net/daemons.h>
#include <daemons.h>
#include <driver/localtime.h>  // this is from the driver include dir
#include <net/smtp.h>
#include <net/socket.h> 

// OB_SAVE provides this object with persistency (for saving 'accesses').
// For this to be effective, a shutdown daemon needs to call remove()
// on this object as the MUD is shutting down (this will cause save_object()
// to be called).
// Check your muds define for this (SAVE or SAVE_OBJ, or OB_SAVE ?)
inherit "/std/save.c";

// Number of times to retry in the event that socket_write() returns
// EESEND or EEWOULDBLOCK.

#define MAXIMUM_RETRIES    3

#define log_info(x, y) log_file(x, ctime(time()) + "\n"); log_file(x, y)

static private int smtpSock;
static private mapping sockets;
static private mapping mode; // To see if a client is in command or data mode.
static private mapping mail; // Keep mail messages being send to us here, when
                             // they're done, tehy can go to the mailer daemon.
static private mapping resolve_pending;
static string *months;

static void setup();
void close_connection(int fd);
void write_data(int fd, mixed data);
string smtp_help();
void smtp2mudmail(int fd);
int accesses;

int query_accesses()
{
	return accesses;
}
// todo make this a simul_efun
string
common_date(int t)
{
	mixed* r;

	r = localtime(t);
	return sprintf("%02d/%s/%d:%02d:%02d:%02d -%02d00",
		r[LT_MDAY], months[r[LT_MON]], r[LT_YEAR], r[LT_HOUR],
		r[LT_MIN], r[LT_SEC], r[LT_GMTOFF] / 3600);
}

static void create()
{ 
	accesses = 0;
	set_persistent(1);
	save::create();
	months = ({"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct",
		"Nov","Dec"});
	sockets = ([]);
	mode = ([]);
	mail = ([]);
	resolve_pending = ([]);
	log_info(LOG_SMTP_ERR, "Created when uptime = " + uptime() + "\n");
	call_out("setup", 5);
}

void
remove()
{
	save::remove();
	log_info(LOG_SMTP_ERR, "Destructed when uptime = " + uptime() + "\n");
	destruct(this_object());
}

int query_prevent_shadow(object ob)
{
	return 1;
}

varargs string query_hostname(int fd, int t)
{
	mapping entry;
	string name;

	entry = sockets[fd];
	if (!undefinedp(entry)) {
		if (t) {
			return entry["address"];
		} else {
			return entry["name"];
		}
	}
	return 0;
}

static void clean_up()
{
 remove();
 return;
} 

static void setup()
{
	if ((smtpSock =
		socket_create(STREAM, "read_callback", "close_callback")) < 0)
	{
		log_info(LOG_SMTP_ERR, "setup: Failed to create socket.\n");
		return;
	}
	if (socket_bind(smtpSock, PORT_SMTP) < 0) {
		socket_close(smtpSock);
		log_info(LOG_SMTP_ERR, "setup: Failed to bind socket to port.\n");
		return;
	}
	if (socket_listen(smtpSock, "listen_callback") < 0) {
		socket_close(smtpSock);
		log_info(LOG_SMTP_ERR, "setup: Failed to listen to socket.\n");
	}
}

static void write_data_retry(int fd, mixed data, int counter)
{
	int rc;

	if (counter == MAXIMUM_RETRIES) {
		close_connection(fd);
		return;
	}
	rc = socket_write(fd, data);
SMTP_DEBUG("write_data: rc = " + rc + ", counter = " + counter + "\n");
	sockets[fd]["write_status"] = rc;
	switch (rc) {
		case EESUCCESS:
			// we're finished with this fd.
			// Oops, Leto :) close_connection(fd);
			//close_connection(fd);
			break;
		case EEALREADY:
			// driver must have set the socket marked as BLOCKED when
			// it was created by socket_accept().  Just wait for
			// write_callback to be called so that we can write out the
			// pending data.
			sockets[fd]["pending"] = data;
			break;
		case EECALLBACK:
			// wait for write_callback before accessing socket fd again.
			break;
		case EEWOULDBLOCK:
		case EESEND:
			// try again in two seconds
			if (counter < MAXIMUM_RETRIES) {
				call_out("retry_write", 2, ({fd, data, counter + 1}));
				return;
			}
			// fall through to the default case and write an error.
		default:
			log_info(LOG_SMTP_ERR, "write_data_retry: " + socket_error(rc) + "\n");
			close_connection(fd);
			break;
	}
}

void retry_write(mixed* args)
{
	write_data_retry(args[0], args[1], args[2]);
}

static void write_data(int fd, mixed data)
{
	write_data_retry(fd, data, 0);
}

static void store_client_info(int fd)
{
	string addr;
	mixed result;
	int port;

	sscanf(socket_address(fd), "%s %d", addr, port);
	sockets[fd] = ([
	 "address" : addr,
	 "name" : addr,
	 "port" : port,
	 "time" : time(),
	 "write_status" : EESUCCESS
	]);
	result = OB_RESOLVER->query_cache(addr, "resolve_callback");
	if (intp(result)) {
		resolve_pending[result] = fd;
	} else {
		sockets[fd]["name"] = result;
	}
}

static void listen_callback(int fd)
{
	int nfd;

	if ((nfd = socket_accept(fd, "read_callback", "write_callback")) < 0) {
		log_info(LOG_SMTP_ERR, "listen_callback: socket_accept failed.\n");
		return;
	}
	smtp_send(nfd, "220 "+MUD_NAME+" ready at "+common_date(time())+"\n");
	store_client_info(nfd);
}

//
// The driver calls write_callback to indicate that the data sent
// by the last call to socket_write() is finally all written to the
// network (or to indicate that a socket created in the blocked state
// is now ready for writing).
//

void write_callback(int fd)
{
	// The status will be EEALREADY only in the event that the socket
	// was created in a blocked state (this object is smart enough not
	// to write to a socket it knows is blocked).
	//
	if (sockets[fd]["write_status"] == EEALREADY) {
		write_data(fd, sockets[fd]["pending"]);
		//
		// its safe to delete the pending data now since its already been sent
		// and since we won't ever have any more pending data for this
		// socket (we might have an EECALLBACK but the driver is
		// responsible for holding the pending data in that case).
		//
		map_delete(sockets[fd], "pending");
	} else {
		//
		// We can close the socket at this point since we only ever send one
		// thing on a given socket before we are through with it.

		// Leto: Not true for smtp, just for http :)
		sockets[fd]["write_status"] = EESUCCESS;
#if 0
		close_connection(fd);
#endif
	}
}

void
log_smtp(int fd, int rc, int nbytes, string cmd)
{
	string msg;
	string bsent;

	if (!sockets[fd]) {
		return;
	}
	bsent = (nbytes) ? sprintf("%d", nbytes) : "-";
	msg = sprintf("%s unknown - [%s] \"%s\" %d %s\n",
		sockets[fd]["name"], common_date(time()), cmd, rc, bsent);
	log_file(LOG_SMTP, msg);
}

// read_callback gets called when the SMTP client sends us a request.
//

static void read_callback(int fd, string str)
{
	string cmd, args, file, url;
	string *request;
	string tmp="", line0;

	if (str=="\r\n" ) {
		smtp_send(fd, "500 Command unrecognized\n");
		return;
	}
/*
	if (tmp = sockets[fd]["read"]) {
		str = tmp + str;
	}
	if (str[<1] != '\n') {
		sockets[fd]["read"] = str;
		return;
	} else {
		map_delete(sockets[fd], "read");
	}
*/
	accesses++;
	request = explode(replace_string(str, "\r", ""), "\n");
SMTP_DEBUG("SMTP:"+str+"\n");
	// Is it a new fd ?
	if(!mode[fd]) {
		mode[fd] = "COMMAND" ;
		mail[fd] = ([]) ;
		
	}
	// Are we in DATA mode? If so, fling it into the body of the mail
	// We rely on the DATA cmd to have initialized an empty string to hold
	// the email in the mail mapping.
	if(mode[fd]=="DATA") { 
SMTP_DEBUG("In data mode.\n");
SMTP_DEBUG("request[0]=="+request[0]+".\n");
SMTP_DEBUG("strlen="+strlen(request[0]));
		if(request[<1]==".")  {
SMTP_DEBUG("got a . line\n");
			mail[fd]["body"] += request[0..<2];
			smtp2mudmail(fd);
			mode[fd]="COMMAND";
			smtp_send(fd, "250 Mail accepted\n");
			return;
			}
		else {
			mail[fd]["body"] += request;
			return;
			}
	} else   // We're in command mode.	
	line0 = request[0];
	if (line0=="" || line0=="\n" ) {
		smtp_send(fd, "500 Command unrecognized");
		return;
	}
	if (sscanf(line0, "%s %s", cmd, args)!=2) {
		cmd = line0;
		args = "";
	}
SMTP_DEBUG("CMD:"+cmd);
	switch(lower_case(cmd)) {
		case "quit":
			call_out( "close_connection",1,fd); // Give some time
			smtp_send(fd, "221 "+MUD_NAME+".mudos.org closing connection\n");
			break;
		case "rset":
			mode[fd]="COMMAND";
			map_delete(mail, mail[fd] ) ;
			smtp_send(fd, "250 Reset state\n");
			break;
		case "noop":
			smtp_send(fd, "250 Okay\n");
			break;
		case "help": 
			smtp_send(fd, smtp_help());
			break;
		case "data":
			if(!mail[fd]["from"]) {
			smtp_send(fd, "503 Need MAIL command\n");
			break;
			}
			if( sizeof(mail[fd]["rcpt"])<1) {
			smtp_send(fd, "503 Need RCPT (recpient)\n");
			break;
			}
			mode[fd]="DATA"; mail[fd]["body"]="";
			smtp_send(fd, "354 Enter mail, end with \".\" on a line by itself\n");
			break;
		case "helo":
			mail[fd]["hostname"]=args;
			smtp_send(fd, "250 "+MUD_NAME+".mudos.org Hello "+args+"\n");
			break;
		case "mail":
			if(args[0..4]!="from:") {
			smtp_send(fd, "500 Command unrecognized\n");
			break;
			}
			if(mail[fd]["from"]) {
			smtp_send(fd, "503 Sender already specified\n");
			break;
			}
			while(args[0]==' ') args = args[1..];
			if(!args || args=="") {
			smtp_send(fd, "501 MAIL FROM requires address as operand\n");
			break;
			}
			mail[fd]["from"]=args;
			mail[fd]["rcpt"]=({});
			smtp_send(fd, "250 <"+args+"> ... Sender okay\n");
			break;
		case "rcpt":
			if(args[0..2]!="to:") {
			smtp_send(fd, "500 Command unrecognized\n");
			break;
			}
			args = args[3..];
			while(args[0]==' ') args = args[1..];
			if(!args || args=="") {
			smtp_send(fd, "501 RCPT TO requires address as operand\n");
			break;
			}
			mail[fd]["rcpt"] +=({args});
			smtp_send(fd, "250 <"+args+"> ... Recipient okay\n");
			break;
		case "vrfy":
			if(user_exists(args))
			smtp_send(fd, "250 "+args+"\n");
			else
			smtp_send(fd, "250 "+args+"... not matched: unknown user\n");
			break;
		case "debug":
			smtp_send(fd, "250 There is no sendmail bug here, you do not need DEBUG :-)\n");
			break;
		case "expn":
			smtp_send(fd, "250 Not yet implemented, Leto.\n");
			break;
		default:
			smtp_send(fd, "500 Command unrecognized\n");
			break;
	}
}

// close_callback is called when any socket is closed unexpectedly
// (by the driver instead of as a result of socket_close()).

static void close_callback(int fd)
{
	if (fd == smtpSock) {
		log_info(LOG_SMTP_ERR,
			"SMTP socket closed unexpectedly; restarting.\n");
		call_out("setup", 5);
	} else {
		if (undefinedp(sockets[fd])) {
			log_info(LOG_SMTP_ERR,
				sprintf("Client socket %d closed unexpectedly\n", fd));
		} else {
			log_info(LOG_SMTP_ERR,
				sprintf("Client socket %s %d closed unexpectedly.\n",
					sockets[fd]["name"], sockets[fd]["port"]));
		}
		map_delete(sockets, fd);
		map_delete(mode, mode[fd]);
		map_delete(mail, mail[fd]);
	}
}

// resolve_callback is called by the resolver object in response to our
// queries to resolve dotted decimal internet addresses into domain name
// style addresses.

void resolve_callback(string theName, string theAddr, int slot)
{
	int *fds;
	int i;
	int fd;

	fd = resolve_pending[slot];
	map_delete(resolve_pending, slot);
	if (!undefinedp(sockets[fd]) && (sockets[fd]["address"] == theAddr)) {
		sockets[fd]["name"] = theName;
	} else {
		log_info(LOG_SMTP_ERR,
			sprintf("Resolved %s to %s after connection closed.\n",
				theAddr, (sizeof(theName) ? theName : "NOT RESOLVED")));
	}
}

static private void smtp_send(int fd, string code)
{
	SMTP_DEBUG(code);
	write_data(fd, code );
}

static void close_connection(int fd)
{
int errcode;

	SMTP_DEBUG("Attempting to close socket\n");
// The code below caused sockets to linger after a "quit" command. -- Leto
#if 0
	if (sockets[fd]["write_status"] == EECALLBACK) {
		// write_callback() will call close_connection() when socket fd
		// is drained.
	SMTP_DEBUG("write_callback() will call close_connection() when socket fd"+
		"is drained.\n");
		return;
	}
#endif
	map_delete(sockets, fd);
	map_delete(resolve_pending, fd);
	errcode = socket_close(fd);
	SMTP_DEBUG("Socket closed with code:"+errcode+".\n");
	map_delete(mode, mode[fd] ) ;
	map_delete(mail, mail[fd] ) ;
}

void smtp2mudmail (int fd) {

mapping mudmail;

	SMTP_DEBUG("Smtp2mudmail called\n");
tell_object(find_player("leto"),"SMTP mail:\n"+dump_variable(mail[fd])+
	"\nSMTP socket:"+dump_variable(sockets[fd])+"\n");

mudmail["to"] = implode(mail[fd]["rcpt"],",");
mudmail["cc"] = "";
mudmail["from"] = mail[fd]["from"];
mudmail["date"] = time(); // Ok, I cheated, but it's tcp/ip. So it's ok :)
mudmail["subject"] = "Intermud mail received" ; // Need to grab real subject ;)
mudmail["message"] = mail[fd]["body"];
MAILBOX_D->send_mail( mudmail );
SMTP_DEBUG("Sent off mudmail to MAILBOX_D.\n");
return;
}

string smtp_help() {

return @EndHelp
250-                   LP-SMTP v0.5 by Leto@Earthmud (paul@via.nl)
250-
250-The following SMTP commands are recognized:
250-
250-   HELO hostname                   - startup and give your hostname
250-   MAIL FROM:<sender-address>      - start transaction from sender
250-   RCPT TO:<recipient-address>     - name recipient for message
250-   VRFY <address>                  - verify deliverability of address
250-   EXPN <address>                  - expand mailing list address
250-   DATA                            - start text of mail message
250-   RSET                            - reset state, drop transaction
250-   NOOP                            - do nothing
250-   DEBUG [level]                   - set debugging level, default 1
250-   HELP                            - produce this help message
250-   QUIT                            - close SMTP connection
250-
250-The normal sequence of events in sending a message is to state the
250-sender address with a MAIL FROM command, give the recipients with
250-as many RCPT TO commands as are required (one address per command)
250-and then to specify the mail message text after the DATA command.
250 Multiple messages may be specified.  End the last one with a QUIT.
EndHelp;
}

