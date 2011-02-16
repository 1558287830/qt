#ifndef SPRITESTATE_H
#define SPRITESTATE_H

#include <QObject>
#include <QVariantMap>

class SpriteState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int frames READ frames WRITE setFrames NOTIFY framesChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QVariantMap to READ to WRITE setTo NOTIFY toChanged)

public:
    explicit SpriteState(QObject *parent = 0);

    QString source() const
    {
        return m_source;
    }

    int frames() const
    {
        return m_frames;
    }

    int duration() const
    {
        return m_duration;
    }

    QString name() const
    {
        return m_name;
    }

    QVariantMap to() const
    {
        return m_to;
    }

signals:

    void sourceChanged(QString arg);

    void framesChanged(int arg);

    void durationChanged(int arg);

    void nameChanged(QString arg);

    void toChanged(QVariantMap arg);

public slots:

    void setSource(QString arg)
    {
        if (m_source != arg) {
            m_source = arg;
            emit sourceChanged(arg);
        }
    }

    void setFrames(int arg)
    {
        if (m_frames != arg) {
            m_frames = arg;
            emit framesChanged(arg);
        }
    }

    void setDuration(int arg)
    {
        if (m_duration != arg) {
            m_duration = arg;
            emit durationChanged(arg);
        }
    }

    void setName(QString arg)
    {
        if (m_name != arg) {
            m_name = arg;
            emit nameChanged(arg);
        }
    }

    void setTo(QVariantMap arg)
    {
        if (m_to != arg) {
            m_to = arg;
            emit toChanged(arg);
        }
    }

private:
    friend class SpriteParticles;
    QString m_source;
    int m_frames;
    int m_duration;
    QString m_name;
    QVariantMap m_to;
};

#endif // SPRITESTATE_H
