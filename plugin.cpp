#include <simPlusPlus/Plugin.h>
#include "config.h"
#include "plugin.h"
#include "stubs.h"

#include "eventfilter.h"
#include <QString>
#include <QWidget>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>

class Plugin : public sim::Plugin
{
public:
    void onInit()
    {
        if(sim::getBoolParam(sim_boolparam_headless))
            throw std::runtime_error("cannot start in headless mode");

        if(!registerScriptStuff())
            throw std::runtime_error("failed to register script stuff");

        setExtVersion("URL Drop Plugin");
        setBuildDate(BUILD_DATE);
    }

    void onUIInit()
    {
        QWidget *mainWindow = reinterpret_cast<QWidget*>(sim::getMainWindow(1));
        eventFilter = new EventFilter(mainWindow, nullptr);
        mainWindow->installEventFilter(eventFilter);
    }

    void onUICleanup()
    {
        QWidget *mainWindow = reinterpret_cast<QWidget*>(sim::getMainWindow(1));
        mainWindow->removeEventFilter(eventFilter);
        eventFilter->deleteLater();
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

private:
    EventFilter *eventFilter;
};

SIM_UI_PLUGIN(PLUGIN_NAME, PLUGIN_VERSION, Plugin)
#include "stubsPlusPlus.cpp"
