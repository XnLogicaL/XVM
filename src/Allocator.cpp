// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/Allocator.h>

namespace xvm {

template<typename T>
LinearAllocator<T>::~LinearAllocator() {
    for (auto& dtor : dtorMap) {
        dtor.second();
    }

    delete[] buf;
}

template<typename T>
void LinearAllocator<T>::resize() {
    T* oldbuf = buf;
    cap *= 2;
    buf = new T[cap];

    for (size_t i = 0; i < cap; ++i) {
        buf[i] = std::move(oldbuf[i]);
    }

    delete[] oldbuf;
}

template<typename T>
T* LinearAllocator<T>::alloc() {
    size_t remBytes = cap - static_cast<size_t>(off - buf);
    void*  aligned = std::align(alignof(T), sizeof(T), off, remBytes);

    if (aligned == NULL) {
        throw std::bad_alloc();
    }

    off = static_cast<std::byte*>(aligned) + sizeof(T);
    return static_cast<T*>(aligned);
}

template<typename T>
void LinearAllocator<T>::registerDtor(T* const obj) {
    dtorMap[obj] = [obj]() { obj->~T(); };
}

ByteAllocator::~ByteAllocator() {
    delete[] buf;
}

void ByteAllocator::resize() {
    auto*  oldbuf = buf;
    size_t oldcap = cap;

    cap *= 2;
    buf = new std::byte[cap];

    std::memcpy(buf, oldbuf, oldcap);
    delete[] oldbuf;
}

void* ByteAllocator::alloc(size_t bytes) {
    void* oldoff = off;
    off += bytes;
    return oldoff;
}

} // namespace xvm
