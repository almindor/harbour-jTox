#ifndef CLIPBOARDADAPTER_H
#define CLIPBOARDADAPTER_H

#include <QObject>
#include <QClipboard>

namespace JTOX {

    class ClipboardAdapter : public QObject
    {
        Q_OBJECT
    public:
        explicit ClipboardAdapter(QObject *parent = 0);

        Q_INVOKABLE void setText(QString text);
    private:
        QClipboard* fClipboard;
    };

}

#endif // CLIPBOARDADAPTER_H
