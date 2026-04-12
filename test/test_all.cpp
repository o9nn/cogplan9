// test/test_all.cpp — Tests for the cog header-only library
// SPDX-License-Identifier: MIT
#include <cog/cog.hpp>
#include <iostream>
#include <cassert>
#include <cstring>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        std::cout << "  " << #name << "... "; \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        std::cout << "PASS" << std::endl; \
    } while(0)

// ── Core: AtomSpace ─────────────────────────────────────────────────────────

static void test_atomspace_create() {
    TEST(atomspace_create);
    cog::AtomSpace as;
    assert(as.size() == 0);
    PASS();
}

static void test_atomspace_add_node() {
    TEST(atomspace_add_node);
    cog::AtomSpace as;
    cog::Handle h = as.add_node(cog::AtomType::CONCEPT_NODE, "cat",
                                cog::TruthValue(0.9f, 0.8f));
    assert(h != cog::UNDEFINED_HANDLE);
    assert(as.size() == 1);
    const cog::Atom* a = as.get_atom(h);
    assert(a != nullptr);
    assert(a->name == "cat");
    assert(a->is_node());
    PASS();
}

static void test_atomspace_add_link() {
    TEST(atomspace_add_link);
    cog::AtomSpace as;
    cog::Handle h1 = as.add_node(cog::AtomType::CONCEPT_NODE, "cat");
    cog::Handle h2 = as.add_node(cog::AtomType::CONCEPT_NODE, "animal");
    cog::Handle link = as.add_link(cog::AtomType::INHERITANCE_LINK,
                                   {h1, h2},
                                   cog::TruthValue(0.95f, 0.9f));
    assert(link != cog::UNDEFINED_HANDLE);
    assert(as.size() == 3);
    const cog::Atom* a = as.get_atom(link);
    assert(a != nullptr);
    assert(a->is_link());
    assert(a->outgoing.size() == 2);
    PASS();
}

static void test_atomspace_remove() {
    TEST(atomspace_remove);
    cog::AtomSpace as;
    cog::Handle h = as.add_node(cog::AtomType::CONCEPT_NODE, "temp");
    assert(as.size() == 1);
    assert(as.remove_atom(h));
    assert(as.size() == 0);
    assert(as.get_atom(h) == nullptr);
    PASS();
}

static void test_atomspace_get_by_type() {
    TEST(atomspace_get_by_type);
    cog::AtomSpace as;
    as.add_node(cog::AtomType::CONCEPT_NODE, "a");
    as.add_node(cog::AtomType::CONCEPT_NODE, "b");
    as.add_node(cog::AtomType::PREDICATE_NODE, "p");
    auto concepts = as.get_by_type(cog::AtomType::CONCEPT_NODE);
    assert(concepts.size() == 2);
    auto preds = as.get_by_type(cog::AtomType::PREDICATE_NODE);
    assert(preds.size() == 1);
    PASS();
}

static void test_truthvalue() {
    TEST(truthvalue);
    cog::TruthValue tv(0.8f, 0.9f);
    assert(tv.strength > 0.79f && tv.strength < 0.81f);
    assert(tv.confidence > 0.89f && tv.confidence < 0.91f);
    std::string s = tv.to_string();
    assert(s.find("stv") != std::string::npos);
    PASS();
}

static void test_attention_value() {
    TEST(attention_value);
    cog::AttentionValue av(100, 50, 10);
    assert(av.sti == 100);
    assert(av.lti == 50);
    assert(av.vlti == 10);
    PASS();
}

// ── Plan9: CogFS ────────────────────────────────────────────────────────────

static void test_cogfs_sync() {
    TEST(cogfs_sync);
    cog::AtomSpace as;
    as.add_node(cog::AtomType::CONCEPT_NODE, "hello",
                cog::TruthValue(1.0f, 0.9f));
    cog::plan9::CogFS fs(as);
    fs.sync();
    auto entries = fs.ls({});
    assert(entries.size() == 3); // nodes/, links/, types/
    PASS();
}

static void test_cogfs_read() {
    TEST(cogfs_read);
    cog::AtomSpace as;
    cog::Handle h = as.add_node(cog::AtomType::CONCEPT_NODE, "world",
                                cog::TruthValue(0.7f, 0.5f));
    cog::plan9::CogFS fs(as);
    fs.sync();
    std::string content = fs.read({"nodes", std::to_string(h)});
    assert(!content.empty());
    assert(content.find("world") != std::string::npos);
    PASS();
}

static void test_cogfs_write() {
    TEST(cogfs_write);
    cog::AtomSpace as;
    cog::plan9::CogFS fs(as);
    bool ok = fs.write({"nodes", "test_node"}, "");
    assert(ok);
    assert(as.size() == 1);
    PASS();
}

// ── Plan9: 9P Message ───────────────────────────────────────────────────────

static void test_9p_message_serialize() {
    TEST(9p_message_serialize);
    cog::plan9::Message msg;
    msg.type = cog::plan9::MsgType::Tversion;
    msg.tag = 0xFFFF;
    msg.msize = 8192;
    msg.version_str = "9P2000";
    auto buf = msg.serialize();
    assert(buf.size() > 4);
    // First 4 bytes are the size (little-endian)
    uint32_t sz = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    assert(sz == buf.size());
    PASS();
}

// ── Plan9: MachSpace ────────────────────────────────────────────────────────

static void test_machspace() {
    TEST(machspace);
    cog::plan9::MachSpace ms;
    auto region = ms.map_atom(42, 4096);
    assert(region.atom_handle == 42);
    assert(region.size == 4096);
    const cog::plan9::MachRegion* found = ms.find(42);
    assert(found != nullptr);
    assert(found->base_addr == region.base_addr);
    assert(ms.unmap(42));
    assert(ms.find(42) == nullptr);
    PASS();
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== cog library tests ===" << std::endl;

    std::cout << "\n[core]" << std::endl;
    test_atomspace_create();
    test_atomspace_add_node();
    test_atomspace_add_link();
    test_atomspace_remove();
    test_atomspace_get_by_type();
    test_truthvalue();
    test_attention_value();

    std::cout << "\n[plan9]" << std::endl;
    test_cogfs_sync();
    test_cogfs_read();
    test_cogfs_write();
    test_9p_message_serialize();
    test_machspace();

    std::cout << "\n" << tests_passed << "/" << tests_run
              << " tests passed." << std::endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
