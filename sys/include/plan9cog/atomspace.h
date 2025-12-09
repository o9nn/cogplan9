/*
 * Plan9Cog AtomSpace Integration
 * AtomSpace header for cognitive knowledge representation
 * Based on OpenCog Collection (OCC) integration
 */

#pragma src "/sys/src/libatomspace"
#pragma lib "libatomspace.a"

typedef struct Atom Atom;
typedef struct AtomSpace AtomSpace;
typedef struct TruthValue TruthValue;
typedef struct AttentionValue AttentionValue;

/* Atom Types */
enum {
	AtomNode = 1,
	AtomLink,
	ConceptNode,
	PredicateNode,
	EvaluationLink,
	InheritanceLink,
	SimilarityLink,
	ImplicationLink,
	ExecutionLink,
	ListLink,
};

/* Truth Value */
struct TruthValue {
	float strength;		/* probability of truth (0.0 to 1.0) */
	float confidence;	/* confidence in the assessment */
	ulong count;		/* evidence count */
};

/* Attention Value (ECAN - Economic Attention Network) */
struct AttentionValue {
	short sti;		/* Short-term importance */
	short lti;		/* Long-term importance */
	short vlti;		/* Very long-term importance */
};

/* Atom - fundamental unit of knowledge */
struct Atom {
	ulong id;		/* unique atom identifier */
	int type;		/* atom type */
	char *name;		/* atom name (for nodes) */
	Atom **outgoing;	/* outgoing set (for links) */
	int noutgoing;		/* number of outgoing atoms */
	TruthValue tv;		/* truth value */
	AttentionValue av;	/* attention value */
};

/* AtomSpace - hypergraph knowledge base */
struct AtomSpace {
	Atom **atoms;		/* array of atoms */
	int natoms;		/* number of atoms */
	int maxatoms;		/* maximum atoms */
	Lock;			/* atomspace lock */
};

/* AtomSpace operations */
AtomSpace*	atomspacecreate(void);
void		atomspacefree(AtomSpace *as);
Atom*		atomcreate(AtomSpace *as, int type, char *name);
Atom*		linkcreate(AtomSpace *as, int type, Atom **outgoing, int n);
Atom*		atomfind(AtomSpace *as, ulong id);
int		atomdelete(AtomSpace *as, ulong id);
TruthValue	atomgettruth(Atom *a);
void		atomsettruth(Atom *a, TruthValue tv);
AttentionValue	atomgetattention(Atom *a);
void		atomsetattention(Atom *a, AttentionValue av);

/* Query operations */
typedef int (*AtomPredicate)(Atom *a, void *arg);
Atom**		atomquery(AtomSpace *as, AtomPredicate pred, void *arg, int *n);
Atom**		atomgetincoming(AtomSpace *as, Atom *a, int *n);

/* Pattern matching */
typedef struct Pattern Pattern;
struct Pattern {
	int type;
	char *name;
	Pattern **outgoing;
	int noutgoing;
	int wildcard;
};

Atom**		atommatch(AtomSpace *as, Pattern *pat, int *n);

/* Serialization */
int		atomspaceexport(AtomSpace *as, int fd);
AtomSpace*	atomspaceimport(int fd);

/* AtomSpace IPC - for distributed cognitive processing */
enum {
	CogAtomCreate = 1,
	CogAtomDelete,
	CogAtomUpdate,
	CogAtomQuery,
	CogAtomSync,
	CogPlnReason,
	CogEcanAlloc,
};

typedef struct CogMsg CogMsg;
struct CogMsg {
	int type;		/* message type */
	ulong atomid;		/* atom identifier */
	int atomtype;		/* atom type */
	char data[8192];	/* message data */
	int ndata;		/* data length */
};

int	cogsend(int fd, CogMsg *msg);
int	cogrecv(int fd, CogMsg *msg);
