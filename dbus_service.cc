//
// Copyright (C) 2012 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "update_engine/dbus_service.h"

#include <string>
#include <vector>

#include <dlcservice/proto_bindings/dlcservice.pb.h>
#include <update_engine/dbus-constants.h>

#include "update_engine/dbus_connection.h"
#include "update_engine/proto_bindings/update_engine.pb.h"
#include "update_engine/update_status_utils.h"

namespace chromeos_update_engine {

using brillo::ErrorPtr;
using chromeos_update_engine::UpdateEngineService;
using dlcservice::DlcModuleList;
using std::string;
using std::vector;
using update_engine::Operation;
using update_engine::StatusResult;
using update_engine::UpdateEngineStatus;
using update_engine::UpdateStatus;

namespace {
// Converts the internal |UpdateEngineStatus| to the protobuf |StatusResult|.
void ConvertToStatusResult(const UpdateEngineStatus& ue_status,
                           StatusResult* out_status) {
  out_status->set_last_checked_time(ue_status.last_checked_time);
  out_status->set_progress(ue_status.progress);
  out_status->set_current_operation(static_cast<Operation>(ue_status.status));
  out_status->set_new_version(ue_status.new_version);
  out_status->set_new_size(ue_status.new_size_bytes);
  out_status->set_is_enterprise_rollback(ue_status.is_enterprise_rollback);
  out_status->set_is_install(ue_status.is_install);
}
}  // namespace

DBusUpdateEngineService::DBusUpdateEngineService(SystemState* system_state)
    : common_(new UpdateEngineService{system_state}) {}

// org::chromium::UpdateEngineInterfaceInterface methods implementation.

bool DBusUpdateEngineService::AttemptUpdate(ErrorPtr* error,
                                            const string& in_app_version,
                                            const string& in_omaha_url) {
  return AttemptUpdateWithFlags(
      error, in_app_version, in_omaha_url, 0 /* no flags */);
}

bool DBusUpdateEngineService::AttemptUpdateWithFlags(
    ErrorPtr* error,
    const string& in_app_version,
    const string& in_omaha_url,
    int32_t in_flags_as_int) {
  update_engine::AttemptUpdateFlags flags =
      static_cast<update_engine::AttemptUpdateFlags>(in_flags_as_int);
  bool interactive = !(flags & update_engine::kAttemptUpdateFlagNonInteractive);
  bool result;
  return common_->AttemptUpdate(
      error,
      in_app_version,
      in_omaha_url,
      interactive ? 0 : update_engine::UpdateAttemptFlags::kFlagNonInteractive,
      &result);
}

bool DBusUpdateEngineService::AttemptInstall(ErrorPtr* error,
                                             const DlcModuleList& request) {
  vector<string> dlc_ids;
  for (const auto& dlc_module_info : request.dlc_module_infos()) {
    if (dlc_module_info.dlc_id().empty()) {
      *error = brillo::Error::Create(
          FROM_HERE, "update_engine", "INTERNAL", "Empty DLC ID passed.");
      return false;
    }
    dlc_ids.push_back(dlc_module_info.dlc_id());
  }
  return common_->AttemptInstall(error, request.omaha_url(), dlc_ids);
}

bool DBusUpdateEngineService::AttemptRollback(ErrorPtr* error,
                                              bool in_powerwash) {
  return common_->AttemptRollback(error, in_powerwash);
}

bool DBusUpdateEngineService::CanRollback(ErrorPtr* error,
                                          bool* out_can_rollback) {
  return common_->CanRollback(error, out_can_rollback);
}

bool DBusUpdateEngineService::ResetStatus(ErrorPtr* error) {
  return common_->ResetStatus(error);
}

bool DBusUpdateEngineService::GetStatus(ErrorPtr* error,
                                        int64_t* out_last_checked_time,
                                        double* out_progress,
                                        string* out_current_operation,
                                        string* out_new_version,
                                        int64_t* out_new_size) {
  UpdateEngineStatus status;
  if (!common_->GetStatus(error, &status)) {
    return false;
  }
  *out_last_checked_time = status.last_checked_time;
  *out_progress = status.progress;
  *out_current_operation = UpdateStatusToString(status.status);
  *out_new_version = status.new_version;
  *out_new_size = status.new_size_bytes;
  return true;
}

bool DBusUpdateEngineService::GetStatusAdvanced(ErrorPtr* error,
                                                StatusResult* out_status) {
  UpdateEngineStatus status;
  if (!common_->GetStatus(error, &status)) {
    return false;
  }

  ConvertToStatusResult(status, out_status);
  return true;
}

bool DBusUpdateEngineService::RebootIfNeeded(ErrorPtr* error) {
  return common_->RebootIfNeeded(error);
}

bool DBusUpdateEngineService::SetChannel(ErrorPtr* error,
                                         const string& in_target_channel,
                                         bool in_is_powerwash_allowed) {
  return common_->SetChannel(error, in_target_channel, in_is_powerwash_allowed);
}

bool DBusUpdateEngineService::GetChannel(ErrorPtr* error,
                                         bool in_get_current_channel,
                                         string* out_channel) {
  return common_->GetChannel(error, in_get_current_channel, out_channel);
}

bool DBusUpdateEngineService::GetCohortHint(ErrorPtr* error,
                                            string* out_cohort_hint) {
  return common_->GetCohortHint(error, out_cohort_hint);
}

bool DBusUpdateEngineService::SetCohortHint(ErrorPtr* error,
                                            const string& in_cohort_hint) {
  return common_->SetCohortHint(error, in_cohort_hint);
}

bool DBusUpdateEngineService::SetP2PUpdatePermission(ErrorPtr* error,
                                                     bool in_enabled) {
  return common_->SetP2PUpdatePermission(error, in_enabled);
}

bool DBusUpdateEngineService::GetP2PUpdatePermission(ErrorPtr* error,
                                                     bool* out_enabled) {
  return common_->GetP2PUpdatePermission(error, out_enabled);
}

bool DBusUpdateEngineService::SetUpdateOverCellularPermission(ErrorPtr* error,
                                                              bool in_allowed) {
  return common_->SetUpdateOverCellularPermission(error, in_allowed);
}

bool DBusUpdateEngineService::SetUpdateOverCellularTarget(
    brillo::ErrorPtr* error,
    const std::string& target_version,
    int64_t target_size) {
  return common_->SetUpdateOverCellularTarget(
      error, target_version, target_size);
}

bool DBusUpdateEngineService::GetUpdateOverCellularPermission(
    ErrorPtr* error, bool* out_allowed) {
  return common_->GetUpdateOverCellularPermission(error, out_allowed);
}

bool DBusUpdateEngineService::GetDurationSinceUpdate(
    ErrorPtr* error, int64_t* out_usec_wallclock) {
  return common_->GetDurationSinceUpdate(error, out_usec_wallclock);
}

bool DBusUpdateEngineService::GetPrevVersion(ErrorPtr* error,
                                             string* out_prev_version) {
  return common_->GetPrevVersion(error, out_prev_version);
}

bool DBusUpdateEngineService::GetRollbackPartition(
    ErrorPtr* error, string* out_rollback_partition_name) {
  return common_->GetRollbackPartition(error, out_rollback_partition_name);
}

bool DBusUpdateEngineService::GetLastAttemptError(
    ErrorPtr* error, int32_t* out_last_attempt_error) {
  return common_->GetLastAttemptError(error, out_last_attempt_error);
}

bool DBusUpdateEngineService::GetEolStatus(ErrorPtr* error,
                                           int32_t* out_eol_status) {
  return common_->GetEolStatus(error, out_eol_status);
}

UpdateEngineAdaptor::UpdateEngineAdaptor(SystemState* system_state)
    : org::chromium::UpdateEngineInterfaceAdaptor(&dbus_service_),
      bus_(DBusConnection::Get()->GetDBus()),
      dbus_service_(system_state),
      dbus_object_(nullptr,
                   bus_,
                   dbus::ObjectPath(update_engine::kUpdateEngineServicePath)) {}

void UpdateEngineAdaptor::RegisterAsync(
    const base::Callback<void(bool)>& completion_callback) {
  RegisterWithDBusObject(&dbus_object_);
  dbus_object_.RegisterAsync(completion_callback);
}

bool UpdateEngineAdaptor::RequestOwnership() {
  return bus_->RequestOwnershipAndBlock(update_engine::kUpdateEngineServiceName,
                                        dbus::Bus::REQUIRE_PRIMARY);
}

void UpdateEngineAdaptor::SendStatusUpdate(
    const UpdateEngineStatus& update_engine_status) {
  StatusResult status;
  ConvertToStatusResult(update_engine_status, &status);

  // TODO(crbug.com/977320): Deprecate |StatusUpdate| signal.
  SendStatusUpdateSignal(status.last_checked_time(),
                         status.progress(),
                         UpdateStatusToString(static_cast<UpdateStatus>(
                             status.current_operation())),
                         status.new_version(),
                         status.new_size());

  // Send |StatusUpdateAdvanced| signal.
  SendStatusUpdateAdvancedSignal(status);
}

}  // namespace chromeos_update_engine
