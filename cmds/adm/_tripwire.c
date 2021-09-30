/*
// File     :  /cmds/adm/_tripwire.c
// Desc     :  A security command to catch hackers inserting password loggers
//             in mudlib objects
// 94-07-25 :  Pallando wrote original code
//             This file is part of the TMI-2 distribution mudlib.
//             Please retain this header if you use any of this code.
*/
#include <priv.h>
#include <mudlib.h>
#include <daemons.h>
#include <commands.h>

#define DEFAULT ([ \
  CMD_SU ".c" : 0, \
  CMD_PASSWD ".c": 0, \
  LOGIN_D ".c" : 0, \
])

inherit DAEMON;

int cmd_tripwire( string arg )
{
    mapping files;
    mixed *data;
    string *names;
    int loop;

    if( !mapp( files = (mapping)this_player()-> query( "tripwire" ) ) )
    {
	write( "Setting default tripwire:\n" );
	files = DEFAULT;
    }

    loop = sizeof( names = keys( files ) );

    // Check each file in turn
    while( loop-- )
    {
	// Check the file still exists
	if( sizeof( data = get_dir( names[loop], -1 ) ) )
	{
	    // See if the file has been touched since you last checked it
	    if( files[names[loop]] != data[0][2] )
	    {
		printf( "%s :\n\tlast touched = %s\n\tlast checked = %s\n",
		    names[loop], ctime( data[0][2] ), 
		  ( files[names[loop]] ? ctime(files[names[loop]]) : "never" ));
		files[names[loop]] = data[0][2];
	    }
	} else {
	    write( names[loop] + " no longer exists.\n" );
	}
    }
    this_player()-> set( "tripwire", files, MASTER_ONLY );
    return 1;
}

string help()
{
    return( @EndText
Syntax: tripwire
Effect: tests if files have been touched since you last checked them
Usage: add the command to your .login file
Nota Bene: "call" can be used to change which files you check
  it defaults to checking files that require the user to enter their password
See also: sitecheck, get_dir
EndText
    );
}

/* EOF */
