/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt scene graph research project.
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

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <node.h>
#include <renderer.h>

#include <solidrectnode.h>

class NodesTest : public QObject
{
    Q_OBJECT

public:
    NodesTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase() {
        delete widget;
    }

    void propegate();
    void propegateWithMultipleRoots();

    void simulatedEffect_data();
    void simulatedEffect();

private:
    QGLWidget *widget;
};

void NodesTest::initTestCase()
{
    widget = new QGLWidget();
    widget->resize(100, 30);
    widget->show();
}

class DummyRenderer : public Renderer
{
public:
    DummyRenderer(RootNode *root)
        : changedNode(0)
        , changedFlags(0)
        , renderCount(0)
    {
        setRootNode(root);
    }

    void render() {
        ++renderCount;
        renderingOrder = ++globalRendereringOrder;
    }

    void nodeChanged(Node *node, Node::DirtyFlags flags) {
        changedNode = node;
        changedFlags = flags;
        Renderer::nodeChanged(node, flags);
    }

    Node *changedNode;
    Node::DirtyFlags changedFlags;

    int renderCount;
    int renderingOrder;
    static int globalRendereringOrder;
};

int DummyRenderer::globalRendereringOrder;

NodesTest::NodesTest()
{
}


void NodesTest::propegate()
{
    RootNode root;
    Node child;
    root.appendChildNode(&child);

    DummyRenderer renderer(&root);

    child.markDirty(Node::DirtyGeometry);

    QCOMPARE(&child, renderer.changedNode);
    QCOMPARE((int) renderer.changedFlags, (int) Node::DirtyGeometry);
}


void NodesTest::propegateWithMultipleRoots()
{
    RootNode root1;
    Node child2;
    RootNode root3;
    Node child4;

    root1.appendChildNode(&child2);
    child2.appendChildNode(&root3);
    root3.appendChildNode(&child4);

    DummyRenderer ren1(&root1);
    DummyRenderer ren2(&root3);

    child4.markDirty(Node::DirtyGeometry);

    QCOMPARE(ren1.changedNode, &child4);
    QCOMPARE(ren2.changedNode, &child4);

    QCOMPARE((int) ren1.changedFlags, (int) Node::DirtyGeometry);
    QCOMPARE((int) ren2.changedFlags, (int) Node::DirtyGeometry);
}



class SimulatedEffectRenderer : public DummyRenderer
{
public:
    SimulatedEffectRenderer(RootNode *root, BasicGeometryNode *c)
        : DummyRenderer(root)
    {
        child = c;
    }

    void render() {
        matrix = child->matrix() ? *child->matrix() : QMatrix4x4();
        DummyRenderer::render();
    }

    BasicGeometryNode *child;
    QMatrix4x4 matrix;
};


class PseudoEffectNode : public Node {
public:
    PseudoEffectNode(Renderer *r)
        : renderer(r)
    {
        setFlag(UsePreprocess);
    }

    void preprocess() {

        if (renderer->rootNode()->parent()) {
            // Mark the root dirty to build a clean state from the root and down
            renderer->rootNode()->markDirty(Node::DirtyAll);
        }

        renderer->renderScene();

        if (renderer->rootNode()->parent()) {
            // Mark the parent of the root dirty to force the root and down to be updated.
            renderer->rootNode()->parent()->markDirty(Node::DirtyAll);
        }
    }

    Renderer *renderer;
};

void NodesTest::simulatedEffect_data()
{
    QTest::addColumn<bool>("connected");

    QTest::newRow("connected") << true;
    QTest::newRow("disconnected") << false;
}

void NodesTest::simulatedEffect()
{
    QFETCH(bool, connected);

    RootNode root;
    RootNode source;
    SolidRectNode geometry(QRectF(0, 0, 1, 1), Qt::red);
    TransformNode xform;

    SimulatedEffectRenderer rootRenderer(&root, &geometry);
    SimulatedEffectRenderer sourceRenderer(&source, &geometry);

    PseudoEffectNode effect(&sourceRenderer);

    /*
      root              Source is redirected into effect using  the SimulatedEffectRenderer
     /    \
  xform  effect
    |
  source
    |
 geometry
    */

    root.appendChildNode(&xform);
    root.appendChildNode(&effect);
    if (connected)
        xform.appendChildNode(&source);
    source.appendChildNode(&geometry);
    xform.translate(1, 2, 3);

    // Clear all dirty states...
    root.updateDirtyStates();

    rootRenderer.renderScene();

    // compare that we got one render call to each
    QCOMPARE(rootRenderer.renderCount, 1);
    QCOMPARE(sourceRenderer.renderCount, 1);
    QVERIFY(sourceRenderer.renderingOrder < rootRenderer.renderingOrder);
    if (connected) // geometry is not rendered in this case, so skip it...
        QCOMPARE(rootRenderer.matrix, xform.matrix());
    QCOMPARE(sourceRenderer.matrix, QMatrix4x4());
}

QTEST_MAIN(NodesTest);

#include "tst_nodestest.moc"
