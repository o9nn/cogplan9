/*
 * Cognitive Tensor Logic - cogtensor.c
 *
 * Kernel-level tensor logic implementation for Plan9Cog.
 * Unifies deep learning tensor operations with symbolic logic
 * based on the observation that logical rules and Einstein
 * summation are essentially the same operation.
 *
 * Key concepts from tensor-logic.org:
 * - Tensor equations as the fundamental construct
 * - Sound reasoning in embedding space
 * - Gradient-based learning + symbolic knowledge
 *
 * This module provides:
 * - Tensor data structures for kernel-level reasoning
 * - Einstein summation operations
 * - Tensor-based truth value computation
 * - Integration with AtomSpace and PLN
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/* Tensor dimensions and limits */
enum
{
	TENSOR_MAX_DIMS = 8,		/* Maximum tensor dimensions */
	TENSOR_MAX_SIZE = 65536,	/* Maximum elements per tensor */
	TENSOR_POOL_SIZE = 1024,	/* Tensor pool size */
	EMBEDDING_DIM = 64,		/* Default embedding dimension */
};

/* Tensor element types */
enum
{
	TensorFloat32 = 0,	/* 32-bit floating point */
	TensorFloat64,		/* 64-bit floating point */
	TensorInt32,		/* 32-bit integer */
	TensorBool,		/* Boolean */
	TensorTruthValue,	/* PLN truth value (strength, confidence) */
};

/* Tensor structure */
typedef struct Tensor Tensor;
struct Tensor
{
	ulong	id;			/* Unique tensor ID */
	int	type;			/* Element type */
	int	ndims;			/* Number of dimensions */
	int	dims[TENSOR_MAX_DIMS];	/* Dimension sizes */
	int	strides[TENSOR_MAX_DIMS]; /* Strides for indexing */
	ulong	nelem;			/* Total elements */
	void	*data;			/* Tensor data */
	int	refcount;		/* Reference count */
	Tensor	*next;			/* Free list link */
	Lock;				/* Tensor lock */
};

/* Truth value tensor (for PLN integration) */
typedef struct TVTensor TVTensor;
struct TVTensor
{
	float	*strength;	/* Strength values */
	float	*confidence;	/* Confidence values */
	int	n;		/* Number of elements */
};

/* Tensor equation for logical inference */
typedef struct TensorEq TensorEq;
struct TensorEq
{
	Tensor	*inputs[8];	/* Input tensors */
	int	ninputs;	/* Number of inputs */
	Tensor	*output;	/* Output tensor */
	char	*einsum;	/* Einstein summation notation */
	int	op;		/* Operation type */
};

/* Embedding space for neural-symbolic reasoning */
typedef struct EmbedSpace EmbedSpace;
struct EmbedSpace
{
	int	dim;		/* Embedding dimension */
	Tensor	**embeddings;	/* Symbol embeddings */
	int	nembeddings;	/* Number of embeddings */
	int	maxembeddings;	/* Maximum embeddings */
	Lock;			/* Space lock */
};

/* Global tensor system state */
static struct TensorSystem
{
	Tensor	*pool;		/* Tensor pool */
	Tensor	*free;		/* Free list */
	int	nalloc;		/* Allocated tensors */
	ulong	nextid;		/* Next tensor ID */
	EmbedSpace *embed;	/* Global embedding space */
	QLock;			/* System lock */
} tensorsys;

/* Forward declarations */
static Tensor* tensoralloc(int type, int ndims, int *dims);
static void tensorfree(Tensor *t);
static int tensorindex(Tensor *t, int *indices);
static float tensorget(Tensor *t, int *indices);
static void tensorset(Tensor *t, int *indices, float val);

/*
 * Initialize tensor logic subsystem
 */
void
cogtensorinit(void)
{
	int i;
	Tensor *t;

	qlock(&tensorsys);

	/* Allocate tensor pool */
	tensorsys.pool = malloc(TENSOR_POOL_SIZE * sizeof(Tensor));
	if(tensorsys.pool == nil){
		qunlock(&tensorsys);
		return;
	}

	/* Initialize free list */
	tensorsys.free = tensorsys.pool;
	for(i = 0; i < TENSOR_POOL_SIZE - 1; i++){
		t = &tensorsys.pool[i];
		memset(t, 0, sizeof(Tensor));
		t->next = &tensorsys.pool[i + 1];
	}
	tensorsys.pool[TENSOR_POOL_SIZE - 1].next = nil;

	tensorsys.nalloc = 0;
	tensorsys.nextid = 1;

	/* Initialize embedding space */
	tensorsys.embed = malloc(sizeof(EmbedSpace));
	if(tensorsys.embed != nil){
		tensorsys.embed->dim = EMBEDDING_DIM;
		tensorsys.embed->maxembeddings = 4096;
		tensorsys.embed->nembeddings = 0;
		tensorsys.embed->embeddings = malloc(
			tensorsys.embed->maxembeddings * sizeof(Tensor*));
	}

	qunlock(&tensorsys);
}

/*
 * Allocate a new tensor
 */
static Tensor*
tensoralloc(int type, int ndims, int *dims)
{
	Tensor *t;
	int i;
	ulong nelem, elemsize;

	if(ndims > TENSOR_MAX_DIMS)
		return nil;

	qlock(&tensorsys);
	t = tensorsys.free;
	if(t == nil){
		qunlock(&tensorsys);
		return nil;
	}
	tensorsys.free = t->next;
	tensorsys.nalloc++;
	qunlock(&tensorsys);

	/* Calculate total elements and strides */
	nelem = 1;
	for(i = ndims - 1; i >= 0; i--){
		t->dims[i] = dims[i];
		t->strides[i] = nelem;
		nelem *= dims[i];
	}

	if(nelem > TENSOR_MAX_SIZE){
		tensorfree(t);
		return nil;
	}

	/* Determine element size */
	switch(type){
	case TensorFloat32:
		elemsize = sizeof(float);
		break;
	case TensorFloat64:
		elemsize = sizeof(double);
		break;
	case TensorInt32:
		elemsize = sizeof(int);
		break;
	case TensorBool:
		elemsize = sizeof(char);
		break;
	case TensorTruthValue:
		elemsize = 2 * sizeof(float);
		break;
	default:
		elemsize = sizeof(float);
	}

	t->data = malloc(nelem * elemsize);
	if(t->data == nil){
		tensorfree(t);
		return nil;
	}
	memset(t->data, 0, nelem * elemsize);

	t->id = tensorsys.nextid++;
	t->type = type;
	t->ndims = ndims;
	t->nelem = nelem;
	t->refcount = 1;
	t->next = nil;

	return t;
}

/*
 * Free a tensor
 */
static void
tensorfree(Tensor *t)
{
	if(t == nil)
		return;

	lock(t);
	t->refcount--;
	if(t->refcount > 0){
		unlock(t);
		return;
	}
	unlock(t);

	if(t->data != nil)
		free(t->data);
	t->data = nil;

	qlock(&tensorsys);
	t->next = tensorsys.free;
	tensorsys.free = t;
	tensorsys.nalloc--;
	qunlock(&tensorsys);
}

/*
 * Compute linear index from multi-dimensional indices
 */
static int
tensorindex(Tensor *t, int *indices)
{
	int i, idx;

	idx = 0;
	for(i = 0; i < t->ndims; i++){
		if(indices[i] < 0 || indices[i] >= t->dims[i])
			return -1;
		idx += indices[i] * t->strides[i];
	}
	return idx;
}

/*
 * Get tensor element
 */
static float
tensorget(Tensor *t, int *indices)
{
	int idx;
	float *fp;

	idx = tensorindex(t, indices);
	if(idx < 0)
		return 0.0;

	fp = (float*)t->data;
	return fp[idx];
}

/*
 * Set tensor element
 */
static void
tensorset(Tensor *t, int *indices, float val)
{
	int idx;
	float *fp;

	idx = tensorindex(t, indices);
	if(idx < 0)
		return;

	fp = (float*)t->data;
	fp[idx] = val;
}

/*
 * Einstein summation - core tensor logic operation
 * Implements contraction over matching indices
 * Example: "ij,jk->ik" for matrix multiplication
 */
Tensor*
tensoreinsum(char *notation, Tensor **inputs, int ninputs)
{
	Tensor *out;
	int outdims[TENSOR_MAX_DIMS];
	int noutdims;
	int i, j, k;
	float sum, val;
	int idx[TENSOR_MAX_DIMS];
	int outidx[TENSOR_MAX_DIMS];

	if(ninputs < 1 || ninputs > 2)
		return nil;

	/* Parse notation and determine output dimensions */
	/* For now, implement common cases */

	if(strcmp(notation, "ij,jk->ik") == 0){
		/* Matrix multiplication */
		if(ninputs != 2)
			return nil;
		if(inputs[0]->ndims != 2 || inputs[1]->ndims != 2)
			return nil;
		if(inputs[0]->dims[1] != inputs[1]->dims[0])
			return nil;

		outdims[0] = inputs[0]->dims[0];
		outdims[1] = inputs[1]->dims[1];
		noutdims = 2;

		out = tensoralloc(TensorFloat32, noutdims, outdims);
		if(out == nil)
			return nil;

		/* Compute matrix product */
		for(i = 0; i < outdims[0]; i++){
			for(k = 0; k < outdims[1]; k++){
				sum = 0.0;
				for(j = 0; j < inputs[0]->dims[1]; j++){
					idx[0] = i; idx[1] = j;
					val = tensorget(inputs[0], idx);
					idx[0] = j; idx[1] = k;
					val *= tensorget(inputs[1], idx);
					sum += val;
				}
				outidx[0] = i; outidx[1] = k;
				tensorset(out, outidx, sum);
			}
		}
		return out;
	}

	if(strcmp(notation, "i,i->") == 0){
		/* Dot product */
		if(ninputs != 2)
			return nil;
		if(inputs[0]->ndims != 1 || inputs[1]->ndims != 1)
			return nil;
		if(inputs[0]->dims[0] != inputs[1]->dims[0])
			return nil;

		outdims[0] = 1;
		noutdims = 1;

		out = tensoralloc(TensorFloat32, noutdims, outdims);
		if(out == nil)
			return nil;

		sum = 0.0;
		for(i = 0; i < inputs[0]->dims[0]; i++){
			idx[0] = i;
			sum += tensorget(inputs[0], idx) * tensorget(inputs[1], idx);
		}
		outidx[0] = 0;
		tensorset(out, outidx, sum);
		return out;
	}

	if(strcmp(notation, "ij->ji") == 0){
		/* Transpose */
		if(ninputs != 1)
			return nil;
		if(inputs[0]->ndims != 2)
			return nil;

		outdims[0] = inputs[0]->dims[1];
		outdims[1] = inputs[0]->dims[0];
		noutdims = 2;

		out = tensoralloc(TensorFloat32, noutdims, outdims);
		if(out == nil)
			return nil;

		for(i = 0; i < inputs[0]->dims[0]; i++){
			for(j = 0; j < inputs[0]->dims[1]; j++){
				idx[0] = i; idx[1] = j;
				val = tensorget(inputs[0], idx);
				outidx[0] = j; outidx[1] = i;
				tensorset(out, outidx, val);
			}
		}
		return out;
	}

	return nil;
}

/*
 * Tensor-based truth value computation
 * Combines neural embedding similarity with logical truth
 */
float
tensortv(Tensor *a, Tensor *b)
{
	float dot, norma, normb, sim;
	int i;
	int idx[1];
	float va, vb;

	if(a->ndims != 1 || b->ndims != 1)
		return 0.0;
	if(a->dims[0] != b->dims[0])
		return 0.0;

	/* Compute cosine similarity */
	dot = 0.0;
	norma = 0.0;
	normb = 0.0;

	for(i = 0; i < a->dims[0]; i++){
		idx[0] = i;
		va = tensorget(a, idx);
		vb = tensorget(b, idx);
		dot += va * vb;
		norma += va * va;
		normb += vb * vb;
	}

	if(norma < 0.0001 || normb < 0.0001)
		return 0.0;

	sim = dot / (sqrt(norma) * sqrt(normb));

	/* Map similarity to truth value [0, 1] */
	return (sim + 1.0) / 2.0;
}

/*
 * Create embedding for a symbol
 */
Tensor*
tensorembed(char *symbol)
{
	Tensor *t;
	int dims[1];
	int i;
	int idx[1];
	ulong hash;
	float val;

	dims[0] = EMBEDDING_DIM;
	t = tensoralloc(TensorFloat32, 1, dims);
	if(t == nil)
		return nil;

	/* Generate pseudo-random embedding from symbol hash */
	hash = 0;
	for(i = 0; symbol[i]; i++)
		hash = hash * 31 + symbol[i];

	for(i = 0; i < EMBEDDING_DIM; i++){
		hash = hash * 1103515245 + 12345;
		val = ((float)(hash % 10000) / 10000.0) * 2.0 - 1.0;
		idx[0] = i;
		tensorset(t, idx, val);
	}

	return t;
}

/*
 * Logical implication in tensor space
 * A -> B is computed as tensor operation
 */
Tensor*
tensorimply(Tensor *a, Tensor *b)
{
	Tensor *inputs[2];

	inputs[0] = a;
	inputs[1] = b;

	/* Use dot product as implication strength */
	return tensoreinsum("i,i->", inputs, 2);
}

/*
 * Tensor-based deduction rule
 * Given A->B and B->C, compute A->C
 */
float
tensordeduction(Tensor *ab, Tensor *bc)
{
	float sab, sbc, sac;
	int idx[1];

	/* Get truth values from tensors */
	idx[0] = 0;
	sab = tensorget(ab, idx);
	sbc = tensorget(bc, idx);

	/* PLN deduction formula in tensor form */
	sac = sab * sbc;

	return sac;
}

/*
 * Softmax activation for attention
 */
void
tensorsoftmax(Tensor *t)
{
	float max, sum, val;
	int i;
	int idx[1];
	float *fp;

	if(t->ndims != 1)
		return;

	fp = (float*)t->data;

	/* Find max for numerical stability */
	max = fp[0];
	for(i = 1; i < t->dims[0]; i++){
		if(fp[i] > max)
			max = fp[i];
	}

	/* Compute exp and sum */
	sum = 0.0;
	for(i = 0; i < t->dims[0]; i++){
		fp[i] = exp(fp[i] - max);
		sum += fp[i];
	}

	/* Normalize */
	if(sum > 0.0001){
		for(i = 0; i < t->dims[0]; i++)
			fp[i] /= sum;
	}
}

/*
 * Tensor attention mechanism
 * Core component of transformer-style reasoning
 */
Tensor*
tensorattention(Tensor *query, Tensor *key, Tensor *value)
{
	Tensor *scores, *weights, *out;
	Tensor *inputs[2];
	int dims[1];
	float scale;
	int i;
	int idx[1];
	float *fp;

	if(query->ndims != 1 || key->ndims != 1 || value->ndims != 1)
		return nil;

	/* Compute attention scores: Q * K^T */
	inputs[0] = query;
	inputs[1] = key;
	scores = tensoreinsum("i,i->", inputs, 2);
	if(scores == nil)
		return nil;

	/* Scale by sqrt(dim) */
	scale = 1.0 / sqrt((float)query->dims[0]);
	idx[0] = 0;
	fp = (float*)scores->data;
	fp[0] *= scale;

	/* Softmax (trivial for single score) */
	fp[0] = 1.0 / (1.0 + exp(-fp[0]));  /* Sigmoid for single value */

	/* Apply attention to value */
	dims[0] = value->dims[0];
	out = tensoralloc(TensorFloat32, 1, dims);
	if(out == nil){
		tensorfree(scores);
		return nil;
	}

	for(i = 0; i < value->dims[0]; i++){
		idx[0] = i;
		tensorset(out, idx, tensorget(value, idx) * fp[0]);
	}

	tensorfree(scores);
	return out;
}

/*
 * Export tensor to AtomSpace as TruthValue
 */
void
tensor2tv(Tensor *t, float *strength, float *confidence)
{
	float s, c;
	int idx[1];

	if(t == nil || t->nelem < 1){
		*strength = 0.0;
		*confidence = 0.0;
		return;
	}

	if(t->type == TensorTruthValue && t->nelem >= 1){
		float *fp = (float*)t->data;
		*strength = fp[0];
		*confidence = fp[1];
		return;
	}

	/* Convert single value to truth value */
	idx[0] = 0;
	s = tensorget(t, idx);

	/* Clamp to [0, 1] */
	if(s < 0.0) s = 0.0;
	if(s > 1.0) s = 1.0;

	/* Confidence based on tensor certainty */
	c = 0.9;  /* High confidence for computed values */

	*strength = s;
	*confidence = c;
}

/*
 * Statistics for tensor system
 */
void
cogtensorstats(int *nalloc, int *nfree, ulong *nextid)
{
	int free_count;
	Tensor *t;

	qlock(&tensorsys);
	*nalloc = tensorsys.nalloc;
	*nextid = tensorsys.nextid;

	free_count = 0;
	for(t = tensorsys.free; t != nil; t = t->next)
		free_count++;
	*nfree = free_count;

	qunlock(&tensorsys);
}

/* Exported tensor functions for kernel use */
Tensor* cogtensornew(int type, int ndims, int *dims) { return tensoralloc(type, ndims, dims); }
void cogtensordel(Tensor *t) { tensorfree(t); }
float cogtensorgetf(Tensor *t, int *idx) { return tensorget(t, idx); }
void cogtensorsetf(Tensor *t, int *idx, float v) { tensorset(t, idx, v); }
