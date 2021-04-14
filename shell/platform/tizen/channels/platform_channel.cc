// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <mutex>
#include <app.h>
#include <cbhm.h>

#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/platform";

// Clipboard.getData constants and variables
std::mutex is_processing_mutex;
static bool is_processing = false;
static constexpr char kTextKey[] = "text";
static constexpr char kTextPlainFormat[] = "text/plain";
static constexpr char kUnknownClipboardFormatError[] =
    "Unknown clipboard format error";
static constexpr char kUnknownClipboardError[] =
    "Unknown error during clipboard data retrieval";

PlatformChannel::PlatformChannel(flutter::BinaryMessenger* messenger)
    : channel_(std::make_unique<flutter::MethodChannel<rapidjson::Document>>(
          messenger, kChannelName, &flutter::JsonMethodCodec::GetInstance())) {
  channel_->SetMethodCallHandler(
      [this](
          const flutter::MethodCall<rapidjson::Document>& call,
          std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

PlatformChannel::~PlatformChannel() {}

void PlatformChannel::HandleMethodCall(
    const flutter::MethodCall<rapidjson::Document>& call,
    std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
  const auto method = call.method_name();

  if (method == "SystemNavigator.pop") {
    ui_app_exit();
    result->Success();
  } else if (method == "SystemSound.play") {
    result->NotImplemented();
  } else if (method == "HapticFeedback.vibrate") {
    result->NotImplemented();
  } else if (method == "Clipboard.getData") {
    const rapidjson::Value& format = call.arguments()[0];

    // https://api.flutter.dev/flutter/services/Clipboard/kTextPlain-constant.html
    // API supports only kTextPlain format, however cbhm API supports also other formats
    if (strcmp(format.GetString(), kTextPlainFormat) != 0) {
      result->Error(kUnknownClipboardFormatError,
                    "Clipboard API only supports text.");
      return;
    }

    // Report error on next calls until current will be finished.
    // Native API - cbhm_selection_get works on static struct, so accessing clipboard parallelly will end
    // with race regarding returning values - cbhm_selection_data_cb will be triggered only for latest call.
    // TODO consider some queuing mechnism instead of returning error for next calls
    {
      std::lock_guard<std::mutex> lock(is_processing_mutex);
      if (is_processing) {
        result->Error(kUnknownClipboardError, "Already processing by other thread.");
        return;
      }
      is_processing = true;
    }

    cbhm_sel_type_e selection_type = CBHM_SEL_TYPE_TEXT;

    cbhm_h cbhm_handle = nullptr;
    int ret = cbhm_open_service (&cbhm_handle);
    if (CBHM_ERROR_NONE != ret) {
      result->Error(kUnknownClipboardError, "Failed to initialize cbhm service.");
      return;
    }

    // additional check if data in clipboard
    ret = cbhm_item_count_get(cbhm_handle);
    if (ret <= 0) {
      result->Error(kUnknownClipboardError, "No clipboard data available.");
      // release the data
      cbhm_close_service (cbhm_handle);
      // unlock guard for further processing
      std::lock_guard<std::mutex> lock(is_processing_mutex);
      is_processing = false;
      return;
    }

    struct method_data_t {
      std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result;
      cbhm_h cbhm_handle;
    };
    // invalidates the result pointer
    method_data_t* data = new method_data_t{};
    data->result = std::move(result);
    data->cbhm_handle = cbhm_handle;

    auto cbhm_selection_data_cb = [](cbhm_h cbhm_handle, const char *buf, size_t len, void *user_data) -> int {
      auto data = static_cast<method_data_t*>(user_data);
      // move unique_ptr from method_data_t and then release memory
      auto result = std::move(data->result);
      cbhm_close_service (data->cbhm_handle);
      delete data;

      FT_LOGD("cbhm_selection_get SUCCESS (%d) %s", len, buf);
      {
        std::lock_guard<std::mutex> lock(is_processing_mutex);
        is_processing = false;
      }
      if (buf) {
        rapidjson::Document document;
        document.SetObject();
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
        document.AddMember(rapidjson::Value(kTextKey, allocator),
                          rapidjson::Value(std::string{buf, len}, allocator), allocator);
        result->Success(document);
        return CBHM_ERROR_NONE;
      } else {
        result->Error(kUnknownClipboardError, "Data buffer is null.");
        return CBHM_ERROR_NO_DATA;
      }
    };

    FT_LOGD("cbhm_selection_get call");
    ret = cbhm_selection_get(cbhm_handle, selection_type, cbhm_selection_data_cb, data);
    if (CBHM_ERROR_NONE != ret) {
      FT_LOGD("cbhm_selection_get error");
      // return error
      data->result->Error(kUnknownClipboardError, "Failed to gather data.");
      // release the data
      cbhm_close_service (data->cbhm_handle);
      delete data;
      // unlock guard for further processing
      std::lock_guard<std::mutex> lock(is_processing_mutex);
      is_processing = false;
      return;
    }
  } else if (method == "Clipboard.setData") {
    result->NotImplemented();
  } else if (method == "Clipboard.hasStrings") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setPreferredOrientations") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setApplicationSwitcherDescription") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setEnabledSystemUIOverlays") {
    result->NotImplemented();
  } else if (method == "SystemChrome.restoreSystemUIOverlays") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setSystemUIOverlayStyle") {
    result->NotImplemented();
  } else {
    FT_LOGI("Unimplemented method: %s", method.c_str());
    result->NotImplemented();
  }
}
