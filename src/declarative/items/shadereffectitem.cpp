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

#include "shadereffectitem.h"
#include "shadereffectnode.h"

#include "material.h"
#include "qsgitem_p.h"

#include "qsgcontext.h"
#include "subtree.h"
#include "qsgcanvas.h"

#include <QtCore/qsignalmapper.h>
#include <QtOpenGL/qglframebufferobject.h>

static const char qt_default_vertex_code[] =
    "uniform highp mat4 qt_ModelViewProjectionMatrix;               \n"
    "attribute highp vec4 qt_Vertex;                                \n"
    "attribute highp vec2 qt_MultiTexCoord0;                        \n"
    "varying highp vec2 qt_TexCoord0;                               \n"
    "void main() {                                                  \n"
    "    qt_TexCoord0 = qt_MultiTexCoord0;                          \n"
    "    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;    \n"
    "}";

static const char qt_default_fragment_code[] =
    "varying highp vec2 qt_TexCoord0;                       \n"
    "uniform lowp sampler2D source;                         \n"
    "void main() {                                          \n"
    "    gl_FragColor = texture2D(source, qt_TexCoord0);    \n"
    "}";

static const char qt_postion_attribute_name[] = "qt_Vertex";
static const char qt_texcoord_attribute_name[] = "qt_MultiTexCoord0";
static const char qt_emptyAttributeName[] = "";


ShaderEffectItem::ShaderEffectItem(QSGItem *parent)
    : QSGItem(parent)
    , m_mesh_resolution(1, 1)
    , m_blending(true)
    , m_dirtyData(true)
    , m_programDirty(true)
{
    setFlag(QSGItem::ItemHasContents);
}

ShaderEffectItem::~ShaderEffectItem()
{
    reset();
}

void ShaderEffectItem::componentComplete()
{
    updateProperties();
    QSGItem::componentComplete();
}

void ShaderEffectItem::setFragmentShader(const QByteArray &code)
{
    if (m_source.fragmentCode.constData() == code.constData())
        return;
    m_source.fragmentCode = code;
    if (isComponentComplete()) {
        reset();
        updateProperties();
    }
    emit fragmentShaderChanged();
}

void ShaderEffectItem::setVertexShader(const QByteArray &code)
{
    if (m_source.vertexCode.constData() == code.constData())
        return;
    m_source.vertexCode = code;
    if (isComponentComplete()) {
        reset();
        updateProperties();
    }
    emit vertexShaderChanged();
}

void ShaderEffectItem::setBlending(bool enable)
{
    if (blending() == enable)
        return;

    m_blending = enable;
    update();

    emit blendingChanged();
}

void ShaderEffectItem::setMeshResolution(const QSize &size)
{
    if (size == m_mesh_resolution)
        return;

    m_mesh_resolution = size;
    update();
}

void ShaderEffectItem::changeSource(int index)
{
    Q_ASSERT(index >= 0 && index < m_sources.size());
    QVariant v = property(m_sources.at(index).name.constData());
    setSource(v, index);
}

void ShaderEffectItem::markDirty()
{
    m_dirtyData = true;
    update();
}

void ShaderEffectItem::setSource(const QVariant &var, int index)
{
    Q_ASSERT(index >= 0 && index < m_sources.size());

    SourceData &source = m_sources[index];

    if (source.item && source.item->parentItem() == this)
        source.item->setParentItem(0);

    source.source = 0;
    source.item = 0;
    if (var.isNull()) {
        return;
    } else if (!qVariantCanConvert<QObject *>(var)) {
        qWarning("Could not assign source of type '%s' to property '%s'.", var.typeName(), source.name.constData());
        return;
    }

    QObject *obj = qVariantValue<QObject *>(var);

    TextureProviderInterface *int3rface = static_cast<TextureProviderInterface *>(obj->qt_metacast("TextureProviderInterface"));
    if (int3rface) {
        source.source = int3rface->textureProvider();
    } else {
        qWarning("Could not assign property '%s', did not implement TextureProviderInterface.", source.name.constData());
    }

    source.item = qobject_cast<QSGItem *>(obj);

    // TODO: Find better solution.
    // 'source.item' needs a canvas to get a scenegraph node.
    // The easiest way to make sure it gets a canvas is to
    // make it a part of the same item tree as 'this'.
    if (source.item && source.item->parentItem() == 0) {
        source.item->setParentItem(this);
        source.item->setVisible(false);
    }
}

void ShaderEffectItem::disconnectPropertySignals()
{
    disconnect(this, 0, this, SLOT(markDirty()));
    for (int i = 0; i < m_sources.size(); ++i) {
        SourceData &source = m_sources[i];
        disconnect(this, 0, source.mapper, 0);
        disconnect(source.mapper, 0, this, 0);
    }
}

void ShaderEffectItem::connectPropertySignals()
{
    QSet<QByteArray>::const_iterator it;
    for (it = m_source.uniformNames.begin(); it != m_source.uniformNames.end(); ++it) {
        int pi = metaObject()->indexOfProperty(it->constData());
        if (pi >= 0) {
            QMetaProperty mp = metaObject()->property(pi);
            if (!mp.hasNotifySignal())
                qWarning("ShaderEffectItem: property '%s' does not have notification method!", it->constData());
            QByteArray signalName("2");
            signalName.append(mp.notifySignal().signature());
            connect(this, signalName, this, SLOT(markDirty()));
        } else {
            qWarning("ShaderEffectItem: '%s' does not have a matching property!", it->constData());
        }
    }
    for (int i = 0; i < m_sources.size(); ++i) {
        SourceData &source = m_sources[i];
        int pi = metaObject()->indexOfProperty(source.name.constData());
        if (pi >= 0) {
            QMetaProperty mp = metaObject()->property(pi);
            QByteArray signalName("2");
            signalName.append(mp.notifySignal().signature());
            connect(this, signalName, source.mapper, SLOT(map()));
            source.mapper->setMapping(this, i);
            connect(source.mapper, SIGNAL(mapped(int)), this, SLOT(changeSource(int)));
        } else {
            qWarning("ShaderEffectItem: '%s' does not have a matching source!", source.name.constData());
        }
    }
}

void ShaderEffectItem::reset()
{
    disconnectPropertySignals();

    m_source.attributeNames.clear();
    m_source.uniformNames.clear();
    m_source.respectsOpacity = false;
    m_source.respectsMatrix = false;

    for (int i = 0; i < m_sources.size(); ++i) {
        const SourceData &source = m_sources.at(i);
        delete source.mapper;
        if (source.item && source.item->parentItem() == this)
            source.item->setParentItem(0);
    }
    m_sources.clear();

    m_programDirty = true;
}

void ShaderEffectItem::updateProperties()
{
    QByteArray vertexCode = m_source.vertexCode;
    QByteArray fragmentCode = m_source.fragmentCode;
    if (vertexCode.isEmpty())
        vertexCode = qt_default_vertex_code;
    if (fragmentCode.isEmpty())
        fragmentCode = qt_default_fragment_code;

    m_source.attributeNames.fill(0, 3);
    lookThroughShaderCode(vertexCode);
    lookThroughShaderCode(fragmentCode);

    if (!m_source.attributeNames.contains(qt_postion_attribute_name))
        qWarning("ShaderEffectItem: Missing reference to \'%s\'.", qt_postion_attribute_name);
    if (!m_source.attributeNames.contains(qt_texcoord_attribute_name))
        qWarning("ShaderEffectItem: Missing reference to \'%s\'.", qt_texcoord_attribute_name);
    if (!m_source.respectsMatrix)
        qWarning("ShaderEffectItem: Missing reference to \'qt_ModelViewProjectionMatrix\'.");

    for (int i = 0; i < m_sources.size(); ++i) {
        QVariant v = property(m_sources.at(i).name);
        setSource(v, i);
    }

    connectPropertySignals();
}

void ShaderEffectItem::lookThroughShaderCode(const QByteArray &code)
{
    // Regexp for matching attributes and uniforms.
    // In human readable form: attribute|uniform [lowp|mediump|highp] <type> <name>
    static QRegExp re(QLatin1String("\\b(attribute|uniform)\\b\\s*\\b(?:lowp|mediump|highp)?\\b\\s*\\b(\\w+)\\b\\s*\\b(\\w+)"));
    Q_ASSERT(re.isValid());

    int pos = -1;

    QString wideCode = QString::fromLatin1(code.constData(), code.size());

    while ((pos = re.indexIn(wideCode, pos + 1)) != -1) {
        QByteArray decl = re.cap(1).toLatin1(); // uniform or attribute
        QByteArray type = re.cap(2).toLatin1(); // type
        QByteArray name = re.cap(3).toLatin1(); // variable name

        if (decl == "attribute") {
            if (name == qt_postion_attribute_name) {
                m_source.attributeNames[0] = qt_postion_attribute_name;
            } else if (name == "qt_MultiTexCoord0") {
                m_source.attributeNames[1] = qt_texcoord_attribute_name;
                if (m_source.attributeNames.at(0) == 0)
                    m_source.attributeNames[0] = qt_emptyAttributeName;
            } else {
                // TODO: Support user defined attributes.
                qWarning("ShaderEffectItem: Attribute \'%s\' not recognized.", name.constData());
            }
        } else {
            Q_ASSERT(decl == "uniform");

            if (name == "qt_ModelViewProjectionMatrix") {
                m_source.respectsMatrix = true;
            } else if (name == "qt_Opacity") {
                m_source.respectsOpacity = true;
            } else {
                m_source.uniformNames.insert(name);
                if (type == "sampler2D") {
                    SourceData d;
                    d.mapper = new QSignalMapper;
                    d.source = 0;
                    d.name = name;
                    d.item = 0;
                    m_sources.append(d);
                }
            }
        }
    }
}

Node *ShaderEffectItem::updatePaintNode(Node *oldNode, UpdatePaintNodeData *data)
{
    ShaderEffectNode *node = static_cast<ShaderEffectNode *>(oldNode);
    if (!node) {
        node = new ShaderEffectNode;
        m_programDirty = true;
        m_dirtyData = true;
    }

    if (m_programDirty) {
        ShaderEffectProgram s = m_source;
        if (s.fragmentCode.isEmpty())
            s.fragmentCode = qt_default_fragment_code;
        if (s.vertexCode.isEmpty())
            s.vertexCode = qt_default_vertex_code;

        node->setProgramSource(s);
        m_programDirty = false;
    }

    // Update blending
    if (((node->AbstractMaterial::flags() & AbstractMaterial::Blending) != 0) != m_blending) {
        node->ShaderEffectMaterial::setFlag(AbstractMaterial::Blending, m_blending);
        node->markDirty(Node::DirtyMaterial);
    }

    if (node->resolution() != m_mesh_resolution)
        node->setResolution(m_mesh_resolution);

    node->setRect(QRectF(0, 0, width(), height()));

    if (m_dirtyData) {
        QVector<QPair<QByteArray, QVariant> > values;
        QVector<QPair<QByteArray, QPointer<QSGTextureProvider> > > textures;
        for (QSet<QByteArray>::const_iterator it = m_source.uniformNames.begin(); 
             it != m_source.uniformNames.end(); ++it) {
            values.append(qMakePair(*it, property(*it)));
        }
        for (int i = 0; i < m_sources.size(); ++i) {
            const SourceData &source = m_sources.at(i);
            textures.append(qMakePair(source.name, source.source));
        }

        node->setData(values, textures);
        m_dirtyData = false;
    }

    node->update();

    return node;
}

