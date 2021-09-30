/*
 *    /www/counter_src.c
 */
#include <mudlib.h>
#include <config.h>
#include <daemons.h>
#include <commands.h>
#include <net/http.h>

inherit DAEMON;

string gateway() {
    return sprintf(@ENDTEXT
<HTML><HEAD><TITLE>LPC COUNTER SOURCE</TITLE></HEAD>
<BODY><H1>Source for the Counter</H1>
<LISTING>%s</LISTING>
</BODY></HTML>
ENDTEXT
, read_file(DIR_WWW "/counter.c"));
}
