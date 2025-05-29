// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#ifndef XVM_ALLOCATOR_H
#define XVM_ALLOCATOR_H

#include <Common.h>

namespace xvm {

template<typename T>
class TempBuf {
    // TempBuf relies on the fact that T is default constructible to ensure an uninitialized buffer
    // can exist.
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

  public:
    size_t size = 0;
    T*     data = NULL;

    explicit TempBuf(size_t size)
      : size(size),
        data(new T[size]) {}

    inline ~TempBuf() {
        delete[] data;
    }

    // clang-format off
    explicit TempBuf(const TempBuf<T>& other) requires std::is_copy_constructible_v<T>;
    explicit TempBuf(TempBuf<T>&& other)      requires std::is_move_constructible_v<T>;

    TempBuf<T>& operator=(const TempBuf<T>& other) requires std::is_copy_assignable_v<T>;
    TempBuf<T>& operator=(TempBuf<T>&& other)      requires std::is_move_assignable_v<T>;
    // clang-format on
};

template<typename T>
class TempObj {
  public:
    T* const obj;

    explicit TempObj()
        requires std::is_default_constructible_v<T>
    : obj(new T) {}

    template<typename... Args>
        requires std::is_constructible_v<T, Args...>
    explicit TempObj(Args&&... args)
      : obj(new T(std::forward<Args>(args)...)) {}

    inline ~TempObj() {
        delete obj;
    }

    // clang-format off
    explicit TempObj(const TempObj<T>& other) requires std::is_copy_constructible_v<T>;
    explicit TempObj(TempObj<T>&& other)      requires std::is_move_constructible_v<T>;

    TempObj<T>& operator=(const TempObj<T>& other) requires std::is_copy_assignable_v<T>;
    TempObj<T>& operator=(TempObj<T>&& other)      requires std::is_move_assignable_v<T>;
    // clang-format on

    T*       operator->();
    const T* operator->() const;
};

template<typename T>
class LinearAllocator {
    // We do static assertions instead of concept constraints to avoid repetition in function
    // template definitions.
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
    static_assert(std::is_move_assignable_v<T>, "T must be move assignable");
    static_assert(std::is_destructible_v<T>, "T must be destructible");

  public:
    using Dtor = std::function<void()>;

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
