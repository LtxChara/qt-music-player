#ifndef ADMINWIDGET_H
#define ADMINWIDGET_H

#include <QWidget>
#include <QtSql/QSqlDatabase> // 连接数据库
#include <QtSql/QSqlError> // 数据库连接失败打印报错语句
#include <QtSql/QSqlQuery> // 数据库操作（增删改查）
#include <QSqlQueryModel>
#include <QDebug>
#include <QMessageBox>
#include "adminlogin.h"
#include"add.h"

namespace Ui {
class adminWidget;
}

class adminWidget : public QWidget
{
    Q_OBJECT

public:
    explicit adminWidget(QWidget *parent = nullptr);
    ~adminWidget();

private slots:
    void on_searchButton_1_clicked();

    void on_searchButton_2_clicked();

    void on_backButton_clicked();

    void on_createUserButton_clicked();

    void on_customContextMenuRequested(const QPoint &pos);
    void on_deleteAction_triggered();

    void on_customContextMenuRequestedForSongs(const QPoint &pos);
    void on_deleteSongAction_triggered();

    void on_showAllUserButton_clicked();

    void on_showAllSongButton_clicked();

private:
    Ui::adminWidget *ui;
};

#endif // ADMINWIDGET_H
