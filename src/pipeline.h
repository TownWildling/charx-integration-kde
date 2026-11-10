#pragma once
#include <QByteArray>
#include <QImage>
#include <QString>
#include <QSize>
namespace Charx {
QByteArray readIcon(const QString &path);
QImage decodeIcon(QByteArray bytes, const QSize &target);
}
