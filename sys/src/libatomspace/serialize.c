/*
 * AtomSpace serialization
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>

int
atomspaceexport(AtomSpace *as, int fd)
{
	int i;
	Atom *a;
	char buf[8192];
	int n;
	
	lock(as);
	
	/* Write header */
	n = snprint(buf, sizeof(buf), "ATOMSPACE %d\n", as->natoms);
	if(write(fd, buf, n) != n){
		unlock(as);
		return -1;
	}
	
	/* Write each atom */
	for(i = 0; i < as->natoms; i++){
		a = as->atoms[i];
		if(a == nil)
			continue;
		
		n = snprint(buf, sizeof(buf), "ATOM %ld %d %s %d %f %f %ld %d %d %d\n",
			a->id, a->type, a->name ? a->name : "",
			a->noutgoing,
			a->tv.strength, a->tv.confidence, a->tv.count,
			a->av.sti, a->av.lti, a->av.vlti);
		
		if(write(fd, buf, n) != n){
			unlock(as);
			return -1;
		}
	}
	
	unlock(as);
	return 0;
}

AtomSpace*
atomspaceimport(int fd)
{
	AtomSpace *as;
	char buf[8192];
	char *p;
	int natoms;
	
	/* Read header */
	if(read(fd, buf, sizeof(buf)) <= 0)
		return nil;
	
	p = buf;
	if(strncmp(p, "ATOMSPACE ", 10) != 0)
		return nil;
	
	p += 10;
	natoms = atoi(p);
	
	as = atomspacecreate();
	if(as == nil)
		return nil;
	
	/* TODO: Read atoms */
	
	return as;
}
