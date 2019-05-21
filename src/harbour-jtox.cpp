/*
    Copyright (C) 2016 Ales Katona.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QString>
#include <sodium.h>
#include "toxcore.h"
#include "toxcoreav.h"
#include "encryptsave.h"
#include "friendmodel.h"
#include "eventmodel.h"
#include "toxme.h"
#include "requestmodel.h"
#include "avatarprovider.h"
#include "dbdata.h"
#include "dirmodel.h"
#include "utils.h"

//#include "pa/context.h"
//#include "pa/sinkportmodel.h"
#include "audiorouter.h"

using namespace JTOX;

int main(int argc, char *argv[])
{
    int result = 0;
    if ( sodium_init() < 0 ) {
        qDebug() << "Error on sodium init";
        return -1;
    }

    register_signals();

    qputenv("NEMO_RESOURCE_CLASS_OVERRIDE", "call");

    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();

    // pulseaudio
//    PA::Context context;
//    PA::SinkPortModel sinkPortModel;

//    QObject::connect(&context, &PA::Context::infoReady, &sinkPortModel, &PA::SinkPortModel::onInfoReady, Qt::QueuedConnection);
//    context.connectToServer();

    // audio routing
//    AudioRouter audioRouter;

    // jTox stuff
    DirModel dirModel;
    EncryptSave encryptSave;
    DBData dbData(encryptSave);
    ToxCore toxCore(encryptSave, dbData);
    ToxCoreAV toxCoreAV(toxCore);
    Toxme toxme(toxCore, encryptSave);
    AvatarProvider* avatarProvider = new AvatarProvider(toxCore, dbData); // freed internally by QT5!
    FriendModel friendModel(toxCore, dbData, avatarProvider);
    EventModel eventModel(toxCore, friendModel, dbData);
    RequestModel requestModel(toxCore, toxme, friendModel, dbData);

    // APP -> ToxCore and others for cleanup
    QObject::connect(app, &QGuiApplication::lastWindowClosed, &toxCoreAV, &ToxCoreAV::beforeToxKill, Qt::DirectConnection); // cleanup before we go destructors!
    // ToxCore -> ToxCoreAV needs to make sure init/kill are followed up properly
    QObject::connect(&toxCore, &ToxCore::clientReset, &toxCoreAV, &ToxCoreAV::onToxInitDone, Qt::DirectConnection); // Ensure we setup AV when tox* is done
    QObject::connect(&toxCore, &ToxCore::beforeToxKill, &toxCoreAV, &ToxCoreAV::beforeToxKill, Qt::DirectConnection); // Ensure we clean up BEFORE toxcore continues the kill
    // ToxCoreAV -> EventModel
    QObject::connect(&toxCoreAV, &ToxCoreAV::outgoingCall, &eventModel, &EventModel::onOutgoingCall);
    QObject::connect(&toxCoreAV, &ToxCoreAV::incomingCall, &eventModel, &EventModel::onIncomingCall);
    QObject::connect(&toxCoreAV, &ToxCoreAV::callStateChanged, &eventModel, &EventModel::onCallStateChanged);
    // AvatarProvider -> FriendModel
    QObject::connect(avatarProvider, &AvatarProvider::profileAvatarChanged, &friendModel, &FriendModel::onProfileAvatarChanged); // update friends when we set a new avatar

    QString qml = QString("qml/%1.qml").arg("harbour-jtox");
    view->rootContext()->setContextProperty("toxcore", &toxCore);
    view->rootContext()->setContextProperty("toxcoreav", &toxCoreAV);
    view->rootContext()->setContextProperty("friendmodel", &friendModel);
    view->rootContext()->setContextProperty("eventmodel", &eventModel);
    view->rootContext()->setContextProperty("toxme", &toxme);
    view->rootContext()->setContextProperty("requestmodel", &requestModel);
    view->rootContext()->setContextProperty("dirmodel", &dirModel);
//    view->rootContext()->setContextProperty("audioRouter", &audioRouter);
    view->rootContext()->setContextProperty("avatarProvider", avatarProvider);

//    view->rootContext()->setContextProperty("sinkPortModel", &sinkPortModel);
    view->engine()->addImageProvider("avatarProvider", avatarProvider); // freed internally by Qt5!

    view->setSource(SailfishApp::pathTo(qml));
    view->show();

    result = app->exec();

    delete view;
    delete app;

    return result;
}
