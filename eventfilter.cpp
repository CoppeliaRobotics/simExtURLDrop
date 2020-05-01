#include "eventfilter.h"

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

EventFilter::EventFilter(QObject *parent)
	: QObject(parent)
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
                qWarning() << "Cannot handle multiple URLs";
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
                    qDebug() << "downloading" << url;
                    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
                    QObject::connect(nam, &QNetworkAccessManager::finished, [=] (QNetworkReply *reply) {
                        QTemporaryFile f(QDir::tempPath() + "/CoppeliaSim.XXXXXX." + type);
                        if(f.open()) {
                            auto data = reply->readAll();
                            reply->deleteLater();
                            f.write(data);
                            f.close();
                            qDebug() << "downloaded" << data.size() << "bytes" << f.fileName();
                            if(type == "ttt")
                                simLoadScene(f.fileName().toLocal8Bit().data());
                            else if(type == "ttm")
                                simLoadModel(f.fileName().toLocal8Bit().data());
                        }
                        nam->deleteLater();
                        return true; // eat event
                    });
                    QNetworkRequest request(url);
                    nam->get(request);
                }
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

