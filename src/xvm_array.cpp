// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_array.h"
#include "xvm_api_impl.h"

namespace xvm {

Array::Array(const Array& other)
  : data(new Value[kArrayCapacity]),
    cap(other.cap),
    csize(other.csize) {
    for (size_t i = 0; i < cap; i++) {
        data[i] = impl::__clone(other.data + i);
    }
}

Array::Array(Array&& other)
  : data(other.data),
    cap(other.cap),
    csize(other.csize) {
    other.cap = 0;
    other.data = NULL;
    other.csize = {};
}

Array& Array::operator=(const Array& other) {
    if (this != &other) {
        delete[] data;

        data = new Value[cap];
        cap = other.cap;
        csize = other.csize;

        for (size_t i = 0; i < cap; i++) {
            data[i] = impl::__clone(other.data + i);
        }
    }

    return *this;
}

Array& Array::operator=(Array&& other) {
    if (this != &other) {
        delete[] data;

        data = other.data;
        cap = other.cap;
        csize = other.csize;

        other.cap = 0;
        other.csize = {};
        other.data = NULL;
    }

    return *this;
}

Array::Array()
  : data(new Value[kArrayCapacity]) {}

Array::~Array() {
    delete[] data;
}

} // namespace xvm
