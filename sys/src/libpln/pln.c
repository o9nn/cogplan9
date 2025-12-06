/*
 * PLN (Probabilistic Logic Networks) implementation
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>
#include <plan9cog/pln.h>

PlnInference*
plninit(AtomSpace *as)
{
	PlnInference *pln;
	
	pln = mallocz(sizeof(PlnInference), 1);
	if(pln == nil)
		return nil;
	
	pln->as = as;
	pln->maxsteps = 1000;
	pln->minconf = 0.1;
	pln->nrules = 0;
	pln->rules = nil;
	
	return pln;
}

void
plnfree(PlnInference *pln)
{
	int i;
	
	if(pln == nil)
		return;
	
	lock(pln);
	for(i = 0; i < pln->nrules; i++){
		if(pln->rules[i]){
			free(pln->rules[i]->name);
			free(pln->rules[i]);
		}
	}
	unlock(pln);
	
	free(pln->rules);
	free(pln);
}

void
plnaddrule(PlnInference *pln, PlnRule *rule)
{
	PlnRule **newrules;
	
	lock(pln);
	newrules = realloc(pln->rules, sizeof(PlnRule*) * (pln->nrules + 1));
	if(newrules != nil){
		pln->rules = newrules;
		pln->rules[pln->nrules++] = rule;
	}
	unlock(pln);
}

/* Deduction formula: P(A->C) given P(A->B) and P(B->C) */
TruthValue
plndeduction(TruthValue ab, TruthValue bc)
{
	TruthValue result;
	
	result.strength = ab.strength * bc.strength;
	result.confidence = ab.confidence * bc.confidence;
	result.count = ab.count + bc.count;
	
	return result;
}

/* Induction formula: P(B->A) given P(A->B) */
TruthValue
plninduction(TruthValue ab, TruthValue ba)
{
	TruthValue result;
	float k = 1.0; /* Indifference prior */
	
	result.strength = (ab.strength * ba.strength) / 
		((ab.strength * ba.strength) + k * (1 - ab.strength) * (1 - ba.strength));
	result.confidence = ab.confidence * ba.confidence * 0.9;
	result.count = ab.count;
	
	return result;
}

/* Abduction formula: P(C->A) given P(A->B) and P(B->C) */
TruthValue
plnabduction(TruthValue ab, TruthValue bc)
{
	TruthValue result;
	
	/* Simplified abduction */
	result.strength = ab.strength * bc.strength * 0.8;
	result.confidence = ab.confidence * bc.confidence * 0.7;
	result.count = ab.count + bc.count;
	
	return result;
}

/* Revision formula: combine two truth values */
TruthValue
plnrevision(TruthValue a, TruthValue b)
{
	TruthValue result;
	float wa, wb, wtotal;
	
	wa = a.confidence;
	wb = b.confidence;
	wtotal = wa + wb;
	
	if(wtotal > 0){
		result.strength = (a.strength * wa + b.strength * wb) / wtotal;
		result.confidence = wtotal / (wtotal + 1);
	} else {
		result.strength = 0.5;
		result.confidence = 0.0;
	}
	result.count = a.count + b.count;
	
	return result;
}

/* AND formula */
TruthValue
plnand(TruthValue a, TruthValue b)
{
	TruthValue result;
	
	result.strength = a.strength * b.strength;
	result.confidence = a.confidence * b.confidence;
	result.count = a.count + b.count;
	
	return result;
}

/* OR formula */
TruthValue
plnor(TruthValue a, TruthValue b)
{
	TruthValue result;
	
	result.strength = a.strength + b.strength - (a.strength * b.strength);
	result.confidence = (a.confidence + b.confidence) / 2;
	result.count = a.count + b.count;
	
	return result;
}

/* NOT formula */
TruthValue
plnnot(TruthValue a)
{
	TruthValue result;
	
	result.strength = 1.0 - a.strength;
	result.confidence = a.confidence;
	result.count = a.count;
	
	return result;
}

/* Direct evaluation */
TruthValue
plneval(PlnInference *pln, Atom *query)
{
	TruthValue tv;
	
	if(query == nil){
		tv.strength = 0.0;
		tv.confidence = 0.0;
		tv.count = 0;
		return tv;
	}
	
	return atomgettruth(query);
}

/* Forward chaining (simplified) */
Atom**
plnforward(PlnInference *pln, Atom *target, int maxsteps, int *n)
{
	Atom **results;
	int step;
	
	results = mallocz(sizeof(Atom*) * maxsteps, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}
	
	/* Simplified forward chaining */
	for(step = 0; step < maxsteps; step++){
		/* TODO: Apply rules and generate new atoms */
	}
	
	*n = 0;
	return results;
}

/* Backward chaining (simplified) */
Atom**
plnbackward(PlnInference *pln, Atom *goal, int maxsteps, int *n)
{
	Atom **results;
	
	results = mallocz(sizeof(Atom*) * maxsteps, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}
	
	/* Simplified backward chaining */
	*n = 0;
	return results;
}

void
plnstats(PlnInference *pln, PlnStats *stats)
{
	memset(stats, 0, sizeof(PlnStats));
	/* TODO: Collect statistics */
}

void
plnresetstats(PlnInference *pln)
{
	/* TODO: Reset statistics */
}
