/* Copyright 2023 Google Inc. All Rights Reserved.

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

#ifndef TENSORFLOW_SERVING_SERVABLES_TENSORFLOW_SERVABLE_H_
#define TENSORFLOW_SERVING_SERVABLES_TENSORFLOW_SERVABLE_H_

#include <stdint.h>

#include <string>

#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "tensorflow_serving/apis/classification.pb.h"
#include "tensorflow_serving/apis/get_model_metadata.pb.h"
#include "tensorflow_serving/apis/inference.pb.h"
#include "tensorflow_serving/apis/predict.pb.h"
#include "tensorflow_serving/apis/regression.pb.h"
#include "tensorflow_serving/servables/tensorflow/predict_response_tensor_serialization_option.h"

namespace tensorflow {
namespace serving {

inline constexpr absl::string_view kSignatureDef = "signature_def";

// Provides a `PredictionService`-like interface. All concrete implementations
// are expected to be thread-safe.
class Servable {
 public:
  Servable(absl::string_view name, int64_t version)
      : name_(std::string(name)), version_(version) {}

  virtual ~Servable() = default;

  // Returns the name associated with this servable.
  absl::string_view name() const { return name_; }

  // Returns the version associated with this servable.
  int64_t version() const { return version_; }

  // RunOptions group the configuration for individual inference executions.
  // The per-request configuration (e.g. deadline) can be passed here.
  struct RunOptions {
    // Priority of the request. Some thread pool implementation will schedule
    // ops based on the priority number. Larger number means higher
    // priority.
    int64_t priority = 1;
    // The deadline for this request.
    absl::Time deadline = absl::InfiniteFuture();
  };

  virtual absl::Status Classify(const RunOptions& run_options,
                                const ClassificationRequest& request,
                                ClassificationResponse* response) = 0;

  virtual absl::Status Regress(const RunOptions& run_options,
                               const RegressionRequest& request,
                               RegressionResponse* response) = 0;

  virtual absl::Status Predict(const RunOptions& run_options,
                               const PredictRequest& request,
                               PredictResponse* response) = 0;

  // Streamed version of `Predict`. Experimental API that is not yet part of the
  // PredictionService API.
  //
  // `response_callback` is called for each streamed output, zero or more times,
  // when the streamed output becomes available. The callback invocation is
  // serialized by the runtime, which means that `response_callback` does not
  // have to be thread-safe, but blocking inside the callback causes the next
  // callback invocation to be delayed. The implementation guarantees that the
  // callback is never called after the `PredictStreamed` method returns.
  virtual absl::Status PredictStreamed(
      const RunOptions& run_options, const PredictRequest& request,
      absl::AnyInvocable<void(PredictResponse)> response_callback) = 0;

  virtual absl::Status MultiInference(const RunOptions& run_options,
                                      const MultiInferenceRequest& request,
                                      MultiInferenceResponse* response) = 0;

  virtual absl::Status GetModelMetadata(const GetModelMetadataRequest& request,
                                        GetModelMetadataResponse* response) = 0;

 private:
  // Metadata of this servable. Currently matches the fields in
  // `ServableId`.
  const std::string name_;
  const int64_t version_;
};

// An "empty" servable where there's no model associated with the servable. All
// methods will return an error.
//
// Empty servables can be used in places where a servable is expected but we
// don't need to load any models. For example, Model Server currently expects
// each task to have at least one servable loaded, but Pathways Serving requires
// only the controller task to initiate loading servables. So we use empty
// servables in non-zero tasks to make sure non-zero tasks don't load anything.
class EmptyServable : public Servable {
 public:
  EmptyServable();

  absl::Status Classify(const RunOptions& run_options,
                        const ClassificationRequest& request,
                        ClassificationResponse* response) override {
    return error_;
  }

  absl::Status Regress(const RunOptions& run_options,
                       const RegressionRequest& request,
                       RegressionResponse* response) override {
    return error_;
  }

  absl::Status Predict(const RunOptions& run_options,
                       const PredictRequest& request,
                       PredictResponse* response) override {
    return error_;
  }

  absl::Status PredictStreamed(
      const RunOptions& run_options, const PredictRequest& request,
      absl::AnyInvocable<void(PredictResponse)> response_callback) override {
    return error_;
  }

  absl::Status MultiInference(const RunOptions& run_options,
                              const MultiInferenceRequest& request,
                              MultiInferenceResponse* response) override {
    return error_;
  }

  absl::Status GetModelMetadata(const GetModelMetadataRequest& request,
                                GetModelMetadataResponse* response) override {
    return error_;
  }

 private:
  absl::Status error_;
};

}  // namespace serving
}  // namespace tensorflow

#endif  // TENSORFLOW_SERVING_SERVABLES_TENSORFLOW_SERVABLE_H_
