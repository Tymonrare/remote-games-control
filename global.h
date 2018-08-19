#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <QTreeWidget>

struct GameData
{
std::string name,
        path;
std::string timeArg, previewPath;
int time, minTime, maxTime, startDelay, steamID = -1, previewID = -1;

std::map<std::string, int> arguments;
std::string taskkill;
bool systemkill = false, steamkill = false;

bool price_is_fixed = false;
int price, timeStep = 30;
};

extern bool loadFullscreen;
extern bool useLocalConnection;
extern const QString xmlOrigin;
void loginAsAdmin();
bool parseXMLLogData(QTreeWidget *widget);

class ShortcutsHandler : public QWidget
{
/*!
 '''Turns Login Dialog into a QObject
*/
 Q_OBJECT
public:
    ShortcutsHandler(QWidget *parent = 0);
public slots:
    void runAdmin();
};

#endif // GLOBAL_H
