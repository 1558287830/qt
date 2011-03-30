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

#ifndef QSGTEXTNODE_P_H
#define QSGTEXTNODE_P_H

#include <node.h>
#include <qsgtext_p.h>

QT_BEGIN_NAMESPACE

class QTextLayout;
class GlyphNodeInterface;
class QTextBlock;
class QColor;
class QTextDocument;
class QSGContext;

class QSGTextNode : public TransformNode
{
public:
    QSGTextNode(QSGContext *);
    ~QSGTextNode();

    static bool isComplexRichText(QTextDocument *);

    void deleteContent();
    void addTextLayout(const QPointF &position, QTextLayout *textLayout, const QColor &color = QColor(),
                       QSGText::TextStyle style = QSGText::Normal, const QColor &styleColor = QColor());
    void addTextDocument(const QPointF &position, QTextDocument *textDocument, const QColor &color = QColor(),
                         QSGText::TextStyle style = QSGText::Normal, const QColor &styleColor = QColor());

private:
    void addTextBlock(const QPointF &position, QTextDocument *textDocument, const QTextBlock &block,
                      const QColor &overrideColor, QSGText::TextStyle style = QSGText::Normal, const QColor &styleColor = QColor());
    GlyphNodeInterface *addGlyphs(const QPointF &position, const QGlyphs &glyphs, const QColor &color,
                                  QSGText::TextStyle style = QSGText::Normal, const QColor &styleColor = QColor());
    void addTextDecorations(const QPointF &position, const QFont &font, const QColor &color,
                            qreal width);

    QSGContext *m_context;
};

QT_END_NAMESPACE

#endif // QSGTEXTNODE_P_H
