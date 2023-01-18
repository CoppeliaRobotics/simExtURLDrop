#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "simPlusPlus/Plugin.h"
#include "config.h"
#include "plugin.h"
#include "stubs.h"

#include "eventfilter.h"
#include <QString>
#include <QWidget>
#ifdef _WIN32
#include <QSettings>
#else
#include <QUrl>
#include <QDesktopServices>
#endif // _WIN32

class Plugin : public sim::Plugin
{
public:
    void onStart()
    {
        if (simGetBoolParam(sim_boolparam_headless)>0)
            throw std::runtime_error("cannot start in headless mode");

        QWidget *mainWindow = reinterpret_cast<QWidget*>(simGetMainWindow(1));
        eventFilter = new EventFilter(mainWindow, nullptr);
        mainWindow->installEventFilter(eventFilter);

        if(!registerScriptStuff())
            throw std::runtime_error("failed to register script stuff");

        setExtVersion("URL Drop Plugin");
        setBuildDate(BUILD_DATE);
    }

    void onEnd()
    {
        QWidget *mainWindow = reinterpret_cast<QWidget*>(simGetMainWindow(1));
        mainWindow->removeEventFilter(eventFilter);
        eventFilter->deleteLater();
    }

    void openURL(openURL_in *in, openURL_out *out)
    {
#ifdef _WIN32
        QSettings httpSetting("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice", QSettings::Registry64Format);
        QString progId(httpSetting.value("ProgId").toString());
        QSettings openCmd(QStringLiteral("HKEY_CLASSES_ROOT\\%1\\shell\\open\\command").arg(progId), QSettings::Registry64Format);
        QString url(QString::fromStdString(in->url));
        QString cmd(openCmd.value("Default").toString().arg(url));
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ::ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ::ZeroMemory(&pi, sizeof(pi));
        QByteArray cmdba = cmd.toLocal8Bit();
        ::CreateProcess(NULL, cmdba.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
#else
        QUrl url = QUrl(QString::fromStdString(in->url));
        QDesktopServices::openUrl(url);
#endif
    }

private:
    EventFilter *eventFilter;
};

SIM_PLUGIN(PLUGIN_NAME, PLUGIN_VERSION, Plugin)
#include "stubsPlusPlus.cpp"
