/*******************************************************************************
* Copyright 2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include "oneapi/dal/algo/svm/infer.hpp"
#include "oneapi/dal/algo/svm/train.hpp"

#include "oneapi/dal/test/engine/common.hpp"
#include "oneapi/dal/test/engine/fixtures.hpp"
#include "oneapi/dal/test/engine/math.hpp"

#include "oneapi/dal/table/homogen.hpp"

namespace oneapi::dal::svm::test {

namespace te = dal::test::engine;
namespace rbf = oneapi::dal::rbf_kernel;
namespace linear = oneapi::dal::linear_kernel;

template <typename TestType>
class svm_batch_test : public te::algo_fixture {
public:
    using Float = std::tuple_element_t<0, TestType>;
    using Method = std::tuple_element_t<1, TestType>;
    using KernelTypeLinear = linear::descriptor<Float, linear::method::dense>;
    using KernelTypeRBF = rbf::descriptor<Float, rbf::method::dense>;

    bool not_available_on_device() {
        constexpr bool is_smo = std::is_same_v<Method, svm::method::smo>;
        return get_policy().is_gpu() && is_smo;
    }

    bool weights_not_available_on_device() {
        return get_policy().is_gpu();
    }

    void check_linear_kernel(
        const table& train_data,
        const table& train_labels,
        const svm::descriptor<Float, Method, svm::task::classification, KernelTypeLinear>& desc,
        const std::int64_t support_vector_count,
        const table& support_indices,
        const table& decision_function,
        const table& labels) {
        CAPTURE(support_vector_count);

        INFO("run training");
        auto train_result = this->train(desc, train_data, train_labels);
        const auto model = train_result.get_model();
        check_train_result(train_data, train_result, support_vector_count, support_indices);

        INFO("run inference");
        const auto infer_result = infer(desc, model, train_data);
        check_infer_result(train_data, infer_result, decision_function, labels);
    }

    void check_rbf_kernel(
        const table& train_data,
        const table& train_labels,
        const svm::descriptor<Float, Method, svm::task::classification, KernelTypeRBF>& desc,
        const std::int64_t support_vector_count,
        const table& support_indices,
        const table& decision_function,
        const table& labels) {
        CAPTURE(support_vector_count);

        INFO("run training");
        auto train_result = this->train(desc, train_data, train_labels);
        const auto model = train_result.get_model();
        check_train_result(train_data, train_result, support_vector_count, support_indices);

        INFO("run inference");
        const auto infer_result = infer(desc, model, train_data);
        check_infer_result(train_data, infer_result, decision_function, labels);
    }

    void check_different_labels(
        const table& train_data,
        const table& train_labels,
        const svm::descriptor<Float, Method, svm::task::classification, KernelTypeLinear>& desc,
        const std::int64_t support_vector_count,
        const table& support_indices,
        const Float* expected_labels) {
        CAPTURE(support_vector_count);

        INFO("run training");
        auto train_result = this->train(desc, train_data, train_labels);
        const auto model = train_result.get_model();
        check_train_result(train_data, train_result, support_vector_count, support_indices);

        INFO("check if first and second class label is expected")
        REQUIRE(model.get_first_class_label() == expected_labels[0]);
        REQUIRE(model.get_second_class_label() == expected_labels[1]);
    }

    void check_weights(
        const table& train_data,
        const table& train_labels,
        const table& train_weights,
        const svm::descriptor<Float, Method, svm::task::classification, KernelTypeLinear>& desc,
        const std::int64_t support_vector_count,
        const table& support_indices,
        const table& test_data,
        const table& expected_decision_function) {
        CAPTURE(support_vector_count);

        INFO("run training");
        auto train_result = this->train(desc, train_data, train_labels, train_weights);
        const auto model = train_result.get_model();
        check_train_result(train_data, train_result, support_vector_count, support_indices);

        INFO("run inference");
        const auto infer_result = infer(desc, model, test_data);
        const auto decision_function = infer_result.get_decision_function();

        INFO("check if decision fuction is expected")
        check_table_match(expected_decision_function, decision_function);
    }

    void check_train_result(const table& train_data,
                            const svm::train_result<>& result,
                            const std::int64_t support_vector_count,
                            const table& support_indices) {
        check_shapes(train_data, result, support_vector_count);
        check_nans(result);

        INFO("check if support_indices values is expected")
        check_table_match(support_indices, result.get_support_indices());
    }

    void check_infer_result(const table& infer_data,
                            const svm::infer_result<>& result,
                            const table& decision_function,
                            const table& labels) {
        check_shapes(infer_data, result);
        check_nans(result);

        INFO("check if decision_function values is expected")
        if (decision_function.has_data())
            check_table_match(decision_function, result.get_decision_function());

        INFO("check if labels values is expected")
        check_table_match(labels, result.get_labels());
    }

    void check_table_match(const table& reference, const table& actual_value) {
        const double tol = te::get_tolerance<Float>(1e-4, 1e-10);
        const double diff = te::rel_error(reference, actual_value, tol);
        CHECK(diff < tol);
    }

    void check_shapes(const table& train_data,
                      const svm::train_result<>& result,
                      const std::int64_t support_vector_count) {
        const auto [support_vectors, support_indices, coeffs] = unpack_result(result);

        INFO("check if support_vector_count is expected")
        REQUIRE(result.get_support_vector_count() == support_vector_count);

        INFO("check if support_vectors shape is expected")
        REQUIRE(support_vectors.get_row_count() == support_vector_count);
        REQUIRE(support_vectors.get_column_count() == train_data.get_column_count());

        INFO("check if support_indices shape is expected")
        REQUIRE(support_indices.get_row_count() == support_vector_count);
        REQUIRE(support_indices.get_column_count() == 1);

        INFO("check if coeffs shape is expected")
        REQUIRE(coeffs.get_row_count() == support_vector_count);
        REQUIRE(coeffs.get_column_count() == 1);
    }

    void check_nans(const svm::train_result<>& result) {
        const auto [support_vectors, support_indices, coeffs] = unpack_result(result);

        INFO("check if there is no NaN in support_vectors")
        REQUIRE(te::has_no_nans(support_vectors));

        INFO("check if there is no NaN in support_indices")
        REQUIRE(te::has_no_nans(support_indices));

        INFO("check if there is no NaN in coeffs")
        REQUIRE(te::has_no_nans(coeffs));
    }

    void check_shapes(const table& infer_data, const svm::infer_result<>& result) {
        const auto [labels, decision_function] = unpack_result(result);

        INFO("check if labels shape is expected")
        REQUIRE(labels.get_row_count() == infer_data.get_row_count());
        REQUIRE(labels.get_column_count() == 1);

        INFO("check if decision_function shape is expected")
        REQUIRE(decision_function.get_row_count() == infer_data.get_row_count());
        REQUIRE(decision_function.get_column_count() == 1);
    }

    void check_nans(const svm::infer_result<>& result) {
        const auto [labels, decision_function] = unpack_result(result);

        INFO("check if there is no NaN in labels")
        REQUIRE(te::has_no_nans(labels));

        INFO("check if there is no NaN in decision_function")
        REQUIRE(te::has_no_nans(decision_function));
    }

private:
    static auto unpack_result(const svm::train_result<>& result) {
        const auto support_vectors = result.get_support_vectors();
        const auto support_indices = result.get_support_indices();
        const auto coeffs = result.get_coeffs();
        return std::make_tuple(support_vectors, support_indices, coeffs);
    }

    static auto unpack_result(const svm::infer_result<>& result) {
        const auto labels = result.get_labels();
        const auto decision_function = result.get_decision_function();
        return std::make_tuple(labels, decision_function);
    }
};

using svm_types = COMBINE_TYPES((float, double), (svm::method::thunder, svm::method::smo));

TEMPLATE_LIST_TEST_M(svm_batch_test,
                     "svm can classify linear separable surface",
                     "[svm][integration][batch][linear]",
                     svm_types) {
    SKIP_IF(this->not_available_on_device());

    using float_t = std::tuple_element_t<0, TestType>;
    using method_t = std::tuple_element_t<1, TestType>;
    using kernel_t = linear::descriptor<float_t, linear::method::dense>;

    constexpr std::int64_t row_count_train = 6;
    constexpr std::int64_t column_count = 2;
    constexpr std::int64_t element_count_train = row_count_train * column_count;

    constexpr std::array<float_t, element_count_train> x_data = {
        -2.0, -1.0, -1.0, -1.0, -1.0, -2.0, 1.0, 1.0, 1.0, 2.0, 2.0, 1.0,
    };
    const auto x = homogen_table::wrap(x_data.data(), row_count_train, column_count);

    constexpr std::array<float_t, row_count_train> y_data = {
        -1.0, -1.0, -1.0, 1.0, 1.0, 1.0,
    };
    const auto y = homogen_table::wrap(y_data.data(), row_count_train, 1);

    const double scale = GENERATE_COPY(0.1, 1.0);
    const double c = GENERATE_COPY(1.0, 10.0);

    const auto kernel_desc = kernel_t{}.set_scale(scale);
    const auto svm_desc =
        svm::descriptor<float_t, method_t, svm::task::classification, kernel_t>{}.set_c(c);

    constexpr std::int64_t support_vector_count = 2;

    constexpr std::array<float_t, support_vector_count> support_indices_data = { 1, 3 };
    const auto support_indices =
        homogen_table::wrap(support_indices_data.data(), support_vector_count, 1);

    constexpr std::array<float_t, row_count_train> decision_function_data = { -1.5, -1.0, -1.5,
                                                                              1.0,  1.5,  1.5 };
    const auto decision_function =
        homogen_table::wrap(decision_function_data.data(), row_count_train, 1);

    constexpr std::array<float_t, row_count_train> labels_data = {
        -1.0, -1.0, -1.0, 1.0, 1.0, 1.0
    };
    const auto labels = homogen_table::wrap(labels_data.data(), row_count_train, 1);

    this->check_linear_kernel(x,
                              y,
                              svm_desc,
                              support_vector_count,
                              support_indices,
                              decision_function,
                              labels);
}

TEMPLATE_LIST_TEST_M(svm_batch_test,
                     "svm can classify linear separable surface with big margin",
                     "[svm][integration][batch][linear]",
                     svm_types) {
    SKIP_IF(this->not_available_on_device());

    using float_t = std::tuple_element_t<0, TestType>;
    using method_t = std::tuple_element_t<1, TestType>;
    using kernel_t = linear::descriptor<float_t, linear::method::dense>;

    constexpr std::int64_t row_count_train = 6;
    constexpr std::int64_t column_count = 2;
    constexpr std::int64_t element_count_train = row_count_train * column_count;

    constexpr std::array<float_t, element_count_train> x_data = {
        -2.0, -1.0, -1.0, -1.0, -1.0, -2.0, 1.0, 1.0, 1.0, 2.0, 2.0, 1.0,
    };
    const auto x = homogen_table::wrap(x_data.data(), row_count_train, column_count);

    constexpr std::array<float_t, row_count_train> y_data = {
        -1.0, -1.0, -1.0, 1.0, 1.0, 1.0,
    };
    const auto y = homogen_table::wrap(y_data.data(), row_count_train, 1);

    const double scale = 1.0;
    const double c = 1e-1;

    const auto kernel_desc = kernel_t{}.set_scale(scale);
    const auto svm_desc =
        svm::descriptor<float_t, method_t, svm::task::classification, kernel_t>{}.set_c(c);

    constexpr std::int64_t support_vector_count = 6;

    constexpr std::array<float_t, support_vector_count> support_indices_data = { 0, 1, 2, 3, 4, 5 };
    const auto support_indices =
        homogen_table::wrap(support_indices_data.data(), support_vector_count, 1);

    constexpr std::array<float_t, row_count_train> decision_function_data = { -1.0, -2.0 / 3,
                                                                              -1.0, 2.0 / 3,
                                                                              1.0,  1.0 };
    const auto decision_function =
        homogen_table::wrap(decision_function_data.data(), row_count_train, 1);

    constexpr std::array<float_t, row_count_train> labels_data = {
        -1.0, -1.0, -1.0, 1.0, 1.0, 1.0,
    };
    const auto labels = homogen_table::wrap(labels_data.data(), row_count_train, 1);

    this->check_linear_kernel(x,
                              y,
                              svm_desc,
                              support_vector_count,
                              support_indices,
                              decision_function,
                              labels);
}

TEMPLATE_LIST_TEST_M(svm_batch_test,
                     "svm can classify linear not separable surface",
                     "[svm][integration][batch][linear]",
                     svm_types) {
    SKIP_IF(this->not_available_on_device());

    using float_t = std::tuple_element_t<0, TestType>;
    using method_t = std::tuple_element_t<1, TestType>;
    using kernel_t = linear::descriptor<float_t, linear::method::dense>;

    constexpr std::int64_t row_count_train = 8;
    constexpr std::int64_t column_count = 2;
    constexpr std::int64_t element_count_train = row_count_train * column_count;

    constexpr std::array<float_t, element_count_train> x_data = {
        -2.0, -1.0, -1.0, -1.0, -1.0, -2.0, 1.0, 1.0, 1.0, 2.0, 2.0, 1.0, -3.0, -3.0, 3.0, 3.0
    };
    const auto x = homogen_table::wrap(x_data.data(), row_count_train, column_count);

    constexpr std::array<float_t, row_count_train> y_data = { -1.0, -1.0, -1.0, 1.0,
                                                              1.0,  1.0,  1.0,  -1.0 };
    const auto y = homogen_table::wrap(y_data.data(), row_count_train, 1);

    const double scale = 1.0;
    const double c = 1.0;

    const auto kernel_desc = kernel_t{}.set_scale(scale);
    const auto svm_desc =
        svm::descriptor<float_t, method_t, svm::task::classification, kernel_t>{}.set_c(c);

    constexpr std::int64_t support_vector_count = 8;

    constexpr std::array<float_t, support_vector_count> support_indices_data = { 0, 1, 2, 3,
                                                                                 4, 5, 6, 7 };
    const auto support_indices =
        homogen_table::wrap(support_indices_data.data(), support_vector_count, 1);

    constexpr std::array<float_t, row_count_train> decision_function_data = {
        -1.0, -2.0 / 3, -1.0, 2.0 / 3, 1.0, 1.0, -2.0, 2.0
    };
    const auto decision_function =
        homogen_table::wrap(decision_function_data.data(), row_count_train, 1);

    constexpr std::array<float_t, row_count_train> labels_data = { -1.0, -1.0, -1.0, 1.0,
                                                                   1.0,  1.0,  -1.0, 1.0 };
    const auto labels = homogen_table::wrap(labels_data.data(), row_count_train, 1);

    this->check_linear_kernel(x,
                              y,
                              svm_desc,
                              support_vector_count,
                              support_indices,
                              decision_function,
                              labels);
}

TEMPLATE_LIST_TEST_M(svm_batch_test,
                     "svm can classify quadric separable surface with rbf kernel",
                     "[svm][integration][batch][rbf]",
                     svm_types) {
    SKIP_IF(this->not_available_on_device());

    using float_t = std::tuple_element_t<0, TestType>;
    using method_t = std::tuple_element_t<1, TestType>;
    using kernel_t = rbf::descriptor<float_t, rbf::method::dense>;

    constexpr std::int64_t row_count_train = 12;
    constexpr std::int64_t column_count = 2;
    constexpr std::int64_t element_count_train = row_count_train * column_count;

    constexpr std::array<float_t, element_count_train> x_data = {
        -2.0, 0.0, -2.0, -1.0, -2.0, 1.0, 2.0, 0.0, 2.0, -1.0, 2.0, 1.0,
        -1.0, 0.0, -1.0, -0.5, -1.0, 0.5, 1.0, 0.5, 1.0, -0.5, 1.0, 0.5
    };
    const auto x = homogen_table::wrap(x_data.data(), row_count_train, column_count);

    constexpr std::array<float_t, row_count_train> y_data = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
                                                              1.0,  1.0,  1.0,  1.0,  1.0,  1.0 };
    const auto y = homogen_table::wrap(y_data.data(), row_count_train, 1);

    const double sigma = 1.0;
    const double c = 1.0;

    const auto kernel_desc = kernel_t{}.set_sigma(sigma);
    const auto svm_desc =
        svm::descriptor<float_t, method_t, svm::task::classification, kernel_t>{}.set_c(c);

    constexpr std::int64_t support_vector_count = 12;

    constexpr std::array<float_t, support_vector_count> support_indices_data = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
    };
    const auto support_indices =
        homogen_table::wrap(support_indices_data.data(), support_vector_count, 1);

    this->check_rbf_kernel(x,
                           y,
                           svm_desc,
                           support_vector_count,
                           support_indices,
                           homogen_table{},
                           y);
}

TEMPLATE_LIST_TEST_M(svm_batch_test,
                     "svm can classify any two labels",
                     "[svm][integration][batch][linear]",
                     svm_types) {
    SKIP_IF(this->not_available_on_device());

    using float_t = std::tuple_element_t<0, TestType>;
    using method_t = std::tuple_element_t<1, TestType>;
    using kernel_t = linear::descriptor<float_t, linear::method::dense>;

    constexpr std::int64_t row_count_train = 6;
    constexpr std::int64_t column_count = 2;
    constexpr std::int64_t element_count_train = row_count_train * column_count;

    using labels_pair_t = std::pair<std::array<float_t, row_count_train>, std::array<float_t, 2>>;

    constexpr std::array<float_t, element_count_train> x_data = { -2.0, -1.0, -1.0, -1.0,
                                                                  -1.0, -2.0, 1.0,  1.0,
                                                                  1.0,  2.0,  2.0,  1.0 };
    const auto x = homogen_table::wrap(x_data.data(), row_count_train, column_count);

    labels_pair_t labels =
        GENERATE_COPY(labels_pair_t({ -1.0, -1.0, -1.0, 1.0, 1.0, 1.0 }, { -1.0, 1.0 }),
                      labels_pair_t({ 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 }, { 0.0, 1.0 }),
                      labels_pair_t({ 0.0, 0.0, 0.0, 2.0, 2.0, 2.0 }, { 0.0, 2.0 }),
                      labels_pair_t({ -1.0, -1.0, -1.0, 0.0, 0.0, 0.0 }, { -1.0, 0.0 }));

    const auto y = homogen_table::wrap(labels.first.data(), row_count_train, 1);

    const double scale = 1.0;
    const double c = 1e-1;

    const auto kernel_desc = kernel_t{}.set_scale(scale);
    const auto svm_desc =
        svm::descriptor<float_t, method_t, svm::task::classification, kernel_t>{}.set_c(c);

    constexpr std::int64_t support_vector_count = 6;

    constexpr std::array<float_t, support_vector_count> support_indices_data = { 0, 1, 2, 3, 4, 5 };
    const auto support_indices =
        homogen_table::wrap(support_indices_data.data(), support_vector_count, 1);

    this->check_different_labels(x,
                                 y,
                                 svm_desc,
                                 support_vector_count,
                                 support_indices,
                                 labels.second.data());
}

TEMPLATE_LIST_TEST_M(svm_batch_test,
                     "svm can classify linear separable surface with weights",
                     "[svm][integration][batch][linear]",
                     svm_types) {
    SKIP_IF(this->weights_not_available_on_device());

    using float_t = std::tuple_element_t<0, TestType>;
    using method_t = std::tuple_element_t<1, TestType>;
    using kernel_t = linear::descriptor<float_t, linear::method::dense>;

    constexpr std::int64_t row_count_train = 6;
    constexpr std::int64_t column_count = 2;
    constexpr std::int64_t element_count_train = row_count_train * column_count;

    using weights_pair_t = std::pair<std::array<float_t, row_count_train>, std::array<float_t, 1>>;

    constexpr std::array<float_t, element_count_train> x_data = { -2.0, 0.0, -1.0, -1.0, 0.0, -2.0,
                                                                  0.0,  2.0, 1.0,  1.0,  2.0, 0.0 };
    const auto x = homogen_table::wrap(x_data.data(), row_count_train, column_count);

    constexpr std::array<float_t, row_count_train> y_data = {
        -1.0, -1.0, -1.0, 1.0, 1.0, 1.0,
    };
    const auto y = homogen_table::wrap(y_data.data(), row_count_train, 1);

    weights_pair_t weights_data =
        GENERATE_COPY(weights_pair_t({ 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 }, { 0.0 }),
                      weights_pair_t({ 10.0, 0.1, 0.1, 0.1, 0.1, 10.0 }, { -0.44 }),
                      weights_pair_t({ 0.1, 0.1, 10.0, 10.0, 0.1, 0.1 }, { 0.44 }));

    const auto weights = homogen_table::wrap(weights_data.first.data(), row_count_train, 1);

    constexpr std::array<float_t, 2> x_test_data = { -1.0, 1.0 };
    const auto x_test = homogen_table::wrap(x_test_data.data(), 1, column_count);

    const double scale = 1.0;
    const double c = 1e-1;

    const auto kernel_desc = kernel_t{}.set_scale(scale);
    const auto svm_desc =
        svm::descriptor<float_t, method_t, svm::task::classification, kernel_t>{}.set_c(c);

    constexpr std::int64_t support_vector_count = 6;

    constexpr std::array<float_t, support_vector_count> support_indices_data = { 0, 1, 2, 3, 4, 5 };
    const auto support_indices =
        homogen_table::wrap(support_indices_data.data(), support_vector_count, 1);

    const auto decision_function = homogen_table::wrap(weights_data.second.data(), 1, 1);

    this->check_weights(x,
                        y,
                        weights,
                        svm_desc,
                        support_vector_count,
                        support_indices,
                        x_test,
                        decision_function);
}

} // namespace oneapi::dal::svm::test
