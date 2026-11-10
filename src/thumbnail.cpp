#include <KIO/ThumbnailCreator>
#include <KPluginFactory>
#include "pipeline.h"

class CharxThumbnail final : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    CharxThumbnail(QObject *parent, const QVariantList &args)
        : KIO::ThumbnailCreator(parent, args) {}

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override
    {
        const QSize target = request.targetSize();
        if (!request.url().isLocalFile() || !target.isValid() || target.width() > 4096 || target.height() > 4096)
            return KIO::ThumbnailResult::fail();
        const auto bytes = Charx::readIcon(request.url().toLocalFile());
        if (bytes.isEmpty())
            return KIO::ThumbnailResult::fail();
        const auto image = Charx::decodeIcon(bytes, target);
        if (image.isNull())
            return KIO::ThumbnailResult::fail();
        return KIO::ThumbnailResult::pass(image);
    }
};
K_PLUGIN_CLASS_WITH_JSON(CharxThumbnail, "charxthumbnail.json")
#include "thumbnail.moc"
