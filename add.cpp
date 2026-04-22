#include "add.h"
#include "ui_add.h"

Add::Add(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Add)
{
    ui->setupUi(this);
}

Add::~Add()
{
    delete ui;
}

void Add::on_pushButton_2_clicked()
{
    QSqlQuery query;
    QString Id = ui->idEdit->text();
    QString username = ui->nameEdit->text();
    QString password = ui->passwordEdit->text();

    // 插入新用户记录
    QString insertQuery = QString("INSERT INTO users (id, name, password) VALUES (%1, '%2', '%3')")
                          .arg(Id)
                          .arg(username)
                          .arg(password);

    if (query.exec(insertQuery)) {
        QMessageBox::information(this, "Success", "Add successfully", QMessageBox::Yes);
        this->close();
        adminWidget* r = new adminWidget;
        r->show();
    } else {
        QMessageBox::warning(this, "Error", "Add failed" + query.lastError().text());
    }
}



void Add::on_pushButton_clicked()
{
    this->close();
    adminWidget* r = new adminWidget;
    r->show();
}
