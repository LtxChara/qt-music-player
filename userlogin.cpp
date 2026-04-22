#include "userlogin.h"
#include "ui_userlogin.h"
#include "userregister.h"
#include "adminlogin.h"
#include "vipuser.h"

userLogin::userLogin(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::userLogin),
    User(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("MelodyCore-Login");
}

userLogin::~userLogin()
{
    delete ui;
    delete User;
}

void userLogin::on_registerButton_clicked()
{
    userRegister* Register = new userRegister;
    this->close();
    Register->show();
}

void userLogin::on_toAdminLoginButton_clicked()
{
    adminLogin* Login = new adminLogin;
    this->close();
    Login->show();
}

void userLogin::on_loginButton_clicked()
{
    QString username = ui->usernameEdit->text().trimmed();
    QString password = ui->passwordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, tr("Login failed"), tr("Username or password cannot be empty."), QMessageBox::Ok);
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT id, name, vip FROM users WHERE name = :name AND password = :password");
    query.bindValue(":name", username);
    query.bindValue(":password", password);

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Failed to execute query: ") + query.lastError().text(), QMessageBox::Ok);
        return;
    }

    if (query.next()) {
        int userId = query.value("id").toInt();            // 获取用户ID
        QString userName = query.value("name").toString(); // 获取用户名
        bool isVip = query.value("vip").toBool();          // 获取用户VIP状态

        delete User;
        if (isVip) {
            User = new VipUser(username, password);
        }
        else {
            User = new NormalUser(username, password);
        }

        User->login();
        emit loginSuccessful(userId, userName); // 发出信号
        this->hide(); // 隐藏登录对话框
    }
    else {
        // 查询执行成功但无匹配记录，说明用户名或密码错误
        QMessageBox::warning(this, tr("Login failed"), tr("Invalid username or password."), QMessageBox::Ok);
        ui->usernameEdit->clear();
        ui->passwordEdit->clear();
        ui->usernameEdit->setFocus();
    }
}