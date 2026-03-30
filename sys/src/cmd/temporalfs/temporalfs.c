/*
 * temporalfs - Temporal AtomSpace File Server
 *
 * Exposes the temporal reasoning layer as a Plan 9 file hierarchy.
 * Atoms can be queried at arbitrary past times, snapshots taken and
 * restored, and history inspected — all via ordinary file operations.
 *
 * Mount point: /mnt/cog/temporal  (default)
 *
 * Hierarchy:
 *   /mnt/cog/temporal/
 *     now/
 *       atoms       - All atoms in current AtomSpace
 *       changed     - Atoms changed since last read
 *     t-1h/
 *       atoms       - Atoms active one hour ago
 *     t-1d/
 *       atoms       - Atoms active one day ago
 *     snap/
 *       <name>/
 *         atoms     - Atoms in named snapshot
 *         info      - Snapshot metadata (time, atom count)
 *       ctl         - Snapshot control: snap <name>, restore <name>, delete <name>
 *       list        - List all snapshots
 *     history/
 *       <atomid>    - Version history of specific atom
 *     prune         - Write timestamp to prune history before that time
 *     ctl           - Control: snap <name>, restore <name>, delete <name>
 *     stats         - Statistics
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <plan9cog/atomspace.h>
#include <plan9cog/temporal.h>

enum
{
	Qroot,

	/* /now/ */
	Qnow,
	Qnowatoms,
	Qnowchanged,

	/* /t-1h/ */
	Qt1h,
	Qt1hatoms,

	/* /t-1d/ */
	Qt1d,
	Qt1datoms,

	/* /snap/ */
	Qsnap,
	Qsnapctl,
	Qsnaplist,
	Qsnapdir,
	Qsnapatoms,
	Qsnapinfo,

	/* /history/ */
	Qhistory,
	Qhistoryfile,

	/* top-level control */
	Qprune,
	Qctl,
	Qstats,
};

typedef struct TFile TFile;
struct TFile
{
	int	qid;
	char	*snapname;	/* For snap sub-files */
	ulong	atomid;		/* For history files */
};

static TemporalSpace	*ts;
static AtomSpace	*as;

/*
 * Format an atom list into buf (at most size bytes).
 * Returns number of bytes written.
 */
static int
fmtatoms(char *buf, int size, Atom **atoms, int natoms)
{
	int i, n;
	char *s;

	s = buf;
	n = size;
	for(i = 0; i < natoms && n > 1; i++){
		Atom *a = atoms[i];
		TruthValue tv;
		int w;

		if(a == nil)
			continue;
		tv = atomgettruth(a);
		w = snprint(s, n, "%ld %d %s %.3f %.3f\n",
			a->id, a->type,
			a->name ? a->name : "",
			tv.strength, tv.confidence);
		if(w < 0)
			break;
		s += w;
		n -= w;
	}
	return (int)(s - buf);
}

/*
 * Read /now/atoms
 */
static void
readnowatoms(Req *r)
{
	char *buf;
	int n;

	buf = emalloc9p(65536);
	n = fmtatoms(buf, 65536, ts->current->atoms, ts->current->natoms);
	buf[n] = '\0';
	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read /now/changed
 */
static void
readnowchanged(Req *r)
{
	char *buf;
	Atom **atoms;
	int natoms, n;
	vlong since;

	/* Report atoms changed in the last minute */
	since = temporalnow() - TimeMin;
	atoms = temporalchanged(ts, since, &natoms);

	buf = emalloc9p(65536);
	n = fmtatoms(buf, 65536, atoms, natoms);
	buf[n] = '\0';
	readstr(r, buf);
	free(buf);
	if(atoms != ts->current->atoms)
		free(atoms);
	respond(r, nil);
}

/*
 * Read atoms active at a given past time offset
 */
static void
readpastatoms(Req *r, vlong offset)
{
	char *buf;
	Atom **atoms;
	int natoms, n;
	vlong t;

	t = temporalnow() - offset;
	atoms = temporalactive(ts, t, &natoms);

	buf = emalloc9p(65536);
	n = fmtatoms(buf, 65536, atoms, natoms);
	buf[n] = '\0';
	readstr(r, buf);
	free(buf);
	if(atoms != ts->current->atoms)
		free(atoms);
	respond(r, nil);
}

/*
 * Read /snap/list
 */
static void
readsnaplist(Req *r)
{
	char *buf, *s;
	Snapshot **snaps;
	int nsnaps, i, size;

	snaps = temporalsnapslist(ts, &nsnaps);
	size = 8192;
	buf = emalloc9p(size);
	s = buf;

	s += snprint(s, size - (s - buf), "# name time atoms\n");
	for(i = 0; i < nsnaps; i++){
		Snapshot *sn = snaps[i];
		s += snprint(s, size - (s - buf), "%s %s %d\n",
			sn->name,
			temporalstr(sn->time),
			sn->state ? sn->state->natoms : 0);
	}
	free(snaps);

	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read /snap/<name>/atoms
 */
static void
readsnapatoms(Req *r, char *snapname)
{
	char *buf;
	AtomSpace *state;
	int n;

	state = temporalrestore(ts, snapname);
	if(state == nil){
		respond(r, "snapshot not found");
		return;
	}

	buf = emalloc9p(65536);
	n = fmtatoms(buf, 65536, state->atoms, state->natoms);
	buf[n] = '\0';
	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read /snap/<name>/info
 */
static void
readsnapinfo(Req *r, char *snapname)
{
	char buf[512];
	Snapshot **snaps;
	int nsnaps, i;

	snaps = temporalsnapslist(ts, &nsnaps);
	for(i = 0; i < nsnaps; i++){
		Snapshot *sn = snaps[i];
		if(strcmp(sn->name, snapname) == 0){
			snprint(buf, sizeof buf,
				"name %s\n"
				"time %s\n"
				"atoms %d\n",
				sn->name,
				temporalstr(sn->time),
				sn->state ? sn->state->natoms : 0);
			free(snaps);
			readstr(r, buf);
			respond(r, nil);
			return;
		}
	}
	free(snaps);
	respond(r, "snapshot not found");
}

/*
 * Read /history/<atomid>
 */
static void
readhistory(Req *r, ulong atomid)
{
	char *buf, *s;
	TemporalAtom **hist;
	int nhist, i, size;

	hist = temporalhistory(ts, atomid, &nhist);
	size = 16384;
	buf = emalloc9p(size);
	s = buf;

	s += snprint(s, size - (s - buf), "# atom %ld history\n", atomid);
	s += snprint(s, size - (s - buf), "# tstart tend strength confidence\n");

	for(i = 0; i < nhist; i++){
		TemporalAtom *ta = hist[i];
		s += snprint(s, size - (s - buf), "%s %s %.3f %.3f\n",
			temporalstr(ta->tstart),
			ta->tend ? temporalstr(ta->tend) : "current",
			ta->tvat.strength, ta->tvat.confidence);
	}
	free(hist);

	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read /stats
 */
static void
readstats(Req *r)
{
	char buf[1024];
	Snapshot **snaps;
	int nsnaps;

	snaps = temporalsnapslist(ts, &nsnaps);
	free(snaps);

	snprint(buf, sizeof buf,
		"atoms_current %d\n"
		"history_slots %d\n"
		"snapshots %d\n"
		"granularity_ns %lld\n"
		"retention_ns %lld\n"
		"basepath %s\n",
		ts->current ? ts->current->natoms : 0,
		ts->nhistory,
		nsnaps,
		ts->granularity,
		ts->retention,
		ts->basepath ? ts->basepath : "");

	readstr(r, buf);
	respond(r, nil);
}

/*
 * Write /ctl or /snap/ctl
 *
 * Commands:
 *   snap <name>    - Create named snapshot
 *   restore <name> - Restore snapshot (replaces current)
 *   delete <name>  - Delete snapshot
 */
static void
writectl(Req *r)
{
	char *cmd;
	char *f[4];
	int nf;

	cmd = emalloc9p(r->ifcall.count + 1);
	memmove(cmd, r->ifcall.data, r->ifcall.count);
	cmd[r->ifcall.count] = '\0';

	nf = tokenize(cmd, f, nelem(f));
	if(nf < 2){
		free(cmd);
		respond(r, "usage: snap|restore|delete <name>");
		return;
	}

	if(strcmp(f[0], "snap") == 0){
		if(temporalsnap(ts, f[1]) == nil){
			free(cmd);
			respond(r, "snap failed");
			return;
		}
	}
	else if(strcmp(f[0], "restore") == 0){
		AtomSpace *state;
		state = temporalrestore(ts, f[1]);
		if(state == nil){
			free(cmd);
			respond(r, "snapshot not found");
			return;
		}
		/*
		 * Swap current to the restored snapshot state.
		 * The previous current AtomSpace is retained in history;
		 * callers should snap before restoring if they wish to
		 * preserve it explicitly.
		 */
		ts->current = state;
	}
	else if(strcmp(f[0], "delete") == 0){
		if(temporaldeletesnap(ts, f[1]) < 0){
			free(cmd);
			respond(r, "snapshot not found");
			return;
		}
	}
	else{
		free(cmd);
		respond(r, "unknown command");
		return;
	}

	free(cmd);
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

/*
 * Write /prune
 *
 * Write a timestamp string to prune history older than that time.
 * Format: "YYYY-MM-DD HH:MM:SS" or a relative offset like "-1h".
 */
static void
writeprune(Req *r)
{
	char buf[64];
	vlong t;
	int n;

	n = r->ifcall.count;
	if(n >= (int)sizeof buf)
		n = sizeof buf - 1;
	memmove(buf, r->ifcall.data, n);
	buf[n] = '\0';

	/* Strip trailing newline */
	if(n > 0 && buf[n-1] == '\n')
		buf[--n] = '\0';

	if(buf[0] == '-'){
		/* Relative offset: -1h, -1d, -30m */
		long val;
		char unit;
		val = atol(buf + 1);
		unit = buf[strlen(buf) - 1];
		switch(unit){
		case 's': t = temporalnow() - val * TimeSec; break;
		case 'm': t = temporalnow() - val * TimeMin; break;
		case 'h': t = temporalnow() - val * TimeHour; break;
		case 'd': t = temporalnow() - val * TimeDay; break;
		default:  t = temporalnow() - val * TimeSec; break;
		}
	} else {
		t = temporalparse(buf);
		if(t == 0){
			respond(r, "invalid time format");
			return;
		}
	}

	temporalprune(ts, t);
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

static void
fsread(Req *r)
{
	TFile *tf;

	tf = r->fid->file->aux;
	if(tf == nil){
		respond(r, "no file context");
		return;
	}

	switch(tf->qid){
	case Qnowatoms:
		readnowatoms(r);
		break;
	case Qnowchanged:
		readnowchanged(r);
		break;
	case Qt1hatoms:
		readpastatoms(r, TimeHour);
		break;
	case Qt1datoms:
		readpastatoms(r, TimeDay);
		break;
	case Qsnaplist:
		readsnaplist(r);
		break;
	case Qsnapatoms:
		readsnapatoms(r, tf->snapname);
		break;
	case Qsnapinfo:
		readsnapinfo(r, tf->snapname);
		break;
	case Qhistoryfile:
		readhistory(r, tf->atomid);
		break;
	case Qstats:
		readstats(r);
		break;
	default:
		respond(r, "not readable");
		break;
	}
}

static void
fswrite(Req *r)
{
	TFile *tf;

	tf = r->fid->file->aux;
	if(tf == nil){
		respond(r, "no file context");
		return;
	}

	switch(tf->qid){
	case Qctl:
	case Qsnapctl:
		writectl(r);
		break;
	case Qprune:
		writeprune(r);
		break;
	default:
		respond(r, "not writable");
		break;
	}
}

Srv fs = {
	.read  = fsread,
	.write = fswrite,
};

static TFile*
mktf(int qid, char *snapname, ulong atomid)
{
	TFile *tf;

	tf = emalloc9p(sizeof(TFile));
	tf->qid = qid;
	tf->snapname = snapname ? estrdup9p(snapname) : nil;
	tf->atomid = atomid;
	return tf;
}

void
usage(void)
{
	fprint(2, "usage: %s [-m mtpt]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *mtpt;
	File *root, *now, *t1h, *t1d, *snap, *history;
	TFile *tf;

	mtpt = "/mnt/cog/temporal";

	ARGBEGIN{
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	/* Initialize AtomSpace */
	as = atomspacecreate();
	if(as == nil)
		sysfatal("cannot create atomspace");

	/* Initialize TemporalSpace */
	ts = temporalinit(as);
	if(ts == nil)
		sysfatal("cannot initialize temporal space");

	/* Build file tree */
	fs.tree = alloctree(nil, nil, DMDIR|0555, nil);
	root = fs.tree->root;

	/* /now/ */
	tf = mktf(Qnow, nil, 0);
	now = createfile(root, "now", nil, DMDIR|0555, tf);

	tf = mktf(Qnowatoms, nil, 0);
	createfile(now, "atoms", nil, 0444, tf);

	tf = mktf(Qnowchanged, nil, 0);
	createfile(now, "changed", nil, 0444, tf);

	/* /t-1h/ */
	tf = mktf(Qt1h, nil, 0);
	t1h = createfile(root, "t-1h", nil, DMDIR|0555, tf);

	tf = mktf(Qt1hatoms, nil, 0);
	createfile(t1h, "atoms", nil, 0444, tf);

	/* /t-1d/ */
	tf = mktf(Qt1d, nil, 0);
	t1d = createfile(root, "t-1d", nil, DMDIR|0555, tf);

	tf = mktf(Qt1datoms, nil, 0);
	createfile(t1d, "atoms", nil, 0444, tf);

	/* /snap/ */
	tf = mktf(Qsnap, nil, 0);
	snap = createfile(root, "snap", nil, DMDIR|0755, tf);

	tf = mktf(Qsnapctl, nil, 0);
	createfile(snap, "ctl", nil, 0220, tf);

	tf = mktf(Qsnaplist, nil, 0);
	createfile(snap, "list", nil, 0444, tf);

	/* /history/ */
	tf = mktf(Qhistory, nil, 0);
	history = createfile(root, "history", nil, DMDIR|0555, tf);
	USED(history);

	/* /prune */
	tf = mktf(Qprune, nil, 0);
	createfile(root, "prune", nil, 0220, tf);

	/* /ctl */
	tf = mktf(Qctl, nil, 0);
	createfile(root, "ctl", nil, 0220, tf);

	/* /stats */
	tf = mktf(Qstats, nil, 0);
	createfile(root, "stats", nil, 0444, tf);

	/* Start serving */
	threadpostmountsrv(&fs, nil, mtpt, MREPL);
	threadexits(nil);
}
