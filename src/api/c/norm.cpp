/*******************************************************
 * Copyright (c) 2025, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <arith.hpp>
#include <backend.hpp>
#include <common/ArrayInfo.hpp>
#include <common/cast.hpp>
#include <common/err_common.hpp>
#include <complex.hpp>
#include <copy.hpp>
#include <handle.hpp>
#include <lu.hpp>
#include <math.hpp>
#include <reduce.hpp>
#include <af/array.h>
#include <af/constants.h>
#include <af/defines.h>
#include <af/lapack.h>
#include <af/traits.hpp>

using af::dim4;
using arrayfire::common::cast;
using detail::arithOp;
using detail::Array;
using detail::cdouble;
using detail::cfloat;
using detail::createEmptyArray;
using detail::createValueArray;
using detail::getScalar;
using detail::reduce;
using detail::reduce_all;
using detail::scalar;

template<typename T>
using normReductionResult =
    typename std::conditional<std::is_same<T, arrayfire::common::half>::value, float,
                              T>::type;

template<typename T>
double matrixNorm(const Array<T> &A, double p) {
    using RT = normReductionResult<T>;
    if (p == 1) {
        Array<RT> colSum = reduce<af_add_t, T, normReductionResult<T>>(A, 0);
        return getScalar<RT>(reduce_all<af_max_t, RT, RT>(colSum));
    }
    if (p == af::Inf) {
        Array<RT> rowSum = reduce<af_add_t, T, RT>(A, 1);
        return getScalar<RT>(reduce_all<af_max_t, RT, RT>(rowSum));
    }

    AF_ERROR("This type of norm is not supported in ArrayFire\n",
             AF_ERR_NOT_SUPPORTED);
}

template<typename T>
double vectorNorm(const Array<T> &A, double p) {
    using RT = normReductionResult<T>;
    if (p == 1) { return getScalar<RT>(reduce_all<af_add_t, T, RT>(A)); }
    if (p == af::Inf) {
        return getScalar<RT>(reduce_all<af_max_t, RT, RT>(cast<RT>(A)));
    } else if (p == 2) {
        Array<T> A_sq = arithOp<T, af_mul_t>(A, A, A.dims());
        return std::sqrt(getScalar<RT>(reduce_all<af_add_t, T, RT>(A_sq)));
    }

    Array<T> P   = createValueArray<T>(A.dims(), scalar<T>(p));
    Array<T> A_p = arithOp<T, af_pow_t>(A, P, A.dims());
    return std::pow(getScalar<RT>(reduce_all<af_add_t, T, RT>(A_p)), (1.0 / p));
}

template<typename T>
double LPQNorm(const Array<T> &A, double p, double q) {
    using RT           = normReductionResult<T>;
    Array<RT> A_p_norm = createEmptyArray<RT>(dim4());

    if (p == 1) {
        A_p_norm = reduce<af_add_t, T, RT>(A, 0);
    } else {
        Array<T> P     = createValueArray<T>(A.dims(), scalar<T>(p));
        Array<RT> invP = createValueArray<RT>(A.dims(), scalar<RT>(1.0 / p));

        Array<T> A_p      = arithOp<T, af_pow_t>(A, P, A.dims());
        Array<RT> A_p_sum = reduce<af_add_t, T, RT>(A_p, 0);
        A_p_norm          = arithOp<RT, af_pow_t>(A_p_sum, invP, invP.dims());
    }

    if (q == 1) {
        return getScalar<RT>(reduce_all<af_add_t, RT, RT>(A_p_norm));
    }

    Array<RT> Q          = createValueArray<RT>(A_p_norm.dims(), scalar<RT>(q));
    Array<RT> A_p_norm_q = arithOp<RT, af_pow_t>(A_p_norm, Q, Q.dims());

    return std::pow(getScalar<RT>(reduce_all<af_add_t, RT, RT>(A_p_norm_q)),
                    (1.0 / q));
}

template<typename T>
double norm(const af_array a, const af_norm_type type, const double p,
            const double q) {
    using BT = typename af::dtype_traits<T>::base_type;

    const Array<BT> A = detail::abs<BT, T>(getArray<T>(a));

    switch (type) {
        case AF_NORM_EUCLID: return vectorNorm(A, 2);
        case AF_NORM_VECTOR_1: return vectorNorm(A, 1);
        case AF_NORM_VECTOR_INF: return vectorNorm(A, af::Inf);
        case AF_NORM_VECTOR_P: return vectorNorm(A, p);
        case AF_NORM_MATRIX_1: return matrixNorm(A, 1);
        case AF_NORM_MATRIX_INF: return matrixNorm(A, af::Inf);
        case AF_NORM_MATRIX_2: return matrixNorm(A, 2);
        case AF_NORM_MATRIX_L_PQ: return LPQNorm(A, p, q);
        default:
            AF_ERROR("This type of norm is not supported in ArrayFire\n",
                     AF_ERR_NOT_SUPPORTED);
    }
}

af_err af_norm(double *out, const af_array in, const af_norm_type type,
               const double p, const double q) {
    try {
        const ArrayInfo &i_info = getInfo(in);
        if (i_info.ndims() > 2) {
            AF_ERROR("solve can not be used in batch mode", AF_ERR_BATCH);
        }

        af_dtype i_type = i_info.getType();
        ARG_ASSERT(1, i_info.isFloating());  // Only floating and complex types
        *out = 0;
        if (i_info.ndims() == 0) { return AF_SUCCESS; }

        switch (i_type) {
            case f32: *out = norm<float>(in, type, p, q); break;
            case f64: *out = norm<double>(in, type, p, q); break;
            case c32: *out = norm<cfloat>(in, type, p, q); break;
            case c64: *out = norm<cdouble>(in, type, p, q); break;
            case f16: *out = norm<arrayfire::common::half>(in, type, p, q); break;
            default: TYPE_ERROR(1, i_type);
        }
    }
    CATCHALL;

    return AF_SUCCESS;
}
