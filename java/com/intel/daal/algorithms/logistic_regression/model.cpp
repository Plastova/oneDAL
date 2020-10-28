/* file: model.cpp */
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

/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>

#include "daal.h"
#include "com_intel_daal_algorithms_logistic_regression_Model.h"

using namespace daal;
using namespace daal::algorithms;
using namespace daal::data_management;

/*
 * Class:     com_intel_daal_algorithms_logistic_regression_Model
 * Method:    cGetBeta
 * Signature:(J)J
 */
JNIEXPORT jlong JNICALL Java_com_intel_daal_algorithms_logistic_1regression_Model_cGetBeta(JNIEnv * env, jobject thisObj, jlong modAddr)
{
    NumericTablePtr * const nt                    = new NumericTablePtr();
    algorithms::logistic_regression::ModelPtr res = *(algorithms::logistic_regression::ModelPtr *)modAddr;

    *nt = res->getBeta();

    return (jlong)nt;
}

/*
 * Class:     com_intel_daal_algorithms_logistic_regression_Model
 * Method:    cGetNumberOfBetas
 * Signature:(J)J
 */
JNIEXPORT jlong JNICALL Java_com_intel_daal_algorithms_logistic_1regression_Model_cGetNumberOfBetas(JNIEnv * env, jobject thisObj, jlong modAddr)
{
    algorithms::logistic_regression::ModelPtr res = *(algorithms::logistic_regression::ModelPtr *)modAddr;

    return (jlong)res->getNumberOfBetas();
}