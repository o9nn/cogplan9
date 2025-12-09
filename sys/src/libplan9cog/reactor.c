/*
 * Cognitive Fusion Reactor - distributed cognitive processing
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <plan9cog.h>

CogFusionReactor*
cogreactorinit(Plan9Cog *p9c, int nworkers)
{
	CogFusionReactor *cfr;
	
	cfr = mallocz(sizeof(CogFusionReactor), 1);
	if(cfr == nil)
		return nil;
	
	cfr->p9c = p9c;
	cfr->nworkers = nworkers;
	cfr->workers = mallocz(sizeof(Chan*) * nworkers, 1);
	if(cfr->workers == nil){
		free(cfr);
		return nil;
	}
	
	/* TODO: Start worker threads */
	
	return cfr;
}

void
cogreactorfree(CogFusionReactor *cfr)
{
	if(cfr == nil)
		return;
	
	lock(cfr);
	
	/* TODO: Stop worker threads */
	free(cfr->workers);
	
	unlock(cfr);
	free(cfr);
}

void
cogreactorsubmit(CogFusionReactor *cfr, void *task)
{
	/* TODO: Submit task to worker */
}

void*
cogreactorresult(CogFusionReactor *cfr)
{
	/* TODO: Get result from worker */
	return nil;
}
