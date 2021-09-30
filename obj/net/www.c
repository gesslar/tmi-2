/*
 * simple www client for even simpler lookups
 * does nothing yet, just dump the html source all at once
 * Hint: I think it's a driver bug. When using victim->more(array);
 * fro within read_callback or close_callback, it doesn't work. In
 * any other place it does. What's more :
 * void bogus() { return 1;} doesn't barf while
 * void bogus() { return find_player("leto");} does... Boggle.
 * So, for now, no more. just dump :(
 *  Leto 12/6/95  (Work in progress !!)
 *  Leto 12/7/95  (Still work in progress !!) done: better parsing
 * Todo: fix the ALT in IMG SRC loop
 */


// ansi is temp :)
#include <ansi.h>

#include <mudlib.h>
#include <config.h>
#include <net/socket.h>
#include <socket.h>
#include <uid.h>

#define HREF "<A HREF"
#define HREFC "</A>"
 
static mapping services;
static int debug_level;
static object www_socket;

object victim;
 
// Prototypes :)
void create();
int www(string argh);
string parse_data(mixed info);
void send_packet(string data);

//#define SOCKET "/std/socket/socket.c"
//#define WWW_SERVER "131.174.8.16 6667"
#define WWW_SERVER "127.0.0.1 2217"

inherit OBJECT;


int read_callback(object socket, mixed info){


if(interactive(environment(this_object()))) 
 {
  info = explode(replace_string(info, "\r", ""), "\n");
   victim = environment(this_object());
 /*
  * the more doesn't work for some reason. The input_to in there
  * doesn't get called, and i have NO clue whatsoever after 3 hours
  * of debugging why. For now, i won't use more
  */

 
    message("network",parse_data(info),victim);
  //victim->more(explode(parse_data(info),"\n")); // Driver bug prevents this ??

  }
else
	{
	message("network",parse_data(info),environment(this_object()));
 }
	return 1;
}
 
int close_callback(object socket){

    socket->remove();
    remove();
    // If it doesn't work, use brute force :)
    if(this_object()) destruct(this_object());
    return 1;
}
  
void send_packet(string data){
 
    www_socket->send(data);
}    
 
void process_error(mixed *info){
 
    message("network","WWW Error:"+dump_variable(info),environment(this_object()));
}    
 
void create(){ 

// if(!clonep()) return;

if(!this_player()) destruct(this_object());
// Leto: tsk tsk tsk Drizzt, remember uids ? :)
seteuid(ROOT_UID);


//    message("network","Initiating www connection.\n",environment(this_object()) );
    set("short","A simple www client");
    set("long","A very simple www client, featuring Deathblade's socket.c");
    set("mass",0);
    set("bulk",0);
    set("id", ({ "www","www client","client" }) );
//    set("undroppable",1); // ?
}

void get_page(string url) {

string type,site,page;
int parsed;

parsed = sscanf(url,"%s://%s/%s",type,site,page);
if(parsed <1) {
  message("error",sprintf("invalid URL,attempting http://%s\n",url),
   environment(this_object()) );
  get_page(sprintf("http://%s",url)); // No we won't recurse indefinetly :)
 return;
 }

if(parsed == 1)  // Bozo's didn't gave the trailing '/', must be netscape users
  {
   get_page(url+"/");
   return;
  }

if(parsed ==2)  // No specified page, use index.html
  page = page+"index.html";


// Now we're ready to handle the request.

switch(type) {

   case("telnet") : 
    {
     message("error", "Telnet is not supported. Wizards can use the terminal for that.\n",
      environment(this_object()) );
     break;
    }
   case("ftp") :
    {
     message("error", "Ftp is not supported. Get real !!!\n",
      environment(this_object()) );
     break;
    }
   case("gopher") :
    {
     message("error", "Gopher is not supported. This is a simple www client!\n",
      environment(this_object()) );
     break;
    }
   case("http"):
    {
     // check site for ":"
     if(strsrch(site,":")==-1) site = site + " 80" ; // default port
      else site = replace_string(site,":"," "); // make it a valid string for
                                                // MudOS sockets
     if(sscanf(site,"%*s.%*s.%*s.%*s %*s")!=5)
     {
      message("error", sprintf("Error in site format for %s. Shit, I prob need to resolve the name you gave me, giving up for now.\n",site),
        environment(this_object()) );
       break;
      }
     www_socket = clone_object(SOCKET, SKT_STYLE_CONNECT, 
      site, (: read_callback :), (: close_callback :) );
      if(!www_socket)
      {
       message("error", sprintf("Can't connect to %s.\n",site),
        environment(this_object()) );
       break;
      }
     write(sprintf("Conection established, attempting to get %s...\n",page));
	send_packet(sprintf("GET /%s HTTP/1.0\n",page));
     write("Waiting for answer...\n");
     break;
    }
   default :
      {
       message("error", "Never heard of an URL like that.\n",
        environment(this_object()) );
       break;
      }
  }
}

void init() { 
    add_action("www","www");
    // add protection for client being dropped here
}

void remove(){  
    if(www_socket) www_socket->remove();
    destruct(this_object());
}

//int move() { return 1; } // not allowed, for Tmi-2 like muds }

int www(string argh) {

  if(!argh || argh=="") 
   {
    notify_fail("No url specified.\n");
    return 0;
   }
 get_page(argh);
 return 1; // Let the command hook handle whatever it is.
}

string parse_data(string *info) {

string contents,tmp1,tmp2,tmp3, *tmp4;
int open,close,done,cols,lines,i,debug;

// Parse the bloody html here :)

//First implode it all to a big string.
if(pointerp(info)) contents = implode(info," ");
else return "Bogus type of:"+typeof(contents);
//Silently remove <HTML> and </HTML>
contents = replace_string(contents,"<HTML>","");
contents = replace_string(contents,"</HTML>","");

rm("tmp/half");
write_file("tmp/half",contents);
// Same for everything in HEAD
open = strsrch(contents,"<HEAD>");
if(open!=-1)
 {
 close = strsrch(contents,"</HEAD>");
  if(close==-1)
  {
   // No appr. closing tag, eat opentag.
   write("Missing </HEAD> tag, attempting to fix lousy html...\n");
   contents= replace_string(contents,"<HEAD>","\n");
  }
  else contents = contents[0..open-1]+contents[close+strlen("</HEAD>")..] ;
 }
// Same for everything in HEADER (sigh!!)
open = strsrch(contents,"<HEADER>");
if(open!=-1)
 {
 close = strsrch(contents,"</HEADER>");
  if(close==-1)
  {
   // No appr. closing tag, eat opentag.
   write("Missing </HEADER> tag, attempting to fix lousy html...\n");
   contents= replace_string(contents,"<HEADER>","\n");
  }
  else contents = contents[0..open-1]+contents[close+strlen("</HEADER>")..] ;
 }
// remove body tags
 contents = replace_string(contents,"<BODY>","\n"); // People assume we start
 contents = replace_string(contents,"</BODY>","\n"); // a new line
// Try and interpret the CENTER tags
while(1)
{
open = strsrch(contents,"<CENTER>");
if(open!=-1)
{
 close = strsrch(contents,"</CENTER>");
 if(close<open)
   say("Barfed on CENTER tag\n");
else {
// need to know how much room we have to center in
cols = environment(this_object())->getenv("COLUMNS") ? to_int(environment(this_object())->getenv("COLUMNS")) : 78;
//cols = 78;
tmp1 = contents[0..open-1]+"\n";
tmp2 = contents[open+8..close-1];
tmp3 = "\n"+ contents[close+9..];
  while(tmp2[0]==' ') tmp2= tmp2[1..];// Strip leading spaces
  while(tmp2[<1]==' ') tmp2= tmp2[0..<2];// Strip trailing spaces
tmp2 = sprintf("%|"+cols+"s", tmp2);
contents = tmp1+tmp2+tmp3 ;
}
}
else break;
}

// Now translate a few one-on-one tags

contents = replace_string(contents,"	"," "); 
contents = replace_string(contents,"<P>","\n\n"); 
contents = replace_string(contents,"<p>","\n\n"); 
contents = replace_string(contents,"</P>",""); 
contents = replace_string(contents,"</p>",""); 
contents = replace_string(contents,"<b>",BOLD);
contents = replace_string(contents,"</b>",NOR);
contents = replace_string(contents,"<BR>","\n--\n");
contents = replace_string(contents,"<br>","\n--\n");
contents = replace_string(contents,"<HR>","\n--\n");
contents = replace_string(contents,"<hr>","\n--\n");
contents = replace_string(contents,"<B>",BOLD);
contents = replace_string(contents,"</B>",NOR);

// Change picture refs into [INLINE]
done = 2;
#define IMGTAG "<IMG"
while( (done!=0) && (debug<10) )
{
string alt,alttmp;
debug++;
 // First the open tag
 open = strsrch(contents,(done==2) ?IMGTAG : lower_case(IMGTAG));
 if(open!=-1 ) 
  {
   tmp1 = contents[0..open-1];
   tmp2 = contents[open..];
   close = strsrch(tmp2,">");
   if(close==-1) 
    {
     say("Bad html IMG tag\n");
     break;
    }
  else {
/* ALT: the next line seems to cause an infinite loop for some reason
        That's why the max of 10 loops has been added.
   alt=(tmp2[0..close+1];

  if(alt && alt!="") alt = replace_string(alt,"ALT","alt");
   if(strsrch(alt,"alt")!=-1)
  {
say("Found alt string\n");
   if (!(sscanf(alt,"%*salt=%s ",alttmp)==2) )
   if (!(sscanf(alt,"%*salt=%s>",alttmp)==2) )
   alt= "[INLINE]";
  }
   else alt = alttmp;

 END ALT
*/


//say("loop\n");
   tmp2=tmp2[close+1..];
//   contents = tmp1 + alt +tmp2;  This should work when loop probs are gone
   contents = tmp1 + "[INLINE]" +tmp2;
   alt="";
 }
}
 else done--;
}
// the <a hrefs>foo</a> tags
done = 2;
while(done!=0)
{
 // First the open tag
 open = strsrch(contents,(done==2) ? HREF : lower_case(HREF));
 if(open!=-1) 
  {
   tmp1 = contents[0..open-1];
   tmp2 = contents[open..];
   close = strsrch(tmp2,">");
   if(close==-1) 
    {
     say("Bad html a href tag\n");
     break;
    }
   tmp2=tmp2[close+1..];
   contents = tmp1 + " " + BLU +tmp2;
   contents = replace_string(contents, ((done==2) ? HREFC : lower_case(HREFC))," "+NOR,1);
  }
 else done--;;
} 

// Remove whitespace
while(strsrch(contents,"  ")!=-1) contents = replace_string(contents,"  "," ");

// And the H tags...

 contents = replace_string(contents,"<h1>","\n\n"+RED); 
 contents = replace_string(contents,"<H1>","\n\n"+RED); 
 contents = replace_string(contents,"</H1>","\n\n"+NOR); 
 contents = replace_string(contents,"</h1>","\n\n"+NOR); 
 contents = replace_string(contents,"<h2>","\n\n"+GRN); 
 contents = replace_string(contents,"<H2>","\n\n"+GRN); 
 contents = replace_string(contents,"</H2>","\n\n"+NOR); 
 contents = replace_string(contents,"</h2>","\n\n"+NOR); 
 contents = replace_string(contents,"<h3>","\n\n"+GRN); 
 contents = replace_string(contents,"<H3>","\n\n"+GRN); 
 contents = replace_string(contents,"</H3>","\n\n"+NOR); 
 contents = replace_string(contents,"</h3>","\n\n"+NOR); 
 contents = replace_string(contents,"<UL>","\n\n"); 
 contents = replace_string(contents,"<ul>","\n\n"); 
 contents = replace_string(contents,"</UL>","\n\n"); 
 contents = replace_string(contents,"</ul>","\n\n"); 
 contents = replace_string(contents,"<LI>",BOLD+"\no "+NOR); 
 contents = replace_string(contents,"<li>",BOLD+"\no "+NOR); 

if(contents[<1]!='\n') contents = contents +"\n";
// Now we do an indent trick. We explode the string to an array with \n
// then we break up cols longer then COLUMNS or 78 chars, and add as much
// white space to the second half as the first half has. Simple ident.

rm("/tmp/unwrapped");
write_file("/tmp/unwrapped",contents);
tmp4 = explode(contents,"\n");
//determine the amount of cols.
cols = environment(this_object())->getenv("COLUMNS") ? to_int(environment(this_object())->getenv("COLUMNS")) : 78;
lines = sizeof(tmp4);

while(i<lines)
 {
  if(strlen(tmp4[i])>cols)
  {
   // Take the first space from cols-- to wrap
   tmp1 = tmp4[i][0..cols];
   open = strsrch(tmp1," ",-1);
   if(open==-1) open = cols-1 ; // We tried...
   tmp1 = tmp1[0..open-1];
   tmp2="";
// ident remainder of line.
   while(tmp4[i][0]==' ') { tmp2 += " " ;  tmp4[i]=tmp4[i][1..]; 
}
   tmp2 += tmp1[open+1..] + tmp4[i][cols+1..];
   tmp4 = tmp4[0..i-1] + ({ tmp1 }) + ({ tmp2 }) + tmp4[i+1..] ;
   }
  else i++;
 }
contents = implode(tmp4,"\n");

while(strsrch(contents,"  ")!=-1) contents = replace_string(contents,"  "," ");
rm("/tmp/wrapped");
write_file("/tmp/wrapped",contents);
return contents;
}

void leto(string foo){ send_packet(foo+"\n"); }
