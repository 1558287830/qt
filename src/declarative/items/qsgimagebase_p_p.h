// Commit: e17a5398bf20b89834d4d6c7f4d9203f192b101f
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

#ifndef QSGIMAGEBASE_P_P_H
#define QSGIMAGEBASE_P_P_H

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

#include "qsgitem_p.h"

#include <private/qdeclarativepixmapcache_p.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;
class QSGImageBasePrivate : public QSGItemPrivate
{
    Q_DECLARE_PUBLIC(QSGImageBase)

public:
    QSGImageBasePrivate()
      : status(QSGImageBase::Null),
        progress(0.0),
        explicitSourceSize(false),
        async(false),
        cache(true),
        mirror(false)
    {
    }

    QDeclarativePixmap pix;
    QSGImageBase::Status status;
    QUrl url;
    qreal progress;
    QSize sourcesize;
    bool explicitSourceSize : 1;
    bool async : 1;
    bool cache : 1;
    bool mirror: 1;
};

QT_END_NAMESPACE

#endif // QSGIMAGEBASE_P_P_H
