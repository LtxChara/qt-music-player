#ifndef NORMALUSER_H
#define NORMALUSER_H

#include "userbase.h"

class NormalUser : public UserBase {
public:
    NormalUser(const QString &username, const QString &password);
    void login() override;
    void logout() override;
};

#endif // NORMALUSER_H
