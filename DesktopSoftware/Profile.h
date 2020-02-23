#ifndef PROFILE_H
#define PROFILE_H

#include <QList>
#include <QString>

struct Profile
{
    QString name;
    QList<QString> actions;
};

#endif // PROFILE_H
