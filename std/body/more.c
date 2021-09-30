#define write(x) message( "more", x, this_object() );

/*
 * more.c
 * description: the more command for the player object
 * author: wayfarer
 *
 * Modified by Watcher@TMI (3/27/93) to add done_more() passing and
 * elimination of 100% more prompts.
 * 
 * Inspiral (4/94) made the input_to()'s not be silent any longer.
 */
 
#include <ansi.h>

varargs string *wild_card (string arg, int keepdots);
static void search_forward (string arg);
static void search_reverse (string arg);
static void even_more(string str);
void do_more_file(string file);

#define CHUNK 24

static private object previous;
static private mixed *files;
static private string more_file, *lines, last_regexp, chunkstr;
static private int more_line, from_file, num_lines, direction, chunk;


void
write_more_line()
{
  write (inverse("--More--("+((more_line*100)/num_lines)+"%)"));
}
 
int more_line_pos() { return (more_line*100/num_lines); }

varargs int
  more (mixed str, int flag, int mask)
{
  int i;
  string tmp, hold, hold2;
  
  if (!str) 
    {
      notify_fail("Usage: more <filename>\n");
      return 0;
    }
  previous = previous_object();
  direction = 1;
  last_regexp = "";
  more_line = 0;
   chunk = (int)this_object()->getenv("LINES") ;
   if (stringp(chunk))  chunk = to_int(chunk);
/*
  if (!chunkstr || !sscanf(chunkstr, "%d", chunk)) 
*/
  if (!chunk || !intp(chunk))
      chunk = CHUNK;
   chunk--;  		// Make room for the more prompt.
  if (stringp(str)) 
    {
      from_file = 1;
      if (!flag)
	{
          files = wild_card(str,1);
	  if (!(i = sizeof(files)))
	    {
	      notify_fail("No such file or directory.\n");
	      return 0;
	    }
          else if (i == 1 && file_size(files[0]) == -2) {
	    files = wild_card(str + "/*");
	    if (!sizeof(files)) {
		notify_fail("Directory is empty.\n");
		return 0;
	    }
          }

	  while (1)
	    {
	      if(file_size(files[0]) <= 0)
		{
		  files = files[1..(sizeof(files) - 1)];
		  notify_fail("All zero length files.\n");
 
	   	  if(!sizeof(files) || files == ({})) {
		  files = 0;  return 0; }
 
		  continue;
		}
 
	      if(!files || !sizeof(files))  return 0;
 
	      if(!mask)
	      hold = "::::::::::::::\n"+files[0]+"\n::::::::::::::\n\n";
	      tmp = files[0];
	      if (sizeof(files) == 1)
		files = ({});
	      else
		files = files[1..(sizeof(files) - 1)];
	      if(!hold)  more(tmp, 1);
	      else {
                if (hold2 = read_file(tmp)) {
                  more(explode(hold, "\n") + explode(hold2, "\n"));
                } else {
                  notify_fail ("*Wrong permissions for opening file " +
                      tmp + " for read.\n");
                  return 0;
                }
	      }
	      return 1;
	    }
	  return 1;
	}
      do_more_file(str);
      previous->done_more();
      return 1;
    }
  else if (pointerp(str)) 
    lines = str;
  else 
    { 
      notify_fail ("Bad argument to more.\n"); 
      return 0; 
    }
  num_lines = sizeof(lines);
  if(!num_lines)  return 1;

  even_more(" ");
  return 1;
}

void
do_more_file(string file)
{
  string tmp;
  more_file = file;
  more_line = 0;
  tmp = read_file (more_file);
  if(!stringp(tmp) || tmp == "") return;    /* Thanks to Zeus */
  lines = explode(tmp,"\n");
  more (lines);
}

varargs void
next_more_file(string arg)
{
    string tmp;

  if (!files || sizeof(files) == 0) {
	if(previous)  previous->done_more();
    return;
    }
  if (!arg)
    {
      write("next file:\n"+
	    ":::::::::::::::::::::::\n"+files[0]+"\n::::::::::::::::::::::\n"+
	    "");
      input_to("next_more_file",1);
      return;
    }
  tmp = files[0];
  if (sizeof(files) == 1)
    files = ({});
  else
    files = files[1..(sizeof(files) - 1)];
  do_more_file(tmp);
}

static void 
even_more(string str) 
{
  int i;
  if (!str) str = " "; 
  str = str[0..0]; 
  switch (str)
    {
    case " ":
      break;
    case "":
#if 0
      // if get_char() ever gets fixed we can enable this part and use it
      if (!this_player()->getenv("getchar")) break;
      write(erase_line());
      if (more_line >= sizeof(lines))
 	return;
      write (lines[more_line]+"\n");
      more_line ++;
      write_more_line();
      input_to ("even_more",0);
      return;
#endif
      break;
    case "b":
    case "B":
      more_line -= chunk * 2;
      if (more_line < 0)
	{
	  more_line = chunk - 1;
	  input_to ("even_more",0);
	  return;
	}
      break;
    case "<":
      more_line = 0;
      break;
    case ">":
      more_line = sizeof(lines) - chunk;
      break;
    case "!":
      write(erase_line() + "!");
      input_to("exec_cmd");
      return;
      break;
    case "/":
      input_to ("search_forward");
      write(erase_line());
      write ("/");
      direction = 1;
      return;
      break;
    case "\\":
      input_to ("search_reverse");
      write(erase_line());
      write ("\\");
      direction = 0;
      return;
      break;
    case "n":
      if (!last_regexp || last_regexp == "")
	{
	  input_to("even_more",0);
	  return;
	}
      write(erase_line());
      if (direction == 1)
	search_forward (last_regexp);
      else
	search_reverse (last_regexp);
      return;
      break;
    case "=":
      write(erase_line());
      write ("" + more_line + " ");
      input_to ("even_more",0);
      return;
      break;
    case "v":
      write(erase_line());
      write ("more for tmi v1.0, written by wayfarer@portals ");
      input_to ("even_more",0);
      return;
      break;
    case "h":
    case "?":
      write(erase_line() + clear_screen());
      write ("------------------------------------------------------------------------------\n" +
	     "                          [more - help page]\n"+
	     "------------------------------------------------------------------------------\n" +
	     "<space>\t\t\tDisplay next page of text.\n"+
	     "B,b\t\t\tDisplay previous page of text.\n"+
	     "<return>\t\tDisplay next page of text.\n"+
	     "<\t\t\tGo to the beginning of the document.\n"+
	     ">\t\t\tGo to the end of the document.\n"+
	     "=\t\t\tDisplay the current line number.\n"+
	     "/\t\t\tRegexp search forward.\n"+
	     "\\\t\t\tRegexp search backward.\n"+
	     "n\t\t\tContinue last regexp search. (maintain direction)\n"+
	     "^L\t\t\tRedraw screen.\n"+
	     "!\t\t\tExecute a command in the mud.\n"+
	     "v\t\t\tPrint the version number.\n" +
	     "h,?\t\t\tPrint this help screen.\n"+
	     "Q,q\t\t\tQuit.\n"+
	     "");
      write_more_line();
      input_to ("even_more",1);
      return;
      break;
    case "":
      write (clear_screen());
      write ( ESC + "[1;1f");
      more_line -= chunk;
      break;
    case "q":
    case "Q":
      write(erase_line());
      write ("\n"); 
        if (previous) previous->done_more(); // Leto
      files = ({ });
      return;
      break;
    default:
      write(erase_line());
      write("Unrecognized command ");
      input_to("even_more",0);
      return;
      break;
    }
  if(more_line) write(erase_line());
  for (i = 0; i < chunk; i++)
    {
      if (more_line >= sizeof(lines))
	{
	  next_more_file();
	  return;
	}
      write (lines[more_line] + "\n");
      more_line++;
 
     if(more_line_pos() >= 100) {  next_more_file();  return;  }
    }
 
  input_to ("even_more",0);
  write_more_line();
  chunk = (int)this_object()->getenv("LINES") ;
  if (stringp(chunk))  chunk = to_int(chunk);
  if (!chunk || !intp(chunk))
      chunk = CHUNK;
  chunk -= 1;
  return;
}


static void
  search_forward (string arg)
{
  string *matches;
  int i;
  
  matches = regexp (lines[more_line .. (sizeof(lines) - 1)],arg);
  if (!matches || (sizeof(matches) == 0))
    {
      write(up_line()+erase_line()+"\n");
      write(inverse("Pattern not found"));
      input_to ("even_more",0);
      return;
    }
  for (i = more_line; i < sizeof(lines); i++)
    {
      if (matches[0] == lines[i])
	{
	  more_line = i;
	  write ("...skipping\n");
	  chunk -= 2;
	  even_more(" ");
	  last_regexp = arg;
	  return;
	}
    }
  return;
}

static void
search_reverse (string arg)
{
  string *matches;
  int i;

  matches = regexp (lines[0 .. (more_line - chunk - 1)],arg);
  if (!matches || (sizeof(matches) == 0))
    {
      write(up_line()+erase_line()+"\n");
      write(inverse("Pattern not found"));
      input_to ("even_more",0);
      return;
    }
  for (i = more_line - chunk - 1; i >= 0; i--)
    {
      if (matches[sizeof(matches) - 1] == lines[i])
	{
	  more_line = i;
	  even_more(" ");
	  last_regexp = arg;
	  return;
	}
    }
  return;
}


static
void
exec_cmd (string arg)
{
  this_player()->force_me(arg);
  write(erase_line()+"\n");
  write_more_line();
  input_to("even_more",0);
}

