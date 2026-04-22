#include "vipuser.h"
#include <QDebug>

VipUser::VipUser(const QString &username, const QString &password)
    : NormalUser(username, password) {}

void VipUser::login() {
    qDebug() << "VIP user" << username << "logged in.";
}

void VipUser::accessVipFeatures() {
    qDebug() << "Accessing VIP features for user" << username;
}
