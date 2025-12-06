/*
 * ECAN (Economic Attention Network) implementation
 */

#include <u.h>
#include <libc.h>
#include <plan9cog.h>

EcanNetwork*
ecaninit(AtomSpace *as, short totalsti, short totallti)
{
	EcanNetwork *ecan;
	
	ecan = mallocz(sizeof(EcanNetwork), 1);
	if(ecan == nil)
		return nil;
	
	ecan->as = as;
	ecan->totalsti = totalsti;
	ecan->totallti = totallti;
	ecan->attentionalfocus = 20; /* Default focus size */
	ecan->focusatoms = nil;
	ecan->nfocus = 0;
	
	return ecan;
}

void
ecanfree(EcanNetwork *ecan)
{
	if(ecan == nil)
		return;
	
	lock(ecan);
	free(ecan->focusatoms);
	unlock(ecan);
	
	free(ecan);
}

void
ecanupdate(EcanNetwork *ecan)
{
	int i, j;
	Atom *a;
	AttentionValue av;
	Atom **sorted;
	int nsorted;
	
	if(ecan == nil || ecan->as == nil)
		return;
	
	lock(ecan);
	
	/* Sort atoms by STI */
	sorted = mallocz(sizeof(Atom*) * ecan->as->natoms, 1);
	nsorted = 0;
	
	for(i = 0; i < ecan->as->natoms; i++){
		a = ecan->as->atoms[i];
		if(a == nil)
			continue;
		sorted[nsorted++] = a;
	}
	
	/* Simple bubble sort by STI */
	for(i = 0; i < nsorted - 1; i++){
		for(j = 0; j < nsorted - i - 1; j++){
			if(sorted[j]->av.sti < sorted[j+1]->av.sti){
				a = sorted[j];
				sorted[j] = sorted[j+1];
				sorted[j+1] = a;
			}
		}
	}
	
	/* Update attentional focus */
	free(ecan->focusatoms);
	ecan->nfocus = nsorted < ecan->attentionalfocus ? nsorted : ecan->attentionalfocus;
	ecan->focusatoms = mallocz(sizeof(Atom*) * ecan->nfocus, 1);
	
	for(i = 0; i < ecan->nfocus; i++){
		ecan->focusatoms[i] = sorted[i];
	}
	
	free(sorted);
	unlock(ecan);
}

void
ecanallocate(EcanNetwork *ecan, Atom *a, short sti)
{
	AttentionValue av;
	
	if(ecan == nil || a == nil)
		return;
	
	av = atomgetattention(a);
	av.sti += sti;
	atomsetattention(a, av);
	
	ecanupdate(ecan);
}

void
ecanspread(EcanNetwork *ecan, Atom *source)
{
	int i, nincoming;
	Atom **incoming;
	AttentionValue av;
	short spread;
	
	if(ecan == nil || source == nil)
		return;
	
	av = atomgetattention(source);
	if(av.sti <= 0)
		return;
	
	incoming = atomgetincoming(ecan->as, source, &nincoming);
	if(incoming == nil || nincoming == 0)
		return;
	
	spread = av.sti / (nincoming + 1);
	
	for(i = 0; i < nincoming; i++){
		ecanallocate(ecan, incoming[i], spread);
	}
	
	free(incoming);
}

Atom**
ecanfocus(EcanNetwork *ecan, int *n)
{
	if(ecan == nil){
		*n = 0;
		return nil;
	}
	
	*n = ecan->nfocus;
	return ecan->focusatoms;
}

void
ecandecay(EcanNetwork *ecan, float rate)
{
	int i;
	Atom *a;
	AttentionValue av;
	
	if(ecan == nil || ecan->as == nil)
		return;
	
	lock(ecan);
	
	for(i = 0; i < ecan->as->natoms; i++){
		a = ecan->as->atoms[i];
		if(a == nil)
			continue;
		
		av = atomgetattention(a);
		av.sti = (short)(av.sti * (1.0 - rate));
		atomsetattention(a, av);
	}
	
	unlock(ecan);
	
	ecanupdate(ecan);
}
