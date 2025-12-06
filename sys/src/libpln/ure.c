/*
 * Unified Rule Engine (URE) implementation
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>
#include <plan9cog/pln.h>

UreChainer*
ureinit(AtomSpace *as, Atom *rulebase)
{
	UreChainer *ure;
	
	ure = mallocz(sizeof(UreChainer), 1);
	if(ure == nil)
		return nil;
	
	ure->pln = plninit(as);
	if(ure->pln == nil){
		free(ure);
		return nil;
	}
	
	ure->rulebase = rulebase;
	ure->maxiter = 100;
	ure->complexity = 1.0;
	
	return ure;
}

void
urefree(UreChainer *ure)
{
	if(ure == nil)
		return;
	
	plnfree(ure->pln);
	free(ure);
}

Atom**
urechain(UreChainer *ure, Atom *target, int *n)
{
	Atom **results;
	int i;
	
	results = mallocz(sizeof(Atom*) * ure->maxiter, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}
	
	/* Simplified URE chaining */
	for(i = 0; i < ure->maxiter; i++){
		/* TODO: Apply rules from rulebase */
	}
	
	*n = 0;
	return results;
}

/* Attention allocation for PLN */
void
plnallocattention(PlnInference *pln, Atom *a, short amount)
{
	AttentionValue av;
	
	av = atomgetattention(a);
	av.sti += amount;
	atomsetattention(a, av);
}

short
plngetattention(PlnInference *pln, Atom *a)
{
	AttentionValue av;
	
	av = atomgetattention(a);
	return av.sti;
}

void
plnspreadattention(PlnInference *pln, Atom *source)
{
	int i, nincoming;
	Atom **incoming;
	AttentionValue av;
	short spread;
	
	av = atomgetattention(source);
	if(av.sti <= 0)
		return;
	
	incoming = atomgetincoming(pln->as, source, &nincoming);
	if(incoming == nil || nincoming == 0)
		return;
	
	spread = av.sti / (nincoming + 1);
	
	for(i = 0; i < nincoming; i++){
		plnallocattention(pln, incoming[i], spread);
	}
	
	free(incoming);
}
