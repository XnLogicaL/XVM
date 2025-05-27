// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/TDict.h>
#include <Interpreter/ApiImpl.h>

namespace xvm {

Dict::Dict(const Dict& other)
  : data(new Dict::HNode[DICT_INITIAL_CAPACITY]),
    capacity(other.capacity),
    csize(other.csize) {
    for (size_t i = 0; i < capacity; ++i) {
        HNode& src = other.data[i];
        HNode* dst = &data[i];
        dst->key = src.key;
        dst->value = src.value.clone();
    }
}

Dict::Dict(Dict&& other)
  : data(other.data),
    capacity(other.capacity),
    csize(other.csize) {
    other.data = NULL;
    other.capacity = 0;
    other.csize = {};
}

Dict& Dict::operator=(const Dict& other) {
    if (this != &other) {
        data = new HNode[other.capacity];
        capacity = other.capacity;
        csize = other.csize;

        for (size_t i = 0; i < capacity; ++i) {
            Dict::HNode& src = other.data[i];
            Dict::HNode* dst = &data[i];
            dst->key = src.key;
            dst->value = src.value.clone();
        }
    }

    return *this;
}

Dict& Dict::operator=(Dict&& other) {
    if (this != &other) {
        data = other.data;
        capacity = other.capacity;
        csize = other.csize;

        other.data = NULL;
        other.capacity = 0;
        other.csize = {};
    }

    return *this;
}

Dict::Dict()
  : data(new HNode[DICT_INITIAL_CAPACITY]) {}

Dict::~Dict() {
    delete[] data;
}

size_t Dict::size() const {
    return impl::__getDictSize(this);
}

Value& Dict::get(const char* key) {
    return *impl::__getDictField(this, key);
}

void Dict::set(const char* key, Value value) {
    impl::__setDictField(this, key, std::move(value));
}

} // namespace xvm
