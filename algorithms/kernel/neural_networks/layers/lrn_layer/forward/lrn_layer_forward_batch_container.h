/* file: lrn_layer_forward_batch_container.h */
/*******************************************************************************
* Copyright 2014-2020 Intel Corporation
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

/*
//++
//  Implementation of the forward local response normalization layer.
//--
*/

#ifndef __LRN_LAYER_FORWARD_BATCH_CONTAINER_H__
#define __LRN_LAYER_FORWARD_BATCH_CONTAINER_H__

#include "neural_networks/layers/lrn/lrn_layer.h"
#include "lrn_layer_forward_kernel.h"

namespace daal
{
namespace algorithms
{
namespace neural_networks
{
namespace layers
{
namespace lrn
{
namespace forward
{
namespace interface1
{
template<typename algorithmFPType, Method method, CpuType cpu>
BatchContainer<algorithmFPType, method, cpu>::BatchContainer(daal::services::Environment::env *daalEnv)
{
    __DAAL_INITIALIZE_KERNELS(internal::LRNKernel, algorithmFPType, method);
}

template<typename algorithmFPType, Method method, CpuType cpu>
BatchContainer<algorithmFPType, method, cpu>::~BatchContainer()
{
    __DAAL_DEINITIALIZE_KERNELS();
}

template<typename algorithmFPType, Method method, CpuType cpu>
services::Status BatchContainer<algorithmFPType, method, cpu>::compute()
{
    lrn::forward::Input *input = static_cast<lrn::forward::Input *>(_in);
    lrn::forward::Result *result = static_cast<lrn::forward::Result *>(_res);

    lrn::Parameter *parameter = static_cast<lrn::Parameter *>(_par);
    daal::services::Environment::env &env = *_env;

    Tensor *inputTensor      = input->get(layers::forward::data).get();
    Tensor *sMinusBetaTensor = result->get(lrn::auxSmBeta).get();
    Tensor *resultTensor     = result->get(layers::forward::value).get();

    __DAAL_CALL_KERNEL(env, internal::LRNKernel, __DAAL_KERNEL_ARGUMENTS(algorithmFPType, method), compute, *inputTensor, *parameter,
                       *sMinusBetaTensor, *resultTensor);
}
} // namespace interface1
} // namespace forward

} // namespace lrn
} // namespace layers
} // namespace neural_networks
} // namespace algorithms
} // namespace daal

#endif