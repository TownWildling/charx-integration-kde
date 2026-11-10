#include <KIO/ThumbnailCreator>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <QCoreApplication>
#include <QImage>
#include <QDebug>
#include <memory>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 4)
        return 2;
    const auto loaded = KPluginFactory::instantiatePlugin<KIO::ThumbnailCreator>(KPluginMetaData(QString::fromLocal8Bit(argv[1])));
    if (!loaded) {
        qCritical() << loaded.errorString;
        return 3;
    }
    std::unique_ptr<KIO::ThumbnailCreator> creator(loaded.plugin);
    const KIO::ThumbnailRequest request(QUrl::fromLocalFile(QString::fromLocal8Bit(argv[2])),
                                        QSize(256, 256), QStringLiteral("application/x-charx"), 1, 0);
    const auto result = creator->create(request);
    if (!result.isValid())
        return 4;
    const auto image = result.image();
    if (image.width() > 256 || image.height() > 256 || !image.save(QString::fromLocal8Bit(argv[3])))
        return 5;
    qInfo() << "Thumbnail:" << image.size();
    return 0;
}
