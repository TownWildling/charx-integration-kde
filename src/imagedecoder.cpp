#include "pipeline.h"
#include <QBuffer>
#include <QImageReader>
#include <QImageIOHandler>

namespace {
QSize fittedSize(const QSize &size, const QSize &bounds)
{
    return size.scaled(bounds, Qt::KeepAspectRatio).expandedTo(QSize(1, 1));
}
}

QImage Charx::decodeIcon(QByteArray bytes, const QSize &target)
{
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    const QByteArray format = reader.format().toLower();
    if (format != "png" && format != "jpeg" && format != "webp")
        return {};
    const QSize size = reader.size();
    if (!size.isValid() || size.width() > 16384 || size.height() > 16384
        || qint64(size.width()) * size.height() > 32 * 1024 * 1024)
        return {};
    reader.setAutoTransform(true);
    const QSize decodeBounds = (reader.transformation() & QImageIOHandler::TransformationRotate90)
        ? target.transposed() : target;
    reader.setScaledSize(fittedSize(size, decodeBounds));
    QImage image = reader.read();
    if (image.isNull())
        return {};
    return image.scaled(fittedSize(image.size(), target), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
