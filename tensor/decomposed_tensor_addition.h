#pragma once
#ifndef TCC_TENSOR_DECOMPOSEDTENSORADDITION_H
#define TCC_TENSOR_DECOMPOSEDTENSORADDITION_H

#include "decomposed_tensor.h"
#include "decomposed_tensor_algebra.h"

namespace tcc {
namespace tensor {

template <typename T>
DecomposedTensor<T>
add(DecomposedTensor<T> const &l, DecomposedTensor<T> const &r) {
    if (l.ndecomp() >= 2) {
        if (r.ndecomp() >= 2) {
            auto const &l_extent = l.tensor(0).range().size();
            auto const &r_extent = r.tensor(1).range().size();
            const auto l_rank = l.rank();
            const auto r_rank = r.rank();
            const auto out_rank = l_rank + r_rank;
            TA::Range l_new_range{l_extent[0], out_rank};
            TA::Range r_new_range{out_rank, r_extent[1], r_extent[2]};

            TA::Tensor<T> L(l_new_range);
            TA::Tensor<T> R(r_new_range);

            // copy into left
            const auto l_vol = L.range().volume();
            auto data = L.data();
            auto ll_data = l.tensor(0).data();
            auto rl_data = r.tensor(0).data();
            // Access data in stride
            for (auto i = 0ul; i < l_vol; i += out_rank) {
                std::copy(ll_data, ll_data + l_rank, data + i);
                ll_data += l_rank;
                std::copy(rl_data, rl_data + r_rank, data + i + l_rank);
                rl_data += r_rank;
            }

            // copy into R
            data = R.data();
            auto lr_data = l.tensor(1).data();
            auto rr_data = r.tensor(1).data();
            auto old_l_vol = l.tensor(1).range().volume();
            auto old_r_vol = r.tensor(1).range().volume();
            // R is copied by rows so we don't have to loop.
            std::copy(lr_data, lr_data + old_l_vol, data);
            std::copy(rr_data, rr_data + old_r_vol, data + old_l_vol);
            return DecomposedTensor<T>(l.cut(), std::move(L), std::move(R));
        }
    }
    assert(false);
    return DecomposedTensor<T>(l.cut());
}

template <typename T>
DecomposedTensor<T>
add(DecomposedTensor<T> const &l, DecomposedTensor<T> const &r,
    TA::Permutation const &p) {
    assert(false);
}

template <typename T>
DecomposedTensor<T> add(DecomposedTensor<T> const &l,
                        DecomposedTensor<T> const &r, const T factor) {
    assert(false);
}

template <typename T>
DecomposedTensor<T>
add(DecomposedTensor<T> const &l, DecomposedTensor<T> const &r, const T factor,
    TA::Permutation const &p) {
    assert(false);
}

template <typename T>
DecomposedTensor<T> add(DecomposedTensor<T> const &l, const T factor) {
    assert(false);
}

template <typename T>
DecomposedTensor<T>
add(DecomposedTensor<T> const &l, const T factor, TA::Permutation const &p) {
    assert(false);
}

template <typename T>
DecomposedTensor<T> &
add_to(DecomposedTensor<T> &l, DecomposedTensor<T> const &r) {
    if (l.ndecomp() == 1) {
        if (r.ndecomp() == 1) {
            l.tensor(0).add_to(r.tensor(0));
        } else {
            constexpr auto NoT = madness::cblas::CBLAS_TRANSPOSE::NoTrans;
            auto gh = TA::math::GemmHelper(NoT, NoT, 3, 2, 3);
            l.tensor(0).gemm(r.tensor(0), r.tensor(1), 1.0, gh);
        }

    } else {
        if (r.ndecomp() == 1) {
            constexpr auto NoT = madness::cblas::CBLAS_TRANSPOSE::NoTrans;
            auto gh = TA::math::GemmHelper(NoT, NoT, 3, 2, 3);
            auto temp = r.tensor(0).clone();
            temp.gemm(l.tensor(0), l.tensor(1), 1.0, gh);
            l = DecomposedTensor<T>(l.cut(), std::move(temp));
        } else {
            l = add(l, r);
            auto const &l_left_extent = l.tensor(0).range().size();
            auto const &l_right_extent = l.tensor(1).range().size();
            const auto long_dim = l_right_extent[1] * l_right_extent[2];
            auto out_dim = l.rank();
            const auto full_rank = std::min(l_left_extent[0], long_dim);

            if (out_dim >= full_rank / 6) {
                algebra::recompress(l);
                out_dim = l.rank();
            }

            if (out_dim > full_rank / 2) {
                l = DecomposedTensor<T>(l.cut(), algebra::combine(l));
            }
        }
    }

    return l;
}

template <typename T>
DecomposedTensor<T> &
add_to(DecomposedTensor<T> &l, DecomposedTensor<T> const &r, const T factor) {
    assert(false);
}

template <typename T>
DecomposedTensor<T> &add_to(DecomposedTensor<T> &l, const T factor) {
    assert(false);
}

} // namespace tensor
} // namespace tcc

#endif // TCC_TENSOR_DECOMPOSEDTENSORADDITION_H
