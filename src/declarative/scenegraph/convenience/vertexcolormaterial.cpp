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

#include "vertexcolormaterial.h"

#include <qglshaderprogram.h>

QT_BEGIN_NAMESPACE

class VertexColorMaterialShader : public AbstractMaterialShader
{
public:
    virtual void updateState(const RenderState &state, AbstractMaterial *newEffect, AbstractMaterial *oldEffect);
    virtual char const *const *attributeNames() const;

    static AbstractMaterialType type;

private:
    virtual void initialize();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    int m_matrix_id;
    int m_opacity_id;
};

AbstractMaterialType VertexColorMaterialShader::type;

void VertexColorMaterialShader::updateState(const RenderState &state, AbstractMaterial *newEffect, AbstractMaterial *)
{
    if (!(newEffect->flags() & AbstractMaterial::Blending) || state.isOpacityDirty())
        m_program.setUniformValue(m_opacity_id, state.opacity());

    if (state.isMatrixDirty())
        m_program.setUniformValue(m_matrix_id, state.combinedMatrix());
}

char const *const *VertexColorMaterialShader::attributeNames() const
{
    static const char *const attr[] = { "vertexCoord", "vertexColor", 0 };
    return attr;
}

void VertexColorMaterialShader::initialize()
{
    m_matrix_id = m_program.uniformLocation("matrix");
    m_opacity_id = m_program.uniformLocation("opacity");
}

const char *VertexColorMaterialShader::vertexShader() const {
    return
        "attribute highp vec4 vertexCoord;              \n"
        "attribute highp vec4 vertexColor;              \n"
        "uniform highp mat4 matrix;                     \n"
        "uniform highp float opacity;                   \n"
        "varying lowp vec4 color;                       \n"
        "void main() {                                  \n"
        "    gl_Position = matrix * vertexCoord;        \n"
        "    color = vertexColor * opacity;             \n"
        "}";
}

const char *VertexColorMaterialShader::fragmentShader() const {
    return
        "varying lowp vec4 color;                       \n"
        "void main() {                                  \n"
        "    gl_FragColor = color;                      \n"
        "}";
}


VertexColorMaterial::VertexColorMaterial(bool opaque) : m_opaque(opaque)
{
    setFlag(Blending, !opaque);
}

void VertexColorMaterial::setOpaque(bool opaque)
{
    setFlag(Blending, !opaque);
    m_opaque = opaque;
}

AbstractMaterialType *VertexColorMaterial::type() const
{
    return &VertexColorMaterialShader::type;
}

AbstractMaterialShader *VertexColorMaterial::createShader() const
{
    return new VertexColorMaterialShader;
}

bool VertexColorMaterial::is(const AbstractMaterial *effect)
{
    return effect->type() == &VertexColorMaterialShader::type;
}

QT_END_NAMESPACE
