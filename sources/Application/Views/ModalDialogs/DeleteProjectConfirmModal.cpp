/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "DeleteProjectConfirmModal.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Utils/DrawUtils.h"
#include "Application/AppWindow.h"
#include "System/System/System.h"
#include <cstring>

#define DELETE_HOLD_MASK (BM_ALT | BM_PLAY | BM_EDIT)
#define DELETE_HOLD_DURATION_MS 2000

bool DeleteProjectConfirmModal::inUse_ = false;
alignas(DeleteProjectConfirmModal) static unsigned char DeleteProjectConfirmModalStorage[sizeof(DeleteProjectConfirmModal)];
void* DeleteProjectConfirmModal::storage_ = DeleteProjectConfirmModalStorage;

DeleteProjectConfirmModal* DeleteProjectConfirmModal::Create(View& view, const char* name) {
    if (inUse_) {
        auto* existing = reinterpret_cast<DeleteProjectConfirmModal*>(storage_);
        existing->~DeleteProjectConfirmModal();
        inUse_ = false;
    }
    inUse_ = true;
    return new (storage_) DeleteProjectConfirmModal(view, name);
}

DeleteProjectConfirmModal::DeleteProjectConfirmModal(View& view, const char* projectName)
    : ModalView(view), projectLine_("Delete \"") {
    projectLine_.append(projectName);
    projectLine_.append("\"");
}

DeleteProjectConfirmModal::~DeleteProjectConfirmModal() {
}

void DeleteProjectConfirmModal::Destroy() {
    this->~DeleteProjectConfirmModal();
    inUse_ = false;
}

void DeleteProjectConfirmModal::UpdateProgress_() {
    const bool comboHeld = (currentMask_ & DELETE_HOLD_MASK) == DELETE_HOLD_MASK;
    const unsigned long now = System::GetInstance()->GetClock();

    if (!comboHeld) {
        if (holdingCombo_ || (holdProgressMs_ != 0)) {
            holdingCombo_ = false;
            holdProgressMs_ = 0;
            isDirty_ = true;
            static_cast<AppWindow&>(w_).SetDirty();
        }
        return;
    }

    if (!holdingCombo_) {
        holdingCombo_ = true;
        holdStartMs_ = now;
        holdProgressMs_ = 0;
        isDirty_ = true;
        static_cast<AppWindow&>(w_).SetDirty();
        return;
    }

    unsigned long elapsed = now - holdStartMs_;
    if (elapsed > DELETE_HOLD_DURATION_MS) {
        elapsed = DELETE_HOLD_DURATION_MS;
    }

    if (holdProgressMs_ != elapsed) {
        holdProgressMs_ = elapsed;
        isDirty_ = true;
        static_cast<AppWindow&>(w_).SetDirty();
    }

    if (holdProgressMs_ >= DELETE_HOLD_DURATION_MS) {
        EndModal(MBL_YES);
    }
}

void DeleteProjectConfirmModal::AnimationUpdate() {
    UpdateProgress_();
}

void DeleteProjectConfirmModal::ProcessButtonMask(uint16_t mask, bool pressed) {
    currentMask_ = mask;

    if (pressed && (mask & BM_ENTER)) {
        EndModal(MBL_CANCEL);
        return;
    }

    UpdateProgress_();
}

void DeleteProjectConfirmModal::DrawView() {
    SetWindow(26, 6);

    SetColor(Theme::View::fg);

    const int projectLineX = (26 - projectLine_.size()) / 2;
    DrawString(projectLineX, 0, projectLine_.c_str());
    DrawString(0, 1, "Press & hold ALT+PLAY+EDIT");

    if (holdingCombo_ || holdProgressMs_ > 0) {
        progressBar_t progressBar;
        fillProgressBar(holdProgressMs_, DELETE_HOLD_DURATION_MS, &progressBar);
        DrawString((26 - 12) / 2, 3, progressBar);
    }

    const char* cancelButton = "Cancel";
    SetColor(Theme::Button::fg(false));
    DrawString((26 - strlen(cancelButton)) / 2, 5, cancelButton);
}