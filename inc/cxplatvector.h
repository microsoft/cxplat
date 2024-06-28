#pragma once

#include "karray.h"

#ifdef _KERNEL_MODE
template <typename T>
class CxPlatVector : public Rtl::KArray<T>
{
    public:
    void
    push_back(
        _In_ T value
        )
    {
        CXPLAT_FRE_ASSERT(append(value));
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
        CXPLAT_FRE_ASSERT(insertAt(dest, start, end));
    }
};
#else

#include <vector>

template <typename T>
using CxPlatVector = std::vector<T>;
#endif