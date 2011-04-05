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


#ifndef DEFAULT_PIXMAPNODE_H
#define DEFAULT_PIXMAPNODE_H

#include <private/qsgadaptationlayer_p.h>

#include "qsgtexturematerial.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGTextureProviderMaterial : public QSGMaterial
{
public:
    QSGTextureProviderMaterial()
        : m_texture(0)
    {
    }

    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
    virtual int compare(const QSGMaterial *other) const;

    void setTexture(QSGTextureProvider *texture);
    QSGTextureProvider *texture() const { return m_texture; }

    void setBlending(bool enable);
    bool blending() const;

    static bool is(const QSGMaterial *effect);

protected:
    QSGTextureProvider *m_texture;
};


class QSGTextureProviderMaterialWithOpacity : public QSGTextureProviderMaterial
{
public:
    QSGTextureProviderMaterialWithOpacity() { }

    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
    void setTexture(QSGTextureProvider *texture);

    static bool is(const QSGMaterial *effect);
};


class QSGDefaultImageNode : public QSGImageNode
{
public:
    QSGDefaultImageNode();
    virtual void setTargetRect(const QRectF &rect);
    virtual void setSourceRect(const QRectF &rect);
    virtual void setTexture(QSGTextureProvider *texture);
    virtual void update();

    virtual void preprocess();

private:
    void updateGeometry();

    QSGTextureProvider *m_texture;
    QRectF m_targetRect;
    QRectF m_sourceRect;

    QSGTextureProviderMaterial m_material;
    QSGTextureProviderMaterialWithOpacity m_materialO;

    uint m_dirtyGeometry : 1;

    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
