#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "simPlusPlus/Plugin.h"
#include "plugin.h"
//#include "stubs.h"
#include "config.h"

#include "eventfilter.h"
#include <QWidget>

class Plugin : public sim::Plugin
{
public:
    void onStart()
    {
        QWidget *mainWindow = reinterpret_cast<QWidget*>(simGetMainWindow(1));
        auto eventFilter = new EventFilter(mainWindow);
        mainWindow->installEventFilter(eventFilter);

        //if(!registerScriptStuff())
        //    throw std::runtime_error("failed to register script stuff");

        simSetModuleInfo(PLUGIN_NAME, 0, "URL Drop Plugin", 0);
        simSetModuleInfo(PLUGIN_NAME, 1, BUILD_DATE, 0);
    }
};

SIM_PLUGIN(PLUGIN_NAME, PLUGIN_VERSION, Plugin)
