/*
 * cogctl - Cognitive system control utility
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <plan9cog.h>

void
usage(void)
{
	fprint(2, "usage: cogctl [-a atomspace] command [args...]\n");
	fprint(2, "commands:\n");
	fprint(2, "  atom create <type> <name>     - create atom\n");
	fprint(2, "  atom list                     - list atoms\n");
	fprint(2, "  atom info <id>                - show atom info\n");
	fprint(2, "  pln infer <query>             - run PLN inference\n");
	fprint(2, "  pln stats                     - show PLN statistics\n");
	fprint(2, "  ecan update                   - update attention allocation\n");
	fprint(2, "  ecan focus                    - show attentional focus\n");
	fprint(2, "  info                          - show system info\n");
	exits("usage");
}

void
cmdatomcreate(Plan9Cog *p9c, int argc, char **argv)
{
	Atom *a;
	int type;
	char *name;
	
	if(argc < 2)
		usage();
	
	type = atoi(argv[0]);
	name = argv[1];
	
	a = atomcreate(p9c->atomspace, type, name);
	if(a == nil)
		sysfatal("atomcreate: %r");
	
	print("Created atom %ld: %s\n", a->id, cogatomstr(a));
}

void
cmdatomlist(Plan9Cog *p9c)
{
	int i;
	Atom *a;
	
	print("Total atoms: %d\n", p9c->atomspace->natoms);
	
	for(i = 0; i < p9c->atomspace->natoms; i++){
		a = p9c->atomspace->atoms[i];
		if(a)
			print("  %s  %s\n", cogatomstr(a), cogtvstr(a->tv));
	}
}

void
cmdatominfo(Plan9Cog *p9c, int argc, char **argv)
{
	Atom *a;
	ulong id;
	
	if(argc < 1)
		usage();
	
	id = atol(argv[0]);
	a = atomfind(p9c->atomspace, id);
	if(a == nil){
		fprint(2, "atom %ld not found\n", id);
		exits("notfound");
	}
	
	print("Atom Information:\n");
	print("  ID: %ld\n", a->id);
	print("  Type: %d\n", a->type);
	print("  Name: %s\n", a->name ? a->name : "(link)");
	print("  Outgoing: %d\n", a->noutgoing);
	print("  Truth Value: %s\n", cogtvstr(a->tv));
	print("  STI: %d\n", a->av.sti);
	print("  LTI: %d\n", a->av.lti);
}

void
cmdplnstats(Plan9Cog *p9c)
{
	PlnStats stats;
	
	plnstats(p9c->pln, &stats);
	
	print("PLN Statistics:\n");
	print("  Total inferences: %ld\n", stats.inferences);
	print("  Forward steps: %ld\n", stats.forward);
	print("  Backward steps: %ld\n", stats.backward);
	print("  Rule matches: %ld\n", stats.rulematch);
	print("  TV computations: %ld\n", stats.tvcompute);
}

void
cmdecanupdate(Plan9Cog *p9c)
{
	EcanNetwork *ecan;
	
	ecan = ecaninit(p9c->atomspace, 1000, 1000);
	if(ecan == nil)
		sysfatal("ecaninit: %r");
	
	ecanupdate(ecan);
	print("ECAN updated\n");
	
	ecanfree(ecan);
}

void
cmdecanfocus(Plan9Cog *p9c)
{
	EcanNetwork *ecan;
	Atom **focus;
	int nfocus, i;
	
	ecan = ecaninit(p9c->atomspace, 1000, 1000);
	if(ecan == nil)
		sysfatal("ecaninit: %r");
	
	ecanupdate(ecan);
	focus = ecanfocus(ecan, &nfocus);
	
	print("Attentional Focus (%d atoms):\n", nfocus);
	for(i = 0; i < nfocus; i++){
		print("  %s  STI=%d\n", cogatomstr(focus[i]), focus[i]->av.sti);
	}
	
	ecanfree(ecan);
}

void
cmdinfo(Plan9Cog *p9c)
{
	CogInfo info;
	
	coginfo(p9c, &info);
	
	print("Plan9Cog System Information:\n");
	print("  Version: %s\n", info.version);
	print("  Uptime: %ld\n", info.uptime);
	print("  Total atoms: %ld\n", info.natoms);
	print("  Total rules: %ld\n", info.nrules);
	print("  Total inferences: %ld\n", info.ninferences);
	print("  Cognitive memory: %ld bytes\n", info.cogmem);
}

void
main(int argc, char **argv)
{
	Plan9Cog *p9c;
	char *cmd;
	
	ARGBEGIN{
	case 'a':
		/* TODO: Load atomspace file */
		EARGF(usage());
		break;
	default:
		usage();
	}ARGEND
	
	if(argc < 1)
		usage();
	
	/* Initialize Plan9Cog */
	p9c = plan9coginit();
	if(p9c == nil)
		sysfatal("plan9coginit: %r");
	
	cmd = argv[0];
	argc--;
	argv++;
	
	/* Dispatch commands */
	if(strcmp(cmd, "atom") == 0){
		if(argc < 1)
			usage();
		if(strcmp(argv[0], "create") == 0){
			cmdatomcreate(p9c, argc-1, argv+1);
		} else if(strcmp(argv[0], "list") == 0){
			cmdatomlist(p9c);
		} else if(strcmp(argv[0], "info") == 0){
			cmdatominfo(p9c, argc-1, argv+1);
		} else {
			usage();
		}
	} else if(strcmp(cmd, "pln") == 0){
		if(argc < 1)
			usage();
		if(strcmp(argv[0], "stats") == 0){
			cmdplnstats(p9c);
		} else {
			usage();
		}
	} else if(strcmp(cmd, "ecan") == 0){
		if(argc < 1)
			usage();
		if(strcmp(argv[0], "update") == 0){
			cmdecanupdate(p9c);
		} else if(strcmp(argv[0], "focus") == 0){
			cmdecanfocus(p9c);
		} else {
			usage();
		}
	} else if(strcmp(cmd, "info") == 0){
		cmdinfo(p9c);
	} else {
		usage();
	}
	
	plan9cogfree(p9c);
	exits(nil);
}
