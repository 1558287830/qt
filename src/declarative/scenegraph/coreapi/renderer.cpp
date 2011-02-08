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

#include "renderer.h"
#include "node.h"
#include "material.h"

#include "adaptationlayer.h"

#include <qsgattributevalue.h>
#include <QGLShaderProgram>
#include <qglframebufferobject.h>
#include <QtGui/qapplication.h>

#include <qdatetime.h>

//#define RENDERER_DEBUG
//#define QT_GL_NO_SCISSOR_TEST

BindableFbo::BindableFbo(QGLContext *ctx, QGLFramebufferObject *fbo) : m_ctx(ctx), m_fbo(fbo)
{
}

void BindableFbo::bind() const
{
    m_ctx->makeCurrent();
    m_fbo->bind();
}


Renderer::Renderer()
    : QObject()
    , m_clear_color(Qt::transparent)
    , m_root_node(0)
    , m_changed_emitted(false)
{
    Q_ASSERT(QGLContext::currentContext());
    initializeGLFunctions();
}

Renderer::~Renderer()
{
    setRootNode(0);
}

void Renderer::setRootNode(RootNode *node)
{
    if (m_root_node == node)
        return;
    if (m_root_node) {
        m_root_node->m_renderers.removeOne(this);
        nodeChanged(m_root_node, Node::DirtyNodeRemoved);
    }
    m_root_node = node;
    if (m_root_node) {
        Q_ASSERT(!m_root_node->m_renderers.contains(this));
        m_root_node->m_renderers << this;
        nodeChanged(m_root_node, Node::DirtyNodeAdded);
    }
}

void Renderer::renderScene()
{
    class B : public Bindable
    {
    public:
        B() : m_ctx(const_cast<QGLContext *>(QGLContext::currentContext())) { }
        void bind() const { m_ctx->makeCurrent(); QGLFramebufferObject::bindDefault(); }
    private:
        QGLContext *m_ctx;
    } b;
    renderScene(b);
}

void Renderer::renderScene(const Bindable &bindable)
{
    if (!m_root_node)
        return;

//    QTime time;
//    time.start();

    preprocess();
    bindable.bind();
    GeometryDataUploader::bind();
    GeometryDataUploader::upload();
    render();
    GeometryDataUploader::release();
    m_changed_emitted = false;

//    printf("rendering time: %d\n", time.elapsed());
}

void Renderer::setProjectMatrixToDeviceRect()
{
    setProjectMatrixToRect(m_device_rect);
}

void Renderer::setProjectMatrixToRect(const QRectF &rect)
{
    QMatrix4x4 matrix;
    matrix.ortho(rect.x(),
                 rect.x() + rect.width(),
                 rect.y() + rect.height(),
                 rect.y(),
                 qreal(0.01),
                 -1);
    setProjectMatrix(matrix);
}

void Renderer::setClearColor(const QColor &color)
{
    m_clear_color = color;
}

void Renderer::setTexture(int unit, const QSGTextureRef &texture)
{
    if (unit < 0)
        return;

    // Select the texture unit and bind the texture.
    glActiveTexture(GL_TEXTURE0 + unit);
    if (texture.isNull()) {
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, texture->textureId());
    }

    if (unit != 0)
        glActiveTexture(GL_TEXTURE0);
}

AbstractMaterialShader *Renderer::prepareMaterial(AbstractMaterial *material)
{
    AbstractMaterialType *type = material->type();
    AbstractMaterialShader *shader = m_materials.value(type);
    if (shader)
        return shader;

    shader = material->createShader();
    m_materials[type] = shader;
    return shader;
}

void Renderer::nodeChanged(Node *node, Node::DirtyFlags flags)
{
    Q_UNUSED(node);
    Q_UNUSED(flags);

    if (flags & Node::DirtyNodeAdded)
        addNodesToPreprocess(node);
    if (flags & Node::DirtyNodeRemoved)
        removeNodesToPreprocess(node);

    if (!m_changed_emitted) {
        // Premature overoptimization to avoid excessive signal emissions
        m_changed_emitted = true;
        emit sceneGraphChanged();
    }
}

void Renderer::materialChanged(GeometryNode *, AbstractMaterial *, AbstractMaterial *)
{
}

void Renderer::preprocess()
{
    Q_ASSERT(m_root_node);

    // ### Because opacity is now in scene graph, we need to do an extra pass here before
    // preprocess, which potentially means a second pass afterwards. The entire conecpt
    // of preprocess does not work anymore, so it needs replacing, so live with the sub-optimal
    // solution for now.
    m_root_node->updateDirtyStates();

    for (QSet<Node *>::const_iterator it = m_nodes_to_preprocess.constBegin();
         it != m_nodes_to_preprocess.constEnd(); ++it) {
        Node *n = *it;
        Node *p = n;
        while (p != m_root_node) {
            if (p->isSubtreeBlocked()) {
                n = 0;
                break;
            }
            p = p->parent();
        }
        if (n) {
            n->preprocess();
        }
    }

    m_root_node->updateDirtyStates();
}

void Renderer::addNodesToPreprocess(Node *node)
{
    for (int i = 0; i < node->childCount(); ++i)
        addNodesToPreprocess(node->childAtIndex(i));
    if (node->flags() & Node::UsePreprocess)
        m_nodes_to_preprocess.insert(node);
}

void Renderer::removeNodesToPreprocess(Node *node)
{
    for (int i = 0; i < node->childCount(); ++i)
        removeNodesToPreprocess(node->childAtIndex(i));
    if (node->flags() & Node::UsePreprocess)
        m_nodes_to_preprocess.remove(node);
}

Renderer::ClipType Renderer::updateStencilClip(const ClipNode *clip)
{
    if (!clip) {
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        return NoClip;
    }

    bool stencilEnabled = false;
    bool scissorEnabled = false;

    glDisable(GL_SCISSOR_TEST);

    int clipDepth = 0;
    QRect clipRect;
    while (clip) {
        QMatrix4x4 matrix = m_projectionMatrix.top();
        if (clip->matrix())
            matrix *= *clip->matrix();

        const QMatrix4x4 &m = matrix;

        // TODO: Check for multisampling and pixel grid alignment.
        bool canUseScissor = (clip->flags() & Node::ClipIsRectangular)
                           && qFuzzyIsNull(m(0, 1)) && qFuzzyIsNull(m(0, 2))
                           && qFuzzyIsNull(m(1, 0)) && qFuzzyIsNull(m(1, 2));

        if (canUseScissor) {
            QRectF bbox = clip->boundingRect();
            qreal invW = 1 / m(3, 3);
            qreal fx1 = (bbox.left() * m(0, 0) + m(0, 3)) * invW;
            qreal fy1 = (bbox.bottom() * m(1, 1) + m(1, 3)) * invW;
            qreal fx2 = (bbox.right() * m(0, 0) + m(0, 3)) * invW;
            qreal fy2 = (bbox.top() * m(1, 1) + m(1, 3)) * invW;

            GLint ix1 = qRound((fx1 + 1) * m_device_rect.width() * qreal(0.5));
            GLint iy1 = qRound((fy1 + 1) * m_device_rect.height() * qreal(0.5));
            GLint ix2 = qRound((fx2 + 1) * m_device_rect.width() * qreal(0.5));
            GLint iy2 = qRound((fy2 + 1) * m_device_rect.height() * qreal(0.5));

            if (!scissorEnabled) {
                clipRect = QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
                glEnable(GL_SCISSOR_TEST);
                scissorEnabled = true;
            } else {
                clipRect &= QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
            }

            if (clipRect.width() > 0 && clipRect.height() > 0)
                glScissor(clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
            else
                glScissor(0, 0, 0, 0);
        } else {
            if (!stencilEnabled) {
                if (!m_clip_program.isLinked()) {
                    m_clip_program.addShaderFromSourceCode(QGLShader::Vertex,
                        "attribute highp vec4 vCoord;       \n"
                        "uniform highp mat4 matrix;         \n"
                        "void main() {                      \n"
                        "    gl_Position = matrix * vCoord; \n"
                        "}");
                    m_clip_program.addShaderFromSourceCode(QGLShader::Fragment,
                        "void main() {                                   \n"
                        "    gl_FragColor = vec4(0.81, 0.83, 0.12, 1.0); \n" // Trolltech green ftw!
                        "}");
                    m_clip_program.bindAttributeLocation("vCoord", 0);
                    m_clip_program.link();
                    m_clip_matrix_id = m_clip_program.uniformLocation("matrix");
                }

                glStencilMask(0xff); // write mask
                glClearStencil(0);
                glClear(GL_STENCIL_BUFFER_BIT);
                glEnable(GL_STENCIL_TEST);
                glDisable(GL_DEPTH_TEST);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glDepthMask(GL_FALSE);

                m_clip_program.bind();
                m_clip_program.enableAttributeArray(0);

                stencilEnabled = true;
            }

            glStencilFunc(GL_EQUAL, clipDepth, 0xff); // stencil test, ref, test mask
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); // stencil fail, z fail, z pass

            Geometry *geometry = clip->geometry();

            const QSGAttributeValue &v = geometry->attributeValue(0);
            glVertexAttribPointer(0, v.tupleSize(), v.type(), GL_FALSE, v.stride(),
                                  GeometryDataUploader::vertexData(geometry));

            m_clip_program.setUniformValue(m_clip_matrix_id, m);

            QPair<int, int> range = clip->indexRange();
            if (geometry->indexCount()) {
                glDrawElements(GLenum(geometry->drawingMode()), range.second - range.first, geometry->indexType()
                               , GeometryDataUploader::indexData(geometry));
            } else {
                glDrawArrays(GLenum(geometry->drawingMode()), range.first, range.second - range.first);
            }

            ++clipDepth;
        }

        clip = clip->clipList();
    }

    if (stencilEnabled) {
        m_clip_program.disableAttributeArray(0);
        glEnable(GL_DEPTH_TEST);
        glStencilFunc(GL_EQUAL, clipDepth, 0xff); // stencil test, ref, test mask
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // stencil fail, z fail, z pass
        glStencilMask(0); // write mask
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        //glDepthMask(GL_TRUE); // must be reset correctly by caller.
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    if (!scissorEnabled)
        glDisable(GL_SCISSOR_TEST);

    return stencilEnabled ? StencilClip : ScissorClip;
}
