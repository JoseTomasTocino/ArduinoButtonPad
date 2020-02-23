#include "Window.h"
#include "ui_Window.h"

#include <QDebug>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QSerialPortInfo>
#include <QSettings>

Window::Window(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Window)
{
    ui->setupUi(this);

    loadConfig();
    createActions();
    createTrayIcon();
    configureSerialPort();

    connect(ui->cmbCurrentProfile, &QComboBox::currentTextChanged, this, &Window::selectedProfile);

    // Serial port management
    connect(&mSerial, &QSerialPort::readyRead, this, &Window::handleReadyRead);
    connect(ui->btnScanPorts, &QPushButton::clicked, this, &Window::configureSerialPort);

    // Profile management buttons
    connect(ui->btnNewProfile, &QPushButton::clicked, this, &Window::createProfile);
    connect(ui->btnSaveProfile, &QPushButton::clicked, this, &Window::saveProfile);
    connect(ui->btnRenameProfile, &QPushButton::clicked, this, &Window::renameProfile);
    connect(ui->btnDuplicateProfile, &QPushButton::clicked, this, &Window::duplicateProfile);
    connect(ui->btnDeleteProfile, &QPushButton::clicked, this, &Window::deleteProfile);
    connect(ui->chkCycleProfiles, &QCheckBox::clicked, this, &Window::toggleCycleProfiles);

    // Window management
    connect(ui->btnClose, &QPushButton::clicked, this, &Window::hide);
    connect(ui->chkShowWinOnStartup, &QCheckBox::clicked, this, &Window::toggleShowWindowOnStartup);    
    connect(ui->btnQuit, &QPushButton::clicked, qApp, &QCoreApplication::quit);

    connect(ui->chkAutorunAtStartup, &QCheckBox::clicked, this, &Window::toggleAutorunAtStartup);

}

Window::~Window()
{
    delete ui;
}

void Window::start()
{
    if (mShowWindowOnStartup)
        show();

    else
        mTrayIcon->showMessage("ArduinoButtonPad running", "");
}

void Window::handleReadyRead()
{
    int msBetweenPresses = 500;

    QDateTime now = QDateTime::currentDateTime();

    QByteArray data = mSerial.readAll();
    QString dataString(data);

    QStringList buttons = dataString.split(QRegularExpression("[\\s]+"));

    static QRegularExpression buttonRe ("b(\\d)");

    for (auto & b: buttons)
    {
        QRegularExpressionMatch match = buttonRe.match(b);

        if (!match.hasMatch())
            continue;

        unsigned buttonNumber = match.captured(1).toUInt();

        if (!mLastPressTimestamps.contains(buttonNumber))
        {
            mLastPressTimestamps[buttonNumber] = QDateTime::fromMSecsSinceEpoch(0);
        }

        qint64 msTo = mLastPressTimestamps[buttonNumber].msecsTo(now);

        if (msTo >= msBetweenPresses)
        {
            mLastPressTimestamps[buttonNumber] = now;
            handleButtonPress(buttonNumber);
        }
    }
}

void Window::handleButtonPress(unsigned i)
{
    const QString command = mProfiles[mCurrentProfile].actions.at(i - 1);

    qDebug() << "Pressed button" << i << "- Running command:" << command;

    if (i == 5 && mCycleProfiles)
    {
        auto it = mProfiles.find(mCurrentProfile);

        it++;

        if (it == mProfiles.end())
            it = mProfiles.begin();

        selectedProfile(it.value().name);
    }

    else
    {
        QProcess::startDetached(command);
    }
}

void Window::selectedProfile(const QString &name)
{
    qDebug() << "Selected profile:" << name;

    Profile & p = mProfiles[name];
    mCurrentProfile = name;

    ui->txtAction1->setText(p.actions.at(0));
    ui->txtAction2->setText(p.actions.at(1));
    ui->txtAction3->setText(p.actions.at(2));
    ui->txtAction4->setText(p.actions.at(3));
    ui->txtAction5->setText(p.actions.at(4));

    if (mTrayIcon)
        mTrayIcon->showMessage("Selected profile: " + name, "Selected profile '" + name + "'.");

    saveConfig();
}

void Window::createProfile()
{
    bool ok;
    QString profileName = QInputDialog::getText(this, "Enter profile name", "Enter profile name", QLineEdit::Normal, "NewProfile", &ok);

    if (!ok || profileName.isEmpty())
        return;

    if (mProfiles.contains(profileName))
    {
        QMessageBox::warning(this, "Error", "Error: profile with name '" + profileName + "' already exists.");
        return;
    }

    Profile p;
    p.name = profileName;
    p.actions << "" << "" << "" << "" << "";
    mProfiles[profileName] = p;

    saveConfig();

    ui->cmbCurrentProfile->addItem(profileName);
    ui->cmbCurrentProfile->setCurrentText(profileName);
}

void Window::saveProfile()
{
    Profile & p = mProfiles[mCurrentProfile];

    p.actions[0] = ui->txtAction1->text();
    p.actions[1] = ui->txtAction2->text();
    p.actions[2] = ui->txtAction3->text();
    p.actions[3] = ui->txtAction4->text();
    p.actions[4] = ui->txtAction5->text();

    saveConfig();

    QMessageBox::information(this, "Saved correctly", "Profile saved correctly");
}

void Window::renameProfile()
{
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename profile", "Enter new name for profile", QLineEdit::Normal, mCurrentProfile, &ok);

    if (!ok || newName.isEmpty())
        return;

    mProfiles[mCurrentProfile].name = newName;
    mProfiles[newName] = mProfiles[mCurrentProfile];
    mProfiles.remove(mCurrentProfile);

    ui->cmbCurrentProfile->setItemText(ui->cmbCurrentProfile->currentIndex(), newName);

    saveConfig();
}

void Window::duplicateProfile()
{
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename profile", "Enter new name for duplicated profile", QLineEdit::Normal, mCurrentProfile, &ok);

    if (!ok || newName.isEmpty())
        return;

    if (mProfiles.contains(newName))
    {
        QMessageBox::warning(this, "Error", "Error: profile with name '" + newName + "' already exists.");
        return;
    }

    Profile p = mProfiles[mCurrentProfile];
    p.name = newName;

    mProfiles[newName] = p;
    ui->cmbCurrentProfile->addItem(newName);
    ui->cmbCurrentProfile->setCurrentText(newName);

    saveConfig();
}

void Window::deleteProfile()
{
    if (QMessageBox::question(this, "Confirm deletion", "Are you sure you want to delete profile '" + mCurrentProfile + "'?") == QMessageBox::Yes)
    {
        mProfiles.remove(mCurrentProfile);
        ui->cmbCurrentProfile->removeItem(ui->cmbCurrentProfile->currentIndex());

        saveConfig();
    }
}

void Window::toggleCycleProfiles(bool value)
{
    mCycleProfiles = value;

    if (mCycleProfiles)
    {
        ui->lblButton5->setText("Button 5 (inactive):");
    }
    else
    {
        ui->lblButton5->setText("Button 5:");
    }

    saveConfig();
}

void Window::loadConfig()
{
    QSettings settings("config.ini", QSettings::IniFormat);

    qDebug() << "Using" << settings.fileName();

    mShowWindowOnStartup = settings.value("Application/ShowWindowOnStartup", "true").toBool();
    ui->chkShowWinOnStartup->setChecked(mShowWindowOnStartup);

    mCycleProfiles = settings.value("Application/CycleProfiles", "false").toBool();
    ui->chkCycleProfiles->setChecked(mCycleProfiles);

    mAutorunAtStartup = settings.value("Application/AutorunAtStartup", false).toBool();
    ui->chkAutorunAtStartup->setChecked(mAutorunAtStartup);

    for (const QString group: settings.childGroups())
    {
        if (group.startsWith("Profile_"))
        {
            Profile p;
            p.name = group.mid(8);

            qDebug() << "Processing profile:" << p.name;

            for (unsigned i = 0; i < 5; ++i)
            {
                p.actions << settings.value(QString("%1/Action%2").arg(group).arg(i + 1)).toString();
            }

            mProfiles[p.name] = p;
        }
    }

    // Generate a default profile if no profiles were loaded
    if (mProfiles.empty())
    {
        qDebug() << "Creating default profile";

        Profile p;
        p.name = "Default";
        p.actions << "notepad" << "wordpad" << "calc" << "explorer" << "control";

        mProfiles["Default"] = p;
        saveConfig();
    }

    // Populate profile list
    ui->cmbCurrentProfile->addItems(mProfiles.keys());

    // Enable current profile
    QString mCurrentProfile = settings.value("Application/CurrentProfile", "Default").toString();

    if (!mProfiles.contains(mCurrentProfile))
    {
        mCurrentProfile = mProfiles.keys().first();
    }

    selectedProfile(mCurrentProfile);
}

void Window::createActions()
{
    mPreferencesAction = new QAction("Show &preferences", this);
    connect(mPreferencesAction, &QAction::triggered, this, &Window::show);

    mQuitAction = new QAction("&Quit", this);
    connect(mQuitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void Window::createTrayIcon()
{
    mTrayIconMenu = new QMenu(this);
    mTrayIconMenu->addAction(mPreferencesAction);
    mTrayIconMenu->addSeparator();
    mTrayIconMenu->addAction(mQuitAction);

    mTrayIcon = new QSystemTrayIcon(this);
    mTrayIcon->setContextMenu(mTrayIconMenu);
    mTrayIcon->setIcon(QIcon(":/icon"));
    mTrayIcon->show();
}

void Window::configureSerialPort()
{
    for (auto & portInfo: QSerialPortInfo::availablePorts())
    {
        if (portInfo.description().contains("CH340"))
        {
            mSerial.setPort(portInfo);
            break;
        }
    }

    if (mSerial.portName().isEmpty())
    {
        ui->txtSerialInfo->setText("Current serial port: none");
        return;
    }

    ui->txtSerialInfo->setText("Current serial port: " + mSerial.portName());
    mSerial.setBaudRate(QSerialPort::Baud9600);
    mSerial.open(QIODevice::ReadOnly);
}

void Window::toggleShowWindowOnStartup(bool value)
{
    mShowWindowOnStartup = value;
    saveConfig();
}

void Window::toggleAutorunAtStartup(bool value)
{
    mAutorunAtStartup = value;

#ifdef Q_OS_WIN32
    QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

    if (mAutorunAtStartup)
    {
        qDebug() << "Enabling autorun...";
        settings.setValue("ArduinoButtonPad", QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    }
    else
    {
        qDebug() << "Disabling autorun...";
        settings.remove("ArduinoButtonPad");
    }
    settings.sync();
#endif

    saveConfig();
}

void Window::saveConfig()
{
    qDebug() << "Saving configuration...";

    QSettings settings("config.ini", QSettings::IniFormat);

    // Save general settings
    settings.setValue("Application/ShowWindowOnStartup", mShowWindowOnStartup);
    settings.setValue("Application/CycleProfiles", mCycleProfiles);
    settings.setValue("Application/CurrentProfile", mCurrentProfile);
    settings.setValue("Application/AutorunAtStartup", mAutorunAtStartup);

    // Save profiles
    for (Profile & p: mProfiles)
    {
        qDebug() << "Saving profile" << p.name;

        for (unsigned i = 0; i < 5; ++i)
        {
            settings.setValue(QString("Profile_%1/Action%2").arg(p.name).arg(i + 1), p.actions[i]);
        }
    }

    settings.sync();
}




