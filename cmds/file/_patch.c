/*
// Val@TMI (Jan 95) <Tero.Pelander@utu.fi>
// Use at own risk.  It should work but no guaranties.
// Currently all context lines must match when locating a match.
// Val added unified diff and added some protection from loops 29-Nov-95
*/
// #define cmd_patch cmd_patch2 // test version

#define TMPFILE ("/tmp/patch."+geteuid(previous_object()))

// Define this if explode() removes only one of the delimiters
// from the begining.
#undef NEW_EXPLODE

// Strip I first characters from string.
#define STRIP(S,I) (S[I..<1])

#define MIN(A,B) (((A)<(B))?(A):(B))
#define MAX(A,B) (((A)>(B))?(A):(B))

#define IS_VERBOSE is_verbose
#define WRITE(X) write(X)
#define VERBOSE(X) {if( IS_VERBOSE ) WRITE(X);}
#define ERR(X) write(X)

#define NORMAL_DIFF	1
#define CONTEXT_DIFF	2
#define UNIFIED_DIFF	3
#define DIFF_TYPE_LIST ({ "normal", "context", "unified" })
#define HUNK_MAX	100
#define H_OPT	2
#define H_LEN	4
#define H_LOC	6


#include <mudlib.h>

inherit DAEMON;

int diff_type, diff_line, diff_max, last_offset, last_frozen_line;
int is_verbose;

// Apply the hunk.
// If hunk == 0 return all unreturned lines from target.
// Returns: new lines upto the end of fixed lines.
string *
apply_hunk( string *target, mixed *hunk )
{
    int old_frozen, upto_line;

    // No more changes to apply.
    if( !hunk ) return target[last_frozen_line..sizeof(target)-1];

    old_frozen = last_frozen_line;
    upto_line = hunk[H_LOC] + last_offset - 1;
    if( hunk[H_LEN] ) upto_line -= 1;
    last_frozen_line = upto_line + hunk[H_LEN] + 1;
    if( upto_line + 1 < old_frozen ) {
	// This one shouldn`t appear if the locate_hunk() is correct.
	ERR( "misordered hunks! output would be garbled\n" );
	throw( "Error\n" );
	return 0;
    }

    return target[old_frozen..upto_line]
	+ hunk[1][0..hunk[H_LEN+1]-1];
}

// Create the output for reject file for hunks that could not be applied.
// The output is always in contex diff format.
string *
abort_hunk( mixed *hunk )
{
    int i, need0, need1;
    string head0, head1;

    for( i = hunk[H_LEN]; i--; ) {
	hunk[0][i] = sprintf( "%c %s", hunk[H_OPT][i], hunk[0][i] );
	if( hunk[H_OPT][i] != ' ' ) need0 = 1;
    }
    for( i = hunk[H_LEN+1]; i--; ) {
	hunk[1][i] = sprintf( "%c %s", hunk[H_OPT+1][i], hunk[1][i] );
	if( hunk[H_OPT+1][i] != ' ' ) need1 = 1;
    }

    if( !hunk[H_LEN] ) hunk[H_LEN]++;
    if( !hunk[H_LEN+1] ) hunk[H_LEN+1]++;
    head0 = sprintf("*** %d,%d ****", hunk[H_LOC], hunk[H_LOC]+hunk[H_LEN]-1 );
    head1 = sprintf("--- %d,%d ----", hunk[H_LOC+1]+last_offset,
		    hunk[H_LOC+1]+last_offset+hunk[H_LEN+1]-1 );

    return ({ "***************", head0 }) +
	( need0 ? hunk[0][0..hunk[H_LEN]]   : ({}) ) +
	({ head1 }) +
	( need1 ? hunk[1][0..hunk[H_LEN+1]] : ({}) );
}

// Find the location where the hunk should be applied.
// Sets globals: last_offset
// Returns: The fuzz factor. -1 if the location could not be found.
// Bugs: the fuzz (=how many context lines was ignored) is not used.
int
locate_hunk( string *target, mixed *hunk )
{
    int first_guess, max_pos_offset, max_neg_offset;
    int offset, offset_max, old_offset;

    old_offset = last_offset;
    first_guess = hunk[H_LOC] + last_offset;
    if( hunk[H_LEN] ) first_guess--;
    max_pos_offset = sizeof( target ) - first_guess - hunk[H_LEN];
    max_neg_offset = first_guess - last_frozen_line;
    if( max_pos_offset < 0 ) {
	max_neg_offset += max_pos_offset;
	first_guess += max_pos_offset;
	last_offset += max_pos_offset;
	max_pos_offset = 0;
    }
    if( max_neg_offset < 0 ) {
	if( ( max_pos_offset += max_neg_offset ) < 0 ) {
	    last_offset = old_offset;
	    return -1;
	}
	first_guess -= max_neg_offset;
	last_offset -= max_neg_offset;
	max_neg_offset = 0;
    }

    // Nothing known about the original file.
    if( !hunk[H_LEN] ) return 0;

    {
	int i, start, max;

	max = hunk[H_LEN];
	start = first_guess;
	while( i < max && hunk[0][i++] == target[start++] ) ;
	if( i == max ) return 0;

	for( offset_max=MAX(max_pos_offset,max_neg_offset);
	     offset++<offset_max; )
	{
	    if( offset <= max_pos_offset ) {
		start = first_guess + offset; i = 0;
		while( i < max && hunk[0][i++] == target[start++] ) ;
		if( i == max ) {
		    last_offset += offset;
		    return 0;
		}
	    }
	    if( offset <= max_neg_offset ) {
		start = first_guess - offset; i = 0;
		while( i < max && hunk[0][i++] == target[start++] ) ;
		if( i == max ) {
		    last_offset -= offset;
		    return 0;
		}
	    }
	} // for
    }

    // Failed to find a match.
    last_offset = old_offset;
    return -1;
}


// For -R flag.
void
reverse_hunk( mixed *hunk )
{
    int tmp;
#define SWAP(A,B,T) T=A;A=B;B=T
    SWAP(hunk[0], hunk[1], tmp);
    SWAP(hunk[2], hunk[3], tmp);
    SWAP(hunk[4], hunk[5], tmp);
#undef SWAP
}


#define ERR_AT(X) (ERR(sprintf("Malformed diff at line %d.\n\"%s\"\n",(X)+1,\
  diff[X][0..77])),-1)
// Find the limits of the next hunk.
// Changes the contents of 'hunk'.
// Returns: >0 if found, 0 if not, -1 if error.
int
next_hunk( string *diff, mixed *hunk )
{
    int i, tmp, part, fail;
    string str;

    if( diff_line >= diff_max ) return 0;
    i = diff_line;

    hunk[H_LEN] = hunk[H_LEN+1] = 0;

    if( diff_type == UNIFIED_DIFF ) {
	int part1, part2;

	str = diff[diff_line];
	if( sscanf( str, "@@ -%d,%d +%d,%d @@%s",
		   hunk[H_LOC], hunk[H_LEN], hunk[H_LOC+1], hunk[H_LEN+1],
		   str ) != 5 )
	{
	    return 0;
	}
	if( hunk[H_LEN] > HUNK_MAX || hunk[H_LEN+1] > HUNK_MAX ) {
	    ERR( sprintf( "Too big hunk at line %d.\n", i+1 ) );
	    return -1;
	}
	part = hunk[H_LEN] + hunk[H_LEN+1];
	part1 = part2 = 0;
	while( ++i < diff_max && part1 + part2 < part ) {
	    str = STRIP( diff[i], 1 );
	    switch( diff[i][0] ) {
	      case 0:
		// Special case for empty lines.
		str = "";
	      case ' ':
		if( hunk[H_LEN] > part1 && hunk[H_LEN+1] > part2 ) {
		    hunk[H_OPT][part1] = hunk[H_OPT+1][part2] = ' ';
		    hunk[0][part1++] = hunk[1][part2++] = str;
		    break;
		}
		// fallthrough
	      default:
		return ERR_AT( i );
	      case '-':
		if( hunk[H_LEN] > part1 ) {
		    hunk[H_OPT][part1] = '-';
		    hunk[0][part1++] = str;
		    break;
		}
		return ERR_AT( i );
	      case '+':
		if( hunk[H_LEN+1] > part2 ) {
		    hunk[H_OPT+1][part2] = '+';
		    hunk[1][part2++] = str;
		    break;
		}
		return ERR_AT( i );
	    }
	}
	if( part1 + part2 < part ) {
	    ERR( sprintf( "unexpected end of hunk at line %d\n", i+1 ) );
	    return -1;
	}

	diff_line = i;

    } else
    if( diff_type == CONTEXT_DIFF ) {
	int start_missing, replace_needed, part_len, hunk_part;

	if( diff[diff_line][0..7] != "********" ) return 0;
	if( ++i >= diff_max || !sscanf( diff[i], "*** %s", str ) ) {
	    return ERR_AT( i );
	}
	if( sscanf( str, "%d,%d", tmp, part_len ) == 2 && tmp ) {
	    part_len -= tmp - 1;
	} else {
	    part_len = tmp?1:0;
	}
	hunk[H_LOC] = tmp;
	if( part_len > HUNK_MAX ) {
	    ERR( sprintf( "Too big hunk at line %d.\n", i+1 ) );
	    return -1;
	}
	while( ++i < diff_max ) {
	    switch( ( str = diff[i] )[0] ) {
	      case '-':
		if( sscanf( str, "--- %s", str ) ) {
		    if( part != 0 && part != 1 ) return ERR_AT( i );
		    if( part_len ) {
			if( part == 1 ) return ERR_AT( i );
			start_missing = replace_needed = 1;
		    }
		    part = 2;
		    if( sscanf( str, "%d,%d", tmp, part_len ) == 2 && tmp ) {
			part_len -= tmp - 1;
		    } else {
			part_len = tmp?1:0;
		    }
		    hunk[H_LOC+1] = tmp;
		    hunk_part = 1;
		    if( part_len > HUNK_MAX ) {
			ERR( sprintf( "Too big hunk at line %d.\n", i+1 ) );
			return -1;
		    }
		    break;
		}
		replace_needed--;
		// fall through
	      case '!': case '+':
		replace_needed++;
		// fall through
	      case ' ':
		if( str[1] != ' ' || !part_len ) fail = 1;
		else {
		    part_len--;
		    if( !(part & 1) && part++ == 2 ) hunk[H_LEN+1] = 0;
		    tmp = str[0];
		    str = STRIP(str,2);
		    if( part == 1 || (start_missing && tmp==' ') ) {
			hunk[H_OPT][hunk[H_LEN]] = tmp;
			hunk[0][hunk[H_LEN]++] = str;
		    }
		    if( part == 3 || tmp == ' ' ) {
			hunk[H_OPT+1][hunk[H_LEN+1]] = tmp;
			hunk[1][hunk[H_LEN+1]++] = str;
		    }
		}
		break;
	      default:
		fail = 1;
	    }
	    if( fail ) break;
	}
	if( (part == 3) ? part_len : (part == 2) ? replace_needed : 1 ) {
	    ERR( sprintf( "unexpected end of hunk at line %d\n", i+1 ) );
	    return -1;
	}
	diff_line = i;

    } else {
	int len1, len2;

	if( !sscanf( diff[diff_line], "%d%s", tmp, str ) ) return 0;
	if( sscanf( str, ",%d%s", len1, str ) ) {
	    len1 -= tmp - 1;
	} else {
	    len1 = tmp?1:0;
	}
	hunk[H_LOC] = tmp;
	if( ( part = member_array( str[0], "dca" ) ) == -1 ||
	    !sscanf( STRIP(str,1), "%d%s", tmp, str ) )
	{
	    return 0;
	}
	if( sscanf( str, ",%d%s", len2, str ) ) {
	    len2 -= tmp - 1;
	} else {
	    len2 = tmp?1:0;
	}
	hunk[H_LOC+1] = tmp;
	if( str != "" ) return 0;
	if( part == 0 ) {
	    if( len2 > 1 ) return 0;
	    len2 = 0;
	} else if( part == 2 ) {
	    if( len1 > 1 ) return 0;
	    len1 = 0;
	}

	if( len1 > HUNK_MAX || len2 > HUNK_MAX ) {
	    ERR( sprintf( "Too big hunk at line %d.\n", diff_line+1 ) );
	    return -1;
	}

	i = diff_line;
	while( ++i < diff_max ) {
	    switch( ( str = diff[i] )[0] ) {
	      case '<':
		if( part == 2 || str[1] != ' ' || !len1 ) {
		    fail = 1;
		} else {
		    len1--;
		    hunk[H_OPT][hunk[H_LEN]] = '-';
		    hunk[0][hunk[H_LEN]++] = STRIP(str,2);
		}
		break;
	      case '>':
		if( part != 2 || str[1] != ' ' || !len2 ) {
		    fail = 1;
		} else {
		    len2--;
		    hunk[H_OPT+1][hunk[H_LEN+1]] = '+';
		    hunk[1][hunk[H_LEN+1]++] = STRIP(str,2);
		}
		break;
	      case '-':
		if( str[0..2] == "---" && part == 1 && !len1 ) {
		    part++;
		    break;
		}
		// fall through
	      default:
		fail = 1;
	    }
	    if( fail ) break;
	}
	if( len1 || len2 ) return ERR_AT( i );
	diff_line = i;
    }

    return diff_line;
}


// Has to be private (or static) because of file_size().
// Called by next_patch().
private string
make_fname( string file, string path, int strip_path )
{
    if( file ) {
	sscanf( file, "%s ", file );
	sscanf( file, "%s\t", file );
	if( strip_path ) {
	    while( sscanf( file, "%*s/%s", file ) );
	}
	file = resolv_path( path, file );
	if( file_size( file ) >= 0 ) return file;
    }
}

// Namelines: '*** ', '--- ', '+++ ', 'Index:'
// Looks for the first diff line and what mode it is.
// Sets globals: diff_type, diff_line, last_offset, last_frozen_line
// Returns: file_name or "" if none found. 0 is returned at errors.
static string
next_patch( string *diff, string file_name, string file_dir )
{
    int i, guess_type, start_line;
    string str, s1, fname_old, fname_new, fname_ind;

    // Set global variables
    diff_type = last_offset = last_frozen_line = 0;

    if( diff_line >= diff_max ) {
	VERBOSE( "done\n" );
	return 0;
    }
    VERBOSE( "Hmm..." );

    for( i = diff_line - 1; !diff_type && ++i < diff_max; ) {
	switch( ( str = diff[i] )[0] ) {
	  case '*':
	    if( str[0..7] ==  "********" ) {
		guess_type = CONTEXT_DIFF;
		start_line = i;
	    } else
	    if( guess_type == CONTEXT_DIFF ) {
		if( sscanf( str, "*** %*d" ) ) {
		    diff_type = CONTEXT_DIFF;
		}
	    } else {
		sscanf( str, "*** %s", fname_old );
	    }
	    break;
	  case '-':
	    sscanf( str, "--- %s", fname_new );
	    break;
	  case '+':
	    // Actually old/new are switched in unified diff
	    if( sscanf( str, "+++ %s", fname_old ) ) {
		guess_type = UNIFIED_DIFF;
		start_line = i;
	    }
	    break;
	  case '@':
	    if( guess_type == UNIFIED_DIFF && str[0..3] == "@@ -" ) {
		diff_type = UNIFIED_DIFF;
		start_line = i;
	    }
	    break;
	  case 'I':
	    if( sscanf( str, "Index:%s", fname_ind ) ) {
		sscanf( fname_ind, " %s", fname_ind );
	    }
	    break;
	  case '0'..'9':
	    if( ( sscanf( str, "%sa%s", s1, str ) ||
		  sscanf( str, "%sc%s", s1, str ) ||
		  sscanf( str, "%sd%s", s1, str ) ) &&
	        !sizeof( regexp( ({ s1+str }), "[^0-9,]" ) ) )
	    {
		guess_type = NORMAL_DIFF;
		start_line = i;
	    }
	    break;
	  case '>': case '<':
	    if( str[1] == ' ' && guess_type == NORMAL_DIFF ) {
		diff_type = NORMAL_DIFF;
	    }
	} // switch
	if( guess_type && i != start_line ) guess_type = 0;
    }
    if( !diff_type ) {
	if( diff_line ) {
	    VERBOSE( "  Ignoring the trailing garbage.\ndone\n" );
	} else {
	    ERR( "  I can't seem to find a patch in there anywhere.\n" );
	}
	return 0;
    }

    VERBOSE( sprintf( "%sooks like a %s diff to me...\n",
		     (diff_line ? "The next patch l" : "L"),
		     DIFF_TYPE_LIST[ diff_type-1 ]  ) );
    if( diff_line != start_line && IS_VERBOSE ) {
	int flag;

	if( flag = (start_line - 10 > diff_line) ) {
	    diff_line = start_line - 10;
	}
	WRITE( sprintf( "The text leading up to this was%s:\n",
		      flag ? " (last 10 lines)" : "" ) );
	WRITE( "--------------------------\n" );
	while( diff_line < start_line ) {
	    WRITE( sprintf( "|%s\n", diff[diff_line++] ) );
	}
	WRITE( "--------------------------\n" );
    }
    diff_line = start_line;

    for( i = 0; !file_name && i < 2; i++ ) {
	string tmp_new, tmp_old;

	tmp_new = make_fname( fname_new, file_dir, i );
	tmp_old = make_fname( fname_old, file_dir, i );
	if( tmp_new && tmp_old ) {
	    file_name = (strlen(tmp_new)<strlen(tmp_old)) ? tmp_new : tmp_old;
	} else if( tmp_new ) {
	    file_name = tmp_new;
	} else if( tmp_old ) {
	    file_name = tmp_old;
	} else {
	    file_name = make_fname( fname_ind, file_dir, i );
	}
    }

    if( file_name ) {
	return file_name;
    }

    ERR( "No file to patch.  Skipping...\n" );
    return "";
}

private string *
get_lines( string file_name )
{
    string file;

    switch( file_size( file_name ) ) {
      case -2: { write( file_name + " is a directory.\n" ); return 0; }
      case -1: { write( file_name + " is not a file or no permission.\n" );
		 return 0; }
    }
    file = read_file( file_name );
    if( !file ) {
	write( file_name + " is unreadable.\n" );
	return 0;
    }
#ifdef NEW_EXPLODE
    if( file[0] == '\n' ) {
	return ({ "" }) + explode( file, "\n" );
    }
#else
    {
	int i;

	while( file[i] == '\n' ) i++;
	if( i ) {
	    string *tmp;

	    tmp = allocate( i );
	    while( i-- ) tmp[i] = "";
	    return tmp + explode( file, "\n" );
	}
    }
#endif
    return explode( file, "\n" );
}

int
cmd_patch( string str )
{
    string *cmdline, *diff, target_file, target_dir;
    int reverse;

    seteuid( getuid( previous_object() ) );
    is_verbose = 1;

    cmdline = str ? explode( str, " " ) : ({ });
    while( sizeof( cmdline ) && cmdline[0][0] == '-' ) {
	int i;

	str = cmdline[0];
	cmdline = cmdline[1..sizeof(cmdline)-1];
	if( str == "-" ) break;
	i = 1;
	do {
	    switch( str[i] ) {
	      case 'R':
		reverse = 1;
		break;
	      case 's':
		is_verbose = 0;
		break;
	      default:
		notify_fail( sprintf( "Illegal flag: %c\n", str[i] ) );
		return 0;
	    }
	} while( ++i < strlen( str ) );
    }
    if( sizeof( cmdline ) < 1 || sizeof( cmdline ) > 2 ) {
	notify_fail( "Syntax: patch [-Rs] <diff_file> [target]\n" );
	return 0;
    }
    if( sizeof( cmdline ) == 2 ) {
	switch( file_size( target_file = resolv_path( "cwd", cmdline[1] ) ) ) {
	  case -1:
	    notify_fail( target_file +
			 " is not a file or directory or no permission.\n");
	    return 0;
	  case -2:
	    target_dir = target_file;
	    target_file = 0;
	}
    } else {
	target_dir = "cwd";
    }

    if( !pointerp( diff = get_lines( resolv_path( "cwd", cmdline[0] ) ) ) ) {
	return 1;
    }


    {
	string *target, *patched, *reject, file_name;
	int hunk_idx, fuzz, err;
	mixed *hunk;

	// Reset global variables
	diff_line = 0;
	diff_max = sizeof( diff );

	// Go through the diff file and apply each patch
	while( file_name = next_patch( diff, target_file, target_dir ) ){
	    if( file_name == "" || !( target = get_lines( file_name ) ) ) {
		file_name = 0;
	    } else {
		VERBOSE( sprintf( "Patching file %s...\n", file_name ) );
	    }

	    hunk = ({ allocate(HUNK_MAX), allocate(HUNK_MAX),
		      allocate(HUNK_MAX), allocate(HUNK_MAX),
		      0, 0, 0, 0, 0 });
	    patched = reject = ({ });

	    for( hunk_idx = 1; err = next_hunk( diff, hunk ); hunk_idx++ ) {
		if( err < 0 ) return 1;
		if( reverse ) reverse_hunk( hunk );
		if( !file_name ) {
		    VERBOSE( sprintf( "Hunk #%d ignored at %d.\n",
				      hunk_idx, hunk[H_LOC+1] + last_offset
				     ) );
		} else if( (fuzz=locate_hunk( target, hunk )) < 0 ){
		    reject += abort_hunk( hunk );
		    VERBOSE( sprintf( "Hunk #%d failed at %d.\n",
				      hunk_idx, hunk[H_LOC+1] + last_offset
				     ) );
		} else {
		    patched += apply_hunk( target, hunk );
		    VERBOSE( sprintf( "Hunk #%d succeeded at %d%s.\n",
				      hunk_idx, hunk[H_LOC+1] + last_offset,
				     //	fuzz not implemented yet
				     // fuzz?sprintf(" with fuzz %d", fuzz):"",
				      last_offset?sprintf(" (offset %d lines)",
							  last_offset) : ""
				     ) );
		}
	    }

	    // Just a sanity check
	    if( hunk_idx == 1 ) {
		VERBOSE( "Sorry. I was wrong.\n" );
		diff_line++;
		continue;
	    }

	    // Time to write the results to disk.
	    if( file_name ) {
		string tmp_file;

		patched += apply_hunk( target, 0 );
		rm( tmp_file = TMPFILE );
		write_file( tmp_file,
			    sizeof(patched)?implode( patched, "\n" )+"\n":"");
		rm( str = file_name + "~" );
		// Oh why doesn`t the return values of rename() get reversed?
		if( rename( file_name, str ) ) {
		    ERR( sprintf(
			"Failed to make a backup.  The patch is in %s.\n",
				 tmp_file ) );
		} else {
		    rename( tmp_file, file_name );
		}
		if( sizeof( reject ) ) {
		    rm( str = file_name + ".rej" );
		    write_file( str, implode( reject, "\n" ) + "\n" );
		    ERR( sprintf( "Wrote rejects to %s.\n", str ) );
		}
	    }
	} // while
    }
    return 1;
}

string
help()
{
    return( "Syntax: patch [-Rs] <diff_file> [target]\n\
Applies a normal, context or unified diff to a file.  The target can be a\n\
file or a directory.  If it is a directory it is used instead of current\n\
directory to resolve the file name in context diff.\n\
 -R  make a reverse patch\n\
 -s  work silently unless an error occurs\n"
	  );
}
