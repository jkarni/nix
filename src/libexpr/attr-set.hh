#pragma once

#include "nixexpr.hh"
#include "symbol-table.hh"
#include "gc.hh"

#include <algorithm>
#include <optional>

namespace nix {


class EvalState;
struct Value;

/* Map one attribute name to its value. */
struct Attr
{
    Symbol name;
    Value * value;
    Pos * pos;
    Attr(Symbol name, Value * value, Pos * pos = &noPos)
        : name(name), value(value), pos(pos) { };
    Attr() : pos(&noPos) { };
    bool operator < (const Attr & a) const
    {
        return name < a.name;
    }
};

/* Bindings contains all the attributes of an attribute set. It is defined
   by its size and its capacity, the capacity being the number of Attr
   elements allocated after this structure, while the size corresponds to
   the number of elements already inserted in this structure. */
class Bindings : public Object
{
public:
    typedef uint32_t size_t;

private:
    // FIXME: eliminate size_. We can just rely on capacity by sorting
    // null entries towards the end of the vector.
    size_t size_;
    Attr attrs[0];

    Bindings(size_t capacity) : Object(tBindings, capacity), size_(0) {}
    Bindings(const Bindings & bindings) = delete;

public:

    size_t size() const { return size_; }

    bool empty() const { return !size_; }

    typedef Attr * iterator;

    void push_back(const Attr & attr)
    {
        assert(size_ < capacity());
        gc.assertObject(attr.value);
        attrs[size_++] = attr;
    }

    iterator find(const Symbol & name)
    {
        Attr key(name, 0);
        iterator i = std::lower_bound(begin(), end(), key);
        if (i != end() && i->name == name) return i;
        return end();
    }

    Attr * get(const Symbol & name)
    {
        Attr key(name, 0);
        iterator i = std::lower_bound(begin(), end(), key);
        if (i != end() && i->name == name) return &*i;
        return nullptr;
    }

    Attr & need(const Symbol & name, const Pos & pos = noPos)
    {
        auto a = get(name);
        if (!a)
            throw Error("attribute '%s' missing, at %s", name, pos);
        return *a;
    }

    iterator begin() { return &attrs[0]; }
    iterator end() { return &attrs[size_]; }

    Attr & operator[](size_t pos)
    {
        return attrs[pos];
    }

    void sort();

    size_t capacity() const { return getMisc(); }

    /* Returns the attributes in lexicographically sorted order. */
    std::vector<const Attr *> lexicographicOrder() const
    {
        std::vector<const Attr *> res;
        res.reserve(size_);
        for (size_t n = 0; n < size_; n++)
            res.emplace_back(&attrs[n]);
        std::sort(res.begin(), res.end(), [](const Attr * a, const Attr * b) {
            return (const string &) a->name < (const string &) b->name;
        });
        return res;
    }

    size_t words() const
    {
        return wordsFor(capacity());
    }

    static size_t wordsFor(size_t capacity)
    {
        return 2 + 3 * capacity; // FIXME
    }

    static Ptr<Bindings> allocBindings(size_t capacity);

    friend class GC;
};


}
