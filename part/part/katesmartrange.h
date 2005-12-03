/* This file is part of the KDE libraries
   Copyright (C) 2003,2004,2005 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATESUPERRANGE_H
#define KATESUPERRANGE_H

#include "katesmartcursor.h"
#include <ktexteditor/smartrange.h>
#include <ktexteditor/rangefeedback.h>
#include "kateedit.h"

class KateSmartRange;
class KateDynamicAnimation;

/**
 * Internal Implementation of KTextEditor::SmartRangeNotifier.
 */
class KateSmartRangeNotifier : public KTextEditor::SmartRangeNotifier
{
  Q_OBJECT
  friend class KateSmartRange;

  public:
    KateSmartRangeNotifier(KateSmartRange* owner);

    /**
     * Implementation detail. Returns whether the positionChanged() signal
     * needs to be emitted, as it is a relatively expensive signal to emit.
     */
    bool needsPositionChanges() const;

    bool connectedInternally() const;
    void setConnectedInternally();

  protected:
    virtual void connectNotify(const char* signal);
    virtual void disconnectNotify(const char* signal);

  private:
    KateSmartRange* m_owner;
    bool m_connectedInternally;
};

class KateSmartRangePtr;

/**
 * Internal implementation of KTextEditor::SmartRange.
 * Represents a range of text, from the start() to the end().
 *
 * Also tracks its position and emits useful signals.
 */
class KateSmartRange : public KTextEditor::SmartRange
{
  friend class KateSmartRangeNotifier;

  public:
    /**
     * Constructors.  Take posession of @p start and @p end.
     */
    KateSmartRange(const KTextEditor::Range& range, KateDocument* doc, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviours insertBehaviour = DoNotExpand);
    /// overload
    KateSmartRange(KateDocument* doc, KTextEditor::SmartRange* parent = 0L);

    KateSmartRange(KateSmartCursor* start, KateSmartCursor* end, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviours insertBehaviour = DoNotExpand);
    virtual ~KateSmartRange();

    KateDocument* kateDocument() const;
    KateSmartCursor& kStart() { return *static_cast<KateSmartCursor*>(m_start); }
    KateSmartCursor& kEnd() { return *static_cast<KateSmartCursor*>(m_end); }

    bool isInternal() const { return m_isInternal; }
    void setInternal();

    inline bool isMouseOver() { return m_mouseOver; }
    inline void setMouseOver(bool mouseOver) { m_mouseOver = mouseOver; }

    inline bool isCaretOver() { return m_caretOver; }
    inline void setCaretOver(bool caretOver) { m_caretOver = caretOver; }

    void unbindAndDelete();

    enum AttachActions {
      NoActions   = 0x0,
      TagLines    = 0x1,
      Redraw      = 0x2
    };

    virtual bool hasNotifier() const;
    virtual KTextEditor::SmartRangeNotifier* notifier();
    virtual void deleteNotifier();
    virtual KTextEditor::SmartRangeWatcher* watcher() const;
    virtual void setWatcher(KTextEditor::SmartRangeWatcher* watcher);

    virtual void setParentRange(SmartRange* r);

    inline bool hasDynamic() { return m_dynamic.count(); }
    const QList<KateDynamicAnimation*>& dynamicAnimations() const;
    void addDynamic(KateDynamicAnimation* anim);
    void removeDynamic(KateDynamicAnimation* anim);

    /**
     * Implementation detail. Defines the level of feedback required for any connected
     * watcher / notifier.
     *
    enum FeedbackLevel {
      /// Don't provide any feedback.
      NoFeedback,
      /// Only provide feedback when the range in question is the most specific, wholly encompassing range to have been changed.
      MostSpecificContentChanged,
      /// Provide feedback whenever the contents of a range change.
      ContentChanged,
      /// Provide feedback whenever the position of a range changes.
      PositionChanged
    };
    Q_DECLARE_FLAGS(FeedbackLevels, FeedbackLevel);*/

    bool feedbackEnabled() const { return m_notifier || m_watcher; }
    // request is internal!! Only KateSmartGroup gets to set it to false.
    /*void setFeedbackLevel(int feedbackLevel, bool request = true);*/

    /// One or both of the cursors has been changed.
    void translated(const KateEditInfo& edit);
    void feedbackMostSpecific(KateSmartRange* mostSpecific);
    /// The range has been shifted only
    void shifted();

    void registerPointer(KateSmartRangePtr* ptr);
    void deregisterPointer(KateSmartRangePtr* ptr);

    inline KateSmartRange& operator=(const KTextEditor::Range& r) { setRange(r); return *this; }

  protected:
    virtual void checkFeedback();

  private:
    void init();

    KateSmartRangeNotifier* m_notifier;
    KTextEditor::SmartRangeWatcher* m_watcher;
    KateDynamicAnimation* m_dynamicDoc;
    QList<KateDynamicAnimation*> m_dynamic;
    QList<KateSmartRangePtr*> m_pointers;

    bool  m_mouseOver   :1,
          m_caretOver   :1,
          m_isInternal  :1;
};

/**
 * Used for internal references to external KateSmartRanges
 */
class KateSmartRangePtr
{
  public:
    KateSmartRangePtr(KateSmartRange* range)
      : m_range(range)
    {
      if (m_range)
        m_range->registerPointer(this);
    }

    ~KateSmartRangePtr()
    {
      if (m_range)
        m_range->deregisterPointer(this);
    }

    void deleted()
    {
      m_range = 0L;
    }

    inline KateSmartRangePtr& operator= ( const KateSmartRangePtr& p )
    {
      if (m_range)
        m_range->deregisterPointer(this);

      m_range = p.m_range;

      if (m_range)
        m_range->registerPointer(this);

      return *this;
    }

    inline KateSmartRangePtr& operator= ( KateSmartRange* p )
    {
      if (m_range)
        m_range->deregisterPointer(this);

      m_range = p;

      if (m_range)
        m_range->registerPointer(this);

      return *this;
    }

    inline bool operator== ( const KateSmartRangePtr& p ) const { return m_range == p.m_range; }
    inline bool operator!= ( const KateSmartRangePtr& p ) const { return m_range != p.m_range; }
    inline bool operator== ( const KateSmartRange* p ) const { return m_range == p; }
    inline bool operator!= ( const KateSmartRange* p ) const { return m_range != p; }
    inline KateSmartRange* operator->() const { return m_range; }
    inline operator KateSmartRange*() const { return m_range; }

  private:
    KateSmartRange* m_range;
};

//Q_DECLARE_OPERATORS_FOR_FLAGS(KateSmartRange::FeedbackLevels);

#endif
