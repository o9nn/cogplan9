/*
 * MachSpace - distributed hypergraph memory system
 */

#include <u.h>
#include <libc.h>
#include <plan9cog.h>

MachSpace*
machspaceinit(AtomSpace *local)
{
	MachSpace *ms;
	
	ms = mallocz(sizeof(MachSpace), 1);
	if(ms == nil)
		return nil;
	
	ms->local = local;
	ms->remote = nil;
	ms->nremote = 0;
	ms->hosts = nil;
	
	return ms;
}

void
machspacefree(MachSpace *ms)
{
	int i;
	
	if(ms == nil)
		return;
	
	lock(ms);
	
	for(i = 0; i < ms->nremote; i++){
		free(ms->hosts[i]);
	}
	free(ms->hosts);
	free(ms->remote);
	
	unlock(ms);
	free(ms);
}

int
machspaceconnect(MachSpace *ms, char *host)
{
	char **newhosts;
	AtomSpace **newremote;
	
	if(ms == nil || host == nil)
		return -1;
	
	lock(ms);
	
	newhosts = realloc(ms->hosts, sizeof(char*) * (ms->nremote + 1));
	newremote = realloc(ms->remote, sizeof(AtomSpace*) * (ms->nremote + 1));
	
	if(newhosts == nil || newremote == nil){
		unlock(ms);
		return -1;
	}
	
	ms->hosts = newhosts;
	ms->remote = newremote;
	
	ms->hosts[ms->nremote] = strdup(host);
	ms->remote[ms->nremote] = nil; /* TODO: Connect to remote */
	ms->nremote++;
	
	unlock(ms);
	return 0;
}

Atom*
machspacefind(MachSpace *ms, ulong id)
{
	Atom *a;
	int i;
	
	if(ms == nil)
		return nil;
	
	/* Search local first */
	a = atomfind(ms->local, id);
	if(a != nil)
		return a;
	
	/* Search remote spaces */
	lock(ms);
	for(i = 0; i < ms->nremote; i++){
		if(ms->remote[i]){
			a = atomfind(ms->remote[i], id);
			if(a != nil){
				unlock(ms);
				return a;
			}
		}
	}
	unlock(ms);
	
	return nil;
}

/* Internal: copy atom from source to destination atomspace */
static Atom*
copyatom(AtomSpace *dst, Atom *src)
{
	Atom *copy;
	Atom **outgoing;
	int i;
	TruthValue tv;
	AttentionValue av;

	if(src == nil)
		return nil;

	if(src->noutgoing > 0 && src->outgoing != nil){
		/* Copy outgoing atoms first */
		outgoing = mallocz(sizeof(Atom*) * src->noutgoing, 1);
		if(outgoing == nil)
			return nil;

		for(i = 0; i < src->noutgoing; i++){
			/* For links, we need to find or copy the outgoing atoms */
			outgoing[i] = atomfind(dst, src->outgoing[i]->id);
			if(outgoing[i] == nil){
				outgoing[i] = copyatom(dst, src->outgoing[i]);
			}
		}

		copy = linkcreate(dst, src->type, outgoing, src->noutgoing);
		free(outgoing);
	} else {
		copy = atomcreate(dst, src->type, src->name);
	}

	if(copy == nil)
		return nil;

	/* Copy truth and attention values */
	tv = atomgettruth(src);
	atomsettruth(copy, tv);
	av = atomgetattention(src);
	atomsetattention(copy, av);

	return copy;
}

/* Internal: merge atom into destination, updating truth values */
static void
mergeatom(Atom *dst, Atom *src)
{
	TruthValue tvdst, tvsrc, tvmerged;

	if(dst == nil || src == nil)
		return;

	/* Merge truth values using revision */
	tvdst = atomgettruth(dst);
	tvsrc = atomgettruth(src);

	/* Simple merge: average strength, combined confidence */
	tvmerged.strength = (tvdst.strength + tvsrc.strength) / 2.0;
	tvmerged.confidence = (tvdst.confidence + tvsrc.confidence) / 2.0;
	tvmerged.count = tvdst.count + tvsrc.count;

	atomsettruth(dst, tvmerged);
}

/* Synchronize local and remote atomspaces */
int
machspacesync(MachSpace *ms)
{
	int i, j;
	Atom *local, *remote, *existing;
	int synced = 0;

	if(ms == nil)
		return -1;

	lock(ms);

	/* For each remote atomspace */
	for(i = 0; i < ms->nremote; i++){
		if(ms->remote[i] == nil)
			continue;

		/* Sync remote atoms to local */
		for(j = 0; j < ms->remote[i]->natoms; j++){
			remote = ms->remote[i]->atoms[j];
			if(remote == nil)
				continue;

			/* Check if atom exists locally */
			existing = atomfind(ms->local, remote->id);
			if(existing != nil){
				/* Merge truth values */
				mergeatom(existing, remote);
			} else {
				/* Copy atom to local */
				copyatom(ms->local, remote);
			}
			synced++;
		}

		/* Sync local atoms to remote */
		for(j = 0; j < ms->local->natoms; j++){
			local = ms->local->atoms[j];
			if(local == nil)
				continue;

			/* Check if atom exists in remote */
			existing = atomfind(ms->remote[i], local->id);
			if(existing != nil){
				/* Merge truth values */
				mergeatom(existing, local);
			} else {
				/* Copy atom to remote */
				copyatom(ms->remote[i], local);
			}
			synced++;
		}
	}

	unlock(ms);

	return synced;
}

/* Create an atom in MachSpace (local first, then sync) */
Atom*
machspacecreate(MachSpace *ms, int type, char *name)
{
	Atom *a;

	if(ms == nil || ms->local == nil)
		return nil;

	a = atomcreate(ms->local, type, name);
	return a;
}

/* Create a link in MachSpace */
Atom*
machspacelink(MachSpace *ms, int type, Atom **outgoing, int n)
{
	Atom *a;

	if(ms == nil || ms->local == nil)
		return nil;

	a = linkcreate(ms->local, type, outgoing, n);
	return a;
}

/* Query atoms across all spaces */
Atom**
machspacequery(MachSpace *ms, AtomPredicate pred, void *arg, int *n)
{
	Atom **results, **local, **remote;
	int nlocal, nremote, i, j, count, maxresults;

	if(ms == nil){
		*n = 0;
		return nil;
	}

	maxresults = 256;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}

	count = 0;

	/* Query local atomspace */
	local = atomquery(ms->local, pred, arg, &nlocal);
	if(local != nil){
		for(i = 0; i < nlocal && count < maxresults; i++){
			results[count++] = local[i];
		}
		free(local);
	}

	/* Query remote atomspaces */
	lock(ms);
	for(i = 0; i < ms->nremote; i++){
		if(ms->remote[i] == nil)
			continue;

		remote = atomquery(ms->remote[i], pred, arg, &nremote);
		if(remote != nil){
			for(j = 0; j < nremote && count < maxresults; j++){
				/* Avoid duplicates */
				int dup = 0;
				int k;
				for(k = 0; k < count; k++){
					if(results[k]->id == remote[j]->id){
						dup = 1;
						break;
					}
				}
				if(!dup){
					results[count++] = remote[j];
				}
			}
			free(remote);
		}
	}
	unlock(ms);

	*n = count;
	return results;
}
