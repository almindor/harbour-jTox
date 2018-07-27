# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = harbour-jtox

QT += sql

CONFIG += sailfishapp
CONFIG(debug,debug|release){ TOX_PATH = extra/i486 }
# CONFIG(debug,debug|release){ TOX_PATH = extra/armv7hl }
CONFIG(release,debug|release){ TOX_PATH = extra/armv7hl }

QMAKE_LFLAGS += -Wl,-lc -Wl,-lz

INCLUDEPATH += $$TOX_PATH/include

SOURCES += \
    src/c_callbacks.cpp \
    src/toxme.cpp \
    src/utils.cpp \
    src/friend.cpp \
    src/friendmodel.cpp \
    src/toxcore.cpp \
    src/event.cpp \
    src/eventmodel.cpp \
    src/encryptsave.cpp \
    src/requestmodel.cpp \
    src/friendrequest.cpp \
    src/dbdata.cpp \
    src/harbour-jtox.cpp \
    src/dirmodel.cpp \
    src/avatarprovider.cpp

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    translations/*.ts \
    extra/nodes.json

nodes.files += extra/nodes.json
nodes.path = /usr/share/$${TARGET}/nodes

# register libraries to variable lib
lib.files += $$TOX_PATH/lib/libsodium.a \
$$TOX_PATH/lib/libtoxcore.a \
$$TOX_PATH/lib/libtoxencryptsave.a
# $$TOX_PATH/lib/libtoxdns.a

# linking
LIBS += \
-L$$PWD/$$TOX_PATH/lib \
-ltoxencryptsave \
-ltoxcore \
-lsodium
# -ltoxdns \

# set install path and register the variable to INSTALLS
lib.path = /usr/share/$${TARGET}/lib
INSTALLS += lib nodes

SAILFISHAPP_ICONS = 86x86 108x108 128x128 256x256

# to disable building translations every time, comment out the
# following CONFIG line
CONFIG += sailfishapp_i18n

TRANSLATIONS += \
    translations/harbour-jtox-de.ts \
    translations/harbour-jtox-es.ts \
    translations/harbour-jtox-fr_FR.ts \
    translations/harbour-jtox-sv.ts \
    translations/harbour-jtox-ru.ts \

HEADERS += \
    src/c_callbacks.h \
    src/toxme.h \
    src/utils.h \
    src/friend.h \
    src/friendmodel.h \
    src/toxcore.h \
    src/event.h \
    src/eventmodel.h \
    src/encryptsave.h \
    src/requestmodel.h \
    src/friendrequest.h \
    src/dbdata.h \
    src/dirmodel.h \
    src/avatarprovider.h

DISTFILES += \
    qml/pages/About.qml \
    qml/pages/Account.qml \
    qml/pages/Friends.qml \
    qml/pages/Chat.qml \
    qml/components/FriendItem.qml \
    qml/components/UserStatusIndicator.qml \
    qml/components/QRCode.qml \
    qml/js/qqr.js \
    qml/pages/Messages.qml \
    qml/components/MessageItem.qml \
    qml/components/RequestItem.qml \
    qml/pages/Requests.qml \
    qml/pages/Import.qml \
    qml/pages/QRID.qml \
    qml/pages/AddFriend.qml \
    qml/pages/Password.qml \
    qml/components/UserTypingIndicator.qml \
    qml/components/InputItem.qml \
    rpm/harbour-jtox.changes.in \
    rpm/harbour-jtox.spec \
    rpm/harbour-jtox.yaml \
    harbour-jtox.desktop \
    qml/harbour-jtox.qml \
    qml/pages/Details.qml \
    qml/js/common.js \
    qml/components/MessageItemText.qml \
    qml/components/MessageItemFile.qml \
    qml/components/AltInputItem.qml \
    qml/components/Avatar.qml
