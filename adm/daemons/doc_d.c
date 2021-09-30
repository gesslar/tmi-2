// A daemon to read the documentation directories into a mapping,
// and store for future reference (saves many disk accesses
// during runtime).  Interfaces with the new 'help' command, or
// will do when it's finished.
// Blue@TMI-2, 941018.

// To do: integrate command help with CMD_D
//        re-add facility for 'help <topic>' somehow.

#include <config.h>
#include <man.h>
#include <uid.h>
#include <daemons.h>
#include <help.h>

#define SEPARATOR '.'
#define DESC_FILE_NAME ".description"

// #define DEBUG(x) tell_object(find_player("blue"), x+"\n");
#define DEBUG(x)

inherit SAVE_D ;

void create();
void rehash_all();
void rehash_dirs(string *dirs_to_do);
string *note_files(mixed *dir_contents,
    string base_dir, string description);
void add_file(string name, string base_dir, string description);
int ignoredoc(string name);
string nameof(string name);
string *get_docs_on(string name, string *paths);
mapping get_all_the_docs();
string *command_docs(string name, string *paths);
string get_desc(string path);

mapping all_the_docs;
int last_rehash;

void
create() {
    if (!restore_data())
        rehash_all();
}

void
rehash_all() {
    DEBUG("rehash_all started.")
    all_the_docs = ([ ]);
    seteuid(getuid());
    rehash_dirs( ({ DOC_DIR }) );
}

void
rehash_dirs(string *dirs_to_do) {
    mixed *dir_contents;
    string description;

    DEBUG("Dirs_to_do: "+identify(dirs_to_do)+write("DEBUG EXECUTED\n"))
    description = get_desc(dirs_to_do[0]);
    dir_contents = get_dir(dirs_to_do[0], -1);
    DEBUG("dir_contents: "+identify(dir_contents))
    dir_contents = note_files(dir_contents, dirs_to_do[0], description);
    DEBUG("dir_contents: "+identify(dir_contents))
    dir_contents += dirs_to_do[1..sizeof(dirs_to_do)-1];
    DEBUG("dir_contents: "+identify(dir_contents))
    if (sizeof(dir_contents)) {
        call_out("rehash_dirs", 1, dir_contents);
    }
    else {
        last_rehash = time();
        save_data();
    }
}

string *
note_files(mixed *dir_contents, string base_dir, string description) {
    int i, sz;
    string *new_contents;

    sz = sizeof(dir_contents);
    new_contents = ({ });
    for (i=0;i<sz;i++) {
        if (dir_contents[i][1]==-2) {
            new_contents+=({ base_dir + dir_contents[i][0]+"/" });
        }
        else {
            add_file(dir_contents[i][0], base_dir, description);
        }
    }
    return new_contents;
}


void
add_file(string name, string base_dir, string description) {
    string shortname;

    shortname = nameof(name);
    if (ignoredoc(name)) return;
    if (!all_the_docs[shortname]) {
        all_the_docs[shortname] = ({ description, base_dir+name });
    }
//  else if (stringp(all_the_docs[name])) {
//      all_the_docs[name] = ({ all_the_docs[name], description });
//  }
    else if (pointerp(all_the_docs[shortname])) {
//      all_the_docs[name] += ({ description });
        all_the_docs[shortname] += ({ description, base_dir+name });
    }
// if none of these, something's wrong.
    else {
        log_file("doc_d_errors", name+": "+identify(all_the_docs["name"])+"\n");
    }
}

int
ignoredoc(string name) {
    if (name==DESC_FILE_NAME) return 1;
//  if (name=="README") return 1;
    return 0;
}

string
nameof(string name) {
    int sepoint;
    sepoint = strsrch(name, SEPARATOR, -1);
    if (sepoint==-1) return name;
    return name[0..sepoint-1];
}

string *
get_docs_on(string name, string *paths) {
    string *a;
    string *b;
    a = copy(all_the_docs[name]);
    b = command_docs(name, paths);
    if (!a) return b;
    if (!b) return a;
    return a + b;
}

mapping
get_all_the_docs() {
    return copy(all_the_docs);
}

string *
command_docs(string name, string *paths) {
    string *names;
    int i, j;
    names = ({ });
    j = sizeof(paths);
    for (i=0;i<j;i++) {
        if (file_size(sprintf("%s/_%s.c", paths[i], name)) > 0) {
            names += ({ get_desc(paths[i]), sprintf("%s/_%s", paths[i], name) });
        }
    }
    return names;
}

string get_desc(string path) {
    string description;

    if (!(description = read_file(sprintf("%s/%s", path, DESC_FILE_NAME))))
        description = sprintf("%s\n", path);
    return description;
}

