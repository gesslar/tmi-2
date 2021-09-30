// A command to save all your mail to a single file so you can FTP it
// off the mud.
// Based closely on Huthar's Intermud Mailer.
// Numerous credits are found in that file.
// Mobydick is hacking on this as of 12-28-94.

#include <mailer.h>
#include <config.h>
#include <mudlib.h>
#include <uid.h>

inherit SECURE_OBJECT;

// A lot of these vars are no longer needed but I haven't cleaned em out yet.
static int cmsg, maxmsg, start_flag, in_mailer, quitmailflag;
static string owner, tmpfilename;
static mapping newmsg;

static void clean_up_house() ;

mapping	my_groups;
mapping	*mailbox;
//      contains:
//      "from", "date", "cc", "subject", "message", "flags", "to"


static void parse_mailcmd( string cmd );
void get_text();
static void mail( string arg );
static void write_mail (int arg, string filenm) ;

void create() {
	seteuid(getuid()) ;
}

int cmd_despool (string str) {
  int i, j;
  
  my_groups = ([   ]);
  mailbox = ({ }) ;
  owner = ( string ) previous_object() -> link_data( "name" );
  if (!owner) {
    return 0 ;
  }

  
  tmpfilename = temp_file( owner, this_player() ) + ".m";
write (user_mbox_file( owner )+"\n") ;
  restore_object( user_mbox_file( owner ), 1 );
  maxmsg = sizeof( mailbox );
  
  i = maxmsg;

  if( !i ) {
    cmsg = 0;
    return 0;
  }
  
  for( cmsg =  - 1, i = 0; i <maxmsg; i++ )
  if( !( mailbox[i]["flags"]&MAIL_READ ) ) {
    cmsg = i;
    break;
  }
  
  if( cmsg ==  - 1 ) {
    cmsg = maxmsg - 1;
    if( cmsg < 0 )
      cmsg = 0;
  }
  write ("This will move all the mail in your mailbox into a file\n"+
	"called \"<your name>_mail.txt\" in your directory if you have\n"+
	"one, and in /tmp if you don't. You can then use FTP to transfer\n"+
	"the file to your local disk. You can read it using elm or other\n"+
	"mailers there if you want to do that. The mail in your box will be\n"+
	"deleted. You cannot undo this. Sure you want to? ") ;
  input_to("check_clean") ;
   return 1 ;
} // init

static
void
write_mail (int num, string filenm) {
  string tmp_msg, to, cc;
  int i, max;

  to = cc = "";
  max = sizeof( mailbox[num]["to"] );

  for( i = 0; i <max; i++ )
    to += capitalize( mailbox[num]["to"][i] ) + " ";
  max = sizeof( mailbox[num]["cc"] );

  if( max )  {
    cc = "Cc     : ";

    for( i = 0; i < max; i++ )    {
      if( !mailbox[num]["cc"][i] )
        continue;
      cc += capitalize( mailbox[num]["cc"][i] ) + " ";
    }
    cc += "\n";
  }

  if( !mailbox[num]["from"] )
    mailbox[num]["from"] = "Unknown Sender (mailerob)!";

  tmp_msg = sprintf(
    "From %s@TMI-2 %s\nTo: %s\nFrom: \"%s\" <%s@tmi-2>\nCc:%s\n"+
	"Subject: %s\n\n%s\n",
    mailbox[num]["from"],
    ctime( mailbox[num]["date"] ),
    to,
    capitalize( mailbox[num]["from"] ), mailbox[num]["from"],
    cc,
    mailbox[num]["subject"],
    MAILMESG_D -> get_mesg( mailbox[num]["idx"] )
    );


    write_file(filenm,tmp_msg) ;
} // write_mail

// Does the job.

static void check_clean(string str) {
     if (!str || str[0]!='y') {
	write ("Aborted.\n") ;
	return ;
    }
    clean_up_house() ;
   owner = "" ;
}

static void clean_up_house() {
    string filenm ;
    int i ;

    filenm = user_path(owner) ;
    if (file_size(filenm)!=-2) filenm = "/tmp/" ;
    filenm += owner + "_mail.txt" ;
    for (i=0;i<sizeof(mailbox);i++) {
	write_mail(i, filenm) ;
	MAILMESG_D -> delete_mesg( mailbox[i]["idx"], owner );
    }
    rm(user_mbox_file(owner)+__SAVE_EXTENSION__) ;
    owner = "" ;
}

int help() {
write (@END_TEXT
Usage: despool

This command will take every mail message in your mail box and write it
to a file in your directory if you have one, and in /tmp if you don't.
The name of the file will be <your name>_mail.txt.  The messages will be
deleted from your mailbox. You can FTP the file back to your local
host, so that you can store those messages in your own disk space instead
of taking up space on the MUD's (surely full) disk.
Warning! This process is irreversible! Do not do this unless you are
sure you want to (unless your mud admin tells you to - then do it right
away ;)
END_TEXT
	) ;
	return 1 ;
}
