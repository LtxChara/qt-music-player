#ifndef USERREGISTER_H
#define USERREGISTER_H

#include <QDialog>
#include <QSqlQuery>

namespace Ui {
class userRegister;
}

class userRegister : public QDialog
{
    Q_OBJECT

public:
    explicit userRegister(QWidget *parent = nullptr);
    ~userRegister();

private slots:
    void on_backButton_clicked();

    void on_registerSureButton_clicked();

private:
    Ui::userRegister *ui;
    int getSelectedStyles(int styles[10]);
};

#endif // USERREGISTER_H
