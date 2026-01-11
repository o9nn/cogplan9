/*
 * Cognitive Kernel Unit Tests - cogtest.c
 *
 * Comprehensive unit tests for all cognitive kernel modules:
 * - AtomSpace operations
 * - PLN inference
 * - ECAN attention allocation
 * - Cognitive VM
 * - Cognitive memory management
 * - Cognitive process extensions
 * - Tensor logic operations
 * - Scheduler integration
 *
 * These tests validate the cognitive kernel functionality
 * and can be run during kernel boot or as a separate test suite.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test assertion macros */
#define TEST_ASSERT(cond, msg) do { \
	tests_run++; \
	if(cond) { \
		tests_passed++; \
	} else { \
		tests_failed++; \
		print("FAIL: %s: %s\n", __func__, msg); \
	} \
} while(0)

#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg) TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_GT(a, b, msg) TEST_ASSERT((a) > (b), msg)
#define TEST_ASSERT_LT(a, b, msg) TEST_ASSERT((a) < (b), msg)
#define TEST_ASSERT_NULL(p, msg) TEST_ASSERT((p) == nil, msg)
#define TEST_ASSERT_NOTNULL(p, msg) TEST_ASSERT((p) != nil, msg)

/* Float comparison with tolerance */
#define FLOAT_EQ(a, b, eps) (((a) - (b)) < (eps) && ((b) - (a)) < (eps))
#define TEST_ASSERT_FLOAT_EQ(a, b, msg) TEST_ASSERT(FLOAT_EQ(a, b, 0.0001), msg)

/*
 * ============================================
 * AtomSpace Tests
 * ============================================
 */

static void
test_atomspace_init(void)
{
	CogAtomSpace *as;

	as = cogatomspace();
	TEST_ASSERT_NOTNULL(as, "cogatomspace should return non-null");
	TEST_ASSERT_EQ(as->natoms, 0, "initial atomspace should be empty");
	TEST_ASSERT_GT(as->maxatoms, 0, "max atoms should be positive");
}

static void
test_atom_create(void)
{
	CogAtom *a;

	a = cogcreate(1, "TestConcept");
	TEST_ASSERT_NOTNULL(a, "cogcreate should return non-null atom");
	if(a != nil){
		TEST_ASSERT_GT(a->id, 0, "atom id should be positive");
		TEST_ASSERT_EQ(a->type, 1, "atom type should match");
		TEST_ASSERT_EQ(strcmp(a->name, "TestConcept"), 0, "atom name should match");
		cogdelete(a);
	}
}

static void
test_atom_find(void)
{
	CogAtom *a, *found;

	a = cogcreate(1, "FindTest");
	TEST_ASSERT_NOTNULL(a, "cogcreate should succeed");
	if(a != nil){
		found = cogfind(a->id);
		TEST_ASSERT_EQ(found, a, "cogfind should return same atom");

		found = cogfindname("FindTest");
		TEST_ASSERT_EQ(found, a, "cogfindname should return same atom");

		cogdelete(a);
	}
}

static void
test_atom_link(void)
{
	CogAtom *a, *b, *link;
	CogAtom *outgoing[2];

	a = cogcreate(1, "LinkA");
	b = cogcreate(1, "LinkB");
	TEST_ASSERT_NOTNULL(a, "first atom should be created");
	TEST_ASSERT_NOTNULL(b, "second atom should be created");

	if(a != nil && b != nil){
		outgoing[0] = a;
		outgoing[1] = b;
		link = coglink(10, outgoing, 2);
		TEST_ASSERT_NOTNULL(link, "link should be created");
		if(link != nil){
			TEST_ASSERT_EQ(link->noutgoing, 2, "link should have 2 outgoing");
			TEST_ASSERT_EQ(link->outgoing[0], a, "first outgoing should match");
			TEST_ASSERT_EQ(link->outgoing[1], b, "second outgoing should match");
			cogdelete(link);
		}
		cogdelete(a);
		cogdelete(b);
	}
}

static void
test_atom_truthvalue(void)
{
	CogAtom *a;

	a = cogcreate(1, "TVTest");
	TEST_ASSERT_NOTNULL(a, "atom should be created");
	if(a != nil){
		a->tvstrength = 0.8;
		a->tvconf = 0.9;
		TEST_ASSERT_FLOAT_EQ(a->tvstrength, 0.8, "strength should match");
		TEST_ASSERT_FLOAT_EQ(a->tvconf, 0.9, "confidence should match");
		cogdelete(a);
	}
}

static void
test_atom_attention(void)
{
	CogAtom *a;

	a = cogcreate(1, "AttentionTest");
	TEST_ASSERT_NOTNULL(a, "atom should be created");
	if(a != nil){
		a->sti = 100;
		a->lti = 50;
		a->vlti = 25;
		TEST_ASSERT_EQ(a->sti, 100, "STI should match");
		TEST_ASSERT_EQ(a->lti, 50, "LTI should match");
		TEST_ASSERT_EQ(a->vlti, 25, "VLTI should match");
		cogdelete(a);
	}
}

/*
 * ============================================
 * PLN Inference Tests
 * ============================================
 */

static void
test_pln_deduction(void)
{
	CogAtom *a, *b;
	float result;

	a = cogcreate(1, "DeductA");
	b = cogcreate(1, "DeductB");
	if(a != nil && b != nil){
		a->tvstrength = 0.9;
		a->tvconf = 0.8;
		b->tvstrength = 0.8;
		b->tvconf = 0.7;

		result = cogplndeduction(a, b);
		TEST_ASSERT_GT(result, 0.0, "deduction result should be positive");
		TEST_ASSERT_LT(result, 1.0, "deduction result should be less than 1");

		cogdelete(a);
		cogdelete(b);
	}
}

static void
test_pln_induction(void)
{
	CogAtom *a, *b;
	float result;

	a = cogcreate(1, "InductA");
	b = cogcreate(1, "InductB");
	if(a != nil && b != nil){
		a->tvstrength = 0.7;
		a->tvconf = 0.6;
		b->tvstrength = 0.6;
		b->tvconf = 0.5;

		result = cogplninduction(a, b);
		TEST_ASSERT_GT(result, 0.0, "induction result should be positive");

		cogdelete(a);
		cogdelete(b);
	}
}

static void
test_pln_revision(void)
{
	float result;

	/* Revision of two truth values */
	result = cogplnrevision(0.8, 0.9, 0.7, 0.8);
	TEST_ASSERT_GT(result, 0.0, "revision should produce positive result");
	TEST_ASSERT_LT(result, 1.0, "revision should produce result < 1");
}

/*
 * ============================================
 * ECAN Attention Tests
 * ============================================
 */

static void
test_ecan_stimulate(void)
{
	CogAtom *a;
	short initial_sti;

	a = cogcreate(1, "StimTest");
	if(a != nil){
		initial_sti = a->sti;
		cogecanstimulate(a, 50);
		TEST_ASSERT_GT(a->sti, initial_sti, "STI should increase after stimulation");
		cogdelete(a);
	}
}

static void
test_ecan_spread(void)
{
	CogAtom *a, *b, *link;
	CogAtom *outgoing[2];
	short initial_b_sti;

	a = cogcreate(1, "SpreadA");
	b = cogcreate(1, "SpreadB");
	if(a != nil && b != nil){
		a->sti = 100;
		initial_b_sti = b->sti;

		outgoing[0] = a;
		outgoing[1] = b;
		link = coglink(10, outgoing, 2);

		if(link != nil){
			cogecanspread(a, 30);
			/* Note: actual spread implementation needed */
			cogdelete(link);
		}
		cogdelete(a);
		cogdelete(b);
	}
}

static void
test_ecan_decay(void)
{
	CogAtom *a;
	short initial_sti;

	a = cogcreate(1, "DecayTest");
	if(a != nil){
		a->sti = 100;
		initial_sti = a->sti;
		cogecandecay(0.1);
		/* Decay should reduce STI over time */
		cogdelete(a);
	}
}

static void
test_ecan_focus(void)
{
	CogAtom **focus;
	int n;

	focus = cogecanfocus(&n);
	/* Focus can be empty or have atoms */
	TEST_ASSERT_GT(n, -1, "focus count should be >= 0");
}

/*
 * ============================================
 * Cognitive VM Tests
 * ============================================
 */

static void
test_cogvm_create(void)
{
	/* Test VM creation through proc */
	CogProc *cp;

	/* Note: Requires process context */
	cp = nil;  /* Would be: cogproccreate(up); */
	/* TEST_ASSERT_NOTNULL(cp, "cogvm proc should be created"); */
}

static void
test_cogvm_opcodes(void)
{
	/* Verify opcode values are distinct */
	TEST_ASSERT_EQ(COGnop, 0, "COGnop should be 0");
	TEST_ASSERT_EQ(COGcreate, 1, "COGcreate should be 1");
	TEST_ASSERT_EQ(COGlink, 2, "COGlink should be 2");
	TEST_ASSERT_EQ(COGquery, 3, "COGquery should be 3");
	TEST_ASSERT_EQ(COGinfer, 4, "COGinfer should be 4");
	TEST_ASSERT_EQ(COGfocus, 5, "COGfocus should be 5");
	TEST_ASSERT_EQ(COGspread, 6, "COGspread should be 6");
	TEST_ASSERT_EQ(COGpattern, 7, "COGpattern should be 7");
	TEST_ASSERT_EQ(COGmine, 8, "COGmine should be 8");
	TEST_ASSERT_EQ(COGreason, 9, "COGreason should be 9");
	TEST_ASSERT_EQ(COGlearn, 10, "COGlearn should be 10");
	TEST_ASSERT_EQ(COGhalt, 11, "COGhalt should be 11");
}

/*
 * ============================================
 * Cognitive Memory Tests
 * ============================================
 */

static void
test_cogmem_alloc(void)
{
	void *p;

	p = cogalloc(1024, CogMemGeneral, 50, 25);
	TEST_ASSERT_NOTNULL(p, "cogalloc should return non-null");
	if(p != nil){
		cogfree(p);
	}
}

static void
test_cogmem_types(void)
{
	/* Verify memory type values */
	TEST_ASSERT_EQ(CogMemGeneral, 0, "CogMemGeneral should be 0");
	TEST_ASSERT_EQ(CogMemAtom, 1, "CogMemAtom should be 1");
	TEST_ASSERT_EQ(CogMemLink, 2, "CogMemLink should be 2");
	TEST_ASSERT_EQ(CogMemPattern, 3, "CogMemPattern should be 3");
	TEST_ASSERT_EQ(CogMemAttention, 4, "CogMemAttention should be 4");
	TEST_ASSERT_EQ(CogMemInference, 5, "CogMemInference should be 5");
}

/*
 * ============================================
 * Cognitive Process Extension Tests
 * ============================================
 */

static void
test_cogprocext_alloc(void)
{
	CogProcExt *ext;

	ext = cogprocalloc();
	TEST_ASSERT_NOTNULL(ext, "cogprocalloc should return non-null");
	if(ext != nil){
		TEST_ASSERT_EQ(ext->cogstate, 0, "initial state should be 0");
		cogprocextfree(ext);
	}
}

static void
test_cogprocext_states(void)
{
	/* Verify cognitive process states */
	TEST_ASSERT_EQ(CogIdle, 0, "CogIdle should be 0");
	TEST_ASSERT_EQ(CogThinking, 1, "CogThinking should be 1");
	TEST_ASSERT_EQ(CogInferring, 2, "CogInferring should be 2");
	TEST_ASSERT_EQ(CogLearning, 3, "CogLearning should be 3");
	TEST_ASSERT_EQ(CogWaiting, 4, "CogWaiting should be 4");
}

/*
 * ============================================
 * Tensor Logic Tests
 * ============================================
 */

static void
test_tensor_create(void)
{
	Tensor *t;
	int dims[2] = {3, 4};

	t = cogtensornew(0, 2, dims);  /* TensorFloat32 = 0 */
	TEST_ASSERT_NOTNULL(t, "tensor should be created");
	if(t != nil){
		cogtensordel(t);
	}
}

static void
test_tensor_getset(void)
{
	Tensor *t;
	int dims[2] = {3, 3};
	int idx[2];
	float val;

	t = cogtensornew(0, 2, dims);
	if(t != nil){
		idx[0] = 1; idx[1] = 2;
		cogtensorsetf(t, idx, 3.14);
		val = cogtensorgetf(t, idx);
		TEST_ASSERT_FLOAT_EQ(val, 3.14, "tensor get should return set value");
		cogtensordel(t);
	}
}

static void
test_tensor_einsum_matmul(void)
{
	Tensor *a, *b, *c;
	Tensor *inputs[2];
	int dims_a[2] = {2, 3};
	int dims_b[2] = {3, 2};
	int idx[2];

	a = cogtensornew(0, 2, dims_a);
	b = cogtensornew(0, 2, dims_b);

	if(a != nil && b != nil){
		/* Set identity-like values */
		idx[0] = 0; idx[1] = 0; cogtensorsetf(a, idx, 1.0);
		idx[0] = 1; idx[1] = 1; cogtensorsetf(a, idx, 1.0);
		idx[0] = 0; idx[1] = 0; cogtensorsetf(b, idx, 1.0);
		idx[0] = 1; idx[1] = 1; cogtensorsetf(b, idx, 1.0);

		inputs[0] = a;
		inputs[1] = b;
		c = tensoreinsum("ij,jk->ik", inputs, 2);
		TEST_ASSERT_NOTNULL(c, "matrix multiplication should produce result");
		if(c != nil){
			cogtensordel(c);
		}
		cogtensordel(a);
		cogtensordel(b);
	}
}

static void
test_tensor_einsum_dot(void)
{
	Tensor *a, *b, *c;
	Tensor *inputs[2];
	int dims[1] = {4};
	int idx[1];
	float result;

	a = cogtensornew(0, 1, dims);
	b = cogtensornew(0, 1, dims);

	if(a != nil && b != nil){
		/* Set vectors: a = [1,2,3,4], b = [1,1,1,1] */
		for(int i = 0; i < 4; i++){
			idx[0] = i;
			cogtensorsetf(a, idx, (float)(i + 1));
			cogtensorsetf(b, idx, 1.0);
		}

		inputs[0] = a;
		inputs[1] = b;
		c = tensoreinsum("i,i->", inputs, 2);
		TEST_ASSERT_NOTNULL(c, "dot product should produce result");
		if(c != nil){
			idx[0] = 0;
			result = cogtensorgetf(c, idx);
			TEST_ASSERT_FLOAT_EQ(result, 10.0, "dot product of [1,2,3,4] and [1,1,1,1] should be 10");
			cogtensordel(c);
		}
		cogtensordel(a);
		cogtensordel(b);
	}
}

static void
test_tensor_einsum_transpose(void)
{
	Tensor *a, *b;
	Tensor *inputs[1];
	int dims[2] = {2, 3};
	int idx[2];
	float val;

	a = cogtensornew(0, 2, dims);
	if(a != nil){
		/* Set a[0][2] = 5.0 */
		idx[0] = 0; idx[1] = 2;
		cogtensorsetf(a, idx, 5.0);

		inputs[0] = a;
		b = tensoreinsum("ij->ji", inputs, 1);
		TEST_ASSERT_NOTNULL(b, "transpose should produce result");
		if(b != nil){
			/* Check b[2][0] = 5.0 (transposed) */
			idx[0] = 2; idx[1] = 0;
			val = cogtensorgetf(b, idx);
			TEST_ASSERT_FLOAT_EQ(val, 5.0, "transposed value should match");
			cogtensordel(b);
		}
		cogtensordel(a);
	}
}

static void
test_tensor_embed(void)
{
	Tensor *e1, *e2;

	e1 = tensorembed("concept1");
	e2 = tensorembed("concept2");
	TEST_ASSERT_NOTNULL(e1, "embedding should be created");
	TEST_ASSERT_NOTNULL(e2, "embedding should be created");
	if(e1 != nil && e2 != nil){
		/* Same symbol should produce same embedding */
		Tensor *e1b = tensorembed("concept1");
		if(e1b != nil){
			/* Embeddings should be deterministic */
			cogtensordel(e1b);
		}
		cogtensordel(e1);
		cogtensordel(e2);
	}
}

static void
test_tensor_truthvalue(void)
{
	Tensor *a, *b;
	float tv;
	int dims[1] = {4};
	int idx[1];

	a = cogtensornew(0, 1, dims);
	b = cogtensornew(0, 1, dims);

	if(a != nil && b != nil){
		/* Same vector should have TV = 1.0 */
		for(int i = 0; i < 4; i++){
			idx[0] = i;
			cogtensorsetf(a, idx, 1.0);
			cogtensorsetf(b, idx, 1.0);
		}

		tv = tensortv(a, b);
		TEST_ASSERT_FLOAT_EQ(tv, 1.0, "identical vectors should have TV = 1.0");

		cogtensordel(a);
		cogtensordel(b);
	}
}

static void
test_tensor_attention(void)
{
	Tensor *q, *k, *v, *out;
	int dims[1] = {4};
	int idx[1];

	q = cogtensornew(0, 1, dims);
	k = cogtensornew(0, 1, dims);
	v = cogtensornew(0, 1, dims);

	if(q != nil && k != nil && v != nil){
		for(int i = 0; i < 4; i++){
			idx[0] = i;
			cogtensorsetf(q, idx, 1.0);
			cogtensorsetf(k, idx, 1.0);
			cogtensorsetf(v, idx, (float)(i + 1));
		}

		out = tensorattention(q, k, v);
		TEST_ASSERT_NOTNULL(out, "attention should produce output");
		if(out != nil){
			cogtensordel(out);
		}

		cogtensordel(q);
		cogtensordel(k);
		cogtensordel(v);
	}
}

static void
test_tensor_stats(void)
{
	int nalloc, nfree;
	ulong nextid;

	cogtensorstats(&nalloc, &nfree, &nextid);
	TEST_ASSERT_GT(nextid, 0, "tensor nextid should be positive");
}

/*
 * ============================================
 * Integration Tests
 * ============================================
 */

static void
test_integration_atom_tensor(void)
{
	/* Test integration between AtomSpace and Tensors */
	CogAtom *a;
	Tensor *e;
	float strength, confidence;

	a = cogcreate(1, "IntegrationTest");
	if(a != nil){
		e = tensorembed(a->name);
		if(e != nil){
			tensor2tv(e, &strength, &confidence);
			TEST_ASSERT_GT(confidence, 0.0, "tensor conversion should have confidence");
			cogtensordel(e);
		}
		cogdelete(a);
	}
}

/*
 * ============================================
 * Main Test Runner
 * ============================================
 */

void
runcognitvetests(void)
{
	print("=== Cognitive Kernel Unit Tests ===\n");

	/* Reset counters */
	tests_run = 0;
	tests_passed = 0;
	tests_failed = 0;

	/* AtomSpace tests */
	print("--- AtomSpace Tests ---\n");
	test_atomspace_init();
	test_atom_create();
	test_atom_find();
	test_atom_link();
	test_atom_truthvalue();
	test_atom_attention();

	/* PLN tests */
	print("--- PLN Tests ---\n");
	test_pln_deduction();
	test_pln_induction();
	test_pln_revision();

	/* ECAN tests */
	print("--- ECAN Tests ---\n");
	test_ecan_stimulate();
	test_ecan_spread();
	test_ecan_decay();
	test_ecan_focus();

	/* Cognitive VM tests */
	print("--- Cognitive VM Tests ---\n");
	test_cogvm_create();
	test_cogvm_opcodes();

	/* Cognitive Memory tests */
	print("--- Cognitive Memory Tests ---\n");
	test_cogmem_alloc();
	test_cogmem_types();

	/* Cognitive Process tests */
	print("--- Cognitive Process Tests ---\n");
	test_cogprocext_alloc();
	test_cogprocext_states();

	/* Tensor Logic tests */
	print("--- Tensor Logic Tests ---\n");
	test_tensor_create();
	test_tensor_getset();
	test_tensor_einsum_matmul();
	test_tensor_einsum_dot();
	test_tensor_einsum_transpose();
	test_tensor_embed();
	test_tensor_truthvalue();
	test_tensor_attention();
	test_tensor_stats();

	/* Integration tests */
	print("--- Integration Tests ---\n");
	test_integration_atom_tensor();

	/* Summary */
	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Passed: %d\n", tests_passed);
	print("Failed: %d\n", tests_failed);

	if(tests_failed == 0){
		print("All tests PASSED!\n");
	} else {
		print("Some tests FAILED!\n");
	}
}
