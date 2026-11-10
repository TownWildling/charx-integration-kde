#include "pipeline.h"
#include "paths.h"
#include <QProcess>
#include <QElapsedTimer>

QByteArray Charx::readIcon(const QString &path)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(QStringLiteral(CHARX_PYTHON),
                  {QStringLiteral("-I"), QStringLiteral(CHARX_READER), path});
    if (!process.waitForStarted(1000))
        return {};
    QByteArray bytes;
    QElapsedTimer timer;
    timer.start();
    constexpr qsizetype limit = 24 * 1024 * 1024;
    while (process.state() != QProcess::NotRunning && timer.elapsed() < 7000) {
        process.waitForReadyRead(100);
        bytes += process.readAllStandardOutput();
        process.readAllStandardError();
        if (bytes.size() > limit)
            break;
    }
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(1000);
        return {};
    }
    bytes += process.readAllStandardOutput();
    if (bytes.size() > limit || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return {};
    return bytes;
}
