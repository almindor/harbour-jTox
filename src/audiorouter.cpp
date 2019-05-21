#include "audiorouter.h"
#include <QDebug>

namespace JTOX {


    void AudioRouter::managerIsUp()
    {
        qDebug() << "[AR] ManagerIsUp";

        fReady = true;
        fResourceSet.acquire();
    }

    void AudioRouter::resourcesGranted(const QList<ResourcePolicy::ResourceType>& grantedOptionalResources)
    {
        qDebug() << "[AR] Resources granted:" << grantedOptionalResources;

        emit resourcesReady();
    }

    void AudioRouter::updateOK() const
    {
        qDebug() << "[AR] UpdateOK";
    }

    void AudioRouter::resourcesDenied() const
    {
        qWarning() << "[AR] Resource request denied";
    }

    void AudioRouter::resourcesReleased() const
    {
        qWarning() << "[AR] Resources released";
    }

    void AudioRouter::resourcesReleasedByManager() const
    {
        qWarning() << "[AR] Resources released by manager";
    }

    void AudioRouter::lostResources() const
    {
        qWarning() << "[AR] Resources lost";
    }

    void AudioRouter::errorCallback(quint32 code, const char* strError) const
    {
        qWarning() << "[AR] error<" << code << ">:" << strError;

        emit errorOccurred(QString::fromLocal8Bit(strError));
    }

    AudioRouter::AudioRouter(QObject *parent) : QObject(parent),
         fResourceSet("call", parent, false, false)
    {
        fResourceSet.addResource(ResourcePolicy::ResourceType::AudioPlaybackType);

        connect(&fResourceSet, &ResourcePolicy::ResourceSet::managerIsUp, this, &AudioRouter::managerIsUp);
        connect(&fResourceSet, &ResourcePolicy::ResourceSet::resourcesGranted, this, &AudioRouter::resourcesGranted);
        connect(&fResourceSet, &ResourcePolicy::ResourceSet::resourcesDenied, this, &AudioRouter::resourcesDenied);
        connect(&fResourceSet, &ResourcePolicy::ResourceSet::resourcesReleased, this, &AudioRouter::resourcesReleased);
        connect(&fResourceSet, &ResourcePolicy::ResourceSet::resourcesReleasedByManager, this, &AudioRouter::resourcesReleasedByManager);
        connect(&fResourceSet, &ResourcePolicy::ResourceSet::lostResources, this, &AudioRouter::lostResources);
        connect(&fResourceSet, &ResourcePolicy::ResourceSet::errorCallback, this, &AudioRouter::errorCallback);

        if (!fResourceSet.initAndConnect()) {
            qWarning() << "[AR] error on ResourceSet initAndConnect";
        }
    }

    bool AudioRouter::setAudioRoute(int route)
    {
        return false;/*

        if (!fReady) {
            return false;
        }

        qDebug() << "[AR] setting route" << route;
        switch (route) {
            case 0: return fResourceSet.acquire();
            case 1: return fResourceSet.release();
        }

        return false;*/
    }

}
