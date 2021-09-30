
// Leto changed a type cast from string to string * 
 
#define MONITOR_TIME 900
#define WHO find_living( "asdfral" )
#define TELL(x) if(WHO) message( "debug", "MSG: " + x+ "\n", WHO );
 
 
#include <uid.h>
#include <mailer.h>
#include <net/macros.h>
 
static void monitor_mesgs();

string  *remain;
string  mesg;
static  int idx1, idx2;
                                                   
 
static
void increment_idx() {
   if( ++idx1 > 9 ) {
	idx1 = 0 ;
	if( ++idx2 > 9 ) idx2 = 0;
    }
}
      
static
void refresh() {
  mesg = 0;
  remain = ({ });
}
                                               
static
int restore_mesg( int id ) {
  string file;
 
  refresh();
  file = mail_mesg_file( id );
  if( !file_exists( file + __SAVE_EXTENSION__ ) ) return 0;
  return restore_object( file );
}
      
int save_mesg( int id ) {
  int ret;
  string file;
 
  if( !id ) return 0;
  if( file = mail_mesg_file( id ) ) {
    if( !sizeof( remain ) )
       rm( file + __SAVE_EXTENSION__ );
    else
       ret = save_object( file );
  }
  return ret;
}
 
int
valid_access( string frm ) {
   string accessor, *owner;
  
   if( geteuid( previous_object() ) == ROOT_UID )
      return 1;
  
   accessor = base_name( previous_object() );
   if( member_array( accessor, TRUSTED_MAILERS ) > -1 )
      return 1;
  
} // valid_access
 
 
 
string
get_mesg( int id ) {
   if( !restore_mesg( id ) )
     printf( "Unable to restore %d.\n", id );
   if( !mesg || mesg == "" ) mesg = "Empty or lost message!\n";
   return mesg;
}
                      
 
int add_mesg( string *local_to, string msg ) {
   int idx;
 
   refresh();
   idx = time();
 
   // We have to have a unique savefile, or else we have problems later.
   while( file_exists( mail_mesg_file( idx ) + __SAVE_EXTENSION__ ) )
      --idx;
 
   remain = uniq_array( local_to );
   mesg = msg;
   save_mesg( idx );
   return idx;
}
 
 
int
delete_mesg( int id, string user ) {
   refresh();
 
   if( !restore_mesg( id ) ) return 0;
   if( !stringp( user ) || !strlen( user ) ) return -1;
 
   if( member_array( user, remain ) < 0 ) return -2;
   remain -= ({ user });
   save_mesg( id );
   return 1;
}
 
void
create() {
   seteuid( ROOT_UID );
   idx1 = 0;
   idx2 = 0;
// Mobydick disarmed the following line, cause I'm not sure it needs to
// be running unless we do a purge, and it does a lot of file access
// (read slow). We should probably put this on a switch of some kind
// so that we can launch it right after we do a purge, or a lot of people
// despoool their mail, or some such.
//  call_out( "monitor_mesgs", 100 );
   refresh();
} // create
 
static
void monitor_mesgs() {
   int i;
   string *mesgs;
         
   increment_idx();                            
   mesgs = get_dir( MESGDIR + idx1 + "/" + idx2 + "/*" __SAVE_EXTENSION__ );
   i = sizeof( mesgs );
 
   while( i-- ) { 
      sscanf( mesgs[i], "%s" __SAVE_EXTENSION__, mesgs[i] );
      call_out( "monitor_mesg2", 5 + (10*i), mesgs[i] );
   }
 
   call_out( "monitor_mesgs", MONITOR_TIME );
   return;
}                                           

static
void monitor_mesg2( string mesg ) {
   int i, j, id;
   string *tmp, *tmp2;
 
   tmp = ({ });
   tmp2 = ({ });
TELL( "Verify: " + mesg );
   restore_mesg( id = to_int( mesg ) );
   remain = uniq_array( remain );
   tmp = (string *) MAILBOX_D -> verify_mbox( copy( remain ), id ); //Leto
 
   j = sizeof( tmp );
   while( j-- )
      if( user_exists( tmp[j] ) ) tmp2 += ({ tmp[j] });
  
   if( sizeof( remain ) == sizeof( tmp2 ) ) return;  
   remain = tmp2;
   save_mesg( id );
}   
 
/* EOF */
