// Commit: ac5c099cc3c5b8c7eec7a49fdeb8a21037230350
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSGIMAGE_P_P_H
#define QSGIMAGE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsgimagebase_p_p.h"
#include "qsgimage_p.h"
#include <private/qsgtextureprovider_p.h>

QT_BEGIN_NAMESPACE

class QSGImagePrivate;
class QSGImageTextureProvider : public QSGTextureProvider
{
public:
    QSGImageTextureProvider(QObject *parent = 0);
    void setImage(const QImage &image);
    QSGTextureRef texture();

    virtual WrapMode horizontalWrapMode() const;
    virtual WrapMode verticalWrapMode() const;
    virtual Filtering filtering() const;
    virtual Filtering mipmapFiltering() const;

    void setHorizontalWrapMode(WrapMode mode);
    void setVerticalWrapMode(WrapMode mode);
    void setFiltering(Filtering filtering);

    QSGTextureRef tex;

private:
    uint m_hWrapMode : 1;
    uint m_vWrapMode : 1;
    uint m_filtering : 2;
};

class QSGImagePrivate : public QSGImageBasePrivate
{
    Q_DECLARE_PUBLIC(QSGImage)

public:
    QSGImagePrivate();

    QSGImage::FillMode fillMode;
    qreal paintedWidth;
    qreal paintedHeight;
    QSGImageTextureProvider *textureProvider;
    void setPixmap(const QPixmap &pix);

    bool pixmapChanged : 1;
};

QT_END_NAMESPACE

#endif // QSGIMAGE_P_P_H
