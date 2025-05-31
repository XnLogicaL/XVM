// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_array.h"
#include "xvm_api_impl.h"

namespace xvm {

Array::Array(const Array& other)
  : data(new Value[ARRAY_INITAL_CAPACITY]),
    capacity(other.capacity),
    csize(other.csize) {
    for (size_t i = 0; i < capacity; i++) {
        data[i] = other.data[i].clone();
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
            data[i] = other.data[i].clone();
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
  : data(new Value[ARRAY_INITAL_CAPACITY]) {}

Array::~Array() {
    delete[] data;
}

size_t Array::size() const {
    return impl::__getArraySize(this);
}

Value& Array::get(size_t position) {
    return *impl::__getArrayField(this, position);
}

void Array::set(size_t position, Value&& value) {
    impl::__setArrayField(this, position, std::move(value));
}

} // namespace xvm
