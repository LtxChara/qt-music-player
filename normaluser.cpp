#include "normaluser.h"
#include <QDebug>

NormalUser::NormalUser(const QString &username, const QString &password)
    : UserBase(username, password) {}

void NormalUser::login() {
    qDebug() << "Normal user" << username << "logged in.";
}

void NormalUser::logout() {
    qDebug() << "Normal user" << username << "logged out.";
}
