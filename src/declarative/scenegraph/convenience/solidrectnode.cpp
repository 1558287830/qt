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

#include "solidrectnode.h"
#include "utilities.h"
#include "flatcolormaterial.h"


SolidRectNode::SolidRectNode(const QRectF &rect, const QColor &color, qreal opacity)
{
    setRect(rect);
    m_material.setColor(color);
    m_material.setOpacity(opacity);
    setMaterial(&m_material);
}

SolidRectNode::SolidRectNode(qreal x, qreal y, qreal w, qreal h, const QColor &color, qreal opacity)
{
    setRect(QRectF(x, y, w, h));
    m_material.setColor(color);
    m_material.setOpacity(opacity);
    setMaterial(&m_material);
}

void SolidRectNode::updateGeometry()
{
    Geometry *g = geometry();
    if (g->isNull()) {
        updateGeometryDescription(Utilities::getRectGeometryDescription(), GL_UNSIGNED_SHORT);
    }
    Utilities::setupRectGeometry(g, m_rect);
    markDirty(Node::DirtyGeometry);

    setBoundingRect(m_rect);
}

void SolidRectNode::setRect(const QRectF &rect)
{
    m_rect = rect;
    updateGeometry();
}

void SolidRectNode::setColor(const QColor &color)
{
    if (color != m_material.color()) {
        m_material.setColor(color);
        setMaterial(&m_material); // Indicate that the material has changed.
    }
}



