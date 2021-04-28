// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>

#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/platform";

PlatformChannel::PlatformChannel(flutter::BinaryMessenger* messenger)
    : channel_(std::make_unique<flutter::MethodChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &flutter::JsonMethodCodec::GetInstance())) {
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
    Clipboard::GetData(call, std::move(result));
  } else if (method == "Clipboard.setData") {
    Clipboard::SetData(call, std::move(result));
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

// Clipboard constants and variables
namespace Clipboard {

// naive implementation using std::string as a container of internal clipboard
// data
std::string string_clipboard = "";

static constexpr char kTextKey[] = "text";
static constexpr char kTextPlainFormat[] = "text/plain";
static constexpr char kUnknownClipboardFormatError[] =
    "Unknown clipboard format error";
static constexpr char kUnknownClipboardError[] =
    "Unknown error during clipboard data retrieval";

void GetData(
    const flutter::MethodCall<rapidjson::Document>& call,
    std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
  const rapidjson::Value& format = call.arguments()[0];

  // https://api.flutter.dev/flutter/services/Clipboard/kTextPlain-constant.html
  // API supports only kTextPlain format
  if (strcmp(format.GetString(), kTextPlainFormat) != 0) {
    result->Error(kUnknownClipboardFormatError,
                  "Clipboard API only supports text.");
    return;
  }

  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
  document.AddMember(rapidjson::Value(kTextKey, allocator),
                      rapidjson::Value(string_clipboard, allocator),
                      allocator);
  result->Success(document);
}

void SetData(
    const flutter::MethodCall<rapidjson::Document>& call,
    std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
  const rapidjson::Value& document = *call.arguments();
  rapidjson::Value::ConstMemberIterator itr = document.FindMember(kTextKey);
  if (itr == document.MemberEnd()) {
    result->Error(kUnknownClipboardError, "Invalid message format");
    return;
  }
  string_clipboard = itr->value.GetString();
  result->Success();
}
}  // namespace Clipboard
