#ifndef VIPUSER_H
#define VIPUSER_H

#include "normaluser.h"

class VipUser : public NormalUser {
public:
    VipUser(const QString &username, const QString &password);
    void login() override;
    void accessVipFeatures();
};

#endif // VIPUSER_H
