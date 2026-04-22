#ifndef USERBASE_H
#define USERBASE_H

#include <QString>

class UserBase {
public:
    UserBase(const QString &username, const QString &password);
    virtual ~UserBase();

    virtual void login() = 0;
    virtual void logout() = 0;

protected:
    QString username;
    QString password;
};


#endif // USERBASE_H
