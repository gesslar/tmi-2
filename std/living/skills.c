// This is your basic skill package.
// It should be kept simple so as not to confuse new coders.
// written by Deishu@TMI (a.k.a. Buddha@TMI) on 6-13-92
// revised so that it might work on 9-20-92 by Buddha
// Skill improvement routines added on 9-30-92 by Mobydick

// the skills mapping is private so that that people can't get to it
// except through these functions.

#define NUM_SKILLS 12

private mapping skills;
private mapping skill_points ;
private mapping skill_stats ;
private static mapping tmp_skills;

// This function is a basic one to call when in doubt about whether
// there are any skill values or not.

void init_skills() {
	if (!skills) {
		skills = allocate_mapping(NUM_SKILLS) ;
		skill_stats = allocate_mapping(NUM_SKILLS) ;
		skill_points = allocate_mapping(NUM_SKILLS) ;
		skills = ([]);
		tmp_skills = ([]);
		skill_stats = ([]) ;
		skill_points = ([]) ;
	}
	if (!tmp_skills) {
		tmp_skills = ([ ]) ;
	}
}

// Dangerous. Don't call this.
// It should only be called if you think someone's stats have been
// totally messed up and need to be completely erased. That means never.
// Usually you want to use init_skills instead.

void wipe_skills() {
		skills = ([]);
		tmp_skills = ([]);
		skill_stats = ([]) ;
		skill_points = ([]) ;
}

// This function returns a copy of the skill mapping. Because it is a
// copy you cannot affect the user's skills through it.

mapping query_skills() {
	return copy(skills) ;
}

// This function returns the skill points, same caveat.

mapping query_skill_points() {
	return copy(skill_points) ;
}

// And this one the statistics.

mapping query_skill_stats() {
	return copy(skill_stats) ;
}

// Okay, call this function to get someone's skill values in a particular
// skill, for example, foo  = (int)this_player()->query_skill("pick locks");
// if there is no value for that skill, 0 is returned.

int query_skill(string what) {
	if (!skills || sizeof(skills) == 0 || member_array(what,keys(skills)) == -1) return 0 ;
	if (tmp_skills && sizeof(tmp_skills)>0 && member_array(what,keys(tmp_skills)) != -1) {
		return skills[what] + tmp_skills[what];
	}
	return skills[what];
}

// Don't add skills unless you know what you're doing.  But this function
// will let you.  Use it wisely, because extra skills take up space.
// Also, using this function is sort of a bad idea after the skill is created.
// better to use modify_skill(), below.
// This should probably be called ONLY from creation.c when a new player
// is created.
// When a skill is set up, the current level of the associated player stat
// is stored. This works in this mudlib because stats do not change. If you
// rewrite the mudlib so that stats DO change, then instead of storing the
// number, you'll need to store the name of the stat and query each time,
// or update the skill_stats mapping whenever you change the stats.

void set_skill(string what, int val, string stat) {
	init_skills();
	if (undefinedp(skills[what])) {	
		skills += ([ what : val ]);
		tmp_skills += ([ what : 0 ]) ;
		skill_points += ([ what : 0 ]) ;
		skill_stats += ([ what : this_object()->query("stat/"+stat) ]) ;
		return;
	}
	else skills[what] = val;
	return;
}	

// Call this for modifying a skill.
// A cool feature is that if you want a time limit bonus, then you can
// pass a value to make the skill go down again in a given amount of time.
// A bug it already has is that if the user quits... the change may stick.

varargs void modify_skill(string what, int val, int time) {
	init_skills();
	if (undefinedp(tmp_skills[what])) 
                tmp_skills += ([ what : val ]);
        else tmp_skills[what] = val;
	call_out ("remove_bonus", time, what, val) ;
	return;
}

static void remove_bonus(string what, int val) {
	if (tmp_skills && tmp_skills[what]) tmp_skills[what] -= val;
}
		
// This function will improve a player's skill level in a given skill.
// It's called each time a player uses his skill. Each time the player
// uses his skill, the total number of skill points is incremented. When the
// player has used the skill a sufficient number of times, he improves
// his skill level by 1. The program is currently set up so that you have
// have (50-stat)*(2N+1) skill points to go from level N to level N+1 in the
// skill, where stat is the associated stat for the skill (ie, strength for
// combat skills, intelligence for spellcasting skills, etc.) This formula
// is chosen because it implies that the total number of skill points needed
// to go from level 0 to level N is N^2 * (50-stat).
// The function takes as an argument the number of skill points to give the
// player. It should be set up so that using a combat skill gives 1 skill
// point and other activities increase skills at a roughly equivalent
// rate.

void improve_skill (string what, int points) {

	int i ;

	skill_points[what] = skill_points[what] + points ;
	if (skill_points[what] > (50-skill_stats[what])*(2*skills[what]+1)) {
		skills[what] = skills[what] + 1 ;
		skill_points[what] = 0 ;
		tell_object(this_object(),"You feel more skilled, somehow.\n") ;
	}
}

// This function changes the level of the skill str to i, no other changes.
// Not usually a good thing to call: usually you should improve the
// player's skill by calling improve_skill. The one place this function
// is called from is the training hall, where the improvement takes place
// directly on the skill level rather than by use.
// function lets you.

void change_skill_level(string str, int i) {
	skills[str]=i ;
}

// This function lets you erase a particular skill from a player. It seems
// unlikely that you would ever need this, but just in case you do, it
// is here.

void remove_skill (string str) {
	map_delete (skills,str) ;
	map_delete (skill_points,str) ;
	map_delete (skill_stats, str) ;
}
