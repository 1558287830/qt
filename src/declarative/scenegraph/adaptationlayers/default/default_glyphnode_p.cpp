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

#include "default_glyphnode_p.h"
#include "geometry.h"

#include <qglshaderprogram.h>

#include <private/qtextureglyphcache_gl_p.h>
#include <private/qfontengine_p.h>
#include <private/qglextensions_p.h>

class TextMaskMaterialData : public AbstractShaderEffectProgram
{
public:
    TextMaskMaterialData();

    virtual void updateRendererState(Renderer *renderer, Renderer::Updates updates);
    virtual void updateEffectState(Renderer *renderer, AbstractEffect *newEffect, AbstractEffect *oldEffect);
private:
    virtual void initialize();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;
    virtual const Attributes attributes() const;

    int m_matrix_id;
    int m_color_id;
    int m_textureScale_id;
    int m_opacity_id;
};

const char *TextMaskMaterialData::vertexShader() const {
    return
        "uniform highp mat4 matrix;                     \n"
        "uniform highp vec2 textureScale;               \n"
        "attribute highp vec4 vCoord;                   \n"
        "attribute highp vec2 tCoord;                   \n"
        "varying highp vec2 sampleCoord;                \n"
        "void main() {                                  \n"
        "     sampleCoord = tCoord * textureScale;      \n"
        "     gl_Position = matrix * vCoord;            \n"
        "}";
}

const char *TextMaskMaterialData::fragmentShader() const {
    return
        "varying highp vec2 sampleCoord;                \n"
        "uniform sampler2D texture;                     \n"
        "uniform lowp vec4 color;                       \n"
        "void main() {                                  \n"
        "    gl_FragColor = color * texture2D(texture, sampleCoord).a; \n"
        "}";
}

const AbstractShaderEffectProgram::Attributes TextMaskMaterialData::attributes() const {
    static const QGL::VertexAttribute ids[] = { QGL::Position, QGL::TextureCoord0, QGL::VertexAttribute(-1) };
    static const char *const names[] = { "vCoord", "tCoord", 0 };
    static const Attributes attr = { ids, names };
    return attr;
}

TextMaskMaterialData::TextMaskMaterialData()
{
}

void TextMaskMaterialData::initialize()
{
    AbstractShaderEffectProgram::initialize();
    m_matrix_id = m_program.uniformLocation("matrix");
    m_color_id = m_program.uniformLocation("color");
    m_textureScale_id = m_program.uniformLocation("textureScale");
}

void TextMaskMaterialData::updateRendererState(Renderer *renderer, Renderer::Updates updates)
{
    if (updates & Renderer::UpdateMatrices)
        m_program.setUniformValue(m_matrix_id, renderer->combinedMatrix());
}

void TextMaskMaterialData::updateEffectState(Renderer *renderer, AbstractEffect *newEffect, AbstractEffect *oldEffect)
{
    Q_ASSERT(oldEffect == 0 || newEffect->type() == oldEffect->type());
    TextMaskMaterial *material = static_cast<TextMaskMaterial *>(newEffect);
    TextMaskMaterial *oldMaterial = static_cast<TextMaskMaterial *>(oldEffect);

    QVector4D color(material->color().redF(), material->color().greenF(),
                    material->color().blueF(), material->color().alphaF());
    color *= material->opacity();

    if (oldMaterial == 0
           || material->color() != oldMaterial->color()
           || material->opacity() != oldMaterial->opacity()) {
        m_program.setUniformValue(m_color_id, color);
    }

    bool updated = material->ensureUpToDate();
    Q_ASSERT(!material->texture().isNull());

    Q_ASSERT(oldMaterial == 0 || !oldMaterial->texture().isNull());
    if (updated
            || oldMaterial == 0
            || oldMaterial->texture()->textureId() != material->texture()->textureId()) {
        m_program.setUniformValue(m_textureScale_id, QVector2D(1.0 / material->cacheTextureWidth(),
                                                               1.0 / material->cacheTextureHeight()));
        renderer->setTexture(0, material->texture());

        // Set the mag/min filters to be linear. We only need to do this when the texture
        // has been recreated.
        if (updated) {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    }
}

TextMaskMaterial::TextMaskMaterial(QFontEngine *fontEngine)
    : m_texture(0), m_glyphCache(), m_fontEngine(fontEngine), m_originalFontEngine(fontEngine), m_opacity(1.0)
{
    Q_ASSERT(m_fontEngine != 0);

#if defined(Q_WS_MAC)
    // A QFont is always backed by a multi font engine on Mac. SInce QGlyphs contains a
    // non-font-merging font at this point, we can assume the multi engine contains a
    // single font engine which represents the font needed to display the glyphs.
    if (m_originalFontEngine->type() == QFontEngine::Multi) {
        m_fontEngine = static_cast<QFontEngineMulti *>(m_originalFontEngine)->engine(0);
    }
#endif

    m_originalFontEngine->ref.ref();
    init();
}

TextMaskMaterial::~TextMaskMaterial()
{
    Q_ASSERT(m_originalFontEngine != 0);
    if (!m_originalFontEngine->ref.deref())
        delete m_originalFontEngine;
}

void TextMaskMaterial::init()
{
    QFontEngineGlyphCache::Type type = QFontEngineGlyphCache::Raster_A8;
    setFlags(Blending);

    m_glyphCache = m_fontEngine->glyphCache(0, type, QTransform());
    if (!m_glyphCache || m_glyphCache->cacheType() != type) {
        m_glyphCache = new QGLTextureGlyphCache(0, type, QTransform());
        m_fontEngine->setGlyphCache(0, m_glyphCache.data());
    }
}

void TextMaskMaterial::populate(const QPointF &p,
                                const QVector<quint32> &glyphIndexes,
                                const QVector<QPointF> &glyphPositions,
                                Geometry *geometry,
                                QRectF *boundingRect,
                                QPointF *baseLine)
{
    QVector<QFixedPoint> fixedPointPositions;
    for (int i=0; i<glyphPositions.size(); ++i)
        fixedPointPositions.append(QFixedPoint::fromPointF(glyphPositions.at(i)));

    QTextureGlyphCache *cache = glyphCache();

    cache->populate(m_fontEngine, glyphIndexes.size(), glyphIndexes.constData(),
                           fixedPointPositions.data());

    int margin = cache->glyphMargin();

    Q_ASSERT(geometry->indexType() == GL_UNSIGNED_SHORT);
    geometry->setVertexCount(glyphIndexes.size() * 4);
    geometry->setIndexCount(glyphIndexes.size() * 6);
    QVector4D *vp = (QVector4D *)geometry->vertexData();
    ushort *ip = geometry->ushortIndexData();

    QPointF position(p.x(), p.y() - m_fontEngine->ascent().toReal());
    bool supportsSubPixelPositions = m_fontEngine->supportsSubPixelPositions();
    for (int i=0; i<glyphIndexes.size(); ++i) {
         QFixed subPixelPosition;
         if (supportsSubPixelPositions)
             subPixelPosition = cache->subPixelPositionForX(QFixed::fromReal(glyphPositions.at(i).x()));

         QTextureGlyphCache::GlyphAndSubPixelPosition glyph(glyphIndexes.at(i), subPixelPosition);
         const QTextureGlyphCache::Coord &c = cache->coords.value(glyph);

         QPointF glyphPosition = glyphPositions.at(i) + position;
         int x = qRound(glyphPosition.x()) + c.baseLineX - margin;
         int y = qRound(glyphPosition.y()) - c.baseLineY - margin;

         *boundingRect |= QRectF(x + margin, y + margin, c.w, c.h);

         float cx1 = x;
         float cx2 = x + c.w;
         float cy1 = y;
         float cy2 = y + c.h;

         float tx1 = c.x;
         float tx2 = (c.x + c.w);
         float ty1 = c.y;
         float ty2 = (c.y + c.h);

         if (baseLine->isNull())
             *baseLine = glyphPosition;

         vp[4 * i + 0] = QVector4D(cx1, cy1, tx1, ty1);
         vp[4 * i + 1] = QVector4D(cx2, cy1, tx2, ty1);
         vp[4 * i + 2] = QVector4D(cx1, cy2, tx1, ty2);
         vp[4 * i + 3] = QVector4D(cx2, cy2, tx2, ty2);

         int o = i * 4;
         ip[6 * i + 0] = o + 0;
         ip[6 * i + 1] = o + 2;
         ip[6 * i + 2] = o + 3;
         ip[6 * i + 3] = o + 3;
         ip[6 * i + 4] = o + 1;
         ip[6 * i + 5] = o + 0;
    }
}

AbstractEffectType *TextMaskMaterial::type() const
{
    static AbstractEffectType type;
    return &type;
}

QGLTextureGlyphCache *TextMaskMaterial::glyphCache() const
{
    return static_cast<QGLTextureGlyphCache*>(m_glyphCache.data());
}

AbstractEffectProgram *TextMaskMaterial::createProgram() const
{
    return new TextMaskMaterialData;
}

int TextMaskMaterial::compare(const AbstractEffect *o) const
{
    Q_ASSERT(o && type() == o->type());
    const TextMaskMaterial *other = static_cast<const TextMaskMaterial *>(o);
    if (m_glyphCache != other->m_glyphCache)
        return m_glyphCache - other->m_glyphCache;
    if (m_opacity != other->m_opacity) {
        qreal o1 = m_opacity, o2 = other->m_opacity;
        return int(o2 < o1) - int(o1 < o2);
    }
    QRgb c1 = m_color.rgba();
    QRgb c2 = other->m_color.rgba();
    return int(c2 < c1) - int(c1 < c2);
}

void TextMaskMaterial::updateGlyphCache(const QGLContext *context)
{
    QGLTextureGlyphCache *cache = glyphCache();
    if (cache->context() != 0 && cache->context() != context) {
        // ### Clean up and change context?
        qWarning("TextMaskMaterial::setContext: Glyph cache already belongs to different context");
    } else {
#if !defined(QT_OPENGL_ES_2)
        QGLContext *ctx = const_cast<QGLContext *>(context);
        bool success = qt_resolve_version_2_0_functions(ctx)
                       && qt_resolve_buffer_extensions(ctx);
        Q_ASSERT(success);
#endif

        if (cache->context() == 0)
            cache->setContext(context);

        cache->fillInPendingGlyphs();
    }
}

bool TextMaskMaterial::ensureUpToDate()
{
    QSize glyphCacheSize(glyphCache()->width(), glyphCache()->height());
    if (glyphCacheSize != m_size) {
        QSGTexture *t = new QSGTexture();
        t->setTextureId(glyphCache()->texture());
        t->setTextureSize(QSize(glyphCache()->width(), glyphCache()->height()));
        t->setStatus(QSGTexture::Ready);
        t->setOwnsTexture(false);
        m_texture = QSGTextureRef(t);

        m_size = glyphCacheSize;

        return true;
    } else {
        return false;
    }
}

int TextMaskMaterial::cacheTextureWidth() const
{
    return glyphCache()->width();
}

int TextMaskMaterial::cacheTextureHeight() const
{
    return glyphCache()->height();
}
