#include "adminlogin.h"
#include "ui_adminlogin.h"

adminLogin::adminLogin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::adminLogin),
    adminUser(nullptr)
{
    ui->setupUi(this);

    this->setWindowTitle("MelodyCore-AdminLogin");
}

adminLogin::~adminLogin()
{
    delete ui;
    delete adminUser;
}

void adminLogin::on_adminBackButton_clicked()
{
    userLogin *Login = new userLogin;
    this->close();
    Login->show();
}

void adminLogin::on_adminLoginButton_clicked()
{
    QString username = ui->adminNameEdit->text().trimmed();
    QString password = ui->adminPasswordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, tr("Login failed"), tr("Username or password cannot be empty."), QMessageBox::Ok);
        return;
    }

    QSqlQuery query;    // 操作数据库
    query.prepare("SELECT id FROM admins WHERE name = :name AND password = :password");
    query.bindValue(":name", username);
    query.bindValue(":password", password);

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Failed to execute query: ") + query.lastError().text(), QMessageBox::Ok);
        return;
    }

    if (query.next()) {
        delete adminUser;
        adminUser = new AdminUser(username, password);
        adminUser->login();
        this->close();
        adminWidget* aw = new adminWidget;
        aw->show();
    }
    else {
        // 查询执行成功但无匹配记录
        QMessageBox::warning(this, tr("Login failed"), tr("Invalid username or password."), QMessageBox::Ok);
        ui->adminNameEdit->clear();
        ui->adminPasswordEdit->clear();
        ui->adminNameEdit->setFocus();
    }
}
