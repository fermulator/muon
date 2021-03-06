// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_permission_helper.h"

#include <string>

#include "brave/browser/brave_permission_manager.h"
#include "brightray/browser/media/media_stream_devices_controller.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::WebContentsPermissionHelper);

namespace atom {

namespace {

void MediaAccessAllowed(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback,
    bool allowed) {
  // TODO(bridiver) - IOThread >>
  brightray::MediaStreamDevicesController controller(request, callback);
  if (allowed)
    controller.TakeAction();
  else
    controller.Deny(content::MEDIA_DEVICE_PERMISSION_DENIED);
}

void OnPointerLockResponse(content::WebContents* web_contents, bool allowed) {
  if (web_contents)
    web_contents->GotResponseToLockMouseRequest(allowed);
}

void OnPermissionResponse(const base::Callback<void(bool)>& callback,
                          blink::mojom::PermissionStatus status) {
  if (status == blink::mojom::PermissionStatus::GRANTED)
    callback.Run(true);
  else
    callback.Run(false);
}

}  // namespace

WebContentsPermissionHelper::WebContentsPermissionHelper(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

WebContentsPermissionHelper::~WebContentsPermissionHelper() {
}

void WebContentsPermissionHelper::RequestPermission(
    content::PermissionType permission,
    const base::Callback<void(bool)>& callback,
    const GURL& security_origin, bool user_gesture) {
  auto rfh = web_contents_->GetMainFrame();
  auto permission_manager = static_cast<brave::BravePermissionManager*>(
      web_contents_->GetBrowserContext()->GetPermissionManager());
  GURL origin;
  if (security_origin.is_empty()) {
    origin = web_contents_->GetLastCommittedURL();
  } else {
    origin = security_origin;
  }
  permission_manager->RequestPermission(
      permission, rfh, origin, user_gesture,
      base::Bind(&OnPermissionResponse, callback));
}

void WebContentsPermissionHelper::RequestFullscreenPermission(
    const base::Callback<void(bool)>& callback) {
  RequestPermission((content::PermissionType)(PermissionType::FULLSCREEN),
                    callback);
}

void WebContentsPermissionHelper::RequestMediaAccessPermission(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& response_callback) {
  auto callback = base::Bind(&MediaAccessAllowed, request, response_callback);
  // The permission type doesn't matter here, AUDIO_CAPTURE/VIDEO_CAPTURE
  // are presented as same type in content_converter.h.
  RequestPermission(content::PermissionType::AUDIO_CAPTURE, callback,
                    request.security_origin);
}

void WebContentsPermissionHelper::RequestWebNotificationPermission(
    const base::Callback<void(bool)>& callback) {
  RequestPermission(content::PermissionType::NOTIFICATIONS, callback);
}

void WebContentsPermissionHelper::RequestPointerLockPermission(
    bool user_gesture) {
  RequestPermission((content::PermissionType)(PermissionType::POINTER_LOCK),
                    base::Bind(&OnPointerLockResponse, web_contents_),
                    GURL(),
                    user_gesture);
}

void WebContentsPermissionHelper::RequestOpenExternalPermission(
    const base::Callback<void(bool)>& callback,
    bool user_gesture) {
  RequestPermission((content::PermissionType)(PermissionType::OPEN_EXTERNAL),
                    callback,
                    GURL(),
                    user_gesture);
}

void WebContentsPermissionHelper::RequestProtocolRegistrationPermission(
    const base::Callback<void(bool)>& callback,
    bool user_gesture) {
  RequestPermission((content::PermissionType)
      (PermissionType::PROTOCOL_REGISTRATION),
                    callback,
                    GURL(),
                    user_gesture);
}

}  // namespace atom
