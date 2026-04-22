#include "adminwidget.h"
#include "ui_adminwidget.h"

adminWidget::adminWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::adminWidget)
{
    ui->setupUi(this);

    this->setWindowTitle("MelodyCore-AdminManage");

    ui->backButton->setIcon(QIcon(":/new/prefix1/pic/Back.png"));
    ui->backButton->setStyleSheet("QPushButton{border:none;color:rgb(122, 197, 205);}" "QPushButton:hover{background-color: #DCDCDC;border:none;color:rgb(255, 255, 255);}");

    ui->userInfoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->userInfoTable->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->songDataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->songDataTable->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->userInfoTable, &QTableView::customContextMenuRequested, this, &adminWidget::on_customContextMenuRequested);
    connect(ui->songDataTable, &QTableView::customContextMenuRequested, this, &adminWidget::on_customContextMenuRequestedForSongs);
}

adminWidget::~adminWidget()
{
    delete ui;
}

void adminWidget::on_searchButton_1_clicked()
{
    QString username = ui->searchUserEdit->text();

    static QSqlQueryModel *model = new QSqlQueryModel(ui->userInfoTable);
    // 构建查询语句
    QString queryString = QString("SELECT * FROM users WHERE name = '%1'").arg(username);

    // 执行查询并更新模型
    model->setQuery(queryString);

    // 检查是否有查询结果
    if (model->rowCount() == 0)
    {
       QMessageBox::information(this, "Tips", "No user message matched.");
    }
    else
    {
       // 更新表格视图以显示查询结果
       ui->userInfoTable->setModel(model);
    }
}

void adminWidget::on_searchButton_2_clicked()
{
    QString songname = ui->searchSongEdit->text();

    static QSqlQueryModel *model = new QSqlQueryModel(ui->songDataTable);
    // 构建查询语句
    QString queryString = QString("SELECT * FROM songs WHERE title = '%1'").arg(songname);

    // 执行查询并更新模型
    model->setQuery(queryString);

    // 检查是否有查询结果
    if (model->rowCount() == 0)
    {
       QMessageBox::information(this, "Tips", "No song message matched.");
    }
    else
    {
       // 更新表格视图以显示查询结果
       ui->songDataTable->setModel(model);
    }
}

void adminWidget::on_backButton_clicked()
{
    adminLogin *Login = new adminLogin;
    this->close();
    Login->show();
}

void adminWidget::on_createUserButton_clicked()
{
    Add* a = new Add;
    a->show();
}

void adminWidget::on_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->userInfoTable->indexAt(pos);
    if (index.isValid()) {
        QMenu contextMenu(this);

        QAction *deleteAction = new QAction("Delete", this);
        connect(deleteAction, &QAction::triggered, this, &adminWidget::on_deleteAction_triggered);

        contextMenu.addAction(deleteAction);
        contextMenu.exec(ui->userInfoTable->viewport()->mapToGlobal(pos));
    }
}

void adminWidget::on_deleteAction_triggered()
{
    QModelIndexList selectedRows = ui->userInfoTable->selectionModel()->selectedRows();
    if (!selectedRows.isEmpty()) {
        int row = selectedRows.first().row();
        QString username = ui->userInfoTable->model()->index(row, 1).data().toString();
        QSqlQuery query;
        query.prepare("DELETE FROM users WHERE name = :username");
        query.bindValue(":username", username);

        if (query.exec()) {
            QMessageBox::information(this, "Tips", "User deleted successfully.");
            on_searchButton_1_clicked();
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete user.");
        }
    }
}

void adminWidget::on_customContextMenuRequestedForSongs(const QPoint &pos)
{
    QModelIndex index = ui->songDataTable->indexAt(pos);
    if (index.isValid()) {
        QMenu contextMenu(this);

        QAction *deleteAction = new QAction("Delete", this);
        connect(deleteAction, &QAction::triggered, this, &adminWidget::on_deleteSongAction_triggered);

        contextMenu.addAction(deleteAction);
        contextMenu.exec(ui->songDataTable->viewport()->mapToGlobal(pos));
    }
}

void adminWidget::on_deleteSongAction_triggered()
{
    QModelIndexList selectedRows = ui->songDataTable->selectionModel()->selectedRows();
    if (!selectedRows.isEmpty()) {
        int row = selectedRows.first().row();
        QString songTitle = ui->songDataTable->model()->index(row, 1).data().toString();

        QSqlQuery query;
        query.prepare("DELETE FROM songs WHERE title = :title");
        query.bindValue(":title", songTitle);

        if (query.exec()) {
            QMessageBox::information(this, "Tips", "Song deleted successfully.");
            on_searchButton_2_clicked();
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete song.");
        }
    }
}

void adminWidget::on_showAllUserButton_clicked()
{
    static QSqlQueryModel *model = new QSqlQueryModel(ui->userInfoTable);
        QString queryString = "SELECT * FROM users";
        model->setQuery(queryString);

        // Check if there are results
        if (model->rowCount() == 0)
        {
            QMessageBox::information(this, "Tips", "No users found.");
        }
        else
        {
            ui->userInfoTable->setModel(model);
        }
}

void adminWidget::on_showAllSongButton_clicked()
{
    static QSqlQueryModel *model = new QSqlQueryModel(ui->songDataTable);
        QString queryString = "SELECT * FROM songs";
        model->setQuery(queryString);

        // Check if there are results
        if (model->rowCount() == 0)
        {
            QMessageBox::information(this, "Tips", "No songs found.");
        }
        else
        {
            ui->songDataTable->setModel(model);
        }
}
