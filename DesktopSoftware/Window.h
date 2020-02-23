#ifndef WIDGET_H
#define WIDGET_H

#include "Profile.h"

#include <QWidget>
#include <QDialog>
#include <QSerialPort>
#include <QDateTime>
#include <QMap>
#include <QMenu>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui { class Window; }
QT_END_NAMESPACE

class Window : public QDialog
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);
    ~Window();

    void start();

private slots:
    void handleReadyRead();
    void handleButtonPress(unsigned i);

    void selectedProfile(const QString & name);

    void createProfile();
    void saveProfile();
    void renameProfile();
    void duplicateProfile();
    void deleteProfile();

    void toggleCycleProfiles(bool value);

    void toggleShowWindowOnStartup(bool value);
    void toggleAutorunAtStartup(bool value);

    void configureSerialPort();

private:
    void loadConfig();
    void saveConfig();

    void createActions();
    void createTrayIcon();

    // Widgets
    Ui::Window *ui;
    bool mShowWindowOnStartup = true;

    // Serial port comms
    QSerialPort mSerial;

    // Button handling
    QMap<unsigned, QDateTime> mLastPressTimestamps;

    // Tray icon
    QSystemTrayIcon * mTrayIcon = nullptr;

    // Tray icon menu and actions
    QMenu * mTrayIconMenu;
    QAction * mPreferencesAction;
    QAction * mQuitAction;

    // Profile management
    QString mCurrentProfile;
    QMap<QString, Profile> mProfiles;
    bool mCycleProfiles = false;

    // Other
    bool mAutorunAtStartup = false;
};
#endif // WIDGET_H
