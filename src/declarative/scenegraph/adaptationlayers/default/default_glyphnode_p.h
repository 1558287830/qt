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

#ifndef TEXTMASKMATERIAL_H
#define TEXTMASKMATERIAL_H

#include <material.h>
#include <qshareddata.h>
#include <utilities.h>

class QFontEngineGlyphCache;
class QGLTextureGlyphCache;
class QFontEngine;
class Geometry;
class TextMaskMaterial: public AbstractEffect
{
public:
    TextMaskMaterial(QFontEngine *fontEngine);
    ~TextMaskMaterial();

    virtual AbstractEffectType *type() const;
    virtual AbstractEffectProgram *createProgram() const;
    virtual int compare(const AbstractEffect *other) const;

    void setColor(const QColor &color) { m_color = color; }
    const QColor &color() const { return m_color; }

    const QSGTextureRef &texture() const { return m_texture; }

    void updateGlyphCache(const QGLContext *context);

    int cacheTextureWidth() const;
    int cacheTextureHeight() const;

    void setOpacity(qreal opacity) { m_opacity = opacity; }
    qreal opacity() const { return m_opacity; }

    bool ensureUpToDate();

    QGLTextureGlyphCache *glyphCache() const;
    void populate(const QPointF &position,
                  const QVector<quint32> &glyphIndexes, const QVector<QPointF> &glyphPositions,
                  Geometry *geometry, QRectF *boundingRect, QPointF *baseLine);

private:
    void init();

    QSGTextureRef m_texture;
    QExplicitlySharedDataPointer<QFontEngineGlyphCache> m_glyphCache;
    QFontEngine *m_fontEngine;
    QFontEngine *m_originalFontEngine;
    QColor m_color;
    qreal m_opacity;
    QSize m_size;
};

#endif // TEXTMASKMATERIAL_H
