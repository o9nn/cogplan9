// cog/core/core.hpp — Shared types: Atom, AtomSpace, TruthValue, AttentionValue
// Header-only, C++11, zero external dependencies
// SPDX-License-Identifier: MIT
#ifndef COG_CORE_HPP
#define COG_CORE_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace cog {

// ─────────────────────────────────────────────────────────────────────────────
// Handle — Opaque atom identifier
// ─────────────────────────────────────────────────────────────────────────────
typedef uint32_t Handle;
static const Handle UNDEFINED_HANDLE = 0;

// ─────────────────────────────────────────────────────────────────────────────
// AtomType — Classification of atoms
// ─────────────────────────────────────────────────────────────────────────────
enum class AtomType : uint16_t {
    // Nodes
    CONCEPT_NODE = 0,
    PREDICATE_NODE,
    VARIABLE_NODE,
    NUMBER_NODE,
    SCHEMA_NODE,
    GROUNDED_SCHEMA_NODE,
    // Links
    INHERITANCE_LINK,
    EVALUATION_LINK,
    LIST_LINK,
    SET_LINK,
    AND_LINK,
    OR_LINK,
    NOT_LINK,
    IMPLICATION_LINK,
    EQUIVALENCE_LINK,
    EXECUTION_LINK,
    BIND_LINK,
    // Sentinel
    ATOM_TYPE_COUNT
};

inline const char* atom_type_name(AtomType t) {
    switch (t) {
    case AtomType::CONCEPT_NODE:         return "ConceptNode";
    case AtomType::PREDICATE_NODE:       return "PredicateNode";
    case AtomType::VARIABLE_NODE:        return "VariableNode";
    case AtomType::NUMBER_NODE:          return "NumberNode";
    case AtomType::SCHEMA_NODE:          return "SchemaNode";
    case AtomType::GROUNDED_SCHEMA_NODE: return "GroundedSchemaNode";
    case AtomType::INHERITANCE_LINK:     return "InheritanceLink";
    case AtomType::EVALUATION_LINK:      return "EvaluationLink";
    case AtomType::LIST_LINK:            return "ListLink";
    case AtomType::SET_LINK:             return "SetLink";
    case AtomType::AND_LINK:             return "AndLink";
    case AtomType::OR_LINK:              return "OrLink";
    case AtomType::NOT_LINK:             return "NotLink";
    case AtomType::IMPLICATION_LINK:     return "ImplicationLink";
    case AtomType::EQUIVALENCE_LINK:     return "EquivalenceLink";
    case AtomType::EXECUTION_LINK:       return "ExecutionLink";
    case AtomType::BIND_LINK:            return "BindLink";
    default:                             return "Unknown";
    }
}

// Helper: is this type a node (vs a link)?
inline bool atom_type_is_node(AtomType t) {
    return t <= AtomType::GROUNDED_SCHEMA_NODE;
}

// ─────────────────────────────────────────────────────────────────────────────
// TruthValue — Probabilistic truth value (strength, confidence)
// ─────────────────────────────────────────────────────────────────────────────
struct TruthValue {
    float strength;
    float confidence;

    TruthValue() : strength(0.0f), confidence(0.0f) {}
    TruthValue(float s, float c) : strength(s), confidence(c) {}

    std::string to_string() const {
        std::ostringstream os;
        os << "(stv " << strength << " " << confidence << ")";
        return os.str();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// AttentionValue — ECAN attention allocation (STI, LTI, VLTI)
// ─────────────────────────────────────────────────────────────────────────────
struct AttentionValue {
    short sti;
    short lti;
    short vlti;

    AttentionValue() : sti(0), lti(0), vlti(0) {}
    AttentionValue(short s, short l, short v) : sti(s), lti(l), vlti(v) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Atom — A single knowledge unit in the AtomSpace
// ─────────────────────────────────────────────────────────────────────────────
struct Atom {
    Handle              handle;
    AtomType            type;
    std::string         name;       // For nodes
    std::vector<Handle> outgoing;   // For links
    TruthValue          tv;
    AttentionValue      av;

    Atom() : handle(UNDEFINED_HANDLE), type(AtomType::CONCEPT_NODE) {}

    bool is_node() const { return atom_type_is_node(type); }
    bool is_link() const { return !is_node(); }

    std::string to_short_string() const {
        std::ostringstream os;
        os << "(" << atom_type_name(type);
        if (is_node()) {
            os << " \"" << name << "\"";
        } else {
            os << " [";
            for (size_t i = 0; i < outgoing.size(); ++i) {
                if (i > 0) os << " ";
                os << outgoing[i];
            }
            os << "]";
        }
        os << " " << tv.to_string() << ")";
        return os.str();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// AtomSpace — In-memory hypergraph knowledge base
// ─────────────────────────────────────────────────────────────────────────────
class AtomSpace {
public:
    AtomSpace() : next_handle_(1) {}

    // Number of atoms
    size_t size() const { return atoms_.size(); }

    // Add a node
    Handle add_node(AtomType type, const std::string& name,
                    TruthValue tv = TruthValue()) {
        Atom a;
        a.handle = next_handle_++;
        a.type = type;
        a.name = name;
        a.tv = tv;
        atoms_[a.handle] = a;
        return a.handle;
    }

    // Add a link
    Handle add_link(AtomType type, const std::vector<Handle>& outgoing,
                    TruthValue tv = TruthValue()) {
        Atom a;
        a.handle = next_handle_++;
        a.type = type;
        a.outgoing = outgoing;
        a.tv = tv;
        atoms_[a.handle] = a;
        return a.handle;
    }

    // Remove an atom by handle
    bool remove_atom(Handle h) {
        return atoms_.erase(h) > 0;
    }

    // Get atom by handle (nullptr if not found)
    const Atom* get_atom(Handle h) const {
        auto it = atoms_.find(h);
        return (it != atoms_.end()) ? &it->second : nullptr;
    }

    // Get all handles of a given type
    std::vector<Handle> get_by_type(AtomType type) const {
        std::vector<Handle> result;
        for (auto& kv : atoms_) {
            if (kv.second.type == type)
                result.push_back(kv.first);
        }
        return result;
    }

    // Iterate over all atoms
    template <typename Fn>
    void foreach_atom(Fn fn) const {
        for (auto& kv : atoms_) {
            fn(kv.second);
        }
    }

private:
    Handle next_handle_;
    std::unordered_map<Handle, Atom> atoms_;
};

} // namespace cog

#endif // COG_CORE_HPP
