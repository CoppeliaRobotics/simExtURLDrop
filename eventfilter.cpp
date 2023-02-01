#include "eventfilter.h"
#include "stubs.h"

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDir>
#include <QTemporaryFile>
#include <QMetaEnum>

#include "simPlusPlus/Plugin.h"

EventFilter::EventFilter(QWidget *mainWin, QObject *parent)
	: QObject(parent),
      mainWindow(mainWin)
{
}

EventFilter::~EventFilter()
{
}

QUrl rewriteURL(QUrl url)
{
    QString urlStr = url.toString();

    {
        QRegularExpression re_githubFile("https://github.com/(.+)/(.+)/blob/(.+)/(.+)");
        auto m = re_githubFile.match(urlStr);
        if(m.hasMatch()) {
            urlStr = QStringLiteral("https://raw.githubusercontent.com/%1/%2/%3/%4").arg(m.captured(1), m.captured(2), m.captured(3), m.captured(4));
            return QUrl(urlStr);
        }
    }

    return url;
}

bool EventFilter::eventFilter(QObject *obj, QEvent *event)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<QEvent::Type>();
    if(event->type() == QEvent::DragEnter)
    {
        auto e = static_cast<QDragEnterEvent*>(event);
        e->acceptProposedAction();
    }
    else if(event->type() == QEvent::Drop)
    {
        auto e = static_cast<QDropEvent*>(event);
        const QMimeData *mimeData = e->mimeData();
        if(mimeData->hasUrls()) {
            QList<QUrl> urlList = mimeData->urls();
            if(urlList.size() > 1)
                sim::addLog(sim_verbosity_warnings, "Cannot handle multiple URLs");
            auto url = urlList.at(0);
            if(!url.isLocalFile())
            {
                auto fileName = url.fileName();
                QString type;
                if(fileName.endsWith(".ttt")) type = "ttt";
                if(fileName.endsWith(".ttm")) type = "ttm";
                if(type == "ttt" || type == "ttm")
                {
                    url = rewriteURL(url);
                    sim::addLog(sim_verbosity_infos, "downloading %s...", url.toString().toStdString());
                    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
                    QNetworkRequest request(url);
                    QNetworkReply *reply = nam->get(request);
                    progressDialog = new QProgressDialog(QStringLiteral("Downloading %1...").arg(fileName), "", 0, 10000000, nullptr);
                    progressDialog->setMinimumWidth(400);
                    progressDialog->setMinimumDuration(500);
                    progressDialog->setCancelButton(nullptr);
                    progressDialog->setValue(0);
                    progressDialog->show();
                    QObject::connect(reply, &QNetworkReply::downloadProgress, [=] (qint64 bytesReceived, qint64 bytesTotal) {
                        sim::addLog(sim_verbosity_infos, "%s: downloaded %d bytes out of %d", fileName.toStdString(), bytesReceived, bytesTotal);
                        progressDialog->setMaximum(bytesTotal);
                        progressDialog->setValue(bytesReceived);
                    });
                    QObject::connect(reply, &QNetworkReply::finished, [=] {
                        progressDialog->deleteLater();
                        QTemporaryFile f(QDir::tempPath() + "/CoppeliaSim.XXXXXX." + type);
                        if(f.open()) {
                            auto data = reply->readAll();
                            reply->deleteLater();
                            f.write(data);
                            f.close();
                            sim::addLog(sim_verbosity_infos, "%s: finished downloading %d bytes", fileName.toStdString(), data.size());
                            if(type == "ttt")
                                sim::loadScene(f.fileName().toStdString());
                            else if(type == "ttm")
                                sim::loadModel(f.fileName().toStdString());
                        }
                        nam->deleteLater();
                    });
                    QObject::connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [=] (QNetworkReply::NetworkError code) {
                        progressDialog->deleteLater();
                        sim::addLog(sim_verbosity_scripterrors, "%s: download failed: %s", fileName.toStdString(), reply->errorString().toStdString());
                        nam->deleteLater();
                    });
                    return true; // eat event
                }
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

