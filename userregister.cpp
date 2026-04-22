#include "userregister.h"
#include "ui_userregister.h"
#include "userlogin.h"

userRegister::userRegister(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::userRegister)
{
    ui->setupUi(this);

    this->setWindowTitle("MelodyCore-Register");
}

userRegister::~userRegister()
{
    delete ui;
}

void userRegister::on_backButton_clicked()
{
    userLogin *Login = new userLogin;
    this->close();
    Login->show();
}

int userRegister::getSelectedStyles(int styles[10])
{
    int count = 0;
    styles[0] = 0; // 第一个元素恒为0

    if (ui->classicalCheckBox->isChecked()) { styles[1] = 20; count++; } else { styles[1] = 0; }
    if (ui->jazzCheckBox->isChecked()) { styles[2] = 20; count++; } else { styles[2] = 0; }
    if (ui->rockCheckBox->isChecked()) { styles[3] = 20; count++; } else { styles[3] = 0; }
    if (ui->popCheckBox->isChecked()) { styles[4] = 20; count++; } else { styles[4] = 0; }
    if (ui->bluesCheckBox->isChecked()) { styles[5] = 20; count++; } else { styles[5] = 0; }
    if (ui->electronicCheckBox->isChecked()) { styles[6] = 20; count++; } else { styles[6] = 0; }
    if (ui->countryCheckBox->isChecked()) { styles[7] = 20; count++; } else { styles[7] = 0; }
    if (ui->folkCheckBox->isChecked()) { styles[8] = 20; count++; } else { styles[8] = 0; }
    if (ui->hiphopCheckBox->isChecked()) { styles[9] = 20; count++; } else { styles[9] = 0; }

    return count;
}

void userRegister::on_registerSureButton_clicked()
{
    QSqlQuery query;

    // 查询当前最大ID值
    int maxId = 0;
    if (query.exec("SELECT MAX(id) FROM users")) {
        if (query.next()) {
            maxId = query.value(0).toInt();
        }
    } else {
        QMessageBox::critical(this, "failure", "Can't get max ID：" + query.lastError().text());
        return;
    }

    // 生成新的ID值
    int Id = maxId + 1;

    QString username = ui->registerUsernameEdit->text();
    QString password = ui->registerPasswordEdit->text();
    QString password2 = ui->ensurePasswordEdit->text();

    // 获取选中的风格
    int favoriteStyles[10];
    int selectedCount = getSelectedStyles(favoriteStyles);

    if (selectedCount != 3) {
        QMessageBox::warning(this, "Error", "Please select exactly 3 types.");
        return;
    }

    if (password == password2)
    {
        // 检查用户名是否已存在
        QString checkUserQuery = QString("SELECT * FROM users WHERE name='%1'").arg(username);
        if (query.exec(checkUserQuery) && query.next())
        {
            QMessageBox::warning(this, "Error", "User exists！");
            return;
        }

        // 将风格数组转换为字符串以存储
        QString favoriteStylesStr;
        for (int i = 0; i < 10; i++) {
            favoriteStylesStr += QString::number(favoriteStyles[i]);
            if (i < 9) favoriteStylesStr += ",";
        }

        // 插入新用户记录
        QString insertQuery = QString("INSERT INTO users (id, name, password, favorite_styles, vip) VALUES (%1, '%2', '%3', '%4', 0)")
                                     .arg(Id)
                                     .arg(username)
                                     .arg(password)
                                     .arg(favoriteStylesStr);

        if (query.exec(insertQuery))
        {
            QMessageBox::information(this, "Success", "Registration successful！", QMessageBox::Yes);
            this->close();
            userLogin* r = new userLogin;
            r->show();
        }
        else
        {
            QMessageBox::warning(this, "Error", "Registration failed：" + query.lastError().text());
        }
    }
    else
    {
        QMessageBox::warning(this, "Error", "Entered passwords differ！");
    }

    ui->registerUsernameEdit->clear();
    ui->registerPasswordEdit->clear();
    ui->ensurePasswordEdit->clear();
    ui->registerUsernameEdit->setFocus();
}
