// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Allocator.h>

namespace xvm {

// clang-format off
// ^^ we do this because concept syntax is still partially supported by clang-format

template<typename T>
TempBuf<T>::TempBuf(const TempBuf<T>& other)
    requires std::is_copy_constructible_v<T>
  : size(other.size)
  , data(new T[size]) {
    std::memcpy(data, other.data, size);
}

template<typename T>
TempBuf<T>::TempBuf(TempBuf<T>&& other)
    requires std::is_move_constructible_v<T>
  : size(other.size)
  , data(other.data) {
    other.size = 0;
    other.data = NULL;
}

template<typename T>
TempBuf<T>& TempBuf<T>::operator=(const TempBuf<T>& other)
    requires std::is_copy_assignable_v<T>
{
    if (this != &other) {
        size = other.size;
        data = new T[size];
        std::memcpy(data, other.data, size);
    }

    return *this;
}

template<typename T>
TempBuf<T>& TempBuf<T>::operator=(TempBuf<T>&& other)
    requires std::is_move_assignable_v<T>
{
    if (this != &other) {
        size = other.size;
        data = other.data;
        other.size = 0;
        other.data = NULL;
    }

    return *this;
}

template<typename T>
TempObj<T>::TempObj(const TempObj<T>& other)
    requires std::is_copy_constructible_v<T>
  : obj(new T(other)) {}

template<typename T>
TempObj<T>::TempObj(TempObj<T>&& other)
    requires std::is_move_constructible_v<T>
{
    other.obj = NULL;
}

// clang-format on

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
    if (remBytes == 0) {
        resize();
    }

    T* aligned = std::align(alignof(T), sizeof(T), off, remBytes);
    if (aligned == NULL) {
        throw std::bad_alloc();
    }

    off = aligned + sizeof(T);
    return aligned;
}

template<typename T>
void LinearAllocator<T>::registerDtor(T* const obj) {
    dtorMap[obj] = [obj]() { obj->~T(); };
}

template<typename T>
ByteAllocator<T>::~ByteAllocator() {
    delete[] buf;
}

template<typename T>
void ByteAllocator<T>::resize() {
    auto*  oldbuf = buf;
    size_t oldcap = cap;

    cap *= 2;
    buf = new std::byte[cap];

    std::memcpy(buf, oldbuf, oldcap);
    delete[] oldbuf;
}

template<typename T>
T* ByteAllocator<T>::alloc() {
    return off++;
}

template<typename T>
T* ByteAllocator<T>::allocBytes(size_t bytes) {
    void* oldoff = off;
    off += bytes;
    return oldoff;
}

template<typename T>
T* ByteAllocator<T>::fromArray(const T* array) {
    size_t   len = 0;
    const T* ptr = array;

    while (*ptr != T{} /* zero value */) {
        ++ptr;
        ++len;
    }

    T* alloca = allocBytes(len + 1);

    std::memcpy(alloca, array, len);
    std::memset(alloca + len, 0, 1);

    return alloca;
}

} // namespace xvm
