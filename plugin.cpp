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
#include <QWidget>

class Plugin : public sim::Plugin
{
public:
    void onStart()
    {
        if (simGetBooleanParameter(sim_boolparam_headless)>0)
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

private:
    EventFilter *eventFilter;
};

SIM_PLUGIN(PLUGIN_NAME, PLUGIN_VERSION, Plugin)
