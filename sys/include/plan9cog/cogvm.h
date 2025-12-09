/*
 * Plan9Cog Cognitive VM Extensions
 * Enhanced virtual memory management for cognitive operations
 * Based on Cognumach cognitive VM integration
 */

#pragma src "/sys/src/9/port"
#pragma lib "libcognitive.a"

typedef struct CogSegment CogSegment;
typedef struct CogMemory CogMemory;

/* Cognitive Memory Attributes */
enum {
	CogMemNormal = 0,	/* normal memory */
	CogMemHypergraph = 1,	/* hypergraph memory for AtomSpace */
	CogMemPattern = 2,	/* pattern memory for PLN */
	CogMemAttention = 4,	/* attention allocation memory (ECAN) */
	CogMemShared = 8,	/* shared cognitive memory */
	CogMemPersist = 16,	/* persistent cognitive memory */
};

/* Cognitive Segment - specialized memory region for cognitive operations */
struct CogSegment {
	ulong base;		/* base address */
	ulong size;		/* segment size */
	int attrs;		/* cognitive attributes */
	void *atomspace;	/* associated AtomSpace */
	Lock;			/* segment lock */
};

/* Cognitive Memory Manager */
struct CogMemory {
	CogSegment **segs;	/* cognitive segments */
	int nsegs;		/* number of segments */
	int maxsegs;		/* maximum segments */
	ulong totalsize;	/* total cognitive memory */
	ulong usedsize;		/* used cognitive memory */
	Lock;			/* memory manager lock */
};

/* Cognitive VM operations */
CogMemory*	cogmeminit(void);
void		cogmemfree(CogMemory *cm);
CogSegment*	cogmemalloc(CogMemory *cm, ulong size, int attrs);
void		cogmemsegfree(CogMemory *cm, CogSegment *seg);
void*		cogmemmap(CogSegment *seg, ulong offset, ulong len);
void		cogmemunmap(CogSegment *seg, void *addr, ulong len);

/* Cognitive memory statistics */
typedef struct CogMemStats CogMemStats;
struct CogMemStats {
	ulong totalsegs;	/* total segments */
	ulong hypergraphsegs;	/* hypergraph segments */
	ulong patternsegs;	/* pattern segments */
	ulong attentionsegs;	/* attention segments */
	ulong totalmem;		/* total memory */
	ulong usedmem;		/* used memory */
	ulong freemem;		/* free memory */
};

void		cogmemstats(CogMemory *cm, CogMemStats *stats);

/* Cognitive page fault handler */
int		cogmempagefault(void *addr, int write);

/* Hypergraph memory optimization */
void		coghypergraphopt(CogSegment *seg);

/* Pattern memory caching */
typedef struct CogPatternCache CogPatternCache;
struct CogPatternCache {
	void *patterns;		/* cached patterns */
	int npatterns;		/* number of patterns */
	ulong hits;		/* cache hits */
	ulong misses;		/* cache misses */
};

CogPatternCache*	cogpatcacheinit(CogSegment *seg);
void			cogpatcachefree(CogPatternCache *cache);
void*			cogpatcachelookup(CogPatternCache *cache, void *key);
void			cogpatcacheinsert(CogPatternCache *cache, void *key, void *val);

/* Attention-based memory allocation (ECAN integration) */
enum {
	AttentionThresholdHigh = 100,
	AttentionThresholdMid = 50,
	AttentionThresholdLow = 10,
};

typedef struct AttentionAlloc AttentionAlloc;
struct AttentionAlloc {
	short threshold;	/* attention threshold */
	ulong allocated;	/* allocated memory */
	ulong freed;		/* freed memory */
};

AttentionAlloc*	attallocinit(CogSegment *seg);
void		attallocfree(AttentionAlloc *aa);
void*		attalloc(AttentionAlloc *aa, ulong size, short sti);
void		attfree(AttentionAlloc *aa, void *addr);

/* Cognitive garbage collection */
typedef struct CogGC CogGC;
struct CogGC {
	ulong collected;	/* collected memory */
	ulong scanned;		/* scanned objects */
	ulong retained;		/* retained objects */
};

void		coggcrun(CogMemory *cm, CogGC *stats);
void		coggcmark(void *obj);
void		coggcsweep(CogMemory *cm);
