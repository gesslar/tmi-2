// This file is part of the TMI mudlib distribution.
// Please include this header if you use this code.
// Written by Buddha@TMI, modularized and revised (92-05-01)
// Expanded to use a superior security system by Buddha (92-05-31)
// Temporary flag attribute system added by Watcher (93-02-21)
// Prop_logic written by Pallando (93-03-20)
// Calls to delete_access() added by Pallando (94-03-26)
// NB - This file does not make use of the full potential of prop_logic yet.

#ifdef SECURE
#include <priv.h>
#include <driver/origin.h>
inherit "std/priv";
#endif /* SECURE */

mapping ob_data; // obscure name so it won't break code
static mapping tmp_ob_data; //	Temporary flag attrib mapping

#include <uid.h>
#include "/std/object/prop_logic.c"

// A basic init function.

void init_ob() {
        if( !ob_data ) ob_data = ([]);
        if( !tmp_ob_data ) tmp_ob_data = ([]);
}

//  A query of the data sets ... for admins and root access only.

mapping query_ob_data() { 
   if(!adminp(geteuid(previous_object())) &&
       geteuid(previous_object()) != ROOT_UID)  return 0;
#ifdef SECURE
  return copy(ob_data);
#else
  return ob_data;
#endif
}

mapping query_temp_ob_data() {
   if(!adminp(geteuid(previous_object())) &&
      geteuid(previous_object()) != ROOT_UID)  return 0;
#ifdef SECURE
  return copy(tmp_ob_data);
#else
   return tmp_ob_data;
#endif
}


// The generic set(label, value) function.
// You can specify elements deeper in the tree by passing a chain of elements
// separated by slashes...  thus, use could be:
// set("short", "a generic object");
// set("limb/left leg/hp", 63);

nomask varargs int set( string label, mixed value, int access_level ) {
        if( !ob_data ) init_ob();
        if( !label ) return 0;
     while( label[0] == '/' && strlen(label) > 1 )
	label = label[1..<1];
#ifdef SECURE
#if 0
        if( !valid_write( label, previous_object() ) ) return 0;
#else
        if (origin() != ORIGIN_LOCAL &&
              !valid_write(label, previous_object()))
            return 0;
#endif
#endif /* SECURE */
        _set( ob_data, strsrch(label,"/")!=-1?explode(label,"/"):({label}), value );
#ifdef SECURE
        if( access_level ) set_access( label, access_level );
#endif /* SECURE */
        return 1;
}

varargs int set_temp( string label, mixed value, int access_level ) {
        if( !tmp_ob_data ) init_ob();
        if( !label ) return 0;
#ifdef SECURE
#if 0
        if( !valid_write( label, previous_object() ) ) return 0;
#else
        if (origin() != ORIGIN_LOCAL &&
              !valid_write(label, previous_object()))
            return 0;
#endif
#endif /* SECURE */
        _set( tmp_ob_data, strsrch(label,"/")!=-1?explode(label,"/"):({label}), value );
#ifdef SECURE
        if( access_level ) set_access( label, access_level );
#endif /* SECURE */
        return 1;
}





varargs void delete( string label ) {
        if( !ob_data ) init_ob();
        if( !label ) return 0;
#ifdef SECURE
#if 0
        if( !valid_write( label, previous_object() ) ) return;
#else
        if (origin() != ORIGIN_LOCAL &&
              !valid_write(label, previous_object()))
            return;
#endif
	delete_access( label );
#endif /* SECURE */
        _delete( ob_data, strsrch(label,"/")!=-1?explode(label,"/"):({label}) );
}

varargs void delete_temp( string label ) {
        if( !label ) return 0;
        if( !tmp_ob_data ) init_ob();
#ifdef SECURE
#if 0
        if( !valid_write( label, previous_object() ) ) return;
#else
        if (origin() != ORIGIN_LOCAL &&
            !valid_write(label, previous_object()))
            return;
#endif
	delete_access( label );
#endif /* SECURE */
        _delete( tmp_ob_data, strsrch(label,"/")!=-1?explode(label,"/"):({label}) );
}





// This is for querying data.
// If you wish to make certain parts of the structure private,
// then redefine this, and use check for the private section, before
// returning ::query().

nomask mixed query( string label ) {
        mixed value;
        string func, arg;

        if( !ob_data ) init_ob();
        if( !label ) return 0;
#ifdef SECURE
#if 0
        if( !valid_read( label, previous_object() ) ) return;
#else
        if (origin() != ORIGIN_LOCAL &&
              !valid_read(label, previous_object()))
            return;
#endif
#endif /* SECURE */
        value = _query( ob_data, strsrch(label,"/")!=-1?explode(label,"/"):({label}) );
#ifdef SECURE
        value = copy( value );
#endif /* SECURE */
        if( stringp( value ) && sscanf( value, "@@%s", func ) ) {
// Mobydick added the next line as a security patch on 5-24-94. It means,
// basically, you can't call_other except to functions beginning "query_".
		if (func[0..5]!="query_") return 0 ;
                if( sscanf( func, "%s:%s", func, arg ) )
                        return call_other( this_object(), func, arg );
                else
                        return call_other( this_object(), func );
        }
        return value;
}

nomask mixed query_temp( string label ) {
        mixed value;
        string func, arg;
        
        if( !tmp_ob_data ) init_ob();
        if( !label ) return 0;
#ifdef SECURE
#if 0
        if( !valid_read( label, previous_object() ) ) return;
#else
        if (origin() != ORIGIN_LOCAL &&
              !valid_read(label, previous_object()))
            return;
#endif
#endif /* SECURE */
        value = _query( tmp_ob_data, strsrch(label,"/")!=-1?explode(label,"/"):({label}) );
#ifdef SECURE
        value = copy( value );
#endif /* SECURE */
        if( stringp( value ) && sscanf( value, "@@%s", func ) ) {
                if( sscanf( func, "%s:%s", func, arg ) )
                        return call_other( this_object(), func, arg );
                else
                        return call_other( this_object(), func );
        }
        return value;
}




// This one is just for adding to arrays and mappings..
// (though actually it will work for integers, strings, etc)
// This needs to be more thoroughly tested but it seems to work.
// to call it, use the "level1/level2/level3" format of the other functions.

void add( string label, mixed arg ) {
        mixed foo;
#ifdef SECURE
#if 0
        if( !valid_write( label, previous_object() ) ) return;
#else
        if (origin() != ORIGIN_LOCAL &&
              !valid_write(label, previous_object()))
            return;
#endif
#endif /* SECURE */
        foo = query( label );
        if( !foo ) set( label, arg );
        else set( label, foo + arg );
        return;
}

void add_temp( string label, mixed arg ) {
        mixed foo;
#ifdef SECURE
#if 0
        if( !valid_write( label, previous_object() ) ) return;
#else
        if (origin() != ORIGIN_LOCAL &&
              !valid_write(label, previous_object()))
            return;
#endif
#endif /* SECURE */
        foo = query_temp( label );
        if( !foo ) set_temp( label, arg );
        else set_temp( label, foo + arg );
        return;
}

/* EOF */
