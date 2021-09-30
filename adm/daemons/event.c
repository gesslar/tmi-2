#pragma save_binary
 
//	File	:   /adm/daemons/event.c
//	Creator	:   Glenn Ferguson (Watcher@TMI)   12/12/92
//
//	System time manager
//	- Allows events to be triggered at preset time settings
//
// Patched check_* and perform_event to the correct type, Leto 120194
// Patched a bit more and fixed security hole in query_stack, Leto 16121994
// And a bit more. It seems the event cmd needs the original
// array, don't ask me why. So, we give it. Leto 221294

#include <mudlib.h>
#include <uid.h>
#include <commands.h>
#include <logs.h>
#include <driver/origin.h>

inherit DAEMON ;

#define SAVE_FILE	"/data/adm/daemons/event"	// Save data file
 
#define INTERVAL	60			// Time check interval in sec.
#define IGNORE		900			// Remove event after X seconds
#define HOUR_DIFF	3600			// Seconds in an hour
#define DAY_DIFF	(24 * HOUR_DIFF)	// Seconds in a day
#define WEEK_DIFF	(7 * DAY_DIFF)		// Seconds in a week
#define SETUP_ARRAY	({ })			// Base storage array
 
static void load_events();
static void save_events();
static void ERROR(string mesg, string err);
static void check_once();
static void check_hourly();
static void check_daily();
static void check_weekly();
static void perform_event(string access, string file, string func, string arg);
static int sort_stack(mixed *event1, mixed *event2);
 int add_event(string type, mixed *event);
 int remove_event(string type, mixed *event);
static int initiate_sweep();
mixed *query_stack(string type);
 
mixed *HOURLY, *DAILY, *WEEKLY, *ONCE;

create() {

	seteuid(getuid(this_object()));		// Setup save permissions
	
	HOURLY = SETUP_ARRAY;				
	DAILY = SETUP_ARRAY;			// Setup event arrays
	WEEKLY = SETUP_ARRAY;
	ONCE = SETUP_ARRAY;
 
	load_events();				// Load previous saved events
	initiate_sweep();			// Start time check loop
 
}
 
 
// Load preset event stacks from save file
 
static void load_events() {
   string err;
 
   err = catch( restore_object(SAVE_FILE) );
   if(err) {  ERROR("Could not load data file", err);  return;  }
 
}
 
 
// Save present event stacks into save file
 
static void save_events() {
   string err;
 
   err = catch ( save_object(SAVE_FILE) );
   if(err) {  ERROR("Could not save data file", err);  return;  }

}
 
 
// Stack accessing error output to error file with time stamp
 
static void ERROR(string mesg, string err) {
 
   write_file(EVENT_LOG, mesg + "\t[" + extract(ctime(time()),4,15) + "]\n");
   
   if(err && err != "")
   write_file(EVENT_LOG, " " + err + "\n\n");
   else write_file(EVENT_LOG, "\n");

return;  }
 
 
// Cycling stack check function at INTERVAL intervals
 
static int initiate_sweep() {

   remove_call_out("initiate_sweep");
   call_out("initiate_sweep", INTERVAL);	// Set next time check
 
   check_once();				// Perform once stack check
   check_hourly();				// Perform hourly stack check
   check_daily();				// Perform daily stack check
   check_weekly();				// Perform weekly stack check
 
return 1;  }
 
//  Event stack processing functions - Called by initiate_sweep()
 
static void check_daily() {
   mixed *tmp;
   int loop;
 
   if(DAILY == SETUP_ARRAY)  return ;		// No events stored
 
   while( DAILY[loop] ) {
   tmp = DAILY[loop];				// Temp. store event data
 
   if(time() < DAILY[loop][0])  return ;	// Check again next interval
 
   if(time() - DAILY[loop][0] <= IGNORE)
      perform_event(DAILY[loop][4], DAILY[loop][1], DAILY[loop][2],
		    DAILY[loop][3]);

   // Remove present event, then reschedule it in the stack
   // in a day's time with same parameters.
 
   DAILY = exclude_array(DAILY, loop);
if(!add_event("daily", ({ tmp[0]+DAY_DIFF, tmp[1], tmp[2], tmp[3], tmp[4] }) )) write("Event error1\n");
   save_events();
 
   if(DAILY == SETUP_ARRAY) break;
   }
 
return ; }
 
static void check_hourly() {
   mixed *tmp;
   int loop;
 
   if(HOURLY == SETUP_ARRAY)  return ;		// No events stored

   while( HOURLY[loop] ) {
   tmp = HOURLY[loop];
 
   if(time() < HOURLY[loop][0])  return ;	// Check again next interval
 
   if(time() - HOURLY[loop][0] <= IGNORE)
      perform_event(HOURLY[loop][4], HOURLY[loop][1], HOURLY[loop][2],
		    HOURLY[loop][3]);
 
   // Remove present event, then reschedule it in the stack
 
   HOURLY = exclude_array(HOURLY, loop);
catch(add_event("hourly", ({tmp[0]+HOUR_DIFF, tmp[1], tmp[2], tmp[3], tmp[4]})));
   save_events();
 
   if(HOURLY == SETUP_ARRAY) break;
   }
 
return ; }
 
static void check_weekly() {
   mixed *tmp;
   int loop;
 
   if(WEEKLY == SETUP_ARRAY)  return ;		// No events stored
 
   while( WEEKLY[loop] ) {
   tmp = WEEKLY[loop];
 
   if(time() < WEEKLY[loop][0])  return ;	// Check again next interval
 
   if(time() - WEEKLY[loop][0] <= IGNORE)
      perform_event(WEEKLY[loop][4], WEEKLY[loop][1], WEEKLY[loop][2],
		    WEEKLY[loop][3]);
 
   // Remove present event, then reschedule it in the stack
   // in a week's time with same parameters.
 
   WEEKLY = exclude_array(WEEKLY, loop);
if(!add_event("weekly", ({ tmp[0]+WEEK_DIFF, tmp[1], tmp[2], tmp[3], tmp[4] }))) write("Event error3\n");
   save_events();
 
   if(WEEKLY == SETUP_ARRAY) break;
   }
 
return ; }
 
static void check_once() {
   int loop;
 
   if(ONCE == SETUP_ARRAY)  return ;		// No events stored
 
   while( ONCE[loop] ) {
 
   if(time() < ONCE[loop][0])  return ;	// Check again next interval

   if(time() - ONCE[loop][0] <= IGNORE)
     perform_event(ONCE[loop][4], ONCE[loop][1], ONCE[loop][2], ONCE[loop][3]);

   // Remove event from the stack and procede to next event
 
   ONCE = exclude_array(ONCE, loop);
   save_events();
 
   if(ONCE == SETUP_ARRAY) break;
   }
 
return ; }
 
 
// Perform preset event on remote object
 
static void perform_event(string access, string file, string func, string arg) {
   object what;
   string permis, err;
 
   // Attempt to locate requested target file
 
   err = catch( call_other(file, "???") );
 
   if(!err)  what = find_object(file);
 
   if(err || !what) {
   ERROR("Event error:  Could not load " + file, err);
   return ; }
 
   // Get user's command permission string
 
   permis = geteuid(what);
 
   // If attempting to access ROOT object without ROOT permissions,
   // block and log illegal access attempt
 
   if(permis == ROOT_UID && access != ROOT_UID &&
      !adminp(access)) {
   ERROR("Event error:  Illegal root access attempt by " + access,
	 "  " + func + "(" + arg + ")  on  " + file);
   return ; }
 
   // Perform stored function call on specified object
 
   err = catch( call_other(file, func, arg) );

   if(err) ERROR("Event error: " + file + " " + func + "(" + arg + ")", err);

return ; }
 
 
//  Add new event to a specified event stack
 
 int add_event(string type, mixed *event) {
   mixed *tmp;
   int loop, flag;
 
   // Check accessing permissions ... ensure root command
 
  //  if(geteuid(previous_object()) != ROOT_UID &&
  //  previous_object() != this_object())       return 0;
 
    if (previous_object()) {
      if(geteuid(previous_object()) != ROOT_UID) return 0; }
  else if( origin()!=ORIGIN_LOCAL) return 0;
   if(sizeof(event) != 5)  return 0;		// Invalid event array
 
   // Store specific stack into a temporary array
 
   if(type == "once")		tmp = ONCE;
   else if(type == "hourly")	tmp = HOURLY;
   else if(type == "daily")	tmp = DAILY;
   else if(type == "weekly")	tmp = WEEKLY;
   else return 0;				// Invalid type format
 
   //  Add new event to stack, then sort events in increasing time
 
   tmp += ({ event });
   tmp = sort_array(tmp, "sort_stack", this_object());
 
   // Replace updated event arrays from tmp
 
   if(type == "once")		ONCE = tmp;
   else if(type == "hourly")	HOURLY = tmp;
   else if(type == "daily")	DAILY = tmp;
   else if(type == "weekly")	WEEKLY = tmp;
 
   save_events();				// Save updated event stack
 
return 1; }
 
 
//  Event stack sorting routine 
 
static int sort_stack(mixed *event1, mixed *event2) {
  return ( event1[0] - event2[0] );  }
 
 
//  Event removal from specific event stack
 
 int remove_event(string type, mixed *event) {
   mixed *tmp;
   int pos;
 
   // Check accessing permissions ... ensure root command
 
   if(geteuid(previous_object()) != ROOT_UID)	return 0 ;
 
   // Store stack array into temporary array
 
   if(type == "once")		tmp = ONCE;
   else if(type == "hourly")	tmp = HOURLY;
   else if(type == "daily")	tmp = DAILY;
   else if(type == "weekly")	tmp = WEEKLY;
   else return 0 ;
 
   pos = member_array(event, tmp);
   if(pos == -1)  return 0 ;			// No such event found
 
   tmp -= ({ event });
 
   // Replace updated array back into stack
 
   if(type == "once")		ONCE = tmp;
   else if(type == "hourly")	HOURLY = tmp;
   else if(type == "daily")	DAILY = tmp;
   else if(type == "weekly")	WEEKLY = tmp;
 
   save_events();
 
return 1 ; }
 
 
// External stack request query for event command
 
mixed *query_stack(string type) {

if(base_name(previous_object())!=CMD_EVENT) {

    if(type == "once") return copy(ONCE);
    else if(type == "hourly") return copy(HOURLY);
    else if(type == "daily") return copy(DAILY);
    else if(type == "weekly") return copy(WEEKLY);
    else if(type == "all")	
return ({ copy(ONCE), copy(HOURLY), copy(DAILY), copy (WEEKLY) });
		return ({});
     }
else {
    if(type == "once") return ONCE;
    else if(type == "hourly") return HOURLY;
    else if(type == "daily") return DAILY;
    else if(type == "weekly") return WEEKLY;
    else if(type == "all")	
return ({ ONCE, HOURLY, DAILY, WEEKLY });
  return ({});
		 }
} 
