//#pragma save_binary
//      File    :  /adm/daemons/network/nntp.c
//     Creator  :  Leto@Earth (Earth  Meister) [29/03/94]
//               Drizzt@TMI-2 (Socket Meister) [01/04/95]
// 
 
#include <priv.h>
#include <uid.h>
#include <config.h>
#include <net/nntp.h>
#include <net/socket.h>
#include <net/daemons.h>
#include <net/macros.h>
#include <mudlib.h>
 
inherit DAEMON ;
#define DEBUG(x) tell_object(find_player("leto"),"NNTP:"+x)

 
int mysocket;
mapping sockets;
 
/* Retrieval prototypes */
 
string get_article(string message_id);
string get_head(string message_id);
string get_body(string message_id);
int check_article(string message_id);
string nntp_time(int mudtime);
void nntp_post(mapping msg,string board_name, string poster);
 
/* Networking prototypes */
 
void read_callback(int fd, string str);
void close_callback(int fd);
void write_callback(int fd);
void listen_callback(int fd);
void write_callback(int fd);
 
/* NNTP commands prototypes */
 
void cmd_list(int fd, string rest);
void cmd_quit(int fd);
void cmd_get(int fd, string rest);
void cmd_next(int fd);
void cmd_stat(int fd, string rest);
void cmd_body(int fd, string rest);
void cmd_head(int fd, string rest);
void cmd_help(int fd);
void cmd_slave(int fd);
void cmd_newnews(int fd, string parsedate);
void cmd_article(int fd, string mesg_id);
void cmd_ihave(int fd, string mesg_id);
 
/*              -=] Begin Article Manipulation Section [=-                 */
 
/* 
        Name: get_article()
     Purpose: Returns an article with given id, and newsgroup
*/
string get_article(string message_id) {
   string article;
   
   DEBUG("IN GET ARTICLE");
   // Just in case, let's check if we have one.
   if (check_article(message_id)) {
      DEBUG("file:"+NNTP_DIR+"/"+message_id);
      article = read_file(NNTP_DIR+"/"+message_id);   
      return article;  
   } else 
        return("999 Unknown message-id or group.\n"); // 999 is wrong 
        // You shouldn't ask for something that's not there
}
 
/*
        Name: check_article()
     Purpose: To check for an existance of an article
*/
int check_article(string message_id) {
                // Check to see if we have a certain article.
                // To reject a duplicate article from an nntp server
 
    // first check if we have that newsgroup 
    DEBUG("IN CHECKARTICLE:group=");
    if (!directory_exists(NNTP_DIR)){
        // Looks like we dont have it
        DEBUG("newsgroup DOESN'T exist:\n");
        DEBUG(NNTP_DIR);
        return 0;
    } else if (file_exists(NNTP_DIR+"/"+message_id)) {
        DEBUG("article exists\n");
        return 1;
    }
    else return 0; // group ok, no such article though
}
 
/*              -=] Begin NNTP utilities Section [=-                      */
 
/*
        Name: nntp_time()
     Purpose: To make a time format suitable for NNTP usage
*/
 
string nntp_time(int mudtime){
        string wrong,day,monthname,month,time,year;
        
        wrong = ctime(mudtime);
        sscanf(wrong,"%s %s  %s %s %s",day,monthname,month,time,year);
        if (!month) { 
                sscanf(wrong,"%s %s %s %s %s",day,monthname,month,time,year);
        }
        return (day+", "+month+" "+monthname+" "+year+" "+time+" "+TIME_ZONE);
}
 
/*
        Name: nntp_post()
     Purpose: Write an article to file
              Use RFC822, and RFC950
*/
void nntp_post(mapping msg, string board_name, string poster) {
        string NNTP_OUT;
 
        DEBUG("Intermud posting:"+NNTP_DIR + board_name+"/"+(string)msg["id_long"]);
        NNTP_OUT = NNTP_DIR + board_name+"/"+(string)msg["id_long"];
        write_file(NNTP_OUT,"Path: "+"@"+capitalize(MUD_NAME)+".org!"+poster+"\n");
        write_file(NNTP_OUT,"From: "+msg["poster"]+".org\n");
        write_file(NNTP_OUT,"Newsgroups: "+board_name+"\n");
        write_file(NNTP_OUT,"Subject: "+msg["title"]+"\n");
        write_file(NNTP_OUT,"Message-ID: "+msg["id_long"]+"\n");
        write_file(NNTP_OUT,"Date: "+NNTP_D->nntp_time(msg["time"])+"\n");
        write_file(NNTP_OUT,"Reply-to: "+NNTP_ACCOUNT+ "\n");
        write_file(NNTP_OUT,"Distribution: "+ "World"+"\n");
        write_file(NNTP_OUT,"Organisation: A mud called "+capitalize(MUD_NAME)+"\n");
        write_file(NNTP_OUT,"Comment: For problems contact "+ADMIN_EMAIL+"\n");
        write_file(NNTP_OUT,"NNTP-POSTING-HOST: "+"LPNNTP version " +           
                NNTP_VERSION + " at "+ capitalize(MUD_NAME)+"\n");
        write_file(NNTP_OUT,"\n");
        write_file(NNTP_OUT,msg["body"]);
        write_file(NNTP_OUT,".\n"); 
} 
 
/*              -=] Begin Socket Section [=-                               */
/* 
    Drizzt Note: 
        For right now, we will not make this NM compatable.. #1, I do not
        Know NM code, and #2, they can do it themselves (or someone who 
        wants to make a bi-compatable version (Leto)
*/  
 
/*
        Name: create()
     Purpose: Setup sockets, etc.  Initialize object variables
*/
 
create(){
        int i;
 
        mysocket = 0;
        sockets = ([]);
        /* Sockets looks like:
                ([ fd : logintime ])
                                     */
        mysocket = socket_create(STREAM, "read_callback", "close_callback");
        if(mysocket < 0){
                log_file("nntpd", sprintf("%25-s: %s\n",
                        "Create Error",socket_error(mysocket)));
                return 0;
        }
        i = socket_bind(mysocket, PORT_NNTP);
        if(i < 0){
                socket_close(mysocket);
                log_file("nntpd",sprintf("%25-s: %s\n",
                        "Bind Error",socket_error(i)));
                return 0;
        }
        i = socket_listen(mysocket, "listen_callback");
        if(i < 0){        
                socket_close(mysocket);
                log_file("nntpd",sprintf("%25-s: %s\n",
                        "Listen Error",socket_error(i)));
                return 0;
        }
}
 
/*
        Name: close_callback()
     Purpose: Closes a connection with a socket
*/
 
void close_callback(int fd) {
   log_file("nntpd", sprintf("%25-s: %23-s %s\n",
        "Close",socket_address(fd),(time() - sockets[fd])/60+" minutes logged"));
    socket_close(fd);
    map_delete(sockets, fd);
}
 
/* 
        Name: listen_callback()
     Purpose: To listen for incoming connections, and send them on their
              way when they get here
*/
 
void listen_callback(int fd){
        int socket;
 
        socket = socket_accept(fd, "read_callback", "write_callback");
        if(socket < 0){
                log_file("nntpd",sprintf("%25-s: %s\n",
                        "Accept Error",socket_error(socket)));
                return;
        }
        sockets += ([ socket : time() ]);
        write_callback(socket);
}
 
/*
        Name: write_callback()
     Purpose: Dont really know the purpose of this in a "listening" object
              In clients its used to tell the object when its ok to write
              to the socket
*/
void write_callback(int fd){
        socket_write(fd, "201 "+capitalize(MUD_NAME)+
                "LPNNTP server ready (No Posting Yet [Kick Leto])\n");
        log_file("nntpd",sprintf("%25-s: %s\n",
                "Accepted",socket_address(fd)));
}
 
/*
        Name: read_callback()
     Purpose: Called whenever the server receives information from the
              Client
*/
void read_callback(int fd, string str) {
        string cmd, rest, mesg_id, newsgroup;
 
        if(!str || str == "") {
                socket_write(fd, NNTP_BAD_CMD);        
                return;
        }
   //str = replace_string(str,"\r","\n");
    str = explode(replace_string(str, "\r", ""), "\n")[0];
        DEBUG("NNTP:"+str+"\n");
        sscanf(str, "%s %s", cmd, rest);
        if (!cmd || (cmd=="")) cmd = str;
      DEBUG("nntpcmd="+cmd+".\n");
    DEBUG("nntprest="+rest+".\n");
        switch(lower_case(cmd)) {
                case "quit": cmd_quit(fd);return; 
                case "list": cmd_list(fd,rest); return;
                case "newnews": cmd_newnews(fd,rest); return;
                case "help": cmd_help(fd); return;
                case "article": cmd_article(fd, rest); return;
                case "head": cmd_head(fd, rest); return;
                case "body": cmd_body(fd, rest); return;
                case "ihave": cmd_ihave(fd, rest); return;
                case "stat": cmd_stat(fd, rest); return;
                case "next": cmd_next(fd); return;
                case "slave": cmd_slave(fd); return;
                default: socket_write(fd, "500 Command unrecognized.\n"); return;
        }
}
 
/*
        Name: cmd_next()
     Purpose: To return the next mesg_id header
*/
 
void cmd_next(int fd) {
        // this should display the next mesg_id header as follows
        // "223 <article#, local# to the server> <mesg_id> "+
        // "Article retrieved; request text separately.\n"
        return ;
}
 
/*
        Name: cmd_body()
     Purpose: Return the body of an article
*/
void cmd_body(int fd, string rest) {
        if(!check_article(rest)) {
                socket_write(fd,"430 No article by "+rest+", sorry.\n");
                return;
        } else {
                socket_write(fd,
             "222 0 "+rest+"Article retrieved, body follows.\n");
                get_body(rest);
                return;
        }
}
       
/*
        Name: cmd_head()
     Purpose: Return the headers of an article
*/
void cmd_head(int fd, string rest) {
 
        if(!check_article(rest)) {
                socket_write(fd,"430 No article by "+rest+", sorry.\n");
                return;
        } else {
                socket_write(fd,
              "221 0 "+rest+"Article retrieved, head follows.\n");
                get_head(rest);
                return;
        }
}
 
/*
        Name: cmd_article
     Purpose: return both headers, and body of an article
*/
        
void cmd_article(int fd, string rest) {
 
        if(!check_article(rest)) {
                socket_write(fd,"430 No article by "+rest+", sorry.\n");
                return;
        } else {
                socket_write(fd,
      "220 0 "+rest+"Article retrieved, head and body folow.\n"+
               get_article(rest) );
                return;
        }
}       
 
/*
        Name: cmd_stat()
     Purpose: To retrieve an article, and possible ensure its existance
*/
void cmd_stat(int fd, string rest) {
        // return current mesg id
        if(!check_article(rest)) {
                socket_write(fd,"430 No article by "+rest+", sorry.\n");
                return;
        } else {
                socket_write(fd,
                        "203 0 "+rest+" Article retrieved, request text separately.\n");
                return;
        }
}
 
/*
        Name: cmd_slave()
     Purpose: To write about leto's sex life :)
*/
void cmd_slave(int fd) {
DEBUG("In slave cmd\n");
        socket_write(fd,
        "202 Kinky, kinky.  I don't support such perversions.\n");
        return;
}
 
/*
        Name: cmd_help()
     Purpose: List help (probably for people telneting to server)
*/
void cmd_help(int fd) {
        socket_write(fd,
                "100 This server accepts the following commands:\n"+
                "ARTICLE     BODY         GROUP\n"+
                "HEAD        LAST         LIST\n"+
                "NEXT        POST         QUIT\n"+
                "STAT        NEWGROUPS    HELP\n"+
                "IHAVE       NEWNEWS      SLAVE\n"+
                "\n"+
                "Send bug reports to Leto@Tmi-2   (earth@borg.xs4all.nl)\n"+
                "                    Drizzt@Tmi-2 (jtravis@herbie.unl.edu)\n"+
                ".\n");
        return;
}
 
/*
        Name: cmd_list()
     Purpose: To list the NNTP groups currently available
*/
void cmd_list(int fd, string rest) {
 
        //if (!rest || rest=="")  
                //socket_write(fd,  implode(NNTP_GROUPS, "\n")); 
          shout("dummy\n");
 
        return;
}
 
/*
        Name: cmd_quit()
     Purpose: To disconnect the client from the server
*/
void cmd_quit(int fd) {
        socket_write(fd, "205 Thanks for using "+ capitalize(MUD_NAME) +
                "'s LPnntp, Goodbye\n" );
        close_callback(fd);
        return;
}
 
void cmd_newnews(int fd, string parsedate) {
 
	int date,time;
	sscanf(parsedate,"%d %d",date,time);
 
   // format is  ddmmyy hhmmss  in two ints
return;
}
 
/*
void cmd_article(int fd, string mesg_id) {
 
		if(!check_article(mesg_id)) {
			socket_write(fd,"430 No article by "+mesg_id+", sorry.\n");
			return;
		} else {
		socket_write(fd,
		// 221 is a wild guess, needs to be checked !! Leto
			 "221 0 "+mesg_id+"Article retrieved, body follows.\n"+
			 get_article(mesg_id));
		return;
		}
}
*/
void cmd_ihave(int fd, string mesg_id) {
 
   //check existence of mesg_id, and ask article if not
   //here, else, tell em we don't want it
	 return;
}
 
 

