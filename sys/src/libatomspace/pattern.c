/*
 * Pattern matching for AtomSpace
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>

static int
patternmatchatom(Atom *a, Pattern *pat)
{
	int i;
	
	if(pat->wildcard)
		return 1;
	
	if(pat->type != 0 && a->type != pat->type)
		return 0;
	
	if(pat->name != nil && a->name != nil)
		if(strcmp(a->name, pat->name) != 0)
			return 0;
	
	if(pat->noutgoing != a->noutgoing)
		return 0;
	
	for(i = 0; i < pat->noutgoing; i++){
		if(!patternmatchatom(a->outgoing[i], pat->outgoing[i]))
			return 0;
	}
	
	return 1;
}

Atom**
atommatch(AtomSpace *as, Pattern *pat, int *n)
{
	Atom **results;
	int i, count, maxresults;
	
	maxresults = 128;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}
	
	count = 0;
	lock(as);
	for(i = 0; i < as->natoms; i++){
		if(as->atoms[i] && patternmatchatom(as->atoms[i], pat)){
			if(count >= maxresults){
				maxresults *= 2;
				results = realloc(results, sizeof(Atom*) * maxresults);
			}
			results[count++] = as->atoms[i];
		}
	}
	unlock(as);
	
	*n = count;
	return results;
}
