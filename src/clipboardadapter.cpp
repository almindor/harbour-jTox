#include "clipboardadapter.h"
#include <QGuiApplication>

namespace JTOX {

    ClipboardAdapter::ClipboardAdapter(QObject* parent) : QObject(parent)
    {
        fClipboard = QGuiApplication::clipboard();
    }

    void ClipboardAdapter::setText(QString text)
    {
        fClipboard->setText(text, QClipboard::Clipboard);
        fClipboard->setText(text, QClipboard::Selection);
    }

}
