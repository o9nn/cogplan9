# Plan9Cog - AGI-enabled Plan 9 Operating System

Plan9Cog is an extension of Plan 9 4th Edition that integrates artificial general intelligence (AGI) capabilities directly into the operating system. It combines concepts from OpenCog Collection (OCC), HurdCog, and Cognumach to create a unified cognitive computing platform.

## Features

### Cognitive Computing Core

- **AtomSpace**: Hypergraph-based knowledge representation system
- **PLN (Probabilistic Logic Networks)**: Uncertain reasoning and inference
- **ECAN (Economic Attention Network)**: Resource allocation and attention management
- **URE (Unified Rule Engine)**: Forward and backward chaining inference
- **Pattern Matching**: Query and match complex graph patterns

### Distributed Cognitive Processing

- **MachSpace**: Distributed hypergraph memory system
- **Cognitive Fusion Reactor**: Multi-process cognitive task processing
- **Cognitive File Server**: 9P-based access to cognitive operations
- **Cognitive IPC**: Inter-process communication for cognitive operations

### Tools and Utilities

- **cogfs**: Cognitive file server exposing AtomSpace via 9P
- **cogctl**: Command-line cognitive system control utility
- **Cognitive Dashboard**: Real-time monitoring (planned)

## Quick Start

### Building

Build all Plan9Cog components:

```bash
cd /sys/src
# Build libraries
cd libatomspace && mk install && cd ..
cd libpln && mk install && cd ..
cd libplan9cog && mk install && cd ..

# Build commands
cd cmd/cogctl && mk install && cd ../..
cd cmd/cogfs && mk install && cd ../..
```

### Running

Start the cognitive file server:

```bash
cogfs -m /mnt/cog
```

Use the control utility:

```bash
# Show system information
cogctl info

# Create an atom
cogctl atom create 1 'concept:human'

# List all atoms
cogctl atom list

# Update attention allocation
cogctl ecan update

# Show attentional focus
cogctl ecan focus
```

## Programming Example

```c
#include <u.h>
#include <libc.h>
#include <plan9cog.h>

void
main(int argc, char **argv)
{
    Plan9Cog *p9c;
    Atom *cat, *animal, *inheritance;
    TruthValue tv;
    
    /* Initialize Plan9Cog system */
    p9c = plan9coginit();
    if(p9c == nil)
        sysfatal("plan9coginit: %r");
    
    /* Create knowledge: Cat is an Animal */
    cat = atomcreate(p9c->atomspace, ConceptNode, "cat");
    animal = atomcreate(p9c->atomspace, ConceptNode, "animal");
    
    Atom *outgoing[2] = {cat, animal};
    inheritance = linkcreate(p9c->atomspace, InheritanceLink, outgoing, 2);
    
    /* Assign truth value (90% strength, 80% confidence) */
    tv.strength = 0.9;
    tv.confidence = 0.8;
    tv.count = 10;
    atomsettruth(inheritance, tv);
    
    print("Created: %s with %s\n", 
          cogatomstr(inheritance), 
          cogtvstr(atomgettruth(inheritance)));
    
    /* Clean up */
    plan9cogfree(p9c);
    exits(nil);
}
```

Compile and link:

```bash
8c -FTVw myprogram.c
8l -o myprogram myprogram.8 -lplan9cog -lpln -latomspace
```

## Architecture

Plan9Cog follows a layered architecture:

```
┌─────────────────────────────────────────────────┐
│  AGI Research Platform                           │
│  (URE, Pattern Mining, Cognitive Dashboard)     │
├─────────────────────────────────────────────────┤
│  Cognitive Services                              │
│  (AtomSpace, PLN, ECAN, MachSpace, cogfs)       │
├─────────────────────────────────────────────────┤
│  Plan9 Kernel with Cognitive Extensions          │
│  (Cognitive VM, AtomSpace IPC, Memory Mgmt)     │
├─────────────────────────────────────────────────┤
│  Plan 9 Kernel                                   │
└─────────────────────────────────────────────────┘
```

## Components

### Libraries

- **libatomspace** (`/sys/src/libatomspace`): Core AtomSpace implementation
- **libpln** (`/sys/src/libpln`): Probabilistic Logic Networks
- **libplan9cog** (`/sys/src/libplan9cog`): Main Plan9Cog library with ECAN, MachSpace, and system integration

### Commands

- **cogfs** (`/sys/src/cmd/cogfs`): Cognitive file server
- **cogctl** (`/sys/src/cmd/cogctl`): Cognitive control utility

### Headers

- `/sys/include/plan9cog.h` - Main Plan9Cog header
- `/sys/include/plan9cog/atomspace.h` - AtomSpace API
- `/sys/include/plan9cog/pln.h` - PLN API
- `/sys/include/plan9cog/cogvm.h` - Cognitive VM extensions

## Documentation

- **PLAN9COG_GUIDE.md**: Complete integration guide with API reference and examples
- **README**: This file (quick start and overview)

## Key Concepts

### AtomSpace

A hypergraph-based knowledge representation where:
- **Nodes**: Represent concepts (ConceptNode, PredicateNode, etc.)
- **Links**: Represent relationships (InheritanceLink, SimilarityLink, etc.)
- **Truth Values**: Probabilistic strength and confidence
- **Attention Values**: Short-term, long-term, and very-long-term importance

### PLN (Probabilistic Logic Networks)

Reasoning system for uncertain inference:
- **Deduction**: A→B, B→C ⇒ A→C
- **Induction**: A→B ⇒ B→A (with probability)
- **Abduction**: A→B, B→C ⇒ C→A (with probability)
- **Revision**: Combine evidence from multiple sources
- **Boolean operations**: AND, OR, NOT with uncertainty

### ECAN (Economic Attention Network)

Attention allocation system:
- **STI (Short-Term Importance)**: Immediate relevance
- **LTI (Long-Term Importance)**: Persistent significance
- **Attentional Focus**: Top N atoms by STI
- **Attention Spreading**: Propagate importance through graph
- **Attention Decay**: Gradual importance reduction

## Use Cases

### Knowledge Representation

Build semantic networks representing domain knowledge:

```bash
cogctl atom create 1 'person:john'
cogctl atom create 1 'person:mary'
cogctl atom create 1 'relation:knows'
# Create EvaluationLink for "John knows Mary"
```

### Uncertain Reasoning

Perform probabilistic inference over uncertain knowledge:

```c
/* Given: Cat→Animal (0.9, 0.8), Animal→LivingThing (0.95, 0.9) */
/* Infer: Cat→LivingThing via deduction */
result = plndeduction(cat_animal_tv, animal_living_tv);
```

### Attention Management

Prioritize important knowledge for processing:

```bash
cogctl ecan update    # Update attention allocation
cogctl ecan focus     # Show most important atoms
```

### Distributed Knowledge

Share knowledge across multiple systems:

```c
MachSpace *ms = machspaceinit(atomspace);
machspaceconnect(ms, "cpu1");
machspaceconnect(ms, "cpu2");
```

## Performance

- AtomSpace scales to millions of atoms
- PLN inference complexity depends on rule depth
- ECAN attention allocation is O(n log n) for sorting
- Pattern matching uses efficient graph traversal
- Thread-safe operations with fine-grained locking

## Compatibility

- Plan 9 4th Edition base system
- Works on 386, amd64, arm, and other Plan 9 architectures
- Integrates with standard Plan 9 services
- Compatible with 9P file protocol
- Can be used from Plan 9 C and rc shell

## Future Directions

1. **Natural Language Processing**: Integrate Link Grammar and Relex
2. **Pattern Mining**: Learn patterns from system operation
3. **Neural Networks**: Add sub-symbolic learning
4. **Distributed Synchronization**: Real-time AtomSpace replication
5. **Cognitive Dashboard**: Web-based monitoring interface
6. **Hardware Acceleration**: GPU support for large-scale inference
7. **Bootable System**: ISO image with full AGI OS

## Contributing

Plan9Cog is part of the broader AGI-OS project integrating cognitive computing into operating systems. Contributions welcome in:

- Additional PLN formulas and rules
- Pattern mining algorithms
- Natural language integration
- Performance optimization
- Documentation and examples
- Testing and validation

## Related Projects

- **OpenCog Collection (OCC)**: Original AGI framework
- **HurdCog**: Cognitive operating system on GNU Hurd
- **Cognumach**: Cognitive extensions to Mach microkernel
- **Plan 9**: Distributed operating system from Bell Labs

## License

Plan9Cog inherits the Plan 9 Foundation license. See LICENSE file.

## References

- Plan 9 Documentation: http://plan9.bell-labs.com/sys/doc/
- OpenCog: https://opencog.org
- PLN Book: "Probabilistic Logic Networks" by Goertzel et al.
- AtomSpace: OpenCog AtomSpace documentation

## Contact

For questions, issues, or contributions, please refer to the project repository.

---

**Plan9Cog**: Bringing Artificial General Intelligence to Plan 9
