#ifndef ADMINLOGIN_H
#define ADMINLOGIN_H

#include <QDialog>
#include "userlogin.h"
#include "adminwidget.h"
#include "adminuser.h"

namespace Ui {
class adminLogin;
}

class adminLogin : public QDialog
{
    Q_OBJECT

public:
    explicit adminLogin(QWidget *parent = nullptr);
    ~adminLogin();

private slots:
    void on_adminBackButton_clicked();

    void on_adminLoginButton_clicked();

signals:
    void loginSuccessful(int userId); // 添加信号

private:
    Ui::adminLogin *ui;
    AdminUser *adminUser;
};

#endif // ADMINLOGIN_H
