#pragma once

#ifdef _KERNEL_MODE
#include "karray.h"

template <typename T>
using CxPlatVector = Rtl::KArray<T>;

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
