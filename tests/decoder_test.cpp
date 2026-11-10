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
    QImage portrait(1200, 800, QImage::Format_RGB32);
    portrait.fill(Qt::red);
    QByteArray rotatedJpeg;
    QBuffer jpegBuffer(&rotatedJpeg);
    if (!jpegBuffer.open(QIODevice::WriteOnly) || !portrait.save(&jpegBuffer, "JPEG"))
        return 5;
    // APP1 with a little-endian EXIF orientation tag set to 6 (90° clockwise).
    rotatedJpeg.insert(2, QByteArray::fromHex(
        "ffe1002245786966000049492a0008000000010012010300010000000600000000000000"));
    const auto rotated = Charx::decodeIcon(rotatedJpeg, QSize(100, 300));
    if (rotated.size() != QSize(100, 150)) {
        qCritical() << "Unexpected rotated thumbnail size:" << rotated.size();
        return 6;
    }
    for (const QSize size : {QSize(4096, 16), QSize(16, 4096)}) {
        QImage thin(size, QImage::Format_RGB32);
        thin.fill(Qt::blue);
        QByteArray bytes;
        QBuffer buffer(&bytes);
        if (!buffer.open(QIODevice::WriteOnly) || !thin.save(&buffer, "PNG"))
            return 7;
        const QSize expected = size.width() > size.height() ? QSize(64, 1) : QSize(1, 64);
        const auto thumbnail = Charx::decodeIcon(bytes, QSize(64, 64));
        if (thumbnail.size() != expected) {
            qCritical() << "Unexpected thin thumbnail size:" << thumbnail.size();
            return 8;
        }
    }
    return 0;
}
