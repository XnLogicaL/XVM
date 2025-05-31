// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_array.h"
#include "xvm_api_impl.h"

namespace xvm {

Array::Array(const Array& other)
  : data(new Value[kArrayCapacity]),
    capacity(other.capacity),
    csize(other.csize) {
    for (size_t i = 0; i < capacity; i++) {
        data[i] = impl::__clone(other.data + i);
    }
}

Array::Array(Array&& other)
  : data(other.data),
    capacity(other.capacity),
    csize(other.csize) {
    other.capacity = 0;
    other.data = NULL;
    other.csize = {};
}

Array& Array::operator=(const Array& other) {
    if (this != &other) {
        delete[] data;

        data = new Value[capacity];
        capacity = other.capacity;
        csize = other.csize;

        for (size_t i = 0; i < capacity; i++) {
            data[i] = impl::__clone(other.data + i);
        }
    }

    return *this;
}

Array& Array::operator=(Array&& other) {
    if (this != &other) {
        delete[] data;

        data = other.data;
        capacity = other.capacity;
        csize = other.csize;

        other.capacity = 0;
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

size_t Array::size() const {
    return impl::__getArraySize(this);
}

} // namespace xvm
