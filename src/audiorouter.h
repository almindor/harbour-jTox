#ifndef AUDIOROUTER_H
#define AUDIOROUTER_H

#include <QObject>
#include <resource/qt5/policy/resource-set.h>
#include <resource/qt5/policy/resource.h>
#include <resource/qt5/policy/audio-resource.h>

namespace JTOX {

    class AudioRouter : public QObject
    {
        Q_OBJECT

        ResourcePolicy::ResourceSet fResourceSet;
        bool fReady{false};
    private slots:
        void managerIsUp();
        void resourcesGranted(const QList<ResourcePolicy::ResourceType> &grantedOptionalResources);
        void updateOK() const;
        void resourcesDenied() const;
        void resourcesReleased() const;
        void resourcesReleasedByManager() const;
        void lostResources() const;
        void errorCallback(quint32 code, const char* strError) const;
    signals:
        void resourcesReady() const;
        void errorOccurred(const QString& error) const;
    public:
        explicit AudioRouter(QObject *parent = nullptr);

        Q_INVOKABLE bool setAudioRoute(int route);
    };

}

#endif // AUDIOROUTER_H
