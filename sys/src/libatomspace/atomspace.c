/*
 * AtomSpace implementation for Plan9Cog
 * Hypergraph knowledge representation system
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>

static ulong nextatomid = 1;

AtomSpace*
atomspacecreate(void)
{
	AtomSpace *as;
	
	as = mallocz(sizeof(AtomSpace), 1);
	if(as == nil)
		return nil;
	
	as->maxatoms = 1024;
	as->atoms = mallocz(sizeof(Atom*) * as->maxatoms, 1);
	if(as->atoms == nil){
		free(as);
		return nil;
	}
	
	as->natoms = 0;
	return as;
}

void
atomspacefree(AtomSpace *as)
{
	int i;
	
	if(as == nil)
		return;
	
	lock(as);
	for(i = 0; i < as->natoms; i++){
		if(as->atoms[i]){
			free(as->atoms[i]->name);
			free(as->atoms[i]->outgoing);
			free(as->atoms[i]);
		}
	}
	unlock(as);
	
	free(as->atoms);
	free(as);
}

static Atom*
atomalloc(AtomSpace *as)
{
	Atom *a;
	Atom **newatoms;
	
	a = mallocz(sizeof(Atom), 1);
	if(a == nil)
		return nil;
	
	lock(as);
	
	/* Expand atom array if needed */
	if(as->natoms >= as->maxatoms){
		as->maxatoms *= 2;
		newatoms = realloc(as->atoms, sizeof(Atom*) * as->maxatoms);
		if(newatoms == nil){
			unlock(as);
			free(a);
			return nil;
		}
		as->atoms = newatoms;
	}
	
	a->id = nextatomid++;
	as->atoms[as->natoms++] = a;
	
	unlock(as);
	return a;
}

Atom*
atomcreate(AtomSpace *as, int type, char *name)
{
	Atom *a;
	
	a = atomalloc(as);
	if(a == nil)
		return nil;
	
	a->type = type;
	a->name = name ? strdup(name) : nil;
	a->outgoing = nil;
	a->noutgoing = 0;
	
	/* Default truth value */
	a->tv.strength = 0.5;
	a->tv.confidence = 0.0;
	a->tv.count = 0;
	
	/* Default attention value */
	a->av.sti = 0;
	a->av.lti = 0;
	a->av.vlti = 0;
	
	return a;
}

Atom*
linkcreate(AtomSpace *as, int type, Atom **outgoing, int n)
{
	Atom *a;
	int i;
	
	a = atomalloc(as);
	if(a == nil)
		return nil;
	
	a->type = type;
	a->name = nil;
	a->noutgoing = n;
	
	if(n > 0){
		a->outgoing = mallocz(sizeof(Atom*) * n, 1);
		if(a->outgoing == nil){
			free(a);
			return nil;
		}
		for(i = 0; i < n; i++)
			a->outgoing[i] = outgoing[i];
	} else {
		a->outgoing = nil;
	}
	
	/* Default truth value */
	a->tv.strength = 0.5;
	a->tv.confidence = 0.0;
	a->tv.count = 0;
	
	/* Default attention value */
	a->av.sti = 0;
	a->av.lti = 0;
	a->av.vlti = 0;
	
	return a;
}

Atom*
atomfind(AtomSpace *as, ulong id)
{
	int i;
	Atom *a;
	
	lock(as);
	for(i = 0; i < as->natoms; i++){
		a = as->atoms[i];
		if(a && a->id == id){
			unlock(as);
			return a;
		}
	}
	unlock(as);
	return nil;
}

int
atomdelete(AtomSpace *as, ulong id)
{
	int i;
	Atom *a;
	
	lock(as);
	for(i = 0; i < as->natoms; i++){
		a = as->atoms[i];
		if(a && a->id == id){
			free(a->name);
			free(a->outgoing);
			free(a);
			as->atoms[i] = nil;
			unlock(as);
			return 0;
		}
	}
	unlock(as);
	return -1;
}

TruthValue
atomgettruth(Atom *a)
{
	return a->tv;
}

void
atomsettruth(Atom *a, TruthValue tv)
{
	a->tv = tv;
}

AttentionValue
atomgetattention(Atom *a)
{
	return a->av;
}

void
atomsetattention(Atom *a, AttentionValue av)
{
	a->av = av;
}

Atom**
atomquery(AtomSpace *as, AtomPredicate pred, void *arg, int *n)
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
		if(as->atoms[i] && pred(as->atoms[i], arg)){
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

Atom**
atomgetincoming(AtomSpace *as, Atom *a, int *n)
{
	Atom **results;
	Atom *link;
	int i, j, count, maxresults;
	
	maxresults = 128;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}
	
	count = 0;
	lock(as);
	for(i = 0; i < as->natoms; i++){
		link = as->atoms[i];
		if(link == nil || link->outgoing == nil)
			continue;
		
		for(j = 0; j < link->noutgoing; j++){
			if(link->outgoing[j] == a){
				if(count >= maxresults){
					maxresults *= 2;
					results = realloc(results, sizeof(Atom*) * maxresults);
				}
				results[count++] = link;
				break;
			}
		}
	}
	unlock(as);
	
	*n = count;
	return results;
}

/* IPC operations */
int
cogsend(int fd, CogMsg *msg)
{
	return write(fd, msg, sizeof(CogMsg));
}

int
cogrecv(int fd, CogMsg *msg)
{
	return read(fd, msg, sizeof(CogMsg));
}
