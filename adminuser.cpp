#include "adminuser.h"
#include <QDebug>

AdminUser::AdminUser(const QString &username, const QString &password)
    : UserBase(username, password) {}

void AdminUser::login() {
    qDebug() << "Admin user" << username << "logged in.";
}

void AdminUser::logout() {
    qDebug() << "Admin user" << username << "logged out.";
}

