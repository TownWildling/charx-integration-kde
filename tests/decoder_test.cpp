#include "pipeline.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    for (const char *format : {"PNG", "JPEG", "WEBP"}) {
        QImage original(80, 40, QImage::Format_ARGB32);
        original.fill(QColor(12, 34, 56, 128));
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        if (!original.save(&buffer, format)) {
            qCritical() << "Missing required image codec:" << format;
            return 1;
        }
        const auto image = Charx::decodeIcon(bytes, QSize(32, 32));
        if (image.size() != QSize(32, 16))
            return 2;
        if (QByteArray(format) == "PNG" && image.pixelColor(0, 0).alpha() != 128)
            return 3;
    }
    for (const auto &bytes : {QByteArray(), QByteArray("not an image"),
                              QByteArray("<svg xmlns='http://www.w3.org/2000/svg' width='1' height='1'/>")}) {
        if (!Charx::decodeIcon(bytes, QSize(32, 32)).isNull())
            return 4;
    }
    return 0;
}
