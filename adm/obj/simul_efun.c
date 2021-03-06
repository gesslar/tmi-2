/*
// File:       simul_efun.c
// Purpose:    This object is loaded at boot time.
//             Functions defined in this object can be called by any object in
//             the game in the same way efuns are called, and are said to be
//             simulated efuns (or simul_efuns for short).
// History:
// 92-02-19    Written by Buddha@TMI
//             This file is part of the TMI distribution mudlib.
//             Please retain this header if you have used this file.
// 93-07-20    Re-organised and commented by Pallando@TMI-2
//             If a simul_efun calls another simul_efun from its definition
//             it must be included in this file after the one it uses.
*/

// Note: most simul_efuns ought to #include what they need, so that they
// can be loaded as independent modules for testing.  In order to prevent
// redefinition errors, it's wise to define a symbol (ex.: __CONFIG_H) in
// the include file, and check if it's defined before defining anything
// new.  See config.h for an example.
#include <mudlib.h>
#include <uid.h>
#include <config.h>
#include <writef.h>

// this is first because all_caps.c needs it.
#include "/adm/simul_efun/base_name.c"
#include "/adm/simul_efun/compat09182.c"

// Simul_efuns that do not interact with other simul_efuns.
#include "/adm/simul_efun/adminp.c"
#include "/adm/simul_efun/all_caps.c"
#include "/adm/simul_efun/article.c"
#include "/adm/simul_efun/atoi.c"
/*
#include "/adm/simul_efun/copy.c"
*/
#include "/adm/simul_efun/creator_file.c"
#include "/adm/simul_efun/domain_master.c"
#include "/adm/simul_efun/dump_socket_status.c"
#include "/adm/simul_efun/dump_variable.c"
#include "/adm/simul_efun/emote.c"
#include "/adm/simul_efun/exclude_array.c"
#include "/adm/simul_efun/existence.c"
#include "/adm/simul_efun/find_object_or_load.c"
#include "/adm/simul_efun/format_string.c"
#include "/adm/simul_efun/format_time.c"
#include "/adm/simul_efun/gettype.c"
#include "/adm/simul_efun/get_char.c"
#include "/adm/simul_efun/identify.c"
#include "/adm/simul_efun/get_stack.c"
#include "/adm/simul_efun/idle.c"
#include "/adm/simul_efun/index.c"
#include "/adm/simul_efun/int_string.c"
#include "/adm/simul_efun/mail_package.c"
#include "/adm/simul_efun/match_string.c"
// #include "/adm/simul_efun/merge.c"
#include "/adm/simul_efun/path_file.c"
#include "/adm/simul_efun/pronouns.c"
#include "/adm/simul_efun/say.c"
#include "/adm/simul_efun/shout.c"
#include "/adm/simul_efun/slice_array.c"
//#include "/adm/simul_efun/substr.c"
#include "/adm/simul_efun/system.c"
#include "/adm/simul_efun/tail.c"
#include "/adm/simul_efun/tc.c"
#include "/adm/simul_efun/tell_room.c"
#include "/adm/simul_efun/tilde_path.c"
#include "/adm/simul_efun/uniq_array.c"
#include "/adm/simul_efun/unique_mapping.c"
#include "/adm/simul_efun/unart_indefinite.c"
#include "/adm/simul_efun/un_article.c"
#include "/adm/simul_efun/un_pluralize.c"
#include "/adm/simul_efun/update_file.c"
#include "/adm/simul_efun/vt100.c"
#include "/adm/simul_efun/wrap.c"
#include "/adm/simul_efun/iwrap.c"
// #include "/adm/simul_efun/getopt.c"

// Simul_efuns that are called by other simul_efuns.
// Called by: data get_object overrides
#include "/adm/simul_efun/getoid.c"
// Called by: temp_file
#include "/adm/simul_efun/hiddenp.c"
// Called by: visible
#include "/adm/simul_efun/member_group.c"
// Called by: overrides visible
//#include "/adm/simul_efun/tell_object.c"
// Called by: overrides tell_group
#include "/adm/simul_efun/user_path.c"
// Called by: resolv_path
//#include "/adm/simul_efun/write.c"
// Called by: data overrides resolv_path tell_group writef

// Simul_efuns that are called by and call other simul_efuns.
#include "/adm/simul_efun/resolv_path.c"
// Calls: write user_path Called by: get_object

// Simul_efuns that call other simul_efuns.
#include "/adm/simul_efun/data.c"
// Calls: write base_name 
#include "/adm/simul_efun/domain_log_file.c"
// Calls: log_file
#include "/adm/simul_efun/get_object.c"
// Calls: resolv_path base_name
#include "/adm/simul_efun/overrides.c"
// Calls un_article
#include "/adm/simul_efun/pluralize.c"
// Calls: write tell_object member_group base_name
#include "/adm/simul_efun/tell_group.c"
// Calls: write tell_object 
#include "/adm/simul_efun/temp_file.c"
// Calls: getoid 
#include "/adm/simul_efun/visible.c"
// Calls: member_group hiddenp 
#include "/adm/simul_efun/writef.c"
// Calls: write 

