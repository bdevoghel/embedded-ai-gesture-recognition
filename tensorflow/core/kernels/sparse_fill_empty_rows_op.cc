/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#define EIGEN_USE_THREADS

#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/op_requires.h"
#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_util.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/lib/gtl/inlined_vector.h"
#include "tensorflow/core/platform/errors.h"
#include "tensorflow/core/util/sparse/sparse_tensor.h"

namespace tensorflow {

using CPUDevice = Eigen::ThreadPoolDevice;

template <typename T>
class SparseFillEmptyRowsOp : public OpKernel {
 public:
  explicit SparseFillEmptyRowsOp(OpKernelConstruction* context)
      : OpKernel(context) {}

  void Compute(OpKernelContext* context) override {
    const int kIndicesInput = 0;
    const int kValuesInput = 1;
    const int kDenseShapeInput = 2;
    const int kDefaultValueInput = 3;

    const int kOutputIndicesOutput = 0;
    const int kOutputValuesOutput = 1;
    const int kEmptyRowIndicatorOutput = 2;
    const int kReverseIndexMapOutput = 3;

    const Tensor& indices_t = context->input(kIndicesInput);
    const Tensor& values_t = context->input(kValuesInput);
    const Tensor& dense_shape_t = context->input(kDenseShapeInput);
    const Tensor& default_value_t = context->input(kDefaultValueInput);

    OP_REQUIRES(context, TensorShapeUtils::IsVector(dense_shape_t.shape()),
                errors::InvalidArgument("dense_shape must be a vector, saw: ",
                                        dense_shape_t.shape().DebugString()));
    OP_REQUIRES(context, TensorShapeUtils::IsMatrix(indices_t.shape()),
                errors::InvalidArgument("indices must be a matrix, saw: ",
                                        indices_t.shape().DebugString()));
    OP_REQUIRES(context, TensorShapeUtils::IsVector(values_t.shape()),
                errors::InvalidArgument("values must be a vector, saw: ",
                                        values_t.shape().DebugString()));
    OP_REQUIRES(context, indices_t.dim_size(0) == values_t.dim_size(0),
                errors::InvalidArgument("The length of `values` (", values_t.dim_size(0),
                                        ") must match the first dimension of `indices` (",
                                        indices_t.dim_size(0), ")."));
    OP_REQUIRES(context, TensorShapeUtils::IsScalar(default_value_t.shape()),
                errors::InvalidArgument("default_value must be a scalar, saw: ",
                                        default_value_t.shape().DebugString()));
    // TODO(ebrevdo): add shape checks between values, indices,
    // dense_shape.  Also add check that dense rank > 0.
    // Also add check that dense rank > 0.
    OP_REQUIRES(context, dense_shape_t.NumElements() != 0,
                errors::InvalidArgument("Dense shape cannot be empty."));
    const T& default_value = default_value_t.scalar<T>()();
    const auto indices = indices_t.matrix<int64>();
    const auto values = values_t.vec<T>();
    const auto dense_shape = dense_shape_t.vec<int64>();

    const int64 N = indices_t.shape().dim_size(0);
    const int64 dense_rows = dense_shape(0);

    bool* empty_row_indicator = nullptr;
    if (context->output_required(kEmptyRowIndicatorOutput)) {
      Tensor* empty_row_indicator_t = nullptr;
      OP_REQUIRES_OK(context,
                     context->allocate_output(kEmptyRowIndicatorOutput,
                                              TensorShape({dense_rows}),
                                              &empty_row_indicator_t));
      empty_row_indicator = empty_row_indicator_t->vec<bool>().data();
    }
    int64* reverse_index_map = nullptr;
    if (context->output_required(kReverseIndexMapOutput)) {
      Tensor* reverse_index_map_t = nullptr;
      OP_REQUIRES_OK(context, context->allocate_output(kReverseIndexMapOutput,
                                                       TensorShape({N}),
                                                       &reverse_index_map_t));
      reverse_index_map = reverse_index_map_t->vec<int64>().data();
    }

    int rank = indices_t.shape().dim_size(1);

    if (dense_rows == 0) {
      OP_REQUIRES(
          context, N == 0,
          errors::InvalidArgument("Received SparseTensor with dense_shape[0] = "
                                  "0 but indices.shape[0] = ",
                                  N));
      Tensor* output_indices_t;
      TensorShape output_indices_shape({0, rank});
      OP_REQUIRES_OK(context, context->allocate_output(kOutputIndicesOutput,
                                                       output_indices_shape,
                                                       &output_indices_t));
      Tensor* output_values_t;
      OP_REQUIRES_OK(context, context->allocate_output(kOutputValuesOutput,
                                                       TensorShape({0}),
                                                       &output_values_t));

      // Exit early, nothing more to do.
      return;
    }

    bool rows_are_ordered = true;
    int64 last_indices_row = 0;
    std::vector<int64> csr_offset(dense_rows, 0);
    for (int i = 0; i < N; ++i) {
      const int64 row = indices(i, 0);
      OP_REQUIRES(context, row >= 0 && row < dense_rows,
                  errors::InvalidArgument("indices(", i, ", 0) is invalid: ",
                                          row, " >= ", dense_rows));
      ++csr_offset[row];
      rows_are_ordered = rows_are_ordered & (row >= last_indices_row);
      last_indices_row = row;
    }
    bool all_rows_full = true;
    for (int row = 0; row < dense_rows; ++row) {
      // csr_offset here describes the number of elements in this dense row
      bool row_empty = (csr_offset[row] == 0);
      if (empty_row_indicator) {
        empty_row_indicator[row] = row_empty;
      }
      all_rows_full = all_rows_full & !row_empty;
      // In filled version, each row has at least one element.
      csr_offset[row] = std::max(csr_offset[row], int64{1});
      // Update csr_offset to represent the number of elements up to and
      // including dense_row + 1:
      //  csr_offset(0) == #{elements of row 0}
      //  csr_offset(1) == #{elements of row 1} + #{elements of row 0}
      //  ..
      //  csr_offset(i) == starting index for elements in row i + 1.
      if (row > 0) {
        csr_offset[row] += csr_offset[row - 1];
      }
    }

    if (all_rows_full && rows_are_ordered) {
      context->set_output(kOutputIndicesOutput, indices_t);
      context->set_output(kOutputValuesOutput, values_t);
      if (reverse_index_map) {
        for (int64 i = 0; i < N; ++i) {
          reverse_index_map[i] = i;
        }
      }
    } else {
      Tensor* output_indices_t;
      const int64 N_full = csr_offset[dense_rows - 1];
      TensorShape output_indices_shape({N_full, rank});
      OP_REQUIRES_OK(context, context->allocate_output(kOutputIndicesOutput,
                                                       output_indices_shape,
                                                       &output_indices_t));
      auto output_indices = output_indices_t->matrix<int64>();

      Tensor* output_values_t;
      OP_REQUIRES_OK(context, context->allocate_output(kOutputValuesOutput,
                                                       TensorShape({N_full}),
                                                       &output_values_t));
      auto output_values = output_values_t->vec<T>();

      std::vector<int64> filled_count(dense_rows, 0);

      // Fill in values for rows that are not missing
      for (int64 i = 0; i < N; ++i) {
        const int64 row = indices(i, 0);
        int64& offset = filled_count[row];
        const int64 output_i = ((row == 0) ? 0 : csr_offset[row - 1]) + offset;
        offset++;  // Increment the filled count for this row.
        std::copy_n(&indices(i, 0), rank, &output_indices(output_i, 0));
        output_values(output_i) = values(i);
        // We'll need this reverse index map to backprop correctly.
        if (reverse_index_map) {
          reverse_index_map[i] = output_i;
        }
      }

      // Fill in values for rows that are missing
      for (int64 row = 0; row < dense_rows; ++row) {
        const int64 row_count = filled_count[row];
        if (row_count == 0) {  // We haven't filled this row
          const int64 starting_index = (row == 0) ? 0 : csr_offset[row - 1];
          // Remaining index values were set to zero already.
          // Just need to set the row index in the right location.
          output_indices(starting_index, 0) = row;
          for (int64 col = 1; col < rank; ++col) {
            output_indices(starting_index, col) = 0;
          }
          output_values(starting_index) = default_value;
        }
      }
    }
  }
};

#define REGISTER_KERNELS(type)                            \
  REGISTER_KERNEL_BUILDER(Name("SparseFillEmptyRows")     \
                              .Device(DEVICE_CPU)         \
                              .TypeConstraint<type>("T"), \
                          SparseFillEmptyRowsOp<type>)

TF_CALL_ALL_TYPES(REGISTER_KERNELS);
#undef REGISTER_KERNELS

template <typename T>
class SparseFillEmptyRowsGradOp : public OpKernel {
 public:
  explicit SparseFillEmptyRowsGradOp(OpKernelConstruction* context)
      : OpKernel(context) {}

  void Compute(OpKernelContext* context) override {
    const Tensor* reverse_index_map_t;
    const Tensor* grad_values_t;
    OP_REQUIRES_OK(context,
                   context->input("reverse_index_map", &reverse_index_map_t));
    OP_REQUIRES_OK(context, context->input("grad_values", &grad_values_t));

    const CPUDevice& d = context->eigen_device<CPUDevice>();

    OP_REQUIRES(
        context, TensorShapeUtils::IsVector(reverse_index_map_t->shape()),
        errors::InvalidArgument("reverse_index_map must be a vector, saw: ",
                                reverse_index_map_t->shape().DebugString()));
    OP_REQUIRES(context, TensorShapeUtils::IsVector(grad_values_t->shape()),
                errors::InvalidArgument("grad_values must be a vector, saw: ",
                                        grad_values_t->shape().DebugString()));

    const auto reverse_index_map = reverse_index_map_t->vec<int64>();
    const auto grad_values = grad_values_t->vec<T>();

    const int64 N = reverse_index_map_t->shape().dim_size(0);
    const int64 N_full = grad_values_t->shape().dim_size(0);

    Tensor* d_values_t;
    OP_REQUIRES_OK(context, context->allocate_output(
                                "d_values", TensorShape({N}), &d_values_t));
    auto d_values = d_values_t->vec<T>();
    Tensor* d_default_value_t;
    OP_REQUIRES_OK(context,
                   context->allocate_output("d_default_value", TensorShape({}),
                                            &d_default_value_t));
    T& d_default_value = d_default_value_t->scalar<T>()();
    d_default_value = T();

    Tensor visited_t;
    OP_REQUIRES_OK(context, context->allocate_temp(
                                DT_BOOL, TensorShape({N_full}), &visited_t));
    auto visited = visited_t.vec<bool>();
    visited.device(d) = visited.constant(false);

    for (int i = 0; i < N; ++i) {
      // Locate the index of the output of the forward prop associated
      // with this location in the input of the forward prop.  Copy
      // the gradient into it.  Mark it as visited.
      int64 reverse_index = reverse_index_map(i);
      OP_REQUIRES(
          context, 0 <= reverse_index && reverse_index < N_full,
          errors::InvalidArgument("Elements in reverse index must be in [0, ",
                                  N_full, ") but got ", reverse_index));
      d_values(i) = grad_values(reverse_index);
      visited(reverse_index) = true;
    }
    for (int j = 0; j < N_full; ++j) {
      // The default value gradient gets the accumulated remainder of
      // the backprop values (since the default value was used to fill
      // in these slots in the forward calculation).
      if (!visited(j)) {
        d_default_value += grad_values(j);
      }
    }
  }
};

#define REGISTER_KERNELS(type)                            \
  REGISTER_KERNEL_BUILDER(Name("SparseFillEmptyRowsGrad") \
                              .Device(DEVICE_CPU)         \
                              .TypeConstraint<type>("T"), \
                          SparseFillEmptyRowsGradOp<type>)

TF_CALL_NUMBER_TYPES(REGISTER_KERNELS);
#undef REGISTER_KERNELS
}  // namespace tensorflow
