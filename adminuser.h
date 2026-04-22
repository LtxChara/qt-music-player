#ifndef ADMINUSER_H
#define ADMINUSER_H

#include "userbase.h"

class AdminUser : public UserBase {
public:
    AdminUser(const QString &username, const QString &password);
    void login() override;
    void logout() override;
};

#endif // ADMINUSER_H

