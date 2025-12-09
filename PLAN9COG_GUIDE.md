# Plan9Cog AGI-OS Integration Guide

## Overview

Plan9Cog is an AGI-enabled extension of the Plan 9 operating system that integrates cognitive computing capabilities directly into the OS architecture. It provides a unified framework for artificial general intelligence research and development based on concepts from OpenCog Collection (OCC), HurdCog, and Cognumach.

## Architecture

Plan9Cog follows a three-layer architecture:

### Layer 1: Plan9 Kernel with Cognitive Extensions

The Plan9 kernel is extended with cognitive computing primitives:

- **Cognitive Memory Management**: Specialized memory regions for hypergraph operations
- **AtomSpace IPC**: Inter-process communication for cognitive operations
- **Cognitive VM Extensions**: Virtual memory optimizations for cognitive workloads

Headers: `/sys/include/plan9cog/cogvm.h`

### Layer 2: Cognitive Services

Core cognitive services implemented as libraries and file servers:

- **AtomSpace** (`libatomspace`): Hypergraph knowledge representation system
- **PLN** (`libpln`): Probabilistic Logic Networks for uncertain reasoning
- **ECAN** (`libplan9cog`): Economic Attention Network for resource allocation
- **MachSpace**: Distributed hypergraph memory system
- **Cognitive File Server** (`cogfs`): 9P-based cognitive operations

Headers: `/sys/include/plan9cog/atomspace.h`, `/sys/include/plan9cog/pln.h`

### Layer 3: AGI Research Platform

High-level AGI research tools and utilities:

- **URE**: Unified Rule Engine for forward/backward chaining
- **Pattern Mining**: Learning patterns from system operation
- **Cognitive Fusion Reactor**: Distributed cognitive processing
- **Master Control Dashboard**: Real-time monitoring and control

Main header: `/sys/include/plan9cog.h`

## Building Plan9Cog

### Prerequisites

- Plan 9 4th Edition build environment
- Standard Plan 9 libraries

### Build Instructions

1. Navigate to the source directory:
```
cd /sys/src
```

2. Build the AtomSpace library:
```
cd libatomspace
mk install
```

3. Build the PLN library:
```
cd ../libpln
mk install
```

4. Build the main Plan9Cog library:
```
cd ../libplan9cog
mk install
```

5. Build cognitive utilities:
```
cd ../cmd/cogctl
mk install
cd ../cogfs
mk install
```

### Quick Build

To build all Plan9Cog components:
```
cd /sys/src
for(lib in libatomspace libpln libplan9cog) {
    cd $lib
    mk install
    cd ..
}
cd cmd
for(cmd in cogctl cogfs) {
    cd $cmd
    mk install
    cd ..
}
```

## Using Plan9Cog

### Starting the Cognitive File Server

The cognitive file server (`cogfs`) exposes AtomSpace operations via the 9P protocol:

```
cogfs -m /mnt/cog
```

This mounts the cognitive file server at `/mnt/cog` with the following files:

- `atoms` - List all atoms
- `atomctl` - Control operations for atoms
- `pln` - PLN inference results
- `plnctl` - Control PLN inference
- `ecan` - ECAN attention allocation
- `ecanctl` - Control ECAN
- `pattern` - Pattern matching
- `stats` - System statistics
- `ctl` - General control

### Using the Cognitive Control Utility

The `cogctl` utility provides command-line access to cognitive operations:

#### Create an atom:
```
cogctl atom create 1 'concept:dog'
```

#### List all atoms:
```
cogctl atom list
```

#### Show atom information:
```
cogctl atom info 1
```

#### Show PLN statistics:
```
cogctl pln stats
```

#### Update ECAN attention allocation:
```
cogctl ecan update
```

#### Show attentional focus:
```
cogctl ecan focus
```

#### Show system information:
```
cogctl info
```

## Programming with Plan9Cog

### Basic AtomSpace Operations

```c
#include <u.h>
#include <libc.h>
#include <plan9cog.h>

void
main(int argc, char **argv)
{
    Plan9Cog *p9c;
    Atom *a1, *a2, *link;
    TruthValue tv;
    
    /* Initialize Plan9Cog */
    p9c = plan9coginit();
    if(p9c == nil)
        sysfatal("plan9coginit: %r");
    
    /* Create concept nodes */
    a1 = atomcreate(p9c->atomspace, ConceptNode, "cat");
    a2 = atomcreate(p9c->atomspace, ConceptNode, "animal");
    
    /* Create inheritance link */
    Atom *outgoing[2] = {a1, a2};
    link = linkcreate(p9c->atomspace, InheritanceLink, outgoing, 2);
    
    /* Set truth value */
    tv.strength = 0.9;
    tv.confidence = 0.8;
    tv.count = 10;
    atomsettruth(link, tv);
    
    /* Query and print */
    print("Created link: %s\n", cogatomstr(link));
    print("Truth value: %s\n", cogtvstr(atomgettruth(link)));
    
    plan9cogfree(p9c);
    exits(nil);
}
```

### PLN Inference

```c
#include <u.h>
#include <libc.h>
#include <plan9cog.h>

void
main(int argc, char **argv)
{
    Plan9Cog *p9c;
    TruthValue tv1, tv2, result;
    
    p9c = plan9coginit();
    
    /* Set up truth values */
    tv1.strength = 0.9;
    tv1.confidence = 0.8;
    tv1.count = 10;
    
    tv2.strength = 0.8;
    tv2.confidence = 0.7;
    tv2.count = 8;
    
    /* Apply deduction */
    result = plndeduction(tv1, tv2);
    print("Deduction result: %s\n", cogtvstr(result));
    
    /* Apply revision */
    result = plnrevision(tv1, tv2);
    print("Revision result: %s\n", cogtvstr(result));
    
    plan9cogfree(p9c);
    exits(nil);
}
```

### ECAN Attention Allocation

```c
#include <u.h>
#include <libc.h>
#include <plan9cog.h>

void
main(int argc, char **argv)
{
    Plan9Cog *p9c;
    EcanNetwork *ecan;
    Atom *a;
    Atom **focus;
    int nfocus, i;
    AttentionValue av;
    
    p9c = plan9coginit();
    
    /* Create some atoms */
    a = atomcreate(p9c->atomspace, ConceptNode, "important");
    
    /* Set attention value */
    av.sti = 100;  /* High short-term importance */
    av.lti = 50;   /* Medium long-term importance */
    av.vlti = 10;  /* Low very-long-term importance */
    atomsetattention(a, av);
    
    /* Initialize ECAN */
    ecan = ecaninit(p9c->atomspace, 1000, 1000);
    
    /* Update attention allocation */
    ecanupdate(ecan);
    
    /* Get attentional focus */
    focus = ecanfocus(ecan, &nfocus);
    print("Attentional focus (%d atoms):\n", nfocus);
    for(i = 0; i < nfocus; i++){
        print("  %s  STI=%d\n", cogatomstr(focus[i]), focus[i]->av.sti);
    }
    
    ecanfree(ecan);
    plan9cogfree(p9c);
    exits(nil);
}
```

## API Reference

### AtomSpace API

- `AtomSpace* atomspacecreate(void)` - Create new AtomSpace
- `void atomspacefree(AtomSpace *as)` - Free AtomSpace
- `Atom* atomcreate(AtomSpace *as, int type, char *name)` - Create atom
- `Atom* linkcreate(AtomSpace *as, int type, Atom **outgoing, int n)` - Create link
- `Atom* atomfind(AtomSpace *as, ulong id)` - Find atom by ID
- `int atomdelete(AtomSpace *as, ulong id)` - Delete atom
- `TruthValue atomgettruth(Atom *a)` - Get truth value
- `void atomsettruth(Atom *a, TruthValue tv)` - Set truth value
- `AttentionValue atomgetattention(Atom *a)` - Get attention value
- `void atomsetattention(Atom *a, AttentionValue av)` - Set attention value
- `Atom** atomquery(AtomSpace *as, AtomPredicate pred, void *arg, int *n)` - Query atoms
- `Atom** atomgetincoming(AtomSpace *as, Atom *a, int *n)` - Get incoming links
- `Atom** atommatch(AtomSpace *as, Pattern *pat, int *n)` - Pattern matching

### PLN API

- `PlnInference* plninit(AtomSpace *as)` - Initialize PLN
- `void plnfree(PlnInference *pln)` - Free PLN
- `void plnaddrule(PlnInference *pln, PlnRule *rule)` - Add inference rule
- `TruthValue plndeduction(TruthValue a, TruthValue b)` - Deduction formula
- `TruthValue plninduction(TruthValue a, TruthValue b)` - Induction formula
- `TruthValue plnabduction(TruthValue a, TruthValue b)` - Abduction formula
- `TruthValue plnrevision(TruthValue a, TruthValue b)` - Revision formula
- `TruthValue plnand(TruthValue a, TruthValue b)` - AND formula
- `TruthValue plnor(TruthValue a, TruthValue b)` - OR formula
- `TruthValue plnnot(TruthValue a)` - NOT formula
- `TruthValue plneval(PlnInference *pln, Atom *query)` - Direct evaluation
- `Atom** plnforward(PlnInference *pln, Atom *target, int maxsteps, int *n)` - Forward chaining
- `Atom** plnbackward(PlnInference *pln, Atom *goal, int maxsteps, int *n)` - Backward chaining

### ECAN API

- `EcanNetwork* ecaninit(AtomSpace *as, short totalsti, short totallti)` - Initialize ECAN
- `void ecanfree(EcanNetwork *ecan)` - Free ECAN
- `void ecanupdate(EcanNetwork *ecan)` - Update attention allocation
- `void ecanallocate(EcanNetwork *ecan, Atom *a, short sti)` - Allocate attention
- `void ecanspread(EcanNetwork *ecan, Atom *source)` - Spread attention
- `Atom** ecanfocus(EcanNetwork *ecan, int *n)` - Get attentional focus
- `void ecandecay(EcanNetwork *ecan, float rate)` - Apply attention decay

### Plan9Cog System API

- `Plan9Cog* plan9coginit(void)` - Initialize Plan9Cog
- `void plan9cogfree(Plan9Cog *p9c)` - Free Plan9Cog
- `Plan9Cog* plan9coginstance(void)` - Get global instance
- `void coginfo(Plan9Cog *p9c, CogInfo *info)` - Get system information
- `void cogprint(char *fmt, ...)` - Print cognitive information
- `void cogdebug(int level, char *fmt, ...)` - Debug output
- `char* cogatomstr(Atom *a)` - Convert atom to string
- `char* cogtvstr(TruthValue tv)` - Convert truth value to string

## Examples

See `/sys/src/cmd/cogctl/cogctl.c` for comprehensive examples of using the Plan9Cog API.

## Distributed Cognitive Processing

### MachSpace

MachSpace provides distributed hypergraph memory:

```c
MachSpace *ms;
ms = machspaceinit(p9c->atomspace);
machspaceconnect(ms, "cpu1");
machspaceconnect(ms, "cpu2");
machspacesync(ms);
```

### Cognitive Fusion Reactor

The Cognitive Fusion Reactor enables distributed cognitive processing:

```c
CogFusionReactor *cfr;
cfr = cogreactorinit(p9c, 4);  /* 4 worker processes */
cogreactorsubmit(cfr, task);
result = cogreactorresult(cfr);
```

## Performance Considerations

- AtomSpace operations are thread-safe but lock-protected
- Large knowledge bases may require cognitive memory optimization
- ECAN attention decay should be tuned for workload
- Pattern matching complexity depends on pattern structure
- PLN inference depth affects performance

## Troubleshooting

### Build Errors

- Ensure all libraries are built in order: libatomspace, libpln, libplan9cog
- Check that headers are installed in `/sys/include/plan9cog/`

### Runtime Errors

- Verify cogfs is running before accessing `/mnt/cog`
- Check memory limits for large AtomSpaces
- Use `cogctl info` to diagnose system state

## Future Enhancements

- Pattern mining implementation
- Natural language processing integration
- Distributed AtomSpace synchronization
- Real-time cognitive dashboard
- Neural network integration
- 64-bit support
- Hardware acceleration

## References

- OpenCog Collection: https://opencog.org
- Plan 9 from Bell Labs: http://plan9.bell-labs.com/plan9/
- Probabilistic Logic Networks: PLN book by Ben Goertzel et al.
- Economic Attention Networks: ECAN papers by Ben Goertzel

## License

Plan9Cog is released under the same license as Plan 9 from Bell Labs.

## Authors

Plan9Cog integration based on the AGI-OS integration summary combining OCC, HurdCog, and Cognumach concepts adapted for Plan 9.
