// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#ifndef XVM_ALLOCATOR_H
#define XVM_ALLOCATOR_H

#include <Common.h>

namespace xvm {

template<typename T>
class LinearAllocator {
  public:
    using Dtor = std::function<void()>;

    // We do static assertions instead of concept constraints to avoid repetition in function
    // template definitions.
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
    static_assert(std::is_move_assignable_v<T>, "T must be move assignable");
    static_assert(std::is_destructible_v<T>, "T must be destructible");

    explicit LinearAllocator(size_t size)
      : cap(size),
        buf(new T[size]),
        off(buf) {}

    ~LinearAllocator();

    // Allocators should only be passed around as references, as they are typically tied to state
    // objects.
    XVM_NOCOPY(LinearAllocator);
    XVM_NOMOVE(LinearAllocator);

    void resize();
    T*   alloc();

    template<typename... Args>
        requires std::is_constructible_v<T, Args...>
    inline T* emplace(Args&&... args) {
        T* const allocated_memory = alloc();
        T* const obj = new (allocated_memory) T{std::forward<Args>(args)...};

        registerDtor(obj);
        return obj;
    }

  private:
    void registerDtor(T* const obj);

  private:
    T*     buf;
    T*     off;
    size_t cap;

    std::unordered_map<void*, Dtor> dtorMap;
};

class ByteAllocator {
  public:
    explicit ByteAllocator(size_t size)
      : cap(size),
        buf(new std::byte[size]),
        off(buf) {}

    ~ByteAllocator();

    // Allocators should only be passed around as references, as they are typically tied to state
    // objects.
    XVM_NOCOPY(ByteAllocator);
    XVM_NOMOVE(ByteAllocator);

    void  resize();
    void* alloc(size_t bytes);

  private:
    std::byte* buf;
    std::byte* off;
    size_t     cap;
};

} // namespace xvm

#endif
