// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/TClosure.h>
#include <Interpreter/ApiImpl.h>

namespace xvm {

Closure::Closure(Callable&& callable)
  : callee(std::move(callable)) {}

Closure::Closure(const Closure& other)
  : callee(other.callee),
    upvs(new UpValue[other.count]),
    count(other.count) {
    // UpValues captured twice; close them.
    impl::__closeClosureUpvs(&other);

    for (size_t i = 0; i < count; i++) {
        UpValue& upv = this->upvs[i];
        UpValue& other_upv = other.upvs[i];
        upv.heap = other_upv.heap.clone();
        upv.value = &upv.heap;
        upv.valid = true;
        upv.open = false;
    }
}

Closure::Closure(Closure&& other)
  : callee(other.callee),
    upvs(other.upvs),
    count(other.count) {
    // Only reset upvalues because they are the only owned values.
    other.upvs = NULL;
    other.count = 0;
}

Closure& Closure::operator=(const Closure& other) {
    if (this != &other) {
        this->callee = other.callee;
        this->upvs = new UpValue[other.count];
        this->count = other.count;

        // UpValues captured twice; close them.
        impl::__closeClosureUpvs(&other);

        for (size_t i = 0; i < count; i++) {
            UpValue& upv = this->upvs[i];
            UpValue& other_upv = other.upvs[i];
            upv.heap = other_upv.heap.clone();
            upv.value = &upv.heap;
            upv.valid = true;
            upv.open = false;
        }
    }

    return *this;
}

Closure& Closure::operator=(Closure&& other) {
    if (this != &other) {
        delete[] this->upvs;

        this->callee = other.callee;
        this->upvs = other.upvs;
        this->count = other.count;
        // Only reset upvalues because they are the only owned values.
        other.upvs = NULL;
        other.count = 0;
    }

    return *this;
}

Closure::Closure()
  : upvs(new UpValue[kClosureUpvCount]),
    count(kClosureUpvCount) {}

Closure::~Closure() {
    delete[] upvs;
}

} // namespace xvm
