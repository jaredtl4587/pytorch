#pragma once

#include <torch/nn/options/normalization.h>
#include <torch/nn/functional/padding.h>
#include <torch/nn/functional/pooling.h>
#include <torch/types.h>

namespace torch {
namespace nn {
namespace functional {

namespace detail {
inline Tensor normalize(
    const Tensor& input,
    double p,
    int64_t dim,
    double eps,
    c10::optional<Tensor> out) {
    if (out == c10::nullopt) {
      auto denom = input.norm(p, dim, true).clamp_min(eps).expand_as(input);
      return input / denom;
    } else {
      auto denom = input.norm(p, dim, true).clamp_min(eps).expand_as(input);
      return torch::div_out(*out, input, denom);
    }
}
} // namespace detail

inline Tensor normalize(
    const Tensor& input,
    NormalizeFuncOptions options = {},
    c10::optional<Tensor> out = c10::nullopt) {
  return detail::normalize(input, options.p(), options.dim(), options.eps(), out);
}

// ============================================================================

namespace detail {
inline Tensor layer_norm(const Tensor& input,
                         const std::vector<int64_t>& normalized_shape,
                         const Tensor& weight,
                         const Tensor& bias,
                         double eps) {
    return torch::layer_norm(input, normalized_shape, weight, bias, eps);
}
} // namespace detail

inline Tensor layer_norm(const Tensor& input,
    LayerNormFuncOptions options,
    const Tensor& weight = Tensor(),
    const Tensor& bias = Tensor()) {
  return detail::layer_norm(input, options.normalized_shape(), weight, bias, options.eps());
}

// ============================================================================

namespace detail {
inline Tensor local_response_norm(
    const Tensor& input,
    int64_t size,
    double alpha,
    double beta,
    double k) {
    auto dim = input.dim();
    TORCH_CHECK(dim >=3, "Expected 3D or higher dimensionality input (got ", dim, " dimensions)");
    auto div = input.mul(input).unsqueeze(1);
    if (dim == 3) {
      div = detail::pad(div, /*pad=*/{0, 0, size / 2, (size - 1) / 2}, /*mode=*/torch::kConstant, /*value=*/0);
      div = detail::avg_pool2d(
        div,
        /*kernel_size=*/{size, 1},
        /*stride=*/1,
        /*padding=*/0,
        /*ceil_mode=*/false,
        /*count_include_pad=*/true,
        /*divisor_override=*/c10::nullopt).squeeze(1);
    } else {
      auto sizes = input.sizes();
      div = div.view({sizes[0], 1, sizes[1], sizes[2], -1});
      div = detail::pad(div, /*pad=*/{0, 0, 0, 0, size / 2, (size - 1) / 2}, /*mode=*/torch::kConstant, /*value=*/0);
      div = detail::avg_pool3d(
        div,
        /*kernel_size=*/{size, 1, 1},
        /*stride=*/1,
        /*padding=*/0,
        /*ceil_mode=*/false,
        /*count_include_pad=*/true,
        /*divisor_override=*/c10::nullopt).squeeze(1);
      div = div.view(sizes);
    }
    div = div.mul(alpha).add(k).pow(beta);
    return input / div;
}
} // namespace detail

inline Tensor local_response_norm(
    const Tensor& input,
    LocalResponseNormFuncOptions options) {
  return detail::local_response_norm(input, options.size(), options.alpha(), options.beta(), options.k());
}

} // namespace functional
} // namespace nn
} // namespace torch
