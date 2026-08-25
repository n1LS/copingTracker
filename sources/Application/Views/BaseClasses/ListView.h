/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _LIST_VIEW_H_
#define _LIST_VIEW_H_

#include "Foundation/Types/Colors.h"
#include "ScreenView.h"
#include <cstddef>

#define LIST_VIEW_LINE_LENGTH (SCREEN_WIDTH - 3)

/**
 * Generic scrollable list view with data source and delegate pattern.
 *
 * Protocol design (callbacks not polling):
 * - DataSource: queried for item count, asked to draw each item
 * - Delegate: notified when user interacts (selection, navigation)
 */
class ListView : public ScreenView {
public:
  /**
   * DataSource protocol: provides data and rendering for list items. Implementers may override DrawItem() for custom
   * appearance.
   */
  class DataSource {
  public:
    virtual ~DataSource() = default;

    virtual size_t GetItemCount() const = 0;

    /**
     * Prepare item for drawing. Called before DrawItem(). Sets display properties (colors, text content).
     * Default stores these in the buffer and sets fg/bg. Override if you need custom preparation logic.
     */
    virtual void PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer,
                                    size_t bufferSize) = 0;

    /**
     * Draw a single item at screen position (x, y). Called by ListView after PrepareItemDrawing().
     * Default implementation draws the buffer text with prepared colors. Override for completely custom item rendering
     * (icons, multi-line, etc).
     */
    virtual void DrawItem(int x, int y, int index, bool isSelected, Color fg, Color bg, const char *buffer) = 0;

    virtual const char *GetEmptyStateMessage() const = 0;
  };

  /**
   * Delegate protocol: notified of user interactions.
   */
  class Delegate {
  public:
    virtual ~Delegate() = default;

    virtual void OnItemSelected(int index, int selectedTab = 0) {
    }
    virtual void OnItemNavigated(int index) {
    }
    virtual void OnItemEdit(int index, int selectedTab = 0) {
    }
  };

  ListView(GUIWindow &w, ViewData *viewData, DataSource *dataSource, Delegate *delegate, size_t pageSize,
           int initialTab = 0);
  virtual ~ListView();

  void ProcessButtonMask(uint16_t mask, bool pressed) override;
  void DrawView() override;
  void OnFocus() override;

protected:
  DataSource *GetDataSource() const {
    return dataSource_;
  }
  Delegate *GetDelegate() const {
    return delegate_;
  }

  size_t GetCurrentIndex() const {
    return currentIndex_;
  }
  void SetCurrentIndex(size_t index);

  size_t GetTopIndex() const {
    return topIndex_;
  }
  size_t GetPageSize() const {
    return pageSize_;
  }
  size_t GetItemCount() const {
    return dataSource_->GetItemCount();
  }

  int GetSelectedTab() const {
    return selectedTab_;
  }
  void SetSelectedTab(int index) {
    selectedTab_ = index;
    isDirty_ = true;
  }

  void HandleUp(bool page);
  void HandleDown(bool page);
  void HandleEnter();
  void HandleEdit();

  void EnsureVisible();
  void DrawListItems();
  void DrawScrollBar();
  void DrawEmptyState();

  DataSource *dataSource_;
  Delegate *delegate_;

  size_t pageSize_;
  size_t topIndex_ = 0;
  size_t currentIndex_ = 0;
  int selectedTab_ = 0;
};

#endif // _LIST_VIEW_H_
