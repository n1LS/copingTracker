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

#ifndef _DELETE_PROJECT_CONFIRM_MODAL_H_
#define _DELETE_PROJECT_CONFIRM_MODAL_H_

#include "ModalView.h"
#include "Application/Persistency/PersistenceConstants.h"
#include <etl/string.h>

/**
 * DeleteProjectConfirmModal - Modal dialog for confirming project deletion
 * 
 * Shows a progress bar when ALT+PLAY+EDIT are held to confirm deletion.
 */
class DeleteProjectConfirmModal : public ModalView {
public:
    static DeleteProjectConfirmModal* Create(View& view, const char* projectName);
    virtual ~DeleteProjectConfirmModal();
    
    virtual void Destroy() override;
    virtual void DrawView() override;
    virtual void OnPlayerUpdate(PlayerEventType, unsigned int) override {}
    virtual void OnFocus() override {}
    virtual void ProcessButtonMask(uint16_t mask, bool pressed) override;
    virtual void AnimationUpdate() override;

private:
    DeleteProjectConfirmModal(View& view, const char* projectName);
    void UpdateProgress_();
    
    static bool inUse_;
    static void* storage_;
    
    etl::string<MAX_PROJECT_NAME_LENGTH + 12> projectLine_;
    uint16_t currentMask_ = 0;
    unsigned long holdStartMs_ = 0;
    uint16_t holdProgressMs_ = 0;
    bool holdingCombo_ = false;
};

#endif // _DELETE_PROJECT_CONFIRM_MODAL_H_