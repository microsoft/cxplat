#pragma once

#ifdef _KERNEL_MODE
#include "karray.h"

template <typename T>
class CxPlatVector : public Rtl::KArray<T>
{
    public:
    using Rtl::KArray<T>::KArray;

    bool
    push_back(
        _In_ T value
        )
    {
        return append(value);
    }

    const T* data() const { return &(*this)[0]; }

    size_t size() const { return count(); }

    void
    insert(
        _In_ const iterator &dest,
        _In_ const const_iterator &start,
        _In_ const const_iterator &end
        )
    {
        return insertAt(dest, start, end);
    }
};
#else

#include <vector>

template <typename T>
class CxPlatVector : public std::vector<T>
{
    public:
    using std::vector<T>::vector;

    bool
    push_back(
        _In_ T value
        )
    {
        try {
            std::vector<T>::push_back(value);
        } catch (...) {
            return false;
        }
        return true;
    }

    void
    eraseAt(
        _In_ size_t index
        )
    {
        std::vector<T>::erase(std::vector<T>::begin() + index);
    }
};

#endif
