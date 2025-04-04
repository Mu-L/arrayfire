/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <scan.hpp>

#include <kernel/scan_dim.hpp>
#include <kernel/scan_first.hpp>

namespace arrayfire {
namespace opencl {
template<af_op_t op, typename Ti, typename To>
Array<To> scan(const Array<Ti>& in, const int dim, bool inclusiveScan) {
    Array<To> out = createEmptyArray<To>(in.dims());

    Param Out = out;
    Param In  = in;

    if (dim == 0) {
        kernel::scanFirst<Ti, To, op>(Out, In, inclusiveScan);
    } else {
        kernel::scanDim<Ti, To, op>(Out, In, dim, inclusiveScan);
    }

    return out;
}

#define INSTANTIATE_SCAN(ROp, Ti, To) \
    template Array<To> scan<ROp, Ti, To>(const Array<Ti>&, const int, bool);

#define INSTANTIATE_SCAN_ALL(ROp)           \
    INSTANTIATE_SCAN(ROp, float, float)     \
    INSTANTIATE_SCAN(ROp, double, double)   \
    INSTANTIATE_SCAN(ROp, cfloat, cfloat)   \
    INSTANTIATE_SCAN(ROp, cdouble, cdouble) \
    INSTANTIATE_SCAN(ROp, int, int)         \
    INSTANTIATE_SCAN(ROp, uint, uint)       \
    INSTANTIATE_SCAN(ROp, intl, intl)       \
    INSTANTIATE_SCAN(ROp, uintl, uintl)     \
    INSTANTIATE_SCAN(ROp, char, uint)       \
    INSTANTIATE_SCAN(ROp, schar, int)       \
    INSTANTIATE_SCAN(ROp, uchar, uint)      \
    INSTANTIATE_SCAN(ROp, short, int)       \
    INSTANTIATE_SCAN(ROp, ushort, uint)

INSTANTIATE_SCAN(af_notzero_t, char, uint)
INSTANTIATE_SCAN_ALL(af_add_t)
INSTANTIATE_SCAN_ALL(af_mul_t)
INSTANTIATE_SCAN_ALL(af_min_t)
INSTANTIATE_SCAN_ALL(af_max_t)
}  // namespace opencl
}  // namespace arrayfire
