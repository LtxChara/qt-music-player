#ifndef USERLOGIN_H
#define USERLOGIN_H

#include <QDialog>
#include <QPainter>
#include <QMouseEvent>
#include <QWidget>
#include "widget.h"
#include "normaluser.h"

namespace Ui {
    class userLogin;
}

class userLogin : public QDialog
{
    Q_OBJECT

public:
    explicit userLogin(QWidget* parent = nullptr);
    ~userLogin();

private slots:
    void on_registerButton_clicked();
    void on_toAdminLoginButton_clicked();
    void on_loginButton_clicked();

private:
    Ui::userLogin* ui;
    NormalUser* User;

signals:
    void loginSuccessful(int userId, const QString& userName); // 发出信号
};

#endif // USERLOGIN_H