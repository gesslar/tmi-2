/*
 * Check your login from site is "approved" by you.
 * Put this in your .login.  For added insurance, add ip numbers as well
 * (to fall back on if addr_server fails).
 *
 * 1994.05.07 [robo] created
 * 1994.05.11 [inspie] rewrote it to add wildcards
 * 1994.05.11 [robo] reformatted it, added some comments, made some changes, etc
 * 1994.05.13 [robo] inspie's idea to allow wildcard in suffix portion of
 *                   a specified site name; made this optional via -w
 * 1994.05.15 [robo] pallando's idea to test site list via -t;
 *                   some minor code changes
 */

#include <config.h>
#include <mudlib.h>
#include <logs.h>
#ifndef SITECHECK_LOG
#define SITECHECK_LOG "sitecheck.err"
#endif

inherit DAEMON;

static int destruct_flag, test_flag, wildcard_flag;
static string site, site_num;

#define SYNTAX "Syntax: sitecheck [-dtw] site [sites...]\n"

string help() {
    return(SYNTAX "\n" @ENDHELP
This command (placed in your ~.login file) checks your login is from
a site "approved" by you.  Site check failures are logged, and if the
"-d" option is specified, the caller's connection is dropped.

For added safety: in the event addr_server is unable to resolve the
caller's site (host ip/domain name), you'll want to add ip addresses
(ip numbers) to the site list.

If the "-w" option is given, this command will allow more liberal wildcard
usage in the specified site list, eg the suffix portion of a site name, or
the first three octets of an ip address.  Extreme caution is advised when
using this option.  Examples:  *.240.120.*  calvin.*  hobbes.*

The "-t" option is for testing and overrides the "-d" switch.
ENDHELP
    );
}

/*
 * compare dot portions (address components) of ip number (dotted decimal/
 * dotted quad) with an ip number pattern (ie containing wildcard(s))
 * - returns 1 if a match, 0 otherwise
 */
int dot_match(string *site, string *pattern, int flag) {
    int i, j;

    j = sizeof(pattern);
    if (j != 4)
        return 0;

    while (j--) {
        if (flag) {
            /*
             * match any octect
             */
            if (pattern[j] == "*")
                continue;
        } else {
            /*
             * be strict (last octect only)
             */
            if (j == 3 && pattern[3] == "*")
                continue;
        }

        /*
         * component doesn't match
         */
        if (site[j] != pattern[j])
            return 0;
    }

    /*
     * by process of elimination...it must match
     */
    return 1;
}

static int exit(int ok) {
    if (test_flag) {
        write(sprintf("Site check test %s: %s %s\n",
            ok ? "passed" : "failed",
            site ? site : "",
            site_num ? site_num : ""));
    } else {
        if (!ok) {
            /*
             * site not listed
             */
            log_file(SITECHECK_LOG,
                  sprintf("Site check failed: %s@%s [%s]\n",
                  geteuid(previous_object()), site ? site : site_num,
                  ctime(time())));

            if (destruct_flag) {
                previous_object()->remove();
                if (previous_object())
                    destruct(previous_object());
            }
        }
    }

    return 1;
}

int cmd_sitecheck(string arg) {
    string *sites;
    object ob;
    int i, s, l1, l2;
    string *site_dots, *match_dots;

    /*
     * if no sites listed, just return
     */
    if (!arg || arg == "")
        return 1;

    /*
     * initialize vars
     */
    site = 0; site_num = 0;
    destruct_flag = 0;
    test_flag = 0;
    wildcard_flag = 0;

    /*
     * parse command line args
     */
    arg = replace_string(arg, ",", " ");
    sites = explode(lower_case(arg), " ");

    /*
     * check for options
     */
    if (sites[0][0] == '-') {
        if (strsrch(sites[0], 'd') != -1)
            destruct_flag = 1;
        if (strsrch(sites[0], 't') != -1)
            test_flag = 1;
        if (strsrch(sites[0], 'w') != -1)
            wildcard_flag = 1;
    }

    /*
     * check list of accepted sites
     */
    ob = previous_object()->query_link();
    if (!ob)
        ob = previous_object();
    if (ob) {
        site = query_ip_name( ob );
        if (!site)
            site = query_ip_name( previous_object() );
        if (site) {
            /*
             * 1) check for perfect match
             */
            site = lower_case(site);
            if (member_array(site, sites) != -1)
                return exit(1);

            /*
             * 2) check ip number (from ip name)
             */
            if (sscanf(site, "%*d.%*d.%*d.%*d") != 4) {
                site_num = query_ip_number(ob);
                if (!site_num)
                    site_num = query_ip_number(previous_object());
                if (member_array(site_num, sites) != -1)
                    return exit(1);
            } else {
                site_num = site;
                site = 0;
            }

            /*
             * LAST: loop through wildcards in sites[]
             *   Note: sites[] is altered
             */
            if (strsrch(arg, '*') != -1) {
                /*
                 * here's a quick filter :)
                 */
                sites = regexp(sites, "\\*");
                if (sites && (s = sizeof(sites))) {
                    /*
                     * check site ip name;
                     * default only allows '*' as a prefix, ie "*.domain.name"
                     */
                    if (site) {
                        l1 = strlen(site);
                        i = s;
                        while (i--) {
                            l2 = strlen(sites[i]) - 1;
                            if (l2 > 1 && l1 > l2 &&
                                  sites[i][0..1] == "*." &&
                                  strcmp(site[l1-l2..<1], sites[i][1..<1]) == 0)
                                return exit(1);
                        }

                        if (wildcard_flag) {
                            /*
                             * handle '*' as suffix
                             */
                             i = s;
                             while (i--) {
                                 l2 = strlen(sites[i]) - 2;
                                 if (l2 > 0 && l1 > l2 &&
                                      sites[i][l2..<1] == ".*" &&
                                      strcmp(site[0..l2], sites[i][0..l2]) == 0)
                                    return exit(1);
                             }
                        }
                    }

                    /*
                     * check site ip number
                     */
                    if (site_num) {
                        site_dots = explode(site_num, ".");
                        i = s;
                        while (i--) {
                            match_dots = explode(sites[i], ".");
                            if (dot_match(site_dots, match_dots, wildcard_flag))
                                return exit(1);
                        }
                    }
                }
            }

            return exit(0);
        }
    }

    return 1;
}
