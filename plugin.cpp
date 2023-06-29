#include <simPlusPlus/Plugin.h>
#include "config.h"
#include "plugin.h"
#include "stubs.h"

#include <QString>
#include <QWidget>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class Plugin : public sim::Plugin
{
public:
    void onInit()
    {
        if(!registerScriptStuff())
            throw std::runtime_error("failed to register script stuff");

        setExtVersion("URL Drop Plugin");
        setBuildDate(BUILD_DATE);
    }

    void openURL(openURL_in *in, openURL_out *out)
    {
        QString url = QString::fromStdString(in->url);
#ifdef _WIN32
        QSettings httpSetting("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice", QSettings::Registry64Format);
        QString progId(httpSetting.value("ProgId").toString());
        QSettings openCmd(QStringLiteral("HKEY_CLASSES_ROOT\\%1\\shell\\open\\command").arg(progId), QSettings::Registry64Format);
        QString cmd(openCmd.value("Default").toString().arg(url));
        QStringList args = QProcess::splitCommand(cmd);
        QProcess p;
        p.setProgram(args.takeFirst());
        p.setArguments(args);
        p.startDetached();
#elif __APPLE__
        QString app = "Safari";
        auto useDefaultBrowser = sim::getNamedBoolParam("simURLDrop.useDefaultBrowser");
        if(!useDefaultBrowser || *useDefaultBrowser)
        {
            QSettings launchServ(QDir::homePath() + "/Library/Preferences/com.apple.LaunchServices/com.apple.launchservices.secure.plist", QSettings::NativeFormat);
            QVariantList handlers = launchServ.value("LSHandlers").toList();
            QString browserID = "com.apple.safari";
            for(int i = 0; i < handlers.size(); ++i)
            {
                QVariantMap handlerObj = handlers.at(i).toMap();
                QString handler = handlerObj.value("LSHandlerRoleAll").toString();
                QString contentType = handlerObj.value("LSHandlerContentType").toString();
                QString urlScheme = handlerObj.value("LSHandlerURLScheme").toString();
                if(contentType == "public.html" || urlScheme == "http")
                    browserID = handler;
            }
            if(browserID == "com.apple.safari") app = "Safari";
            else if(browserID == "com.google.chrome") app = "Google Chrome";
            else if(browserID == "org.mozilla.firefox") app = "Firefox";
        }
        QStringList args;
        args << "-l" << "AppleScript" << "-e" << QStringLiteral("tell application \"%1\" to open location \"%2\" & activate application \"%1\"").arg(app).arg(url);
        QProcess p;
        p.start("/usr/bin/osascript", args);
        p.waitForFinished();
#else
        QDesktopServices::openUrl(QUrl(url));
#endif
    }

    void getURL(getURL_in *in, getURL_out *out)
    {
        QEventLoop loop;
        sim::addLog(sim_verbosity_scriptinfos, "downloading %s...", in->url);
        QNetworkAccessManager nam;
        QString url(QString::fromStdString(in->url));
        QNetworkRequest request(url);
        QNetworkReply *reply = nam.get(request);
        std::string errorMsg;
        QObject::connect(reply, &QNetworkReply::downloadProgress, [=] (qint64 bytesReceived, qint64 bytesTotal) {
            sim::addLog(sim_verbosity_infos, "%s: downloaded %d bytes out of %d", in->url, bytesReceived, bytesTotal);
        });
        QObject::connect(reply, &QNetworkReply::finished, [&] {
            QByteArray data = reply->readAll();
            out->data = std::string(data.constData(), data.length());
            reply->deleteLater();
            sim::addLog(sim_verbosity_scriptinfos, "%s: finished downloading %d bytes", in->url, data.size());
            loop.quit();
        });
        QObject::connect(reply, &QNetworkReply::errorOccurred, [&] (QNetworkReply::NetworkError code) {
            sim::addLog(sim_verbosity_scripterrors, "%s: download failed: %s", in->url, reply->errorString().toStdString());
            errorMsg = sim::util::sprintf("download failed: %s", reply->errorString().toStdString());
            loop.quit();
        });
        loop.exec();
        if(!errorMsg.empty())
            throw std::runtime_error(errorMsg);
    }

    void getURLFile(getURLFile_in *in, getURLFile_out *out)
    {
        QEventLoop loop;
        sim::addLog(sim_verbosity_scriptinfos, "downloading %s...", in->url);
        QNetworkAccessManager nam;
        QString url(QString::fromStdString(in->url));
        QNetworkRequest request(url);
        QNetworkReply *reply = nam.get(request);
        std::string errorMsg;
        QObject::connect(reply, &QNetworkReply::downloadProgress, [=] (qint64 bytesReceived, qint64 bytesTotal) {
            sim::addLog(sim_verbosity_infos, "%s: downloaded %d bytes out of %d", in->url, bytesReceived, bytesTotal);
        });
        QObject::connect(reply, &QNetworkReply::finished, [&] {
            QByteArray data = reply->readAll();
            sim::addLog(sim_verbosity_scriptinfos, "%s: finished downloading %d bytes", in->url, data.size());
            std::string path=sim::getStringParam(sim_stringparam_tempdir);
            QUrl url(QString(in->url.c_str()));
            QFileInfo fileInfo(url.path());
            path+="/";
            path+=fileInfo.fileName().toStdString();
            QFile file(QString(path.c_str()));
            if (file.open(QIODevice::WriteOnly))
            {
                qint64 bytesWritten=file.write(data);
                file.close();
                if (bytesWritten == data.size())
                    out->path = path;
                else
                {
                    sim::addLog(sim_verbosity_scripterrors, "error writing file: %s", file.errorString().toStdString());
                    errorMsg = sim::util::sprintf("error writing file: %s", file.errorString().toStdString());
                }
            }
            else
            {
                sim::addLog(sim_verbosity_scripterrors, "error opening file: %s", file.errorString().toStdString());
                errorMsg = sim::util::sprintf("error opening file: %s", file.errorString().toStdString());
            }
            reply->deleteLater();
            loop.quit();
        });
        QObject::connect(reply, &QNetworkReply::errorOccurred, [&] (QNetworkReply::NetworkError code) {
            sim::addLog(sim_verbosity_scripterrors, "%s: download failed: %s", in->url, reply->errorString().toStdString());
            errorMsg = sim::util::sprintf("download failed: %s", reply->errorString().toStdString());
            loop.quit();
        });
        loop.exec();
        if(!errorMsg.empty())
            throw std::runtime_error(errorMsg);
    }
};

SIM_PLUGIN(Plugin)
#include "stubsPlusPlus.cpp"
