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

#include "default_glyphnode.h"
#include "default_glyphnode_p.h"

#include <qglshaderprogram.h>
#include <private/qfont_p.h>

DefaultGlyphNode::DefaultGlyphNode()
    : m_material(0)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
{
    m_geometry.setDrawingMode(GL_TRIANGLES);
    setGeometry(&m_geometry);
}

DefaultGlyphNode::~DefaultGlyphNode()
{
    delete m_material;
}

void DefaultGlyphNode::setColor(const QColor &color)
{
    m_color = color;
    if (m_material != 0) {
        m_material->setColor(color);
        setMaterial(m_material); // Indicate the material state has changed
    }
}

void DefaultGlyphNode::setGlyphs(const QPointF &position, const QGlyphs &glyphs)
{
    if (m_material != 0)
        delete m_material;

    QFontEngine *fe = QFontPrivate::get(glyphs.font())->engineForScript(QUnicodeTables::Common);
    m_material = new TextMaskMaterial(fe);
    m_material->setColor(m_color);

    QRectF boundingRect;
    m_material->populate(position, glyphs.glyphIndexes(), glyphs.positions(), geometry(),
                         &boundingRect, &m_baseLine);
    setMaterial(m_material);
    setBoundingRect(boundingRect);

    Q_ASSERT(QGLContext::currentContext());
    m_material->updateGlyphCache(QGLContext::currentContext());

    markDirty(DirtyGeometry);

#ifdef QML_RUNTIME_TESTING
    description = QLatin1String("glyphs");
#endif
}
