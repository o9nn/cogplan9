/*
 * cogfs - Cognitive File Server
 * Exposes AtomSpace operations via 9P protocol
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <plan9cog.h>

enum {
	Qroot,
	Qatoms,
	Qatomctl,
	Qpln,
	Qplnctl,
	Qecan,
	Qecanctl,
	Qpattern,
	Qstats,
	Qctl,
};

typedef struct Aux Aux;
struct Aux {
	int qid;
};

Plan9Cog *cogctx;

static void
fsread(Req *r)
{
	Aux *aux;
	char buf[8192];
	CogInfo info;
	
	aux = r->fid->file->aux;
	
	switch(aux->qid){
	case Qroot:
		/* Directory listing handled by lib9p */
		respond(r, nil);
		break;
		
	case Qstats:
		coginfo(cogctx, &info);
		snprint(buf, sizeof(buf),
			"version: %s\n"
			"uptime: %ld\n"
			"atoms: %ld\n"
			"rules: %ld\n"
			"inferences: %ld\n"
			"memory: %ld\n",
			info.version, info.uptime, info.natoms,
			info.nrules, info.ninferences, info.cogmem);
		readstr(r, buf);
		respond(r, nil);
		break;
		
	case Qatoms:
		/* List all atoms */
		snprint(buf, sizeof(buf), "Total atoms: %ld\n", 
			cogctx->atomspace->natoms);
		readstr(r, buf);
		respond(r, nil);
		break;
		
	default:
		respond(r, "not implemented");
		break;
	}
}

static void
fswrite(Req *r)
{
	Aux *aux;
	char *cmd;
	
	aux = r->fid->file->aux;
	
	switch(aux->qid){
	case Qatomctl:
		/* Atom control operations */
		cmd = r->ifcall.data;
		if(strncmp(cmd, "create ", 7) == 0){
			/* Create new atom */
			r->ofcall.count = r->ifcall.count;
			respond(r, nil);
		} else {
			respond(r, "unknown command");
		}
		break;
		
	case Qplnctl:
		/* PLN control operations */
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
		break;
		
	default:
		respond(r, "not implemented");
		break;
	}
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	return nil;
}

static void
fsopen(Req *r)
{
	respond(r, nil);
}

static void
fscreate(Req *r)
{
	respond(r, "not supported");
}

static void
fsremove(Req *r)
{
	respond(r, "not supported");
}

Srv cogfs = {
	.read = fsread,
	.write = fswrite,
	.open = fsopen,
	.create = fscreate,
	.remove = fsremove,
};

static void
createfiles(File *root)
{
	Aux *aux;
	File *f;
	
	/* atoms file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qatoms;
	f = createfile(root, "atoms", nil, 0444, aux);
	
	/* atomctl file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qatomctl;
	f = createfile(root, "atomctl", nil, 0666, aux);
	
	/* pln file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qpln;
	f = createfile(root, "pln", nil, 0444, aux);
	
	/* plnctl file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qplnctl;
	f = createfile(root, "plnctl", nil, 0666, aux);
	
	/* ecan file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qecan;
	f = createfile(root, "ecan", nil, 0444, aux);
	
	/* ecanctl file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qecanctl;
	f = createfile(root, "ecanctl", nil, 0666, aux);
	
	/* pattern file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qpattern;
	f = createfile(root, "pattern", nil, 0444, aux);
	
	/* stats file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qstats;
	f = createfile(root, "stats", nil, 0444, aux);
	
	/* ctl file */
	aux = mallocz(sizeof(Aux), 1);
	aux->qid = Qctl;
	f = createfile(root, "ctl", nil, 0666, aux);
}

void
usage(void)
{
	fprint(2, "usage: cogfs [-m mtpt] [-s srvname]\n");
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	char *mtpt, *srvname;
	
	mtpt = "/mnt/cog";
	srvname = nil;
	
	ARGBEGIN{
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 's':
		srvname = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND
	
	/* Initialize Plan9Cog */
	cogctx = plan9coginit();
	if(cogctx == nil)
		sysfatal("plan9coginit: %r");
	
	/* Create file tree */
	cogfs.tree = alloctree(nil, nil, DMDIR|0555, nil);
	createfiles(cogfs.tree->root);
	
	/* Start 9P service */
	threadpostmountsrv(&cogfs, srvname, mtpt, MREPL);
	
	threadexits(nil);
}
