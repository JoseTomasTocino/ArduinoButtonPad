#include "Window.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("JoseTomasTocino");
    QCoreApplication::setOrganizationDomain("JoseTomasTocino.com");
    QCoreApplication::setApplicationName("ArduinoButtonPad");

    QSettings::setDefaultFormat(QSettings::IniFormat);

    QApplication a(argc, argv);

    QApplication::setQuitOnLastWindowClosed(false);

    Window w;
    w.start();

    return a.exec();
}
