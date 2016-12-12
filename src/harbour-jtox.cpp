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
#include "encryptsave.h"
#include "friendmodel.h"
#include "eventmodel.h"
#include "toxme.h"
#include "requestmodel.h"
#include "dbdata.h"
#include "dirmodel.h"
#include "utils.h"

using namespace JTOX;

int main(int argc, char *argv[])
{
    int result = 0;
    if ( sodium_init() < 0 ) {
        qDebug() << "Error on sodium init\n";
        return -1;
    }

    register_signals();

    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();

    DirModel dirModel;
    EncryptSave encryptSave;
    DBData dbData(encryptSave);
    ToxCore toxCore(encryptSave, dbData);
    Toxme toxme(toxCore, encryptSave);
    FriendModel friendModel(toxCore, dbData);
    EventModel eventModel(toxCore, friendModel, dbData);
    RequestModel requestModel(toxCore, toxme, friendModel);

    QString qml = QString("qml/%1.qml").arg("harbour-jtox");
    view->rootContext()->setContextProperty("toxcore", &toxCore);
    view->rootContext()->setContextProperty("friendmodel", &friendModel);
    view->rootContext()->setContextProperty("eventmodel", &eventModel);
    view->rootContext()->setContextProperty("toxme", &toxme);
    view->rootContext()->setContextProperty("requestmodel", &requestModel);
    view->rootContext()->setContextProperty("dirmodel", &dirModel);

    view->setSource(SailfishApp::pathTo(qml));
    view->show();

    result = app->exec();

    delete view;
    delete app;

    return result;
}
