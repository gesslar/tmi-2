//  File    : /cmds/xtra/_event.c
//  Creator : Watcher@TMI  (12/14/92)
//
//  Command which allows users to input scheduled events to the
//  central event daemon for future initiation.

#include <mudlib.h>
#include <daemons.h>
#include <uid.h>
#include <driver/localtime.h>

inherit DAEMON;
inherit TIME_D;

#define SECS_IN_DAY     (60*60*24)
#define YEARS_IN_GOOGLE 4   /* google is a 4 year period including leap year */
#define DAYS_IN_GOOGLE  (4*365+1)
#define SECS_IN_MIN     60
#define MIN_IN_HOUR     60
#define HOURS_IN_DAY    24
#define SECS_IN_HOUR    (60*60)

// maximum earliness (seconds in advance) by which to schedule new events
// In this example, we choose a month
#define MAX_EARLY_SCHED   ( 31 * SECS_IN_DAY )

#define TYPES   ({ "once", "hourly", "daily", "weekly" })

#define MONTHS  ({ "Jan", "Feb", "Mar", "Apr", "May", "Jun", \
                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" })

#define DAYS    ({ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 })

#define SYNTAX \
        "Syntax: event [command] [stack]\n\n" + \
        "        event add       [once/hourly/daily/weekly]\n" + \
        "        event add root  [once/hourly/daily/weekly] (admin only)\n" + \
        "        event remove    [once/hourly/daily/weekly]\n" + \
        "        event list      [once/hourly/daily/weekly]\n"

static int list_stack(string type);
static int add_event(string type, int root);
static int rem_event(string type);
static int root_check(string perm);

int busy;

int cmd_event(string str) {
    string type, form;
    int root;

    notify_fail( SYNTAX );
    if (!str || str == "")
        return 0;

    // Check busy flag to see if the daemon is presently in use
    if (busy) {
        write("Sorry, the event daemon is busy right now.\n" +
              "Please try again in a few minutes.\n");
        return 1;
    }

    if (sscanf(str, "add %s", type) == 1) {
        form = "add";

        // Check to see if root permission settings are requested
        if (sscanf(type, "root %s", type) == 1) {
            if (!root_check(geteuid(this_player()))) {
                write("Only admin may set root permission events.\n");
                return 1;
            }
            root = 1;
        }

        if (member_array(type, TYPES) == -1)
            return 0;
    } else if (sscanf(str, "remove %s", type) == 1) {
        form = "remove";
        if (member_array(type, TYPES) == -1)
            return 0;
    } else if (str == "list") {
        form = "list";
    } else if (sscanf(str, "list %s", type) == 1) {
        form = "list";
        if (member_array(type, TYPES) == -1)
            return 0;
    } else
        return 0;

    if (form == "list")
        return list_stack(type);
    else if (form == "add")
        return add_event(type, root);
    else if (form == "remove")
        return rem_event(type);

    return 0;
}


static int root_check(string perm) {
    if (perm == ROOT_UID || adminp(perm))
        return 1;

    return 0;
}


static int list_stack(string type) {
    mixed *stack;
    int loop, s;

    if (!type || type == "") {            // Check all stacks
        write("Event Stack status:\n");

        s = sizeof(TYPES);
        for (loop = 0; loop < s; loop++) {
            stack = EVENT->query_stack(TYPES[loop]);
            if (!stack || stack == ({ }))
                write(sprintf(" The \"%s\" stack is presently empty.\n",
                      TYPES[loop]));
            else
                write(sprintf(" The \"%s\" stack has %d events queued.\n",
                      TYPES[loop], sizeof(stack)));
        }

        return 1;
    }

    // Read in particular event stack for element check and display
    stack = EVENT->query_stack(type);

    if (!stack || stack == ({ })) {
        write(sprintf(" The \"%s\" stack is presently empty.\n", type));
        return 1;
    }

    // Display queued contents of selected event stack
    write(sprintf(" The \"%s\" stack presently contains:\n", type));

    s = sizeof(stack);
    for (loop = 0; loop < s; loop++)
        if (atoi(stack[loop][3]))
            write(wrap(" " + (loop + 1) + "] " + stack[loop][2] + "(" +
                  stack[loop][3]+ ") on " + stack[loop][1] + " by " +
                  capitalize(stack[loop][4]) + " [" +
                  extract(ctime(stack[loop][0]), 4, 15) + "]\n"));
        else
            write(wrap(" " + (loop + 1) + "] " + stack[loop][2] + "(\"" +
                  stack[loop][3] + "\") on " + stack[loop][1] + " by " +
                  capitalize(stack[loop][4]) + " [" +
                  extract(ctime(stack[loop][0]), 4, 15) + "]\n"));
    return 1;
}


static int rem_event(string type) {
    mixed *stack;

    // Read in particular event stack for element check

    stack = EVENT->query_stack(type);

    if (!stack || stack == ({ })) {
        write(sprintf(" The \"%s\" stack is presently empty.\n", type));
        return 1;
    }

    busy = 1;                    // Set busy flag

    // Display selected stack and request deletion event number
    list_stack(type);
    write("\nDelete which event? [num]  ");
    input_to("del_number", 0, type);

    return 1;
}


static int del_number(string str, string type) {
    mixed *stack;
    int num;

    // Read in selected stack for removal processing
    stack = EVENT->query_stack(type);
    num = atoi(str);

    // Check to see if requested deletion is within event bounds
    if (num < 1 || num > sizeof(stack)) {
        write("There is no such event available for removal.\n");
        busy = 0;
        return 1;
    }

    // Check to see if permissions match or are ROOT to allow deletion
    if (!root_check(geteuid(this_player())) &&
          geteuid(this_player()) != stack[num - 1][4]) {
        write("You do not have the proper permissions to remove that event.\n");
        busy = 0;
        return 1;
    }

    // Tell event daemon to remove selected event
    if (!EVENT->remove_event( type, stack[num - 1] )) {
        write("Error:  Unable to remove requested event.\n");
    } else {
        write(sprintf("Event %d removed from \"%s\" event stack.\n", num, type));
    }

    busy = 0;
    return 1;
}


static int add_event(string type, int root) {
    // Set busy flag on
    busy = 1;

    write("\nInput [target filename],[function],[optional argument]\n: ");
    input_to("receive_event", 0, type, root);

    return 1;
}


static int receive_event(string str, string type, int root) {
    object target;
    string obj, func, arg;

    if (sscanf(str,"%s, %s, %s", obj, func, arg) != 3 &&
          sscanf(str,"%s,%s,%s", obj, func, arg) != 3 &&
          sscanf(str,"%s,%s", obj, func) != 2) {
        write("\nIllegal event addition format.\n");
        busy = 0;
        return 1;
    }

    if (catch( call_other(obj, "???") )) {
        write(sprintf("Error: Could not access %s\n", obj));
        busy = 0;
        return 1;
    }

    target = find_object(obj);

    if (!target) {
        write(sprintf("Error: Could not access %s\n", obj));
        busy = 0;
        return 1;
    }

    if (geteuid(target) == ROOT_UID && !root_check(geteuid(this_player()))) {
        write("You have insufficient permissions to access that object.\n");
        busy = 0;
        return 1;
    }

    write(sprintf("\nCurrent time is: %s GMT\n\n" 
                  "Input activation time [Format: mm/dd/yyyy 24:00]\n: ",
                  ctime(time())));
    input_to("process_time", 0, type, root, obj, func, arg);

    return 1;
}


static int process_time(string str, string type, int root, string obj,
            string func, string arg) {
    string when, permit, target;
    int date, month, day, year, hour, minute;
    int i, yd, googles, ry;
    mixed *ltime;

    if (!str || sscanf(str,"%d/%d/%d %d:%d",month,day,year,hour,minute) != 5) {
        write("Illegal activation time format.\n");
        busy = 0;
        return 1;
    }

    // a 20th century hack
    if (year < 100)
        year += 1900;

    // Discard requests outside standard time/date bounds
    if (month < 1 || month > 12 || day < 1 || day > DAYS[month-1] ||
          minute < 0 || minute > 59 ||
          hour < 0 || hour > 24 || year < 1992 ||
          (month == 2 && day == 28 &&
                !(year % 4 == 0 && (year % 100 > 0 || year % 400 == 0)))) {
        write("Illegal activation time format.\n");
        busy = 0;
        return 1;
    }

    // Set user's permission string...
    // ...but if root is requested, set it
    permit = root ? ROOT_UID : geteuid(this_player());

    // Convert user's date/time input specification to time_t (int)
    // BTW I don't think this code will work in/after the year 2100 AD
    // Code for mktime() follows:

    // First, make some adjustments
    month--;
    year -= 1900;

    // Then, take care of time-of-day
    date = (SECS_IN_MIN * minute) +
           (SECS_IN_HOUR * hour) +
           (SECS_IN_DAY * (day - 1));

    // Next, calculate the number of days already past in the current year
    for (i = 0, yd = 0; i < month; i++) {
        yd += DAYS[i];
        if (i == 1 && (year & 3) == 0)
            yd++; // add an extra day for leap year
    }
    date += yd * SECS_IN_DAY;

    googles = (year - 70) / YEARS_IN_GOOGLE;
    date += googles * DAYS_IN_GOOGLE * SECS_IN_DAY;

    // Calculate the number of days since the last google
    ry = 70 + YEARS_IN_GOOGLE * googles;
    for (i = ry, yd = 0; i < year; i++) {
        yd += 365;
        if ((i & 3) == 0)
            yd++;
    }
    date += yd * SECS_IN_DAY;

    // Lastly, make adjustment for local timezone from GMT
    ltime = localtime(time());
    date -= ltime[LT_GMTOFF];

    if (date > (time() + MAX_EARLY_SCHED)) {
        write("That date is too far in advance of present date.\n");
        busy = 0;
        return 1;
    }

    if (date <= time()) {
        write("That time has already gone past. Pick a time in the future.\n");
        busy = 0;
        return 1;
    }

    if (!EVENT->add_event(type, ({ date, obj, func, arg, permit }))) {
        write("Error: Could not add requested event.\n");
    } else {
        write(sprintf("\nEvent added to the \"%s\" event stack.\n", type));
    }

    busy = 0;
    return 1;
}


int help() {
  write(SYNTAX "\n" @ENDTEXT
This command allows a user to add or remove an event into one of four
event stacks (once, hourly, daily, weekly).  If an event is scheduled
in the "once" stack, then it will occur once at its preset time,
then is removed from the stack.  The other three stacks involve the
event being called at its scheduled time, then rescheduled to the next
time slot corresponding to that particular stack.  Individual stacks
or the entire stack set can be listed using the "list" parameter.

Only the scheduler, or an admin, can remove an event from a stack,
and only an admin may set an event to root permissions.
ENDTEXT
    );
  return 1;
}
