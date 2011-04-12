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


#include "qsgsimpletexturenode.h"

static void qsgsimpletexturenode_update(QSGGeometry *g,
                                        QSGTexture *texture,
                                        const QRectF &rect,
                                        const QRectF &sourceRect)
{
    if (!texture)
        return;
    QSGGeometry::updateTexturedRectGeometry(g, rect, texture->convertToNormalizedSourceRect(sourceRect));
}

/*!
  \class QSGSimpleTextureNode
  \brief The QSGSimpleTextureNode provided for convenience to easily draw
  textured content using the QML scene graph.

  \warning The simple texture node class must have texture before being
  added to the scene graph to be rendered.
*/

/*!
    Constructs a new simple texture node
 */
QSGSimpleTextureNode::QSGSimpleTextureNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    , m_source_rect(0, 0, 1, 1)
{
    setGeometry(&m_geometry);
    setMaterial(&m_opaque_material);
    setOpaqueMaterial(&m_material);
}


/*!
    Sets the source rectangle in pixel coordinates to \a r.

    The source rectangle specifies which part of the texture is used
    for drawing.
 */
void QSGSimpleTextureNode::setSourceRect(const QRectF &r)
{
    if (m_source_rect == r)
        return;
    m_source_rect = r;
    qsgsimpletexturenode_update(&m_geometry, texture(), m_rect, m_source_rect);
}


/*!
    Returns the source rect in pixel coordinates.
 */
QRectF QSGSimpleTextureNode::sourceRect() const
{
    return m_source_rect;
}


/*!
    Sets the target rect of this texture node to \a r
 */
void QSGSimpleTextureNode::setRect(const QRectF &r)
{
    if (m_rect == r)
        return;
    m_rect = r;
    qsgsimpletexturenode_update(&m_geometry, texture(), m_rect, m_source_rect);
}


/*!
    Returns the target rect of this texture node.
 */
QRectF QSGSimpleTextureNode::rect() const
{
    return m_rect;
}

/*!
    Sets the texture of this texture node to \a texture.

    \warning A texture node must have a texture before being added
    to the scenegraph to be rendered.
 */
void QSGSimpleTextureNode::setTexture(QSGTexture *texture)
{
    if (m_material.texture() == texture)
        return;
    m_material.setTexture(texture);
    m_opaque_material.setTexture(texture);
    qsgsimpletexturenode_update(&m_geometry, texture, m_rect, m_source_rect);
}



/*!
    Returns the texture for this texture node
 */
QSGTexture *QSGSimpleTextureNode::texture() const
{
    return m_material.texture();
}
