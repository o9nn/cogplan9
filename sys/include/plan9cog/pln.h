/*
 * Plan9Cog Probabilistic Logic Networks (PLN)
 * Uncertain inference and reasoning engine
 * Based on OCC PLN implementation
 */

#pragma src "/sys/src/libpln"
#pragma lib "libpln.a"

typedef struct PlnRule PlnRule;
typedef struct PlnFormula PlnFormula;
typedef struct PlnInference PlnInference;

#include <plan9cog/atomspace.h>

/* PLN Formula Types */
enum {
	PlnDeduction = 1,
	PlnInduction,
	PlnAbduction,
	PlnRevision,
	PlnModus,
	PlnAnd,
	PlnOr,
	PlnNot,
	PlnInheritance,
	PlnSimilarity,
	PlnIntensional,
	PlnExtensional,
};

/* PLN Formula - truth value computation */
struct PlnFormula {
	int type;		/* formula type */
	TruthValue (*compute)(TruthValue *inputs, int n);
	float (*strength)(TruthValue *inputs, int n);
	float (*confidence)(TruthValue *inputs, int n);
};

/* PLN Rule - inference rule */
struct PlnRule {
	int id;			/* rule identifier */
	char *name;		/* rule name */
	Pattern *premises;	/* rule premises */
	int npremises;		/* number of premises */
	Pattern *conclusion;	/* rule conclusion */
	PlnFormula *formula;	/* truth value formula */
	float weight;		/* rule weight */
};

/* PLN Inference Engine */
struct PlnInference {
	AtomSpace *as;		/* atomspace */
	PlnRule **rules;	/* inference rules */
	int nrules;		/* number of rules */
	int maxsteps;		/* maximum inference steps */
	float minconf;		/* minimum confidence threshold */
	Lock;			/* inference lock */
};

/* PLN operations */
PlnInference*	plninit(AtomSpace *as);
void		plnfree(PlnInference *pln);
void		plnaddrule(PlnInference *pln, PlnRule *rule);
PlnRule*	plncreaterule(char *name, Pattern *premises, int np, Pattern *conclusion, int formulatype);

/* Forward chaining */
Atom**		plnforward(PlnInference *pln, Atom *target, int maxsteps, int *n);

/* Backward chaining */
Atom**		plnbackward(PlnInference *pln, Atom *goal, int maxsteps, int *n);

/* Direct evaluation */
TruthValue	plneval(PlnInference *pln, Atom *query);

/* Built-in PLN formulas */
TruthValue	plndeduction(TruthValue a, TruthValue b);
TruthValue	plninduction(TruthValue a, TruthValue b);
TruthValue	plnabduction(TruthValue a, TruthValue b);
TruthValue	plnrevision(TruthValue a, TruthValue b);
TruthValue	plnand(TruthValue a, TruthValue b);
TruthValue	plnor(TruthValue a, TruthValue b);
TruthValue	plnnot(TruthValue a);

/* Higher-order inference */
typedef struct PlnContext PlnContext;
struct PlnContext {
	Atom *focus;		/* focus atom */
	Atom **premises;	/* premise atoms */
	int npremises;		/* number of premises */
	TruthValue *tvs;	/* premise truth values */
};

PlnContext*	plncontextcreate(Atom *focus);
void		plncontextfree(PlnContext *ctx);
void		plncontextaddpremise(PlnContext *ctx, Atom *premise);
TruthValue	plncontexteval(PlnInference *pln, PlnContext *ctx);

/* PLN statistics */
typedef struct PlnStats PlnStats;
struct PlnStats {
	ulong inferences;	/* total inferences */
	ulong forward;		/* forward chaining steps */
	ulong backward;		/* backward chaining steps */
	ulong rulematch;	/* rule matches */
	ulong tvcompute;	/* truth value computations */
};

void		plnstats(PlnInference *pln, PlnStats *stats);
void		plnresetstats(PlnInference *pln);

/* Unified Rule Engine (URE) integration */
typedef struct UreChainer UreChainer;
struct UreChainer {
	PlnInference *pln;	/* PLN inference engine */
	Atom *rulebase;		/* rule base atom */
	int maxiter;		/* maximum iterations */
	float complexity;	/* complexity penalty */
};

UreChainer*	ureinit(AtomSpace *as, Atom *rulebase);
void		urefree(UreChainer *ure);
Atom**		urechain(UreChainer *ure, Atom *target, int *n);

/* Attention allocation for PLN (ECAN integration) */
void		plnallocattention(PlnInference *pln, Atom *a, short amount);
short		plngetattention(PlnInference *pln, Atom *a);
void		plnspreadattention(PlnInference *pln, Atom *source);
