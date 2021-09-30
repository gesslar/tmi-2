/*
// File     :  /std/priv.c
// Purpose  :  This is an access checking system that is inherited by
//             both generic objects and file security mechanisms.
// 92-05-31 :  Buddha wrote it
// 94-03-26 :  Pallando added delete_access()
*/


#include <uid.h>
#include <priv.h>

private mapping perms;

static nomask int check_access(mixed what);
nomask int set_access(mixed what, int new_access);
nomask int delete_access(string entry);

// debugging stuff
void debug(string str) {
#ifdef DEBUG
      write(str + "\n");
#endif
}

// start with something pretty basic.
void init_access() {
   if(!mapp(perms)) perms = ([ ]);
}

// O.K., this one returns an integer value based on the security level
// in question, as defined in priv.h.  It gets called by valid_write() and
// valid_read(), and is passed a string such as "obj/equip/sword.c".
// it will return the access for the item, or if that is unavailable, the
   // access for the node of the tree that it's on.

static nomask int check_access(string tmp) {
   int i;
   string *parts;
   
   debug("Passed to check_access: " + tmp);
   init_access();
   debug("past init_access()");
   
   // okay, first check for a direct match.  This will be a lot of 
   // cases for variables, less for files.
   
   if (perms[tmp]) return perms[tmp];
   
   // there was no match, so let's split this string up into segments.
   // If there is only one element in the resulting array, then we
   // know it's been passed already.
   
   parts = explode(tmp, "/");
   
   // question, what to do here?  For file security, it probably 
   // means they are trying to write to /, but for variables it sounds
   // like they are adding a new one.... one is harmless, the other isn't.
   // for now, I'll assume that there is another way to lock the root
   // directory... it may be necessary to add a "/" entry.
   // This is my solution, but I suspect there's a better way where
   // I won't have to do this for all objects.
   
   if (sizeof(parts) == 1) {
      debug("Only one level depth to resolve");
      if (file_name(this_object()) == "adm/newaccess")
         return MASTER_ONLY;
      else return PUBLIC;
   }
   
   // okay, let's look for the closest match we can find for this,
   // starting with things most similar and working towards root.
   
   for (i=sizeof(parts)-1;i>=0;i--) {
      tmp = implode(parts[0..i], "/");
      debug("debug: checking for an entry for " + tmp);
      if(perms[tmp]) {
         debug("Matched " + implode(parts, "/") + " with " + tmp);
         return perms[tmp];
        }
   }
   debug("No match, returning PUBLIC status.");
   return PUBLIC;
}


// This function will take either an access string or an array that
// constitutes a path through a variable tree, and set the access for
// the variable or tree segment specified.
   
nomask int set_access(string entry, int new_access) {
   string eff_user;
   int curr_access;
   
   // first, get the identity of who is trying to change the access.
   
   if (!previous_object()) eff_user = geteuid();
   else eff_user = geteuid(previous_object());
   
#ifdef DEBUG
      write(eff_user + " is trying to change access " + entry + " to " +
      new_access + " for " + file_name(this_object()) + "\n");
#endif
      
   // Only two euid can generally change the permissions on things.
   // the owner, because it makes sense, and Root, just because.
   
   if (eff_user != geteuid(this_object()) && eff_user != ROOT_UID)
      return 0;
   
   // now get it's protection level.
   // then, switch through the various types of protection.
   
   curr_access = check_access(entry);
   if(!intp(curr_access))  curr_access = 0;
   
   switch(curr_access) {
      case PUBLIC:
      case READ_ONLY:
      case OWNER_ONLY:
      perms[entry] = new_access;
      return 1;
      break;
      case LOCKED:
      case PRIVATE:
      return 0;
      break;
      case MASTER_ONLY:
      if (eff_user == ROOT_UID) {
         perms[entry] = new_access;
         return 1;
         }
      return 0;
      break;
      default: 
      // the item isn't even in priv.h - probably an error
      perms[entry] = new_access;
      return 1;
      break;
   }
}

// delete() uses delete_access(entry) rather than set_access(entry,0) because
// that is The Right Thing (tm) to do.
nomask int delete_access(string entry)
{
   string eff_user;

   eff_user = ( previous_object() ? geteuid( previous_object() ) : geteuid() );
   if( eff_user != geteuid() && eff_user != ROOT_UID ) return 0; // Fail.

   map_delete( perms, entry );
}

// Now, we do something that's purpose may look familiar to some of you 
// valid_write() and valid_read() get passed the string that represents
// what is being accessed, and is also passed the object doing the action.
// This way, we can ensure that nothing happens that shouldn't.
// the string passed will look like a file name already, i.e. it will

nomask int valid_write(string what, object act_ob) {
   string eff_user;
   int security;

   if (!what) return 0  ;   /* a hack-o by mobydick to fix a crash */
   if (!act_ob) act_ob = previous_object(); // who is this? ourself?
   if (!act_ob) act_ob = this_player();
   if (!act_ob) { 
      log_file("odd_errors", "no act_ob, /std/priv.c line 149");
      return 0;
   }
   eff_user = geteuid(act_ob);
   security = check_access(what);
   debug("check_access ok, security level " + security + " returned.");
   // Okay, let's check the access situation before going further.
   if(!intp(security))  security = 0;
   switch (security) {
      case PUBLIC :
      return 1; break;
      case READ_ONLY :
      case OWNER_ONLY :
      if (eff_user != geteuid() && eff_user != ROOT_UID)
         return 0; break;
      case MASTER_ONLY :
      case LOCKED :
      case PRIVATE :
      if (eff_user != ROOT_UID) return 0; break;
      default : debug("default case reached."); return 0; break;
   }
   // anything that makes it to this point should be ok, right?
   return 1;
}

nomask int valid_read(string what, object act_ob) { 
   string eff_user;
   int security;
   
   if (!act_ob) return 0;
   eff_user = geteuid(act_ob);
   security = check_access(what);
   debug("check_access ok, security level " + security + " returned.");
   // Okay, let's check the access situation before going further.
   if(!intp(security))  security = 0;
   switch (security) {
      case PUBLIC :
      case READ_ONLY :
      case MASTER_ONLY :
      case LOCKED :
      return 1; break;
      case OWNER_ONLY :
      case PRIVATE :
      if (eff_user != geteuid() && eff_user != ROOT_UID)
         return 0; break;
      default : debug("default case reached."); return 0; break;
   }
   // anything that makes it to this point should be ok, right?
   return 1;
}

// This function just checks with the access system and returns the
// present permissions set on an attribute.

int query_permission(string prop) { return check_access(prop); }

