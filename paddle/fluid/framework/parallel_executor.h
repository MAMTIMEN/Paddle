/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include <unordered_set>

#include "paddle/fluid/framework/executor.h"
#include "paddle/fluid/framework/op_info.h"
#include "paddle/fluid/framework/program_desc.h"
#include "paddle/fluid/framework/scope.h"
#include "paddle/fluid/framework/tensor.h"

#include "paddle/fluid/operators/nccl/nccl_gpu_common.h"
#include "paddle/fluid/platform/device_context.h"

namespace paddle {
namespace framework {

class ParallelExecutorPrivate;
class VarHandle;
class OpHandle;
class VarHandleBase;

struct GuardedBool {
 public:
  GuardedBool() {}

  operator bool() const {
    std::lock_guard<std::mutex> g(mtx_);
    return value_;
  }

  GuardedBool& operator=(bool o) {
    std::lock_guard<std::mutex> g(mtx_);
    value_ = o;
    return *this;
  }

 private:
  mutable std::mutex mtx_;
  bool value_;
};

class ParallelExecutor {
 public:
  explicit ParallelExecutor(const std::vector<platform::Place>& places,
                            const std::unordered_set<std::string>& params,
                            const ProgramDesc& startup_program,
                            const ProgramDesc& main_program,
                            const std::string& loss_var_name, Scope* scope);

  void Run(const std::vector<std::string>& fetch_tensors,
           const std::string& fetched_var_name = "fetched_var");

 private:
  ParallelExecutorPrivate* member_;

  void BCastParamsToGPUs(const ProgramDesc& startup_program) const;

  VarHandle* GetVarHandle(const std::string& each_var_name,
                          const platform::Place& place) const;

  void GenerateVar(OpHandle* op_handle, const std::string& each_var_name,
                   const platform::Place& place) const;

  void ConstructDependencyGraph(const std::unordered_set<std::string>& params,
                                const ProgramDesc& main_program,
                                const std::string& loss_var_name) const;

  void BuildNCCLCommunicator() const;

  void RunOp(std::unordered_map<VarHandleBase*, GuardedBool>& pending_vars,
             OpHandle* op) const;

  void PolishGraphToSupportDataHarzaeds() const;
};

}  // namespace framework
}  // namespace paddle
