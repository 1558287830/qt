// Commit: 1337a3e031477aa4d628d01252557dee622629ff
/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsggridview_p.h"
#include "qsgvisualitemmodel_p.h"
#include "qsgflickable_p_p.h"

#include <private/qdeclarativesmoothedanimation_p_p.h>
#include <private/qlistmodelinterface_p.h>

#include <QtGui/qevent.h>
#include <QtCore/qmath.h>
#include <QtCore/qcoreapplication.h>
#include <math.h>

QT_BEGIN_NAMESPACE

//----------------------------------------------------------------------------

class FxGridItem
{
public:
    FxGridItem(QSGItem *i, QSGGridView *v) : item(i), view(v) {
        attached = static_cast<QSGGridViewAttached*>(qmlAttachedPropertiesObject<QSGGridView>(item));
        if (attached)
            attached->setView(view);
    }
    ~FxGridItem() {}

    qreal rowPos() const { return (view->flow() == QSGGridView::LeftToRight ? item->y() : item->x()); }
    qreal colPos() const { return (view->flow() == QSGGridView::LeftToRight ? item->x() : item->y()); }
    qreal endRowPos() const {
        return view->flow() == QSGGridView::LeftToRight
                                ? item->y() + view->cellHeight() - 1
                                : item->x() + view->cellWidth() - 1;
    }
    void setPosition(qreal col, qreal row) {
        if (view->flow() == QSGGridView::LeftToRight) {
            item->setPos(QPointF(col, row));
        } else {
            item->setPos(QPointF(row, col));
        }
    }
    bool contains(int x, int y) const {
        return (x >= item->x() && x < item->x() + view->cellWidth() &&
                y >= item->y() && y < item->y() + view->cellHeight());
    }

    QSGItem *item;
    QSGGridView *view;
    QSGGridViewAttached *attached;
    int index;
};

//----------------------------------------------------------------------------

class QSGGridViewPrivate : public QSGFlickablePrivate
{
    Q_DECLARE_PUBLIC(QSGGridView)

public:
    QSGGridViewPrivate()
    : currentItem(0), flow(QSGGridView::LeftToRight)
    , visibleIndex(0) , currentIndex(-1)
    , cellWidth(100), cellHeight(100), columns(1), requestedIndex(-1), itemCount(0)
    , highlightRangeStart(0), highlightRangeEnd(0), highlightRange(QSGGridView::NoHighlightRange)
    , highlightComponent(0), highlight(0), trackedItem(0)
    , moveReason(Other), buffer(0), highlightXAnimator(0), highlightYAnimator(0)
    , highlightMoveDuration(150)
    , footerComponent(0), footer(0), headerComponent(0), header(0)
    , bufferMode(BufferBefore | BufferAfter), snapMode(QSGGridView::NoSnap)
    , ownModel(false), wrap(false), autoHighlight(true)
    , fixCurrentVisibility(false), lazyRelease(false), layoutScheduled(false)
    , deferredRelease(false), haveHighlightRange(false), currentIndexCleared(false) {}

    void init();
    void clear();
    FxGridItem *createItem(int modelIndex);
    void releaseItem(FxGridItem *item);
    void refill(qreal from, qreal to, bool doBuffer=false);

    void updateGrid();
    void scheduleLayout();
    void layout();
    void updateUnrequestedIndexes();
    void updateUnrequestedPositions();
    void updateTrackedItem();
    void createHighlight();
    void updateHighlight();
    void updateCurrent(int modelIndex);
    void updateHeader();
    void updateFooter();
    void fixupPosition();

    FxGridItem *visibleItem(int modelIndex) const {
        if (modelIndex >= visibleIndex && modelIndex < visibleIndex + visibleItems.count()) {
            for (int i = modelIndex - visibleIndex; i < visibleItems.count(); ++i) {
                FxGridItem *item = visibleItems.at(i);
                if (item->index == modelIndex)
                    return item;
            }
        }
        return 0;
    }

    qreal position() const {
        Q_Q(const QSGGridView);
        return flow == QSGGridView::LeftToRight ? q->contentY() : q->contentX();
    }
    void setPosition(qreal pos) {
        Q_Q(QSGGridView);
        if (flow == QSGGridView::LeftToRight)
            q->QSGFlickable::setContentY(pos);
        else
            q->QSGFlickable::setContentX(pos);
    }
    int size() const {
        Q_Q(const QSGGridView);
        return flow == QSGGridView::LeftToRight ? q->height() : q->width();
    }
    qreal startPosition() const {
        qreal pos = 0;
        if (!visibleItems.isEmpty())
            pos = visibleItems.first()->rowPos() - visibleIndex / columns * rowSize();
        return pos;
    }

    qreal endPosition() const {
        qreal pos = 0;
        if (model && model->count())
            pos = rowPosAt(model->count() - 1) + rowSize();
        return pos;
    }

    bool isValid() const {
        return model && model->count() && model->isValid();
    }

    int rowSize() const {
        return flow == QSGGridView::LeftToRight ? cellHeight : cellWidth;
    }
    int colSize() const {
        return flow == QSGGridView::LeftToRight ? cellWidth : cellHeight;
    }

    qreal colPosAt(int modelIndex) const {
        if (FxGridItem *item = visibleItem(modelIndex))
            return item->colPos();
        if (!visibleItems.isEmpty()) {
            if (modelIndex < visibleIndex) {
                int count = (visibleIndex - modelIndex) % columns;
                int col = visibleItems.first()->colPos() / colSize();
                col = (columns - count + col) % columns;
                return col * colSize();
            } else {
                int count = columns - 1 - (modelIndex - visibleItems.last()->index - 1) % columns;
                return visibleItems.last()->colPos() - count * colSize();
            }
        } else {
            return (modelIndex % columns) * colSize();
        }
        return 0;
    }
    qreal rowPosAt(int modelIndex) const {
        if (FxGridItem *item = visibleItem(modelIndex))
            return item->rowPos();
        if (!visibleItems.isEmpty()) {
            if (modelIndex < visibleIndex) {
                int firstCol = visibleItems.first()->colPos() / colSize();
                int col = visibleIndex - modelIndex + (columns - firstCol - 1);
                int rows = col / columns;
                return visibleItems.first()->rowPos() - rows * rowSize();
            } else {
                int count = modelIndex - visibleItems.last()->index;
                int col = visibleItems.last()->colPos() + count * colSize();
                int rows = col / (columns * colSize());
                return visibleItems.last()->rowPos() + rows * rowSize();
            }
        } else {
            qreal pos = (modelIndex / columns) * rowSize();
            if (header)
                pos += headerSize();
            return pos;
        }
        return 0;
    }

    FxGridItem *firstVisibleItem() const {
        const qreal pos = position();
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxGridItem *item = visibleItems.at(i);
            if (item->index != -1 && item->endRowPos() > pos)
                return item;
        }
        return visibleItems.count() ? visibleItems.first() : 0;
    }

    int lastVisibleIndex() const {
        int lastIndex = -1;
        for (int i = visibleItems.count()-1; i >= 0; --i) {
            FxGridItem *gridItem = visibleItems.at(i);
            if (gridItem->index != -1) {
                lastIndex = gridItem->index;
                break;
            }
        }
        return lastIndex;
    }

    // Map a model index to visibleItems list index.
    // These may differ if removed items are still present in the visible list,
    // e.g. doing a removal animation
    int mapFromModel(int modelIndex) const {
        if (modelIndex < visibleIndex || modelIndex >= visibleIndex + visibleItems.count())
            return -1;
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxGridItem *listItem = visibleItems.at(i);
            if (listItem->index == modelIndex)
                return i + visibleIndex;
            if (listItem->index > modelIndex)
                return -1;
        }
        return -1; // Not in visibleList
    }

    qreal snapPosAt(qreal pos) const {
        Q_Q(const QSGGridView);
        qreal snapPos = 0;
        if (!visibleItems.isEmpty()) {
            pos += rowSize()/2;
            snapPos = visibleItems.first()->rowPos() - visibleIndex / columns * rowSize();
            snapPos = pos - fmodf(pos - snapPos, qreal(rowSize()));
            qreal maxExtent = flow == QSGGridView::LeftToRight ? -q->maxYExtent() : -q->maxXExtent();
            qreal minExtent = flow == QSGGridView::LeftToRight ? -q->minYExtent() : -q->minXExtent();
            if (snapPos > maxExtent)
                snapPos = maxExtent;
            if (snapPos < minExtent)
                snapPos = minExtent;
        }
        return snapPos;
    }

    FxGridItem *snapItemAt(qreal pos) {
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxGridItem *item = visibleItems[i];
            if (item->index == -1)
                continue;
            qreal itemTop = item->rowPos();
            if (itemTop+rowSize()/2 >= pos && itemTop - rowSize()/2 <= pos)
                return item;
        }
        return 0;
    }

    int snapIndex() {
        int index = currentIndex;
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxGridItem *item = visibleItems[i];
            if (item->index == -1)
                continue;
            qreal itemTop = item->rowPos();
            if (itemTop >= highlight->rowPos()-rowSize()/2 && itemTop < highlight->rowPos()+rowSize()/2) {
                index = item->index;
                if (item->colPos() >= highlight->colPos()-colSize()/2 && item->colPos() < highlight->colPos()+colSize()/2)
                    return item->index;
            }
        }
        return index;
    }

    qreal headerSize() const {
        if (!header)
            return 0.0;

        return flow == QSGGridView::LeftToRight
                       ? header->item->height()
                       : header->item->width();
    }


    virtual void itemGeometryChanged(QSGItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) {
        Q_Q(const QSGGridView);
        QSGFlickablePrivate::itemGeometryChanged(item, newGeometry, oldGeometry);
        if (item == q) {
            if (newGeometry.height() != oldGeometry.height()
                || newGeometry.width() != oldGeometry.width()) {
                if (q->isComponentComplete()) {
                    updateGrid();
                    scheduleLayout();
                }
            }
        } else if ((header && header->item == item) || (footer && footer->item == item)) {
            updateHeader();
            updateFooter();
        }
    }

    virtual void fixup(AxisData &data, qreal minExtent, qreal maxExtent);
    virtual void flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                QDeclarativeTimeLineCallback::Callback fixupCallback, qreal velocity);

    // for debugging only
    void checkVisible() const {
        int skip = 0;
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxGridItem *listItem = visibleItems.at(i);
            if (listItem->index == -1) {
                ++skip;
            } else if (listItem->index != visibleIndex + i - skip) {
                for (int j = 0; j < visibleItems.count(); j++)
                    qDebug() << " index" << j << "item index" << visibleItems.at(j)->index;
                qFatal("index %d %d %d", visibleIndex, i, listItem->index);
            }
        }
    }

    QDeclarativeGuard<QSGVisualModel> model;
    QVariant modelVariant;
    QList<FxGridItem*> visibleItems;
    QHash<QSGItem*,int> unrequestedItems;
    FxGridItem *currentItem;
    QSGGridView::Flow flow;
    int visibleIndex;
    int currentIndex;
    int cellWidth;
    int cellHeight;
    int columns;
    int requestedIndex;
    int itemCount;
    qreal highlightRangeStart;
    qreal highlightRangeEnd;
    QSGGridView::HighlightRangeMode highlightRange;
    QDeclarativeComponent *highlightComponent;
    FxGridItem *highlight;
    FxGridItem *trackedItem;
    enum MovementReason { Other, SetIndex, Mouse };
    MovementReason moveReason;
    int buffer;
    QSmoothedAnimation *highlightXAnimator;
    QSmoothedAnimation *highlightYAnimator;
    int highlightMoveDuration;
    QDeclarativeComponent *footerComponent;
    FxGridItem *footer;
    QDeclarativeComponent *headerComponent;
    FxGridItem *header;
    enum BufferMode { NoBuffer = 0x00, BufferBefore = 0x01, BufferAfter = 0x02 };
    int bufferMode;
    QSGGridView::SnapMode snapMode;

    bool ownModel : 1;
    bool wrap : 1;
    bool autoHighlight : 1;
    bool fixCurrentVisibility : 1;
    bool lazyRelease : 1;
    bool layoutScheduled : 1;
    bool deferredRelease : 1;
    bool haveHighlightRange : 1;
    bool currentIndexCleared : 1;
};

void QSGGridViewPrivate::init()
{
    Q_Q(QSGGridView);
    QObject::connect(q, SIGNAL(movementEnded()), q, SLOT(animStopped()));
    q->setFlag(QSGItem::ItemIsFocusScope);
    q->setFlickableDirection(QSGFlickable::VerticalFlick);
    addItemChangeListener(this, Geometry);
}

void QSGGridViewPrivate::clear()
{
    for (int i = 0; i < visibleItems.count(); ++i)
        releaseItem(visibleItems.at(i));
    visibleItems.clear();
    visibleIndex = 0;
    releaseItem(currentItem);
    currentItem = 0;
    createHighlight();
    trackedItem = 0;
    itemCount = 0;
}

FxGridItem *QSGGridViewPrivate::createItem(int modelIndex)
{
    Q_Q(QSGGridView);
    // create object
    requestedIndex = modelIndex;
    FxGridItem *listItem = 0;
    if (QSGItem *item = model->item(modelIndex, false)) {
        listItem = new FxGridItem(item, q);
        listItem->index = modelIndex;
        if (model->completePending()) {
            // complete
            listItem->item->setZ(1);
            listItem->item->setParentItem(q->contentItem());
            model->completeItem();
        } else {
            listItem->item->setParentItem(q->contentItem());
        }
        unrequestedItems.remove(listItem->item);
    }
    requestedIndex = -1;
    return listItem;
}


void QSGGridViewPrivate::releaseItem(FxGridItem *item)
{
    Q_Q(QSGGridView);
    if (!item || !model)
        return;
    if (trackedItem == item) {
        QObject::disconnect(trackedItem->item, SIGNAL(yChanged()), q, SLOT(trackedPositionChanged()));
        QObject::disconnect(trackedItem->item, SIGNAL(xChanged()), q, SLOT(trackedPositionChanged()));
        trackedItem = 0;
    }
    if (model->release(item->item) == 0) {
        // item was not destroyed, and we no longer reference it.
        unrequestedItems.insert(item->item, model->indexOf(item->item, q));
    }
    delete item;
}

void QSGGridViewPrivate::refill(qreal from, qreal to, bool doBuffer)
{
    Q_Q(QSGGridView);
    if (!isValid() || !q->isComponentComplete())
        return;
    itemCount = model->count();
    qreal bufferFrom = from - buffer;
    qreal bufferTo = to + buffer;
    qreal fillFrom = from;
    qreal fillTo = to;
    if (doBuffer && (bufferMode & BufferAfter))
        fillTo = bufferTo;
    if (doBuffer && (bufferMode & BufferBefore))
        fillFrom = bufferFrom;

    bool changed = false;

    int colPos = colPosAt(visibleIndex);
    int rowPos = rowPosAt(visibleIndex);
    int modelIndex = visibleIndex;
    if (visibleItems.count()) {
        rowPos = visibleItems.last()->rowPos();
        colPos = visibleItems.last()->colPos() + colSize();
        if (colPos > colSize() * (columns-1)) {
            colPos = 0;
            rowPos += rowSize();
        }
        int i = visibleItems.count() - 1;
        while (i > 0 && visibleItems.at(i)->index == -1)
            --i;
        modelIndex = visibleItems.at(i)->index + 1;
    }
    int colNum = colPos / colSize();

    FxGridItem *item = 0;

    // Item creation and release is staggered in order to avoid
    // creating/releasing multiple items in one frame
    // while flicking (as much as possible).
    while (modelIndex < model->count() && rowPos <= fillTo + rowSize()*(columns - colNum)/(columns+1)) {
//        qDebug() << "refill: append item" << modelIndex;
        if (!(item = createItem(modelIndex)))
            break;
        item->setPosition(colPos, rowPos);
        visibleItems.append(item);
        colPos += colSize();
        colNum++;
        if (colPos > colSize() * (columns-1)) {
            colPos = 0;
            colNum = 0;
            rowPos += rowSize();
        }
        ++modelIndex;
        changed = true;
        if (doBuffer) // never buffer more than one item per frame
            break;
    }

    if (visibleItems.count()) {
        rowPos = visibleItems.first()->rowPos();
        colPos = visibleItems.first()->colPos() - colSize();
        if (colPos < 0) {
            colPos = colSize() * (columns - 1);
            rowPos -= rowSize();
        }
    }
    colNum = colPos / colSize();
    while (visibleIndex > 0 && rowPos + rowSize() - 1 >= fillFrom - rowSize()*(colNum+1)/(columns+1)){
//        qDebug() << "refill: prepend item" << visibleIndex-1 << "top pos" << rowPos << colPos;
        if (!(item = createItem(visibleIndex-1)))
            break;
        --visibleIndex;
        item->setPosition(colPos, rowPos);
        visibleItems.prepend(item);
        colPos -= colSize();
        colNum--;
        if (colPos < 0) {
            colPos = colSize() * (columns - 1);
            colNum = columns-1;
            rowPos -= rowSize();
        }
        changed = true;
        if (doBuffer) // never buffer more than one item per frame
            break;
    }

    if (!lazyRelease || !changed || deferredRelease) { // avoid destroying items in the same frame that we create
        while (visibleItems.count() > 1
               && (item = visibleItems.first())
                    && item->endRowPos() < bufferFrom - rowSize()*(item->colPos()/colSize()+1)/(columns+1)) {
            if (item->attached->delayRemove())
                break;
//            qDebug() << "refill: remove first" << visibleIndex << "top end pos" << item->endRowPos();
            if (item->index != -1)
                visibleIndex++;
            visibleItems.removeFirst();
            releaseItem(item);
            changed = true;
        }
        while (visibleItems.count() > 1
               && (item = visibleItems.last())
                    && item->rowPos() > bufferTo + rowSize()*(columns - item->colPos()/colSize())/(columns+1)) {
            if (item->attached->delayRemove())
                break;
//            qDebug() << "refill: remove last" << visibleIndex+visibleItems.count()-1;
            visibleItems.removeLast();
            releaseItem(item);
            changed = true;
        }
        deferredRelease = false;
    } else {
        deferredRelease = true;
    }
    if (changed) {
        if (header)
            updateHeader();
        if (footer)
            updateFooter();
        if (flow == QSGGridView::LeftToRight)
            q->setContentHeight(endPosition() - startPosition());
        else
            q->setContentWidth(endPosition() - startPosition());
    } else if (!doBuffer && buffer && bufferMode != NoBuffer) {
        refill(from, to, true);
    }
    lazyRelease = false;
}

void QSGGridViewPrivate::updateGrid()
{
    Q_Q(QSGGridView);
    columns = (int)qMax((flow == QSGGridView::LeftToRight ? q->width() : q->height()) / colSize(), qreal(1.));
    if (isValid()) {
        if (flow == QSGGridView::LeftToRight)
            q->setContentHeight(endPosition() - startPosition());
        else
            q->setContentWidth(endPosition() - startPosition());
    }
}

void QSGGridViewPrivate::scheduleLayout()
{
    Q_Q(QSGGridView);
    if (!layoutScheduled) {
        layoutScheduled = true;
        q->polish();
    }
}

void QSGGridViewPrivate::layout()
{
    Q_Q(QSGGridView);
    layoutScheduled = false;
    if (!isValid() && !visibleItems.count()) {
        clear();
        return;
    }
    if (visibleItems.count()) {
        qreal rowPos = visibleItems.first()->rowPos();
        qreal colPos = visibleItems.first()->colPos();
        int col = visibleIndex % columns;
        if (colPos != col * colSize()) {
            colPos = col * colSize();
            visibleItems.first()->setPosition(colPos, rowPos);
        }
        for (int i = 1; i < visibleItems.count(); ++i) {
            FxGridItem *item = visibleItems.at(i);
            colPos += colSize();
            if (colPos > colSize() * (columns-1)) {
                colPos = 0;
                rowPos += rowSize();
            }
            item->setPosition(colPos, rowPos);
        }
    }
    if (header)
        updateHeader();
    if (footer)
        updateFooter();
    q->refill();
    updateHighlight();
    moveReason = Other;
    if (flow == QSGGridView::LeftToRight) {
        q->setContentHeight(endPosition() - startPosition());
        fixupY();
    } else {
        q->setContentWidth(endPosition() - startPosition());
        fixupX();
    }
    updateUnrequestedPositions();
}

void QSGGridViewPrivate::updateUnrequestedIndexes()
{
    Q_Q(QSGGridView);
    QHash<QSGItem*,int>::iterator it;
    for (it = unrequestedItems.begin(); it != unrequestedItems.end(); ++it)
        *it = model->indexOf(it.key(), q);
}

void QSGGridViewPrivate::updateUnrequestedPositions()
{
    QHash<QSGItem*,int>::const_iterator it;
    for (it = unrequestedItems.begin(); it != unrequestedItems.end(); ++it) {
        if (flow == QSGGridView::LeftToRight) {
            it.key()->setPos(QPointF(colPosAt(*it), rowPosAt(*it)));
        } else {
            it.key()->setPos(QPointF(rowPosAt(*it), colPosAt(*it)));
        }
    }
}

void QSGGridViewPrivate::updateTrackedItem()
{
    Q_Q(QSGGridView);
    FxGridItem *item = currentItem;
    if (highlight)
        item = highlight;

    if (trackedItem && item != trackedItem) {
        QObject::disconnect(trackedItem->item, SIGNAL(yChanged()), q, SLOT(trackedPositionChanged()));
        QObject::disconnect(trackedItem->item, SIGNAL(xChanged()), q, SLOT(trackedPositionChanged()));
        trackedItem = 0;
    }

    if (!trackedItem && item) {
        trackedItem = item;
        QObject::connect(trackedItem->item, SIGNAL(yChanged()), q, SLOT(trackedPositionChanged()));
        QObject::connect(trackedItem->item, SIGNAL(xChanged()), q, SLOT(trackedPositionChanged()));
    }
    if (trackedItem)
        q->trackedPositionChanged();
}

void QSGGridViewPrivate::createHighlight()
{
    Q_Q(QSGGridView);
    bool changed = false;
    if (highlight) {
        if (trackedItem == highlight)
            trackedItem = 0;
        delete highlight->item;
        delete highlight;
        highlight = 0;
        delete highlightXAnimator;
        delete highlightYAnimator;
        highlightXAnimator = 0;
        highlightYAnimator = 0;
        changed = true;
    }

    if (currentItem) {
        QSGItem *item = 0;
        if (highlightComponent) {
            QDeclarativeContext *highlightContext = new QDeclarativeContext(qmlContext(q));
            QObject *nobj = highlightComponent->create(highlightContext);
            if (nobj) {
                QDeclarative_setParent_noEvent(highlightContext, nobj);
                item = qobject_cast<QSGItem *>(nobj);
                if (!item)
                    delete nobj;
            } else {
                delete highlightContext;
            }
        } else {
            item = new QSGItem;
            QDeclarative_setParent_noEvent(item, q->contentItem());
            item->setParentItem(q->contentItem());
        }
        if (item) {
            QDeclarative_setParent_noEvent(item, q->contentItem());
            item->setParentItem(q->contentItem());
            highlight = new FxGridItem(item, q);
            if (currentItem)
                highlight->setPosition(currentItem->colPos(), currentItem->rowPos());
            highlightXAnimator = new QSmoothedAnimation(q);
            highlightXAnimator->target = QDeclarativeProperty(highlight->item, QLatin1String("x"));
            highlightXAnimator->userDuration = highlightMoveDuration;
            highlightYAnimator = new QSmoothedAnimation(q);
            highlightYAnimator->target = QDeclarativeProperty(highlight->item, QLatin1String("y"));
            highlightYAnimator->userDuration = highlightMoveDuration;
            if (autoHighlight) {
                highlightXAnimator->restart();
                highlightYAnimator->restart();
            }
            changed = true;
        }
    }
    if (changed)
        emit q->highlightItemChanged();
}

void QSGGridViewPrivate::updateHighlight()
{
    if ((!currentItem && highlight) || (currentItem && !highlight))
        createHighlight();
    if (currentItem && autoHighlight && highlight && !movingHorizontally && !movingVertically) {
        // auto-update highlight
        highlightXAnimator->to = currentItem->item->x();
        highlightYAnimator->to = currentItem->item->y();
        highlight->item->setWidth(currentItem->item->width());
        highlight->item->setHeight(currentItem->item->height());
        highlightXAnimator->restart();
        highlightYAnimator->restart();
    }
    updateTrackedItem();
}

void QSGGridViewPrivate::updateCurrent(int modelIndex)
{
    Q_Q(QSGGridView);
    if (!q->isComponentComplete() || !isValid() || modelIndex < 0 || modelIndex >= model->count()) {
        if (currentItem) {
            currentItem->attached->setIsCurrentItem(false);
            releaseItem(currentItem);
            currentItem = 0;
            currentIndex = modelIndex;
            emit q->currentIndexChanged();
            updateHighlight();
        } else if (currentIndex != modelIndex) {
            currentIndex = modelIndex;
            emit q->currentIndexChanged();
        }
        return;
    }

    if (currentItem && currentIndex == modelIndex) {
        updateHighlight();
        return;
    }

    FxGridItem *oldCurrentItem = currentItem;
    currentIndex = modelIndex;
    currentItem = createItem(modelIndex);
    fixCurrentVisibility = true;
    if (oldCurrentItem && (!currentItem || oldCurrentItem->item != currentItem->item))
        oldCurrentItem->attached->setIsCurrentItem(false);
    if (currentItem) {
        currentItem->setPosition(colPosAt(modelIndex), rowPosAt(modelIndex));
        currentItem->item->setFocus(true);
        currentItem->attached->setIsCurrentItem(true);
    }
    updateHighlight();
    emit q->currentIndexChanged();
    releaseItem(oldCurrentItem);
}

void QSGGridViewPrivate::updateFooter()
{
    Q_Q(QSGGridView);
    if (!footer && footerComponent) {
        QSGItem *item = 0;
        QDeclarativeContext *context = new QDeclarativeContext(qmlContext(q));
        QObject *nobj = footerComponent->create(context);
        if (nobj) {
            QDeclarative_setParent_noEvent(context, nobj);
            item = qobject_cast<QSGItem *>(nobj);
            if (!item)
                delete nobj;
        } else {
            delete context;
        }
        if (item) {
            QDeclarative_setParent_noEvent(item, q->contentItem());
            item->setParentItem(q->contentItem());
            item->setZ(1);
            QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
            itemPrivate->addItemChangeListener(this, QSGItemPrivate::Geometry);
            footer = new FxGridItem(item, q);
        }
    }
    if (footer) {
        if (visibleItems.count()) {
            qreal endPos = endPosition();
            if (lastVisibleIndex() == model->count()-1) {
                footer->setPosition(0, endPos);
            } else {
                qreal visiblePos = position() + q->height();
                if (endPos <= visiblePos || footer->endRowPos() < endPos)
                    footer->setPosition(0, endPos);
            }
        } else {
            qreal endPos = 0;
            if (header) {
                endPos += flow == QSGGridView::LeftToRight
                                   ? header->item->height()
                                   : header->item->width();
            }
            footer->setPosition(0, endPos);
        }
    }
}

void QSGGridViewPrivate::updateHeader()
{
    Q_Q(QSGGridView);
    if (!header && headerComponent) {
        QSGItem *item = 0;
        QDeclarativeContext *context = new QDeclarativeContext(qmlContext(q));
        QObject *nobj = headerComponent->create(context);
        if (nobj) {
            QDeclarative_setParent_noEvent(context, nobj);
            item = qobject_cast<QSGItem *>(nobj);
            if (!item)
                delete nobj;
        } else {
            delete context;
        }
        if (item) {
            QDeclarative_setParent_noEvent(item, q->contentItem());
            item->setParentItem(q->contentItem());
            item->setZ(1);
            QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
            itemPrivate->addItemChangeListener(this, QSGItemPrivate::Geometry);
            header = new FxGridItem(item, q);
        }
    }
    if (header) {
        if (visibleItems.count()) {
            qreal startPos = startPosition();
            if (visibleIndex == 0) {
                header->setPosition(0, startPos - headerSize());
            } else {
                if (position() <= startPos || header->rowPos() > startPos - headerSize())
                    header->setPosition(0, startPos - headerSize());
            }
        } else {
            header->setPosition(0, 0);
        }
    }
}

void QSGGridViewPrivate::fixupPosition()
{
    moveReason = Other;
    if (flow == QSGGridView::LeftToRight)
        fixupY();
    else
        fixupX();
}

void QSGGridViewPrivate::fixup(AxisData &data, qreal minExtent, qreal maxExtent)
{
    if ((flow == QSGGridView::TopToBottom && &data == &vData)
        || (flow == QSGGridView::LeftToRight && &data == &hData))
        return;

    int oldDuration = fixupDuration;
    fixupDuration = moveReason == Mouse ? fixupDuration : 0;

    if (snapMode != QSGGridView::NoSnap) {
        FxGridItem *topItem = snapItemAt(position()+highlightRangeStart);
        FxGridItem *bottomItem = snapItemAt(position()+highlightRangeEnd);
        qreal pos;
        if (topItem && bottomItem && haveHighlightRange && highlightRange == QSGGridView::StrictlyEnforceRange) {
            qreal topPos = qMin(topItem->rowPos() - highlightRangeStart, -maxExtent);
            qreal bottomPos = qMax(bottomItem->rowPos() - highlightRangeEnd, -minExtent);
            pos = qAbs(data.move + topPos) < qAbs(data.move + bottomPos) ? topPos : bottomPos;
        } else if (topItem) {
            if (topItem->index == 0 && header && position()+highlightRangeStart < header->rowPos()+headerSize()/2)
                pos = header->rowPos() - highlightRangeStart;
            else
                pos = qMax(qMin(topItem->rowPos() - highlightRangeStart, -maxExtent), -minExtent);
        } else if (bottomItem) {
            pos = qMax(qMin(bottomItem->rowPos() - highlightRangeStart, -maxExtent), -minExtent);
        } else {
            QSGFlickablePrivate::fixup(data, minExtent, maxExtent);
            fixupDuration = oldDuration;
            return;
        }
        if (currentItem && haveHighlightRange && highlightRange == QSGGridView::StrictlyEnforceRange) {
            updateHighlight();
            qreal currPos = currentItem->rowPos();
            if (pos < currPos + rowSize() - highlightRangeEnd)
                pos = currPos + rowSize() - highlightRangeEnd;
            if (pos > currPos - highlightRangeStart)
                pos = currPos - highlightRangeStart;
        }

        qreal dist = qAbs(data.move + pos);
        if (dist > 0) {
            timeline.reset(data.move);
            if (fixupDuration)
                timeline.move(data.move, -pos, QEasingCurve(QEasingCurve::InOutQuad), fixupDuration/2);
            else
                timeline.set(data.move, -pos);
            vTime = timeline.time();
        }
    } else if (haveHighlightRange && highlightRange == QSGGridView::StrictlyEnforceRange) {
        if (currentItem) {
            updateHighlight();
            qreal pos = currentItem->rowPos();
            qreal viewPos = position();
            if (viewPos < pos + rowSize() - highlightRangeEnd)
                viewPos = pos + rowSize() - highlightRangeEnd;
            if (viewPos > pos - highlightRangeStart)
                viewPos = pos - highlightRangeStart;

            timeline.reset(data.move);
            if (viewPos != position()) {
                if (fixupDuration)
                    timeline.move(data.move, -viewPos, QEasingCurve(QEasingCurve::InOutQuad), fixupDuration/2);
                else
                    timeline.set(data.move, -viewPos);
            }
            vTime = timeline.time();
        }
    } else {
        QSGFlickablePrivate::fixup(data, minExtent, maxExtent);
    }
    fixupDuration = oldDuration;
}

void QSGGridViewPrivate::flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                                        QDeclarativeTimeLineCallback::Callback fixupCallback, qreal velocity)
{
    Q_Q(QSGGridView);
    moveReason = Mouse;
    if ((!haveHighlightRange || highlightRange != QSGGridView::StrictlyEnforceRange)
        && snapMode == QSGGridView::NoSnap) {
        QSGFlickablePrivate::flick(data, minExtent, maxExtent, vSize, fixupCallback, velocity);
        return;
    }
    qreal maxDistance = 0;
    // -ve velocity means list is moving up
    if (velocity > 0) {
        if (data.move.value() < minExtent) {
            if (snapMode == QSGGridView::SnapOneRow) {
                if (FxGridItem *item = firstVisibleItem())
                    maxDistance = qAbs(item->rowPos() + data.move.value());
            } else {
                maxDistance = qAbs(minExtent - data.move.value());
            }
        }
        if (snapMode == QSGGridView::NoSnap && highlightRange != QSGGridView::StrictlyEnforceRange)
            data.flickTarget = minExtent;
    } else {
        if (data.move.value() > maxExtent) {
            if (snapMode == QSGGridView::SnapOneRow) {
                qreal pos = snapPosAt(-data.move.value()) + rowSize();
                maxDistance = qAbs(pos + data.move.value());
            } else {
                maxDistance = qAbs(maxExtent - data.move.value());
            }
        }
        if (snapMode == QSGGridView::NoSnap && highlightRange != QSGGridView::StrictlyEnforceRange)
            data.flickTarget = maxExtent;
    }
    bool overShoot = boundsBehavior == QSGFlickable::DragAndOvershootBounds;
    if (maxDistance > 0 || overShoot) {
        // This mode requires the grid to stop exactly on a row boundary.
        qreal v = velocity;
        if (maxVelocity != -1 && maxVelocity < qAbs(v)) {
            if (v < 0)
                v = -maxVelocity;
            else
                v = maxVelocity;
        }
        qreal accel = deceleration;
        qreal v2 = v * v;
        qreal overshootDist = 0.0;
        if ((maxDistance > 0.0 && v2 / (2.0f * maxDistance) < accel) || snapMode == QSGGridView::SnapOneRow) {
            // + rowSize()/4 to encourage moving at least one item in the flick direction
            qreal dist = v2 / (accel * 2.0) + rowSize()/4;
            dist = qMin(dist, maxDistance);
            if (v > 0)
                dist = -dist;
            data.flickTarget = -snapPosAt(-(data.move.value() - highlightRangeStart) + dist) + highlightRangeStart;
            qreal adjDist = -data.flickTarget + data.move.value();
            if (qAbs(adjDist) > qAbs(dist)) {
                // Prevent painfully slow flicking - adjust velocity to suit flickDeceleration
                qreal adjv2 = accel * 2.0f * qAbs(adjDist);
                if (adjv2 > v2) {
                    v2 = adjv2;
                    v = qSqrt(v2);
                    if (dist > 0)
                        v = -v;
                }
            }
            dist = adjDist;
            accel = v2 / (2.0f * qAbs(dist));
        } else {
            data.flickTarget = velocity > 0 ? minExtent : maxExtent;
            overshootDist = overShoot ? overShootDistance(v, vSize) : 0;
        }
        timeline.reset(data.move);
        timeline.accel(data.move, v, accel, maxDistance + overshootDist);
        timeline.callback(QDeclarativeTimeLineCallback(&data.move, fixupCallback, this));
        if (!flickingHorizontally && q->xflick()) {
            flickingHorizontally = true;
            emit q->flickingChanged();
            emit q->flickingHorizontallyChanged();
            emit q->flickStarted();
        }
        if (!flickingVertically && q->yflick()) {
            flickingVertically = true;
            emit q->flickingChanged();
            emit q->flickingVerticallyChanged();
            emit q->flickStarted();
        }
    } else {
        timeline.reset(data.move);
        fixup(data, minExtent, maxExtent);
    }
}


//----------------------------------------------------------------------------

QSGGridView::QSGGridView(QSGItem *parent)
    : QSGFlickable(*(new QSGGridViewPrivate), parent)
{
    Q_D(QSGGridView);
    d->init();
}

QSGGridView::~QSGGridView()
{
    Q_D(QSGGridView);
    d->clear();
    if (d->ownModel)
        delete d->model;
    delete d->header;
    delete d->footer;
}

QVariant QSGGridView::model() const
{
    Q_D(const QSGGridView);
    return d->modelVariant;
}

void QSGGridView::setModel(const QVariant &model)
{
    Q_D(QSGGridView);
    if (d->modelVariant == model)
        return;
    if (d->model) {
        disconnect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        disconnect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        disconnect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        disconnect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        disconnect(d->model, SIGNAL(createdItem(int,QSGItem*)), this, SLOT(createdItem(int,QSGItem*)));
        disconnect(d->model, SIGNAL(destroyingItem(QSGItem*)), this, SLOT(destroyingItem(QSGItem*)));
    }
    d->clear();
    d->modelVariant = model;
    QObject *object = qvariant_cast<QObject*>(model);
    QSGVisualModel *vim = 0;
    if (object && (vim = qobject_cast<QSGVisualModel *>(object))) {
        if (d->ownModel) {
            delete d->model;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QSGVisualDataModel(qmlContext(this), this);
            d->ownModel = true;
        }
        if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model))
            dataModel->setModel(model);
    }
    if (d->model) {
        d->bufferMode = QSGGridViewPrivate::BufferBefore | QSGGridViewPrivate::BufferAfter;
        if (isComponentComplete()) {
            refill();
            if ((d->currentIndex >= d->model->count() || d->currentIndex < 0) && !d->currentIndexCleared) {
                setCurrentIndex(0);
            } else {
                d->moveReason = QSGGridViewPrivate::SetIndex;
                d->updateCurrent(d->currentIndex);
                if (d->highlight && d->currentItem) {
                    d->highlight->setPosition(d->currentItem->colPos(), d->currentItem->rowPos());
                    d->updateTrackedItem();
                }
                d->moveReason = QSGGridViewPrivate::Other;
            }
        }
        connect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        connect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        connect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        connect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->model, SIGNAL(createdItem(int,QSGItem*)), this, SLOT(createdItem(int,QSGItem*)));
        connect(d->model, SIGNAL(destroyingItem(QSGItem*)), this, SLOT(destroyingItem(QSGItem*)));
        emit countChanged();
    }
    emit modelChanged();
}

QDeclarativeComponent *QSGGridView::delegate() const
{
    Q_D(const QSGGridView);
    if (d->model) {
        if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void QSGGridView::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QSGGridView);
    if (delegate == this->delegate())
        return;

    if (!d->ownModel) {
        d->model = new QSGVisualDataModel(qmlContext(this));
        d->ownModel = true;
    }
    if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model)) {
        dataModel->setDelegate(delegate);
        if (isComponentComplete()) {
            for (int i = 0; i < d->visibleItems.count(); ++i)
                d->releaseItem(d->visibleItems.at(i));
            d->visibleItems.clear();
            d->releaseItem(d->currentItem);
            d->currentItem = 0;
            refill();
            d->moveReason = QSGGridViewPrivate::SetIndex;
            d->updateCurrent(d->currentIndex);
            if (d->highlight && d->currentItem) {
                d->highlight->setPosition(d->currentItem->colPos(), d->currentItem->rowPos());
                d->updateTrackedItem();
            }
            d->moveReason = QSGGridViewPrivate::Other;
        }
        emit delegateChanged();
    }
}

int QSGGridView::currentIndex() const
{
    Q_D(const QSGGridView);
    return d->currentIndex;
}

void QSGGridView::setCurrentIndex(int index)
{
    Q_D(QSGGridView);
    if (d->requestedIndex >= 0) // currently creating item
        return;
    d->currentIndexCleared = (index == -1);
    if (index == d->currentIndex)
        return;
    if (isComponentComplete() && d->isValid()) {
        d->moveReason = QSGGridViewPrivate::SetIndex;
        d->updateCurrent(index);
    } else {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}

QSGItem *QSGGridView::currentItem()
{
    Q_D(QSGGridView);
    if (!d->currentItem)
        return 0;
    return d->currentItem->item;
}

QSGItem *QSGGridView::highlightItem()
{
    Q_D(QSGGridView);
    if (!d->highlight)
        return 0;
    return d->highlight->item;
}

int QSGGridView::count() const
{
    Q_D(const QSGGridView);
    if (d->model)
        return d->model->count();
    return 0;
}

QDeclarativeComponent *QSGGridView::highlight() const
{
    Q_D(const QSGGridView);
    return d->highlightComponent;
}

void QSGGridView::setHighlight(QDeclarativeComponent *highlight)
{
    Q_D(QSGGridView);
    if (highlight != d->highlightComponent) {
        d->highlightComponent = highlight;
        d->updateCurrent(d->currentIndex);
        emit highlightChanged();
    }
}

bool QSGGridView::highlightFollowsCurrentItem() const
{
    Q_D(const QSGGridView);
    return d->autoHighlight;
}

void QSGGridView::setHighlightFollowsCurrentItem(bool autoHighlight)
{
    Q_D(QSGGridView);
    if (d->autoHighlight != autoHighlight) {
        d->autoHighlight = autoHighlight;
        if (autoHighlight) {
            d->updateHighlight();
        } else if (d->highlightXAnimator) {
            d->highlightXAnimator->stop();
            d->highlightYAnimator->stop();
        }
    }
}

int QSGGridView::highlightMoveDuration() const
{
    Q_D(const QSGGridView);
    return d->highlightMoveDuration;
}

void QSGGridView::setHighlightMoveDuration(int duration)
{
    Q_D(QSGGridView);
    if (d->highlightMoveDuration != duration) {
        d->highlightMoveDuration = duration;
        if (d->highlightYAnimator) {
            d->highlightXAnimator->userDuration = d->highlightMoveDuration;
            d->highlightYAnimator->userDuration = d->highlightMoveDuration;
        }
        emit highlightMoveDurationChanged();
    }
}

qreal QSGGridView::preferredHighlightBegin() const
{
    Q_D(const QSGGridView);
    return d->highlightRangeStart;
}

void QSGGridView::setPreferredHighlightBegin(qreal start)
{
    Q_D(QSGGridView);
    if (d->highlightRangeStart == start)
        return;
    d->highlightRangeStart = start;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightBeginChanged();
}

qreal QSGGridView::preferredHighlightEnd() const
{
    Q_D(const QSGGridView);
    return d->highlightRangeEnd;
}

void QSGGridView::setPreferredHighlightEnd(qreal end)
{
    Q_D(QSGGridView);
    if (d->highlightRangeEnd == end)
        return;
    d->highlightRangeEnd = end;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightEndChanged();
}

QSGGridView::HighlightRangeMode QSGGridView::highlightRangeMode() const
{
    Q_D(const QSGGridView);
    return d->highlightRange;
}

void QSGGridView::setHighlightRangeMode(HighlightRangeMode mode)
{
    Q_D(QSGGridView);
    if (d->highlightRange == mode)
        return;
    d->highlightRange = mode;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit highlightRangeModeChanged();
}

QSGGridView::Flow QSGGridView::flow() const
{
    Q_D(const QSGGridView);
    return d->flow;
}

void QSGGridView::setFlow(Flow flow)
{
    Q_D(QSGGridView);
    if (d->flow != flow) {
        d->flow = flow;
        if (d->flow == LeftToRight) {
            setContentWidth(-1);
            setFlickableDirection(QSGFlickable::VerticalFlick);
        } else {
            setContentHeight(-1);
            setFlickableDirection(QSGFlickable::HorizontalFlick);
        }
        d->clear();
        d->updateGrid();
        refill();
        d->updateCurrent(d->currentIndex);
        emit flowChanged();
    }
}

bool QSGGridView::isWrapEnabled() const
{
    Q_D(const QSGGridView);
    return d->wrap;
}

void QSGGridView::setWrapEnabled(bool wrap)
{
    Q_D(QSGGridView);
    if (d->wrap == wrap)
        return;
    d->wrap = wrap;
    emit keyNavigationWrapsChanged();
}

int QSGGridView::cacheBuffer() const
{
    Q_D(const QSGGridView);
    return d->buffer;
}

void QSGGridView::setCacheBuffer(int buffer)
{
    Q_D(QSGGridView);
    if (d->buffer != buffer) {
        d->buffer = buffer;
        if (isComponentComplete())
            refill();
        emit cacheBufferChanged();
    }
}

int QSGGridView::cellWidth() const
{
    Q_D(const QSGGridView);
    return d->cellWidth;
}

void QSGGridView::setCellWidth(int cellWidth)
{
    Q_D(QSGGridView);
    if (cellWidth != d->cellWidth && cellWidth > 0) {
        d->cellWidth = qMax(1, cellWidth);
        d->updateGrid();
        emit cellWidthChanged();
        d->layout();
    }
}

int QSGGridView::cellHeight() const
{
    Q_D(const QSGGridView);
    return d->cellHeight;
}

void QSGGridView::setCellHeight(int cellHeight)
{
    Q_D(QSGGridView);
    if (cellHeight != d->cellHeight && cellHeight > 0) {
        d->cellHeight = qMax(1, cellHeight);
        d->updateGrid();
        emit cellHeightChanged();
        d->layout();
    }
}

QSGGridView::SnapMode QSGGridView::snapMode() const
{
    Q_D(const QSGGridView);
    return d->snapMode;
}

void QSGGridView::setSnapMode(SnapMode mode)
{
    Q_D(QSGGridView);
    if (d->snapMode != mode) {
        d->snapMode = mode;
        emit snapModeChanged();
    }
}

QDeclarativeComponent *QSGGridView::footer() const
{
    Q_D(const QSGGridView);
    return d->footerComponent;
}

void QSGGridView::setFooter(QDeclarativeComponent *footer)
{
    Q_D(QSGGridView);
    if (d->footerComponent != footer) {
        if (d->footer) {
            delete d->footer;
            d->footer = 0;
        }
        d->footerComponent = footer;
        if (isComponentComplete()) {
            d->updateFooter();
            d->updateGrid();
        }
        emit footerChanged();
    }
}

QDeclarativeComponent *QSGGridView::header() const
{
    Q_D(const QSGGridView);
    return d->headerComponent;
}

void QSGGridView::setHeader(QDeclarativeComponent *header)
{
    Q_D(QSGGridView);
    if (d->headerComponent != header) {
        if (d->header) {
            delete d->header;
            d->header = 0;
        }
        d->headerComponent = header;
        if (isComponentComplete()) {
            d->updateHeader();
            d->updateFooter();
            d->updateGrid();
        }
        emit headerChanged();
    }
}

void QSGGridView::setContentX(qreal pos)
{
    Q_D(QSGGridView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QSGGridViewPrivate::Other;
    QSGFlickable::setContentX(pos);
}

void QSGGridView::setContentY(qreal pos)
{
    Q_D(QSGGridView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QSGGridViewPrivate::Other;
    QSGFlickable::setContentY(pos);
}

void QSGGridView::updatePolish() 
{
    Q_D(QSGGridView);
    QSGFlickable::updatePolish();
    d->layout();
}

void QSGGridView::viewportMoved()
{
    Q_D(QSGGridView);
    QSGFlickable::viewportMoved();
    if (!d->itemCount)
        return;
    d->lazyRelease = true;
    if (d->flickingHorizontally || d->flickingVertically) {
        if (yflick()) {
            if (d->vData.velocity > 0)
                d->bufferMode = QSGGridViewPrivate::BufferBefore;
            else if (d->vData.velocity < 0)
                d->bufferMode = QSGGridViewPrivate::BufferAfter;
        }

        if (xflick()) {
            if (d->hData.velocity > 0)
                d->bufferMode = QSGGridViewPrivate::BufferBefore;
            else if (d->hData.velocity < 0)
                d->bufferMode = QSGGridViewPrivate::BufferAfter;
        }
    }
    refill();
    if (d->flickingHorizontally || d->flickingVertically || d->movingHorizontally || d->movingVertically)
        d->moveReason = QSGGridViewPrivate::Mouse;
    if (d->moveReason != QSGGridViewPrivate::SetIndex) {
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange && d->highlight) {
            // reposition highlight
            qreal pos = d->highlight->rowPos();
            qreal viewPos = d->position();
            if (pos > viewPos + d->highlightRangeEnd - d->rowSize())
                pos = viewPos + d->highlightRangeEnd - d->rowSize();
            if (pos < viewPos + d->highlightRangeStart)
                pos = viewPos + d->highlightRangeStart;
            d->highlight->setPosition(d->highlight->colPos(), qRound(pos));

            // update current index
            int idx = d->snapIndex();
            if (idx >= 0 && idx != d->currentIndex) {
                d->updateCurrent(idx);
                if (d->currentItem && d->currentItem->colPos() != d->highlight->colPos() && d->autoHighlight) {
                    if (d->flow == LeftToRight)
                        d->highlightXAnimator->to = d->currentItem->item->x();
                    else
                        d->highlightYAnimator->to = d->currentItem->item->y();
                }
            }
        }
    }
}

qreal QSGGridView::minYExtent() const
{
    Q_D(const QSGGridView);
    if (d->flow == QSGGridView::TopToBottom)
        return QSGFlickable::minYExtent();
    qreal extent = -d->startPosition();
    if (d->header && d->visibleItems.count())
        extent += d->header->item->height();
    if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
        extent += d->highlightRangeStart;
        extent = qMax(extent, -(d->rowPosAt(0) + d->rowSize() - d->highlightRangeEnd));
    }
    return extent;
}

qreal QSGGridView::maxYExtent() const
{
    Q_D(const QSGGridView);
    if (d->flow == QSGGridView::TopToBottom)
        return QSGFlickable::maxYExtent();
    qreal extent;
    if (!d->model || !d->model->count()) {
        extent = 0;
    } else if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
        extent = -(d->rowPosAt(d->model->count()-1) - d->highlightRangeStart);
        if (d->highlightRangeEnd != d->highlightRangeStart)
            extent = qMin(extent, -(d->endPosition() - d->highlightRangeEnd + 1));
    } else {
        extent = -(d->endPosition() - height());
    }
    if (d->footer)
        extent -= d->footer->item->height();
    const qreal minY = minYExtent();
    if (extent > minY)
        extent = minY;
    return extent;
}

qreal QSGGridView::minXExtent() const
{
    Q_D(const QSGGridView);
    if (d->flow == QSGGridView::LeftToRight)
        return QSGFlickable::minXExtent();
    qreal extent = -d->startPosition();
    if (d->header && d->visibleItems.count())
        extent += d->header->item->width();
    if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
        extent += d->highlightRangeStart;
        extent = qMax(extent, -(d->rowPosAt(0) + d->rowSize() - d->highlightRangeEnd));
    }
    return extent;
}

qreal QSGGridView::maxXExtent() const
{
    Q_D(const QSGGridView);
    if (d->flow == QSGGridView::LeftToRight)
        return QSGFlickable::maxXExtent();
    qreal extent;
    if (!d->model || !d->model->count()) {
        extent = 0;
    } if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
        extent = -(d->rowPosAt(d->model->count()-1) - d->highlightRangeStart);
        if (d->highlightRangeEnd != d->highlightRangeStart)
            extent = qMin(extent, -(d->endPosition() - d->highlightRangeEnd + 1));
    } else {
        extent = -(d->endPosition() - width());
    }
    if (d->footer)
        extent -= d->footer->item->width();
    const qreal minX = minXExtent();
    if (extent > minX)
        extent = minX;
    return extent;
}

void QSGGridView::keyPressEvent(QKeyEvent *event)
{
    Q_D(QSGGridView);
    if (d->model && d->model->count() && d->interactive) {
        d->moveReason = QSGGridViewPrivate::SetIndex;
        int oldCurrent = currentIndex();
        switch (event->key()) {
        case Qt::Key_Up:
            moveCurrentIndexUp();
            break;
        case Qt::Key_Down:
            moveCurrentIndexDown();
            break;
        case Qt::Key_Left:
            moveCurrentIndexLeft();
            break;
        case Qt::Key_Right:
            moveCurrentIndexRight();
            break;
        default:
            break;
        }
        if (oldCurrent != currentIndex()) {
            event->accept();
            return;
        }
    }
    d->moveReason = QSGGridViewPrivate::Other;
    event->ignore();
    QSGFlickable::keyPressEvent(event);
}

void QSGGridView::moveCurrentIndexUp()
{
    Q_D(QSGGridView);
    const int count = d->model ? d->model->count() : 0;
    if (!count)
        return;
    if (d->flow == QSGGridView::LeftToRight) {
        if (currentIndex() >= d->columns || d->wrap) {
            int index = currentIndex() - d->columns;
            setCurrentIndex((index >= 0 && index < count) ? index : count-1);
        }
    } else {
        if (currentIndex() > 0 || d->wrap) {
            int index = currentIndex() - 1;
            setCurrentIndex((index >= 0 && index < count) ? index : count-1);
        }
    }
}

void QSGGridView::moveCurrentIndexDown()
{
    Q_D(QSGGridView);
    const int count = d->model ? d->model->count() : 0;
    if (!count)
        return;
    if (d->flow == QSGGridView::LeftToRight) {
        if (currentIndex() < count - d->columns || d->wrap) {
            int index = currentIndex()+d->columns;
            setCurrentIndex((index >= 0 && index < count) ? index : 0);
        }
    } else {
        if (currentIndex() < count - 1 || d->wrap) {
            int index = currentIndex() + 1;
            setCurrentIndex((index >= 0 && index < count) ? index : 0);
        }
    }
}

void QSGGridView::moveCurrentIndexLeft()
{
    Q_D(QSGGridView);
    const int count = d->model ? d->model->count() : 0;
    if (!count)
        return;
    if (d->flow == QSGGridView::LeftToRight) {
        if (currentIndex() > 0 || d->wrap) {
            int index = currentIndex() - 1;
            setCurrentIndex((index >= 0 && index < count) ? index : count-1);
        }
    } else {
        if (currentIndex() >= d->columns || d->wrap) {
            int index = currentIndex() - d->columns;
            setCurrentIndex((index >= 0 && index < count) ? index : count-1);
        }
    }
}

void QSGGridView::moveCurrentIndexRight()
{
    Q_D(QSGGridView);
    const int count = d->model ? d->model->count() : 0;
    if (!count)
        return;
    if (d->flow == QSGGridView::LeftToRight) {
        if (currentIndex() < count - 1 || d->wrap) {
            int index = currentIndex() + 1;
            setCurrentIndex((index >= 0 && index < count) ? index : 0);
        }
    } else {
        if (currentIndex() < count - d->columns || d->wrap) {
            int index = currentIndex()+d->columns;
            setCurrentIndex((index >= 0 && index < count) ? index : 0);
        }
    }
}

void QSGGridView::positionViewAtIndex(int index, int mode)
{
    Q_D(QSGGridView);
    if (!d->isValid() || index < 0 || index >= d->model->count())
        return;
    if (mode < Beginning || mode > Contain)
        return;

    if (d->layoutScheduled)
        d->layout();
    qreal pos = d->position();
    FxGridItem *item = d->visibleItem(index);
    if (!item) {
        int itemPos = d->rowPosAt(index);
        // save the currently visible items in case any of them end up visible again
        QList<FxGridItem*> oldVisible = d->visibleItems;
        d->visibleItems.clear();
        d->visibleIndex = index - index % d->columns;
        d->setPosition(itemPos);
        // now release the reference to all the old visible items.
        for (int i = 0; i < oldVisible.count(); ++i)
            d->releaseItem(oldVisible.at(i));
        item = d->visibleItem(index);
    }
    if (item) {
        qreal itemPos = item->rowPos();
        switch (mode) {
        case Beginning:
            pos = itemPos;
            break;
        case Center:
            pos = itemPos - (d->size() - d->rowSize())/2;
            break;
        case End:
            pos = itemPos - d->size() + d->rowSize();
            break;
        case Visible:
            if (itemPos > pos + d->size())
                pos = itemPos - d->size() + d->rowSize();
            else if (item->endRowPos() < pos)
                pos = itemPos;
            break;
        case Contain:
            if (item->endRowPos() > pos + d->size())
                pos = itemPos - d->size() + d->rowSize();
            if (itemPos < pos)
                pos = itemPos;
        }
        qreal maxExtent = d->flow == QSGGridView::LeftToRight ? -maxYExtent() : -maxXExtent();
        pos = qMin(pos, maxExtent);
        qreal minExtent = d->flow == QSGGridView::LeftToRight ? -minYExtent() : -minXExtent();
        pos = qMax(pos, minExtent);
        d->moveReason = QSGGridViewPrivate::Other;
        cancelFlick();
        d->setPosition(pos);
    }
    d->fixupPosition();
}

int QSGGridView::indexAt(int x, int y) const
{
    Q_D(const QSGGridView);
    for (int i = 0; i < d->visibleItems.count(); ++i) {
        const FxGridItem *listItem = d->visibleItems.at(i);
        if(listItem->contains(x, y))
            return listItem->index;
    }

    return -1;
}

void QSGGridView::componentComplete()
{
    Q_D(QSGGridView);
    QSGFlickable::componentComplete();
    d->updateHeader();
    d->updateFooter();
    d->updateGrid();
    if (d->isValid()) {
        refill();
        d->moveReason = QSGGridViewPrivate::SetIndex;
        if (d->currentIndex < 0 && !d->currentIndexCleared)
            d->updateCurrent(0);
        else
            d->updateCurrent(d->currentIndex);
        if (d->highlight && d->currentItem) {
            d->highlight->setPosition(d->currentItem->colPos(), d->currentItem->rowPos());
            d->updateTrackedItem();
        }
        d->moveReason = QSGGridViewPrivate::Other;
        d->fixupPosition();
    }
}

void QSGGridView::trackedPositionChanged()
{
    Q_D(QSGGridView);
    if (!d->trackedItem || !d->currentItem)
        return;
    if (d->moveReason == QSGGridViewPrivate::SetIndex) {
        const qreal trackedPos = d->trackedItem->rowPos();
        const qreal viewPos = d->position();
        qreal pos = viewPos;
        if (d->haveHighlightRange) {
            if (d->highlightRange == StrictlyEnforceRange) {
                if (trackedPos > pos + d->highlightRangeEnd - d->rowSize())
                    pos = trackedPos - d->highlightRangeEnd + d->rowSize();
                if (trackedPos < pos + d->highlightRangeStart)
                    pos = trackedPos - d->highlightRangeStart;
            } else {
                if (trackedPos < d->startPosition() + d->highlightRangeStart) {
                    pos = d->startPosition();
                } else if (d->trackedItem->endRowPos() > d->endPosition() - d->size() + d->highlightRangeEnd) {
                    pos = d->endPosition() - d->size() + 1;
                    if (pos < d->startPosition())
                        pos = d->startPosition();
                } else {
                    if (trackedPos < viewPos + d->highlightRangeStart) {
                        pos = trackedPos - d->highlightRangeStart;
                    } else if (trackedPos > viewPos + d->highlightRangeEnd - d->rowSize()) {
                        pos = trackedPos - d->highlightRangeEnd + d->rowSize();
                    }
                }
            }
        } else {
            if (trackedPos < viewPos && d->currentItem->rowPos() < viewPos) {
                pos = d->currentItem->rowPos() < trackedPos ? trackedPos : d->currentItem->rowPos();
            } else if (d->trackedItem->endRowPos() >= viewPos + d->size()
                && d->currentItem->endRowPos() >= viewPos + d->size()) {
                if (d->trackedItem->endRowPos() <= d->currentItem->endRowPos()) {
                    pos = d->trackedItem->endRowPos() - d->size() + 1;
                    if (d->rowSize() > d->size())
                        pos = trackedPos;
                } else {
                    pos = d->currentItem->endRowPos() - d->size() + 1;
                    if (d->rowSize() > d->size())
                        pos = d->currentItem->rowPos();
                }
            }
        }
        if (viewPos != pos) {
            cancelFlick();
            d->calcVelocity = true;
            d->setPosition(pos);
            d->calcVelocity = false;
        }
    }
}

void QSGGridView::itemsInserted(int modelIndex, int count)
{
    Q_D(QSGGridView);
    if (!isComponentComplete())
        return;
    if (!d->visibleItems.count() || d->model->count() <= 1) {
        d->scheduleLayout();
        if (d->itemCount && d->currentIndex >= modelIndex) {
            // adjust current item index
            d->currentIndex += count;
            if (d->currentItem)
                d->currentItem->index = d->currentIndex;
            emit currentIndexChanged();
        } else if (!d->currentIndex || (d->currentIndex < 0 && !d->currentIndexCleared)) {
            d->updateCurrent(0);
        }
        d->itemCount += count;
        emit countChanged();
        return;
    }

    int index = d->mapFromModel(modelIndex);
    if (index == -1) {
        int i = d->visibleItems.count() - 1;
        while (i > 0 && d->visibleItems.at(i)->index == -1)
            --i;
        if (d->visibleItems.at(i)->index + 1 == modelIndex) {
            // Special case of appending an item to the model.
            index = d->visibleIndex + d->visibleItems.count();
        } else {
            if (modelIndex <= d->visibleIndex) {
                // Insert before visible items
                d->visibleIndex += count;
                for (int i = 0; i < d->visibleItems.count(); ++i) {
                    FxGridItem *listItem = d->visibleItems.at(i);
                    if (listItem->index != -1 && listItem->index >= modelIndex)
                        listItem->index += count;
                }
            }
            if (d->currentIndex >= modelIndex) {
                // adjust current item index
                d->currentIndex += count;
                if (d->currentItem)
                    d->currentItem->index = d->currentIndex;
                emit currentIndexChanged();
            }
            d->scheduleLayout();
            d->itemCount += count;
            emit countChanged();
            return;
        }
    }

    // At least some of the added items will be visible
    int insertCount = count;
    if (index < d->visibleIndex) {
        insertCount -= d->visibleIndex - index;
        index = d->visibleIndex;
        modelIndex = d->visibleIndex;
    }

    index -= d->visibleIndex;
    int to = d->buffer+d->position()+d->size()-1;
    int colPos, rowPos;
    if (index < d->visibleItems.count()) {
        colPos = d->visibleItems.at(index)->colPos();
        rowPos = d->visibleItems.at(index)->rowPos();
    } else {
        // appending items to visible list
        colPos = d->visibleItems.at(index-1)->colPos() + d->colSize();
        rowPos = d->visibleItems.at(index-1)->rowPos();
        if (colPos > d->colSize() * (d->columns-1)) {
            colPos = 0;
            rowPos += d->rowSize();
        }
    }

    // Update the indexes of the following visible items.
    for (int i = 0; i < d->visibleItems.count(); ++i) {
        FxGridItem *listItem = d->visibleItems.at(i);
        if (listItem->index != -1 && listItem->index >= modelIndex)
            listItem->index += count;
    }

    bool addedVisible = false;
    QList<FxGridItem*> added;
    int i = 0;
    while (i < insertCount && rowPos <= to + d->rowSize()*(d->columns - (colPos/d->colSize()))/qreal(d->columns)) {
        if (!addedVisible) {
            d->scheduleLayout();
            addedVisible = true;
        }
        FxGridItem *item = d->createItem(modelIndex + i);
        d->visibleItems.insert(index, item);
        item->setPosition(colPos, rowPos);
        added.append(item);
        colPos += d->colSize();
        if (colPos > d->colSize() * (d->columns-1)) {
            colPos = 0;
            rowPos += d->rowSize();
        }
        ++index;
        ++i;
    }
    if (i < insertCount) {
        // We didn't insert all our new items, which means anything
        // beyond the current index is not visible - remove it.
        while (d->visibleItems.count() > index) {
            d->releaseItem(d->visibleItems.takeLast());
        }
    }

    // update visibleIndex
    d->visibleIndex = 0;
    for (QList<FxGridItem*>::Iterator it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            break;
        }
    }

    if (d->itemCount && d->currentIndex >= modelIndex) {
        // adjust current item index
        d->currentIndex += count;
        if (d->currentItem) {
            d->currentItem->index = d->currentIndex;
            d->currentItem->setPosition(d->colPosAt(d->currentIndex), d->rowPosAt(d->currentIndex));
        }
        emit currentIndexChanged();
    }

    // everything is in order now - emit add() signal
    for (int j = 0; j < added.count(); ++j)
        added.at(j)->attached->emitAdd();

    d->itemCount += count;
    emit countChanged();
}

void QSGGridView::itemsRemoved(int modelIndex, int count)
{
    Q_D(QSGGridView);
    if (!isComponentComplete())
        return;

    d->itemCount -= count;
    bool currentRemoved = d->currentIndex >= modelIndex && d->currentIndex < modelIndex + count;
    bool removedVisible = false;

    // Remove the items from the visible list, skipping anything already marked for removal
    QList<FxGridItem*>::Iterator it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxGridItem *item = *it;
        if (item->index == -1 || item->index < modelIndex) {
            // already removed, or before removed items
            if (item->index < modelIndex && !removedVisible) {
                d->scheduleLayout();
                removedVisible = true;
            }
            ++it;
        } else if (item->index >= modelIndex + count) {
            // after removed items
            item->index -= count;
            ++it;
        } else {
            // removed item
            if (!removedVisible) {
                d->scheduleLayout();
                removedVisible = true;
            }
            item->attached->emitRemove();
            if (item->attached->delayRemove()) {
                item->index = -1;
                connect(item->attached, SIGNAL(delayRemoveChanged()), this, SLOT(destroyRemoved()), Qt::QueuedConnection);
                ++it;
            } else {
                it = d->visibleItems.erase(it);
                d->releaseItem(item);
            }
        }
    }

    // fix current
    if (d->currentIndex >= modelIndex + count) {
        d->currentIndex -= count;
        if (d->currentItem)
            d->currentItem->index -= count;
        emit currentIndexChanged();
    } else if (currentRemoved) {
        // current item has been removed.
        d->releaseItem(d->currentItem);
        d->currentItem = 0;
        d->currentIndex = -1;
        if (d->itemCount)
            d->updateCurrent(qMin(modelIndex, d->itemCount-1));
    }

    // update visibleIndex
    d->visibleIndex = 0;
    for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            break;
        }
    }

    if (removedVisible && d->visibleItems.isEmpty()) {
        d->timeline.clear();
        if (d->itemCount == 0) {
            d->setPosition(0);
            d->updateHeader();
            d->updateFooter();
            update();
        }
    }

    emit countChanged();
}

void QSGGridView::destroyRemoved()
{
    Q_D(QSGGridView);
    for (QList<FxGridItem*>::Iterator it = d->visibleItems.begin();
            it != d->visibleItems.end();) {
        FxGridItem *listItem = *it;
        if (listItem->index == -1 && listItem->attached->delayRemove() == false) {
            d->releaseItem(listItem);
            it = d->visibleItems.erase(it);
        } else {
            ++it;
        }
    }

    // Correct the positioning of the items
    d->layout();
}

void QSGGridView::itemsMoved(int from, int to, int count)
{
    Q_D(QSGGridView);
    if (!isComponentComplete())
        return;
    QHash<int,FxGridItem*> moved;

    bool removedBeforeVisible = false;
    FxGridItem *firstItem = d->firstVisibleItem();

    if (from < to && from < d->visibleIndex && to > d->visibleIndex)
        removedBeforeVisible = true;

    QList<FxGridItem*>::Iterator it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxGridItem *item = *it;
        if (item->index >= from && item->index < from + count) {
            // take the items that are moving
            item->index += (to-from);
            moved.insert(item->index, item);
            it = d->visibleItems.erase(it);
            if (item->rowPos() < firstItem->rowPos())
                removedBeforeVisible = true;
        } else {
            if (item->index > from && item->index != -1) {
                // move everything after the moved items.
                item->index -= count;
                if (item->index < d->visibleIndex)
                    d->visibleIndex = item->index;
            } else if (item->index != -1) {
                removedBeforeVisible = true;
            }
            ++it;
        }
    }

    int remaining = count;
    int endIndex = d->visibleIndex;
    it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxGridItem *item = *it;
        if (remaining && item->index >= to && item->index < to + count) {
            // place items in the target position, reusing any existing items
            FxGridItem *movedItem = moved.take(item->index);
            if (!movedItem)
                movedItem = d->createItem(item->index);
            it = d->visibleItems.insert(it, movedItem);
            if (it == d->visibleItems.begin() && firstItem)
                movedItem->setPosition(firstItem->colPos(), firstItem->rowPos());
            ++it;
            --remaining;
        } else {
            if (item->index != -1) {
                if (item->index >= to) {
                    // update everything after the moved items.
                    item->index += count;
                }
                endIndex = item->index;
            }
            ++it;
        }
    }

    // If we have moved items to the end of the visible items
    // then add any existing moved items that we have
    while (FxGridItem *item = moved.take(endIndex+1)) {
        d->visibleItems.append(item);
        ++endIndex;
    }

    // update visibleIndex
    for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            break;
        }
    }

    // Fix current index
    if (d->currentIndex >= 0 && d->currentItem) {
        int oldCurrent = d->currentIndex;
        d->currentIndex = d->model->indexOf(d->currentItem->item, this);
        if (oldCurrent != d->currentIndex) {
            d->currentItem->index = d->currentIndex;
            emit currentIndexChanged();
        }
    }

    // Whatever moved items remain are no longer visible items.
    while (moved.count()) {
        int idx = moved.begin().key();
        FxGridItem *item = moved.take(idx);
        if (d->currentItem && item->item == d->currentItem->item)
            item->setPosition(d->colPosAt(idx), d->rowPosAt(idx));
        d->releaseItem(item);
    }

    d->layout();
}

void QSGGridView::modelReset()
{
    Q_D(QSGGridView);
    d->clear();
    refill();
    d->moveReason = QSGGridViewPrivate::SetIndex;
    d->updateCurrent(d->currentIndex);
    if (d->highlight && d->currentItem) {
        d->highlight->setPosition(d->currentItem->colPos(), d->currentItem->rowPos());
        d->updateTrackedItem();
    }
    d->moveReason = QSGGridViewPrivate::Other;

    emit countChanged();
}

void QSGGridView::createdItem(int index, QSGItem *item)
{
    Q_D(QSGGridView);
    if (d->requestedIndex != index) {
        item->setParentItem(this);
        d->unrequestedItems.insert(item, index);
        if (d->flow == QSGGridView::LeftToRight) {
            item->setPos(QPointF(d->colPosAt(index), d->rowPosAt(index)));
        } else {
            item->setPos(QPointF(d->rowPosAt(index), d->colPosAt(index)));
        }
    }
}

void QSGGridView::destroyingItem(QSGItem *item)
{
    Q_D(QSGGridView);
    d->unrequestedItems.remove(item);
}

void QSGGridView::animStopped()
{
    Q_D(QSGGridView);
    d->bufferMode = QSGGridViewPrivate::NoBuffer;
    if (d->haveHighlightRange && d->highlightRange == QSGGridView::StrictlyEnforceRange)
        d->updateHighlight();
}

void QSGGridView::refill()
{
    Q_D(QSGGridView);
    d->refill(d->position(), d->position()+d->size()-1);
}


QSGGridViewAttached *QSGGridView::qmlAttachedProperties(QObject *obj)
{
    return new QSGGridViewAttached(obj);
}

QT_END_NAMESPACE
