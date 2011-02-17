TARGET  = qmlparticlesplugin
TARGETPATH = Qt/labs/particles
include(../qimportbase.pri)

HEADERS += \
    spriteparticles.h \
    spritestate.h \
    V1/qdeclarativeparticles_p.h \
    pluginmain.h

SOURCES += \
    spriteparticles.cpp \
    spritestate.cpp \
    V1/qdeclarativeparticles.cpp \
    main.cpp

QT += declarative opengl


OTHER_FILES += \
    qmldir

RESOURCES += \
    spriteparticles.qrc

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

symbian:{
    TARGET.UID3 = 0x2002131E
    
    isEmpty(DESTDIR):importFiles.files = qmlparticlesplugin$${QT_LIBINFIX}.dll qmldir
    else:importFiles.files = $$DESTDIR/qmlparticlesplugin$${QT_LIBINFIX}.dll qmldir
    importFiles.path = $$QT_IMPORTS_BASE_DIR/$$TARGETPATH
    
    DEPLOYMENT = importFiles
}

INSTALLS += target qmldir
