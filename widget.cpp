#include "widget.h"
#include "ui_widget.h"
#include "myQSS.h"
#include <QTime>
#include <algorithm>

Widget::Widget(QWidget* parent, int userId, const QString& userName)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , userId(userId)
    , userName(userName)
{
    ui->setupUi(this);

    // UI初始化
    init_UI();

    // 播放初始化
    init_play();
    
    // “我的”页面显示用户名
    ui->userName->setText(userName);

    // 加载当前用户的歌单
    loadUserPlaylists();

    // 连接推荐刷新按钮
    connect(ui->recommendButton, &QPushButton::clicked, this, &Widget::on_recommendButton_clicked);

    // 连接VIP按钮的点击信号到槽函数
    connect(ui->vipButton, &QPushButton::clicked, this, &Widget::on_vipButton_clicked);

    updateLikeListWidget(userId);

    // 设置右键菜单信号连接
    connect(ui->recommendListWidget, &QListWidget::customContextMenuRequested, this, &Widget::showRecommendSongContextMenu);
    connect(ui->recommendListWidget, &QListWidget::itemClicked, this, &Widget::on_recommendListWidget_itemClicked);

    connect(ui->nameListWidget, &QListWidget::customContextMenuRequested, this, &Widget::showContextMenu);
}

Widget::~Widget()
{
    // 安全停止子线程
    if (thread) {
        thread->quit();  // 停止线程事件循环
        thread->wait();  // 等待线程处理完手头动作
    }

    // 安全释放曲库播放器（含独立数据库连接）
    if (musicPlayer) {
        musicPlayer->stopPlayback();
        delete musicPlayer;
        musicPlayer = nullptr;
    }

    // 释放未加入对象树的裸指针，避免悬挂指针与重复释放
    if (myMediaPlayer) {
        delete myMediaPlayer;
        myMediaPlayer = nullptr;
    }
    if (myT) {
        delete myT;
        myT = nullptr;
    }
    if (movie) {
        delete movie;
        movie = nullptr;
    }

    delete ui;
}

//记录拖动起始位置
void Widget::mousePressEvent(QMouseEvent *event)
{
    //实现点击界面中某点，音量条隐藏
    int x = event->pos().x();
    int y = event->pos().y();
    int x_widget = ui->volumeSlider->geometry().x();
    int y_widget = ui->volumeSlider->geometry().y();
    int w = ui->volumeSlider->geometry().width();
    int h = ui->volumeSlider->geometry().height();

    if(!(x >= x_widget && x <= x_widget + w && y >= y_widget && y <= y_widget + h))
    {
        ui->volumeSlider->hide();
    }

    //记录窗口移动的初始位置
    offset = event->globalPos() - pos();
    event->accept();
}

//窗口移动
void Widget::mouseMoveEvent(QMouseEvent *event)
{
    int x=event->pos().x();
    int y=event->pos().y();

    if((y < ui->topicLabel->geometry().height() ) && ( x < ui->closeButton->geometry().x()))
    {
        move(event->globalPos() - offset);
        event->accept();
        setCursor(Qt::ClosedHandCursor);
    }

}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    offset = QPoint();
    event->accept();
    setCursor(Qt::ArrowCursor);
}

void Widget::init_UI()
{
    this->setWindowTitle("MelodyCore");
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);  //窗口为无边框，同时保留系统菜单和最小化按钮
    this->setFixedSize(this->geometry().size());  // 禁止窗口改变尺寸

    ui->volumeSlider->hide();
    ui->homePageButton->setIcon(QIcon(":/new/prefix1/pic/homePage.png"));
    ui->myPageButton->setIcon(QIcon(":/new/prefix1/pic/myPage.png"));

    ui->nameListWidget->verticalScrollBar()->setStyleSheet(ListWidgetStyle());
    ui->localSongListWidget->verticalScrollBar()->setStyleSheet(ListWidgetStyle());
    ui->likeListWidget->verticalScrollBar()->setStyleSheet(ListWidgetStyle());

    ui->rockButton->setIcon(QIcon(":/new/prefix1/pic/Rock.png"));
    ui->popButton->setIcon(QIcon(":/new/prefix1/pic/Pop.png"));
    ui->jazzButton->setIcon(QIcon(":/new/prefix1/pic/Jazz.png"));
    ui->classicalButton->setIcon(QIcon(":/new/prefix1/pic/Classical.png"));
    ui->hip_hopButton->setIcon(QIcon(":/new/prefix1/pic/Hip-hop.png"));
    ui->electronicButton->setIcon(QIcon(":/new/prefix1/pic/Electronic.png"));
    ui->countryButton->setIcon(QIcon(":/new/prefix1/pic/Country.png"));
    ui->folkButton->setIcon(QIcon(":/new/prefix1/pic/Folk.png"));
    ui->bluesButton->setIcon(QIcon(":/new/prefix1/pic/Blues.png"));

    ui->closeButton->setIcon(QIcon(":/new/prefix1/pic/Close.png"));
    ui->closeButton->setStyleSheet("QToolButton{border:none;color:rgb(122, 197, 205);}" "QToolButton:hover{background-color: #FF0000;border:none;color:rgb(255, 255, 255);}");
    ui->minimizeButton->setIcon(QIcon(":/new/prefix1/pic/Min.png"));
    ui->minimizeButton->setStyleSheet("QToolButton{border:none;color:rgb(122, 197, 205);}" "QToolButton:hover{background-color: #1E90FF;border:none;color:rgb(255, 255, 255);}");

    ui->closeButton->setToolTip(u8"Close");
    ui->minimizeButton->setToolTip(u8"Minimize");
    ui->playButton->setToolTip(u8"Play/Pause");
    ui->lastButton->setToolTip(u8"Last Song");
    ui->nextButton->setToolTip(u8"Next Song");
    ui->addSongListButton->setToolTip(u8"Add Songlist");
    ui->addLocalMusicButton->setToolTip(u8"Add Local Song");
    ui->volumeButton->setToolTip(u8"Volume Control");
    ui->clearButton1->setToolTip(u8"Delete Local Song");

    movieToLoad(); //加载gif

    // 跳转页面
    connect(ui->homePageButton, &QPushButton::clicked, [=]()
    {
        ui->stackedWidget->setCurrentIndex(0);
    });

    connect(ui->myPageButton, &QPushButton::clicked, [=]()
    {
        ui->stackedWidget->setCurrentIndex(1);
    });
}

void Widget::init_play()
{
    myMediaPlayer = new myMusicPlayer;

    // 接收到信号sliderNumber后，设置水平进度条的值
    connect(myMediaPlayer, &myMusicPlayer::calculateFinished, this, &Widget::sethSliderValue);

    timer = new QTimer(this);    //定时器0.5s获取一次播放进度条，刷新进度条
    connect(timer, &QTimer::timeout, myMediaPlayer, &myMusicPlayer::setSliderPlayProgress); 

    myT = new MyThread;          // 动态分配空间，不能指定父对象
    thread = new QThread(this);  // 创建子线程
    myT->moveToThread(thread);   // 把自定义线程加入到子线程中

    // 通过MyMusicPlayer类发来的信号转到子线程处理，子线程处理好后，通过信号发送回主线程调用槽函数显示当前时间
    connect(myMediaPlayer, &myMusicPlayer::playPosition, myT, &MyThread::handlePlayPosition);
    connect(myMediaPlayer, &myMusicPlayer::playDuration, myT, &MyThread::handlePlayDuration);
    connect(myT, &MyThread::completePosition, this, &Widget::showMusicPosition);
    connect(myT, &MyThread::completeDuration, this, &Widget::showMusicDuration);
    qDebug() << "Main thread: " << QThread::currentThread();
    thread->start();
}

// 显示背景 gif 动画效果
void Widget::movieToLoad()
{
    movie = new QMovie(":/new/prefix1/pic/mainPageBackground.gif");   //设置GIF格式背景图
    ui->backgroundLabel->setMovie(movie);
    movie->start();
}

void Widget::on_closeButton_clicked()
{
    close();
}

void Widget::on_minimizeButton_clicked()
{
    showMinimized();
}

// 当点击上一曲/下一曲后如果成功则播放
void Widget::setPreOrNextPlay()
{
    useSSHPlayer = 0;  // 本地音乐播放模式
    mediaPath = musicFileIndex.key(valueindex);  //通过对应的索引值找到相应的键值
    qDebug() << "value:" << valueindex << "key:" << mediaPath;
    QListWidgetItem *item = ui->localSongListWidget->item(musicFileIndex.value(mediaPath) - 1);
    if (!item) {
        qWarning() << "setPreOrNextPlay: 无法获取列表项，索引越界。";
        return;
    }
    item->setSelected(true);        //设置是否选中 选中则有高亮

    if (!myMediaPlayer) {
        qWarning() << "setPreOrNextPlay: 本地播放器未初始化。";
        return;
    }

    if (timer) {
        timer->stop();
    }
    myMediaPlayer->pauseMusic();    //先关闭，然后播放

    ui->playButton->setStyleSheet(PaseStyle());
    myMediaPlayer->playMusic(this->mediaPath);
    if (timer) {
        timer->start(500);
    }
}

// 设置进度条，当歌曲播放时，返回当前播放进度
void Widget::sethSliderValue(const int &number)
{
    ui->musicSlider->setValue(number);
}


void Widget::on_playButton_clicked()
{
    if (this->mediaPath.isEmpty())
    {
        return;
    }

    if (useSSHPlayer)
    {
        // 控制曲库音乐播放器
        if (!musicPlayer) {
            qWarning() << "on_playButton_clicked: 曲库播放器未初始化。";
            return;
        }

        if (musicPlayer->isToplay())
        { // 正在播放，暂停
            ui->playButton->setStyleSheet(PlayStyle());
            musicPlayer->pausePlayback();
        }
        else
        { // 暂停或停止中，恢复播放
            ui->playButton->setStyleSheet(PaseStyle());
            musicPlayer->pausePlayback(); // pausePlayback 内部已实现 play/resume 切换
        }
    }
    else
    {
        // 控制本地音乐播放器
        if (!myMediaPlayer) {
            qWarning() << "on_playButton_clicked: 本地播放器未初始化。";
            return;
        }

        if (myMediaPlayer->isMyMusicPlayerPlaying())
        { // 正在播放, 将其关闭
            ui->playButton->setStyleSheet(PlayStyle());
            myMediaPlayer->pauseMusic();
            if (timer) {
                timer->stop();
            }
        }
        else
        { // 没有播放，点击后播放
            ui->playButton->setStyleSheet(PaseStyle());
            myMediaPlayer->playMusic(this->mediaPath);
            if (timer) {
                timer->start(500);
            }
        }
    }
}

/*
1. 如果音乐路径是空，则无效
2. ListWidget列表是从0开始，musicFileIndex的value值是从1开始的，因此要减去1
*/
void Widget::on_lastButton_clicked()
{
    if (this->mediaPath.isEmpty())
    {
        return;
    }

    // 设置是否选中，选中则有高亮，切换歌曲的时候当前的高亮关闭
    QListWidgetItem *item = ui->localSongListWidget->item(musicFileIndex.value(mediaPath) - 1);
    item->setSelected(false);

    // 通过哈希表的键找到对应的索引值，索引值必须大于等于0
    valueindex = musicFileIndex.value(mediaPath) - 1;
    if (valueindex >= 1)
    {
        setPreOrNextPlay();
    }
    else
    {   // 设置播放列表循环
        valueindex = countFiles;
        setPreOrNextPlay();
    }
}

void Widget::on_nextButton_clicked()
{
    if (this->mediaPath.isEmpty())
    {
        return;
    }

    QListWidgetItem *item = ui->localSongListWidget->item(musicFileIndex.value(mediaPath) - 1);
    item->setSelected(false);

    valueindex = musicFileIndex.value(mediaPath) + 1;

    if (valueindex <= countFiles)
    {
        qDebug() << "valueindex:" << valueindex;
        setPreOrNextPlay();
    }
    else
    {   
        valueindex = 1;
        setPreOrNextPlay();
    }
}

void Widget::on_volumeButton_clicked()
{
    if (ui->volumeSlider->isHidden())
    {
        ui->volumeSlider->show();
    }
    else
    {
        ui->volumeSlider->hide();
    }
}

void Widget::on_addLocalMusicButton_clicked()
{
    QFileDialog fileDialog(this);  //创建文件对话框
    fileDialog.setFileMode(QFileDialog::AnyFile);  // 设置文件对话框的文件模式为任意文件
    fileDialog.setNameFilter("Music Files (*.mp3 *.wav *.wma)");  //设置文件对话框的过滤器，只显示符合指定模式的文件

    if (fileDialog.exec())
    { 
        QStringList fileNames = fileDialog.selectedFiles();  // 设置只能选择一个文件
        QString fileName = fileNames[0];
        ui->localSongListWidget->addItem(fileName);  // 将每个文件添加到QListWidget控件
        
        // 文件名作为key，文件号作为value，值从1开始 将索引和文件路径添加到映射中
        musicFileIndex[fileName] = countFiles + 1;
        qDebug() << "value:" << musicFileIndex.value(fileName) << ";  key:" << musicFileIndex.key(countFiles + 1);
    }

    countFiles = ui->localSongListWidget->count();  // 计算添加的音乐文件数目
    qDebug() << "Total dded songs count:" << countFiles;
}

// 双击localSongList清单播放
void Widget::on_localSongListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (!myMediaPlayer) {
        qWarning() << "on_localSongListWidget_itemDoubleClicked: 本地播放器未初始化。";
        return;
    }

    if (timer) {
        timer->stop();
    }
    this->mediaPath = item->text();
    useSSHPlayer = 0;  // 明确切换到本地播放器模式
    qDebug() << "Success to play the local song: " << mediaPath << endl;
    myMediaPlayer->pauseMusic();
    ui->playButton->setStyleSheet(PaseStyle());
    myMediaPlayer->playMusic(this->mediaPath);
    if (timer) {
        timer->start(500);
    }
}

// 单击选中
void Widget::on_localSongListWidget_itemClicked(QListWidgetItem *item)
{
    this->mediaPath = item->text();
}

void Widget::showMusicPosition(QString time)
{
    ui->curTimeLabel->setText(time);
}

// 显示音乐总时间
void Widget::showMusicDuration(QString time)
{
    ui->endTimeLabel->setText(time);
}

// 设置音乐播放器的音量
void Widget::on_volumeSlider_valueChanged(int value)
{
    if (musicPlayer)
    {
        musicPlayer->setMusicVolume(value);
    }
    else if (myMediaPlayer)
    {
        myMediaPlayer->setMusicVolume(value);
    }
    else
    {
        qWarning() << "on_volumeSlider_valueChanged: 无可用播放器实例。";
    }
}

// 当滑动滑块时暂停播放，并关闭进度条刷新定时器（仅本地音乐需要定时器）
void Widget::on_musicSlider_valueChanged(int value)
{
    if (ui->musicSlider->isSliderDown())
    {
        // isSliderDown判断滑块是否被按下
        ui->playButton->setStyleSheet(PlayStyle());
        if (useSSHPlayer) {
            if (musicPlayer) {
                musicPlayer->pausePlayback();
            }
        } else {
            if (myMediaPlayer) {
                myMediaPlayer->pauseMusic(); // 关闭播放
            }
            if (timer && timer->isActive())
            {
                timer->stop(); // 关闭定时器
            }
        }

        // 记录滑块的值
        this->sliderPlayValue = value;
    }
}

// 当释放滑块后读取现在的值并恢复播放
void Widget::on_musicSlider_sliderReleased()
{
    if (mediaPath.isEmpty())
    {
        return;
    }

    if (useSSHPlayer) {
        if (!musicPlayer) {
            qWarning() << "on_musicSlider_sliderReleased: 曲库播放器未初始化。";
            return;
        }
        ui->playButton->setStyleSheet(PaseStyle());
        musicPlayer->pausePlayback(); // 恢复播放
    } else {
        if (!myMediaPlayer) {
            qWarning() << "on_musicSlider_sliderReleased: 本地播放器未初始化。";
            return;
        }

        myMediaPlayer->setMusicPosition(this->sliderPlayValue); // 滑块的值传递改变音乐位置
        ui->playButton->setStyleSheet(PaseStyle());  // 恢复播放
        if (timer) {
            timer->start(500);
        }
    }
}

void Widget::printPlaylist(QSqlQueryModel* model)
{
    // 清理旧模型，防止内存泄漏
    QSqlQueryModel *oldModel = qobject_cast<QSqlQueryModel*>(ui->styleSongtableView->model());
    if (oldModel) {
        oldModel->deleteLater();
    }

    // 断开旧信号，防止重复连接导致槽函数被多次触发
    disconnect(ui->styleSongtableView, &QTableView::customContextMenuRequested, this, &Widget::showSongContextMenu);
    disconnect(ui->styleSongtableView, &QTableView::clicked, this, &Widget::playSelectedSong);

    ui->styleSongtableView->setModel(model);

    // 设置表头名称
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Title"));
    model->setHeaderData(2, Qt::Horizontal, tr("Style"));

    // 允许选择行
    ui->styleSongtableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->styleSongtableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置右键菜单
    ui->styleSongtableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->styleSongtableView, &QTableView::customContextMenuRequested, this, &Widget::showSongContextMenu);

    // 设置单击行时播放歌曲
    connect(ui->styleSongtableView, &QTableView::clicked, this, &Widget::playSelectedSong);

    ui->stackedWidget->setCurrentIndex(2);
}

void Widget::on_classicalButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'classical'");
    printPlaylist(model);
}

void Widget::on_jazzButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'jazz'");
    printPlaylist(model);
}

void Widget::on_popButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'pop'");
    printPlaylist(model);
}

void Widget::on_rockButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'rock'");
    printPlaylist(model);
}

void Widget::on_hip_hopButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'hip-hop'");
    printPlaylist(model);
}

void Widget::on_electronicButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'electronic'");
    printPlaylist(model);
}

void Widget::on_countryButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'country'");
    printPlaylist(model);
}

void Widget::on_folkButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'folk'");
    printPlaylist(model);
}

void Widget::on_bluesButton_clicked()
{
    QSqlQueryModel* model = new QSqlQueryModel(ui->styleSongtableView);

    // 修改SQL查询语句，仅选择歌曲名和风格列
    model->setQuery("SELECT id, title, style FROM songs WHERE style = 'blues'");
    printPlaylist(model);
}


void Widget::playSelectedSong(const QModelIndex& index)
{
    if (musicPlayer)
    {
        musicPlayer->stopPlayback();  // 停止当前播放
        delete musicPlayer;           // 删除旧的音乐播放器实例
        musicPlayer = nullptr;
    }

    if (myMediaPlayer) {
        myMediaPlayer->stopMusic(); // 停止本地音乐，保留实例以便后续复用
    }
    useSSHPlayer = 1;
    QSqlQueryModel* model = qobject_cast<QSqlQueryModel*>(ui->styleSongtableView->model());
    QItemSelectionModel* selection = ui->styleSongtableView->selectionModel();

    if (!selection->hasSelection())
    {
        QMessageBox::warning(this, "No Selection", "Please select a song.");
        return;
    }

    currentSongId = model->data(model->index(index.row(), 0)).toInt();

    // 查询当前歌曲的风格 ID，供 like/dislike 功能使用
    QSqlQuery styleQuery;
    styleQuery.prepare("SELECT style FROM songs WHERE id = :id");
    styleQuery.bindValue(":id", currentSongId);
    if (styleQuery.exec() && styleQuery.next()) {
        QString styleName = styleQuery.value(0).toString();
        styleQuery.prepare("SELECT id FROM styles WHERE style_name = :name");
        styleQuery.bindValue(":name", styleName);
        if (styleQuery.exec() && styleQuery.next()) {
            currentStyleId = styleQuery.value(0).toInt();
        }
    }

    // 查询当前用户是否是VIP
    QSqlQuery userQuery;
    userQuery.prepare("SELECT vip FROM users WHERE id = :userId");
    userQuery.bindValue(":userId", userId); // 假设currentUserId是当前用户的ID
    userQuery.exec();

    bool isUserVip = false;
    if (userQuery.next()) {
        isUserVip = userQuery.value(0).toBool();
    }
    else {
        QMessageBox::warning(this, "Error", "Failed to retrieve user information.");
        return;
    }

    // 查询数据库获取对应的文件路径和VIP状态
    QSqlQuery query;
    query.prepare("SELECT file_path, vip FROM songs WHERE id = :id");
    query.bindValue(":id", currentSongId);
    query.exec();

    QString songPath;
    bool isSongVip = false;
    if (query.next()) {
        songPath = query.value(0).toString();
        isSongVip = query.value(1).toBool();
    }
    else {
        QMessageBox::warning(this, "Error", "Failed to retrieve the song path.");
        return;
    }

    // 判断是否允许播放
    if (isSongVip && !isUserVip) {
        QMessageBox::warning(this, "VIP Access Required", "This song is for VIP members only. Please upgrade your membership to access this content.");
        return;
    }

    mediaPath = songPath;
    musicPlayer = new MusicPlayer(this);
    connect(musicPlayer, &MusicPlayer::calculateFinished, this, &Widget::sethSliderValue);
    connect(musicPlayer, &MusicPlayer::playPosition, myT, &MyThread::handlePlayPosition);
    connect(musicPlayer, &MusicPlayer::playDuration, myT, &MyThread::handlePlayDuration);
    musicPlayer->playSongById(currentSongId, songPath);
}

void Widget::on_clearButton1_clicked()
{
    QMessageBox::StandardButton btn;
    btn = QMessageBox::question(this, "Warning", "This operation is irreversible! \nAre you sure you want to clear it?", QMessageBox::Yes|QMessageBox::No);
    if (btn == QMessageBox::Yes)
    {
        ui->localSongListWidget->clear();
    }
}

void Widget::on_addSongListButton_clicked()
{
    QString newListName = "newlist";

    // 获取新的id
    QSqlQuery query;
    query.exec("SELECT MAX(id) FROM playlists");
    int newId = 1;
    if (query.next()) {
        newId = query.value(0).toInt() + 1;
    }

    // 创建QListWidgetItem并设置歌单ID
    QListWidgetItem* newItem = new QListWidgetItem(newListName);
    newItem->setForeground(QBrush(QColor(255, 240, 160)));
    newItem->setData(Qt::UserRole, newId); // 存储歌单ID
    ui->nameListWidget->addItem(newItem);

    // 插入到数据库
    query.prepare("INSERT INTO playlists (id, name, user_id) VALUES (?, ?, ?)");
    query.addBindValue(newId);
    query.addBindValue(newListName);
    query.addBindValue(userId);
    query.exec();

    // 更新当前用户的play_list列
    query.prepare("SELECT play_list FROM users WHERE id = ?");
    query.addBindValue(userId);
    query.exec();
    QString playList;
    if (query.next()) {
        playList = query.value(0).toString();
    }
    if (!playList.isEmpty()) {
        playList += "," + QString::number(newId);
    } else {
        playList = QString::number(newId);
    }
    query.prepare("UPDATE users SET play_list = ? WHERE id = ?");
    query.addBindValue(playList);
    query.addBindValue(userId);
    query.exec();

    // 右键菜单信号已在 Widget 构造函数中统一连接，此处无需重复连接
}

void Widget::showContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction* action1 = new QAction("Rename", this);
    connect(action1, &QAction::triggered, this, &Widget::renamePlaylist);
    contextMenu.addAction(action1);

    QAction* action2 = new QAction("Delete", this);
    connect(action2, &QAction::triggered, this, &Widget::deletePlaylist);
    contextMenu.addAction(action2);

    contextMenu.exec(ui->nameListWidget->mapToGlobal(pos));
}

void Widget::renamePlaylist()
{
    QListWidgetItem* selectedItem = ui->nameListWidget->currentItem();
    if (selectedItem) {
        QString oldName = selectedItem->text();
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Rename Playlist"), tr("New name:"), QLineEdit::Normal, oldName, &ok);
        if (ok && !newName.isEmpty()) {
            selectedItem->setText(newName);
            int playlistId = selectedItem->data(Qt::UserRole).toInt(); // 获取歌单ID

            // 更新数据库
            QSqlQuery query;
            query.prepare("UPDATE playlists SET name = ? WHERE id = ? AND user_id = ?");
            query.addBindValue(newName);
            query.addBindValue(playlistId); // 使用歌单ID进行更新
            query.addBindValue(userId); // 使用当前用户ID进行更新
            query.exec();

            // 关闭菜单
            QWidget* widget = qobject_cast<QWidget*>(sender()->parent());
            if (widget) {
                QMenu* menu = qobject_cast<QMenu*>(widget);
                if (menu) {
                    menu->hide();
                }
            }
        }
    }
}

void Widget::deletePlaylist()
{
    QListWidgetItem* selectedItem = ui->nameListWidget->currentItem();
    if (selectedItem) {
        int playlistId = selectedItem->data(Qt::UserRole).toInt(); // 获取歌单ID
        delete selectedItem;

        // 从数据库中删除
        QSqlQuery query;
        query.prepare("DELETE FROM playlists WHERE id = ? AND user_id = ?");
        query.addBindValue(playlistId); // 使用歌单ID进行删除
        query.addBindValue(userId); // 使用当前用户ID进行删除
        query.exec();

        // 更新当前用户的play_list列
        query.prepare("SELECT play_list FROM users WHERE id = ?");
        query.addBindValue(userId);
        query.exec();
        QString playList;
        if (query.next()) {
            playList = query.value(0).toString();
        }
        QStringList playListItems = playList.split(",");
        playListItems.removeAll(QString::number(playlistId));
        playList = playListItems.join(",");
        query.prepare("UPDATE users SET play_list = ? WHERE id = ?");
        query.addBindValue(playList);
        query.addBindValue(userId);
        query.exec();

        // 关闭菜单
        QWidget* widget = qobject_cast<QWidget*>(sender()->parent());
        if (widget) {
            QMenu* menu = qobject_cast<QMenu*>(widget);
            if (menu) {
                menu->hide();
            }
        }
    }
}

void Widget::showSongContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Add to Playlist"), this);

    // 获取当前登录用户的所有歌单
    QSqlQuery query;
    query.prepare("SELECT id, name FROM playlists WHERE user_id = ?");
    query.addBindValue(userId);
    query.exec();

    while (query.next()) {
        int playlistId = query.value(0).toInt();
        QString playlistName = query.value(1).toString();
        QAction* action = new QAction("Add to Playlist " + playlistName, this);
        action->setData(playlistId); // 将playlistId存储在action中
        connect(action, &QAction::triggered, this, &Widget::addToPlaylist);
        contextMenu.addAction(action);
    }

    contextMenu.exec(ui->styleSongtableView->viewport()->mapToGlobal(pos));
}


void Widget::addToPlaylist()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        int playlistId = action->data().toInt(); // 获取存储在action中的playlistId

        // 获取选中的歌曲ID
        QItemSelectionModel* selection = ui->styleSongtableView->selectionModel();
        QModelIndexList selectedRows = selection->selectedRows();
        if (!selectedRows.isEmpty()) {
            int songId = selectedRows.first().data().toInt();

            // 检查歌曲是否已经在歌单中
            QSqlQuery query;
            query.prepare("SELECT song_list FROM playlists WHERE id = ?");
            query.addBindValue(playlistId);
            query.exec();
            QString songList;
            if (query.next()) {
                songList = query.value(0).toString();
            }
            QStringList songIds = songList.split(",");
            if (songIds.contains(QString::number(songId))) {
                QMessageBox::information(this, tr("Info"), tr("This Playlist already includes that song！"));
                return;
            }

            // 更新歌单的song_list列
            if (!songList.isEmpty()) {
                songList += "," + QString::number(songId);
            }
            else {
                songList = QString::number(songId);
            }
            query.prepare("UPDATE playlists SET song_list = ? WHERE id = ?");
            query.addBindValue(songList);
            query.addBindValue(playlistId);
            query.exec();
        }

        // 关闭菜单
        QWidget* widget = action->parentWidget();
        if (widget) {
            QMenu* menu = qobject_cast<QMenu*>(widget);
            if (menu) {
                menu->hide();
            }
        }
    }
}



void Widget::on_nameListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (musicPlayer) {
        musicPlayer->stopPlayback();  // 停止当前播放
        delete musicPlayer;  // 删除旧的音乐播放器实例
        musicPlayer = nullptr;
    }
    useSSHPlayer = 0;
    int playlistId = item->data(Qt::UserRole).toInt(); // 获取歌单ID

    QSqlQuery query;
    query.prepare("SELECT song_list FROM playlists WHERE id = ?");
    query.addBindValue(playlistId);
    query.exec();

    QString songList;
    if (query.next()) {
        songList = query.value(0).toString();
    }

    QStringList songIds = songList.split(",");
    // 过滤非数字项，防止 SQL 语法错误与注入风险
    QStringList validSongIds;
    for (const QString &sid : songIds) {
        bool ok;
        sid.trimmed().toInt(&ok);
        if (ok) validSongIds.append(sid.trimmed());
    }

    // 清理旧模型，防止内存泄漏
    QSqlQueryModel *oldModel = qobject_cast<QSqlQueryModel*>(ui->styleSongtableView->model());
    if (oldModel) {
        oldModel->deleteLater();
    }

    // 断开旧信号，防止重复连接
    disconnect(ui->styleSongtableView, &QTableView::customContextMenuRequested, this, &Widget::showSongContextMenu);
    disconnect(ui->styleSongtableView, &QTableView::clicked, this, &Widget::playSelectedSong);

    QSqlQueryModel *model = new QSqlQueryModel(ui->styleSongtableView);
    if (validSongIds.isEmpty()) {
        // 无有效歌曲ID时返回空结果集，避免 IN () 语法错误
        model->setQuery("SELECT id, title, style FROM songs WHERE 1=0");
    } else {
        QString queryStr = "SELECT id, title, style FROM songs WHERE id IN (" + validSongIds.join(",") + ")";
        model->setQuery(queryStr);
    }
    ui->styleSongtableView->setModel(model);

    // 设置表头名称
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Title"));
    model->setHeaderData(2, Qt::Horizontal, tr("Style"));

    // 允许选择行
    ui->styleSongtableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->styleSongtableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置右键菜单
    ui->styleSongtableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->styleSongtableView, &QTableView::customContextMenuRequested, this, &Widget::showSongContextMenu);

    //单击播放音乐
    connect(ui->styleSongtableView, &QTableView::clicked, this, &Widget::playSelectedSong);

    ui->stackedWidget->setCurrentIndex(2);
}

void Widget::loadUserPlaylists()
{
    QSqlQuery query;
    query.prepare("SELECT id, name FROM playlists WHERE user_id = ?");
    query.addBindValue(userId);
    query.exec();

    while (query.next()) {
        int playlistId = query.value(0).toInt();
        QString playlistName = query.value(1).toString();
        QListWidgetItem* item = new QListWidgetItem(playlistName);
        item->setForeground(QBrush(QColor(255, 240, 160)));
        item->setData(Qt::UserRole, playlistId);
        ui->nameListWidget->addItem(item);
    }

}

void Widget::recommendSongs(int userId)
{
    // 清空推荐列表
        ui->recommendListWidget->clear();

        // 获取用户喜好风格
        QSqlQuery query;
        query.prepare("SELECT favorite_styles FROM users WHERE id = ?");
        query.addBindValue(userId);
        if (!query.exec() || !query.next()) {
            qDebug() << "Failed to get favorite styles for user" << userId;
            return;
        }

        // 解析用户喜好风格分数
        QString favoriteStylesStr = query.value(0).toString();
        QStringList favoriteStylesList = favoriteStylesStr.split(",");
        if (favoriteStylesList.isEmpty()) {
            qDebug() << "Failed to parse favorite styles for user" << userId;
            return;
        }

        QList<StyleScore> styleScores; // 使用结构体
        int totalScore = 0;
        for (int i = 1; i < favoriteStylesList.size(); ++i) { // 从1开始，跳过第一个数
            StyleScore styleScore;
            styleScore.styleId = i; // styleId 从 1 开始
            styleScore.score = favoriteStylesList[i].toInt();
            totalScore += styleScore.score;
            styleScores.append(styleScore);
        }

        // 按分数排序风格
        std::sort(styleScores.begin(), styleScores.end(), [](const StyleScore &a, const StyleScore &b) {
            return a.score > b.score;
        });

        // 计算每种风格推荐的歌曲数目
        QList<StyleSongCount> styleSongCounts; // 使用结构体
        int totalRecommendations = 0;
        for (const StyleScore &styleScore : styleScores) {
            if (totalRecommendations >= 10) break; // 最多推荐10首歌

            int maxSongsForStyle = std::min(5, 10 - totalRecommendations); // 每种风格最多推荐5首
            int songCount = (totalScore > 0) ? (styleScore.score * 10 / totalScore) : 0;
            songCount = std::min(songCount, maxSongsForStyle);

            if (songCount > 0) {
                StyleSongCount styleSongCount;
                styleSongCount.styleId = styleScore.styleId;
                styleSongCount.songCount = songCount;
                styleSongCounts.append(styleSongCount);
                totalRecommendations += songCount;
            }
        }

        QList<int> recommendedSongs;
        std::random_device rd;
        std::mt19937 g(rd());

        // 兜底：若用户没有任何偏好（totalScore == 0），直接从所有风格中随机抽取歌曲
        if (totalScore == 0 || styleSongCounts.isEmpty()) {
            query.prepare("SELECT id FROM songs ORDER BY RANDOM() LIMIT 10");
            if (query.exec()) {
                while (query.next()) {
                    recommendedSongs.append(query.value(0).toInt());
                }
            }
        }

        // 按偏好比例收集推荐歌曲
        for (const StyleSongCount &styleSongCount : styleSongCounts)
        {
            int styleId = styleSongCount.styleId;
            int songCount = styleSongCount.songCount;

            // 获取对应风格的歌曲列表
            query.prepare("SELECT song_list FROM styles WHERE id = ?");
            query.addBindValue(styleId);
            if (!query.exec() || !query.next()) continue;

            // 解析歌曲ID列表
            QStringList songList = query.value(0).toString().split(",");
            QList<int> songIds;
            for (const QString &songIdStr : songList) {
                bool ok;
                int songId = songIdStr.toInt(&ok);
                if (ok) songIds.append(songId);
            }

            // 随机打乱并选择要推荐的歌曲
            std::shuffle(songIds.begin(), songIds.end(), g);
            for (int i = 0; i < std::min(songCount, songIds.size()); ++i) {
                recommendedSongs.append(songIds[i]);
            }
        }

        // 确保有10首歌曲，若不足则随机补足（带安全计数器防止死循环）
        int safetyCounter = 0;
        while (recommendedSongs.size() < 10 && safetyCounter < 100) {
            int additionalSongsNeeded = 10 - recommendedSongs.size();
            for (const StyleSongCount &styleSongCount : styleSongCounts) {
                if (additionalSongsNeeded <= 0) break;
                int styleId = styleSongCount.styleId;

                // 获取对应风格的歌曲列表
                query.prepare("SELECT song_list FROM styles WHERE id = ?");
                query.addBindValue(styleId);
                if (!query.exec() || !query.next()) continue;

                // 解析歌曲ID列表
                QStringList songList = query.value(0).toString().split(",");
                QList<int> songIds;
                for (const QString &songIdStr : songList) {
                    bool ok;
                    int songId = songIdStr.toInt(&ok);
                    if (ok) songIds.append(songId);
                }

                // 随机打乱并选择要推荐的歌曲
                std::shuffle(songIds.begin(), songIds.end(), g);
                for (int i = 0; i < std::min(additionalSongsNeeded, songIds.size()); ++i) {
                    recommendedSongs.append(songIds[i]);
                    --additionalSongsNeeded;
                    if (additionalSongsNeeded <= 0) break;
                }
            }

            // 若仍不足，从全库随机补充，避免空 styleSongCounts 导致死循环
            if (recommendedSongs.size() < 10 && additionalSongsNeeded > 0) {
                query.prepare("SELECT id FROM songs ORDER BY RANDOM() LIMIT ?");
                query.addBindValue(additionalSongsNeeded);
                if (query.exec()) {
                    while (query.next()) {
                        int sid = query.value(0).toInt();
                        if (!recommendedSongs.contains(sid)) {
                            recommendedSongs.append(sid);
                        }
                    }
                }
            }

            ++safetyCounter;
        }

        // 随机打乱推荐的歌曲ID顺序
        std::shuffle(recommendedSongs.begin(), recommendedSongs.end(), g);

        // 打印推荐的歌曲到 recommendListWidget
        for (int songId : recommendedSongs) {
            query.prepare("SELECT id, title, style FROM songs WHERE id = ?");
            query.addBindValue(songId);
            if (!query.exec() || !query.next()) continue;

            QString songInfo = QString("%1 - %2 (%3)").arg(query.value(0).toString()).arg(query.value(1).toString()).arg(query.value(2).toString());
            QListWidgetItem *item = new QListWidgetItem(songInfo);
            item->setData(Qt::UserRole, songId); // 将歌曲ID存储在QListWidgetItem中
            ui->recommendListWidget->addItem(item);
        }

        // 右键菜单与单击播放信号已在 Widget 构造函数中统一连接，此处无需重复连接
}

void Widget::showRecommendSongContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Add to Playlist"), this);

    // 获取当前登录用户的所有歌单
    QSqlQuery query;
    query.prepare("SELECT id, name FROM playlists WHERE user_id = ?");
    query.addBindValue(userId);
    query.exec();

    while (query.next()) {
        int playlistId = query.value(0).toInt();
        QString playlistName = query.value(1).toString();
        QAction* action = new QAction("Add to Playlist " + playlistName, this);
        action->setData(playlistId); // 将playlistId存储在action中
        connect(action, &QAction::triggered, this, &Widget::addToPlaylistFromRecommend);
        contextMenu.addAction(action);
    }

    contextMenu.exec(ui->recommendListWidget->viewport()->mapToGlobal(pos));
}

void Widget::addToPlaylistFromRecommend()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        int playlistId = action->data().toInt(); // 获取存储在action中的playlistId

        // 获取选中的歌曲ID
        QListWidgetItem* item = ui->recommendListWidget->currentItem();
        if (!item) return;
        int songId = item->data(Qt::UserRole).toInt();

        // 检查歌曲是否已经在歌单中
        QSqlQuery query;
        query.prepare("SELECT song_list FROM playlists WHERE id = ?");
        query.addBindValue(playlistId);
        query.exec();
        QString songList;
        if (query.next()) {
            songList = query.value(0).toString();
        }
        QStringList songIds = songList.split(",");
        if (songIds.contains(QString::number(songId))) {
            QMessageBox::information(this, tr("Info"), tr("This Playlist already includes that song！"));
            return;
        }

        // 更新歌单的song_list列
        if (!songList.isEmpty()) {
            songList += "," + QString::number(songId);
        }
        else {
            songList = QString::number(songId);
        }
        query.prepare("UPDATE playlists SET song_list = ? WHERE id = ?");
        query.addBindValue(songList);
        query.addBindValue(playlistId);
        query.exec();

        // 关闭菜单
        QWidget* widget = action->parentWidget();
        if (widget) {
            QMenu* menu = qobject_cast<QMenu*>(widget);
            if (menu) {
                menu->hide();
            }
        }
    }
}

void Widget::on_searchButton_clicked()
{
    QString searchText = ui->searchEdit->text().trimmed(); // 获取搜索框中的文本
    if (searchText.isEmpty()) {
        QMessageBox::information(this, tr("Info"), tr("Please enter a search keyword."));
        return;
    }

    // 清理旧模型，防止内存泄漏
    QSqlQueryModel *oldModel = qobject_cast<QSqlQueryModel*>(ui->styleSongtableView->model());
    if (oldModel) {
        oldModel->deleteLater();
    }

    // 断开旧信号，防止重复连接导致槽函数被多次触发
    disconnect(ui->styleSongtableView, &QTableView::customContextMenuRequested, this, &Widget::showSongContextMenu);
    disconnect(ui->styleSongtableView, &QTableView::clicked, this, &Widget::playSelectedSong);

    // 使用参数化查询防止 SQL 注入
    QSqlQueryModel *model = new QSqlQueryModel(ui->styleSongtableView);
    QSqlQuery query;
    query.prepare("SELECT id, title, style FROM songs WHERE title LIKE :pattern");
    query.bindValue(":pattern", "%" + searchText + "%");
    query.exec();
    model->setQuery(query);

    ui->styleSongtableView->setModel(model);

    // 设置表头名称
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Title"));
    model->setHeaderData(2, Qt::Horizontal, tr("Style"));

    // 允许选择行
    ui->styleSongtableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->styleSongtableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置右键菜单
    ui->styleSongtableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->styleSongtableView, &QTableView::customContextMenuRequested, this, &Widget::showSongContextMenu);

    // 单击行时播放歌曲
    connect(ui->styleSongtableView, &QTableView::clicked, this, &Widget::playSelectedSong);

    // 如果没有找到歌曲，显示提示信息
    if (model->rowCount() == 0) {
        QMessageBox::information(this, tr("Info"), tr("The song is not found."));
    }

    // 切换到显示搜索结果的页面
    ui->stackedWidget->setCurrentIndex(2);
}

void Widget::on_recommendButton_clicked()
{
     recommendSongs(userId); // 使用当前用户ID推荐歌曲
}

void Widget::on_recommendListWidget_itemClicked(QListWidgetItem* item)
{
    int songId = item->data(Qt::UserRole).toInt();
    currentSongId = songId;

    // 查询数据库获取对应的文件路径和风格
    QSqlQuery query;
    query.prepare("SELECT file_path, style FROM songs WHERE id = :id");
    query.bindValue(":id", songId);
    query.exec();

    QString songPath;
    QString styleName;
    if (query.next()) {
        songPath  = query.value(0).toString();
        styleName = query.value(1).toString();
    }
    else {
        QMessageBox::warning(this, "Error", "Failed to retrieve the song path.");
        return;
    }

    // 查询风格 ID
    QSqlQuery styleQuery;
    styleQuery.prepare("SELECT id FROM styles WHERE style_name = :name");
    styleQuery.bindValue(":name", styleName);
    if (styleQuery.exec() && styleQuery.next()) {
        currentStyleId = styleQuery.value(0).toInt();
    }

    // 停止本地播放器并切换到曲库播放器（保留实例以便后续切回本地）
    if (myMediaPlayer) {
        myMediaPlayer->stopMusic();
    }

    // 播放选中的歌曲
    if (musicPlayer) {
        musicPlayer->stopPlayback();
        delete musicPlayer;
        musicPlayer = nullptr;
    }

    useSSHPlayer = 1;
    mediaPath = songPath;
    musicPlayer = new MusicPlayer(this);
    connect(musicPlayer, &MusicPlayer::calculateFinished, this, &Widget::sethSliderValue);
    connect(musicPlayer, &MusicPlayer::playPosition, myT, &MyThread::handlePlayPosition);
    connect(musicPlayer, &MusicPlayer::playDuration, myT, &MyThread::handlePlayDuration);
    musicPlayer->playSongById(songId, songPath);
}

void Widget::on_vipButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Upgrade to VIP", "Do you want to spend 30 to upgrade to a VIP user?", QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QSqlQuery query;
        query.prepare("SELECT vip FROM users WHERE id = ?");
        query.addBindValue(userId);

        if (!query.exec() || !query.next()) {
            qDebug() << "Failed to query vip status for user" << userId << query.lastError();
            return;
        }

        int vipStatus = query.value(0).toInt();
        if (vipStatus == 0) {
            query.prepare("UPDATE users SET vip = 1 WHERE id = ?");
            query.addBindValue(userId);

            if (query.exec()) {
                QMessageBox::information(this, "Success", "You have successfully upgraded to a VIP user.");
            } else {
                qDebug() << "Failed to upgrade to VIP for user" << userId << query.lastError();
            }
        } else {
            QMessageBox::information(this, "Already VIP", "You are already a VIP user.");
        }
    }
}

// 增加喜好
void Widget::on_likeButton_clicked()
{
    if (currentSongId == -1 || currentStyleId <= 0)
    {
        QMessageBox::warning(this, "No Song", "No song is currently playing or style is invalid.");
        return;
    }

    adjustUserFavoriteStyle(userId, currentStyleId, 1);
}

// 减少喜好
void Widget::on_dislikeButton_clicked() {
    if (currentSongId == -1 || currentStyleId <= 0) {
        QMessageBox::warning(this, "No Song", "No song is currently playing or style is invalid.");
        return;
    }

    adjustUserFavoriteStyle(userId, currentStyleId, -1);
}

// 调整用户喜好分数
// favorite_styles 格式：10 个逗号分隔的整数，索引 0 为占位符，索引 1~9 分别对应 rock~blues
void Widget::adjustUserFavoriteStyle(int userId, int styleId, int adjustment) {
    if (styleId < 1 || styleId > 9) {
        QMessageBox::warning(this, "Error", "Invalid style ID.");
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT favorite_styles FROM users WHERE id = :id");
    query.bindValue(":id", userId);
    if (!query.exec() || !query.next()) {
        QMessageBox::warning(this, "Error", "Failed to obtain user preference style.");
        return;
    }

    QString favoriteStylesStr = query.value(0).toString();
    QStringList favoriteStylesList = favoriteStylesStr.split(',');

    // 向后兼容：旧数据可能只有 9 个元素，补齐到 10 个
    while (favoriteStylesList.size() < 10) {
        favoriteStylesList.append("0");
    }

    int currentScore = favoriteStylesList[styleId].toInt();
    int newScore = std::max(0, currentScore + adjustment);  // 确保分数不低于0
    favoriteStylesList[styleId] = QString::number(newScore);

    QString updatedFavoriteStylesStr = favoriteStylesList.join(',');

    query.prepare("UPDATE users SET favorite_styles = :favorite_styles WHERE id = :id");
    query.bindValue(":favorite_styles", updatedFavoriteStylesStr);
    query.bindValue(":id", userId);
    if (!query.exec()) {
        QMessageBox::warning(this, "Error", "Failed to update user preference style.");
    }
}

void Widget::updateLikeListWidget(int userId) 
{
    QSqlQuery query;
    query.prepare("SELECT favorite_styles FROM users WHERE id = ?");
    query.addBindValue(userId);
    if (!query.exec() || !query.next()) {
        QMessageBox::warning(this, "Error", "Failed to retrieve user favorite styles.");
        return;
    }

    QString favoriteStylesStr = query.value(0).toString();
    QStringList favoriteStylesList = favoriteStylesStr.split(",");
    if (favoriteStylesList.isEmpty()) {
        QMessageBox::warning(this, "Error", "Invalid favorite styles data.");
        return;
    }

    // 解析用户喜好风格分数
    QList<StyleScore> styleScores;
    for (int i = 1; i < favoriteStylesList.size(); ++i) { // 从1开始，跳过第一个数
        StyleScore styleScore;
        styleScore.styleId = i; // styleId 从 1 开始
        styleScore.score = favoriteStylesList[i].toInt();
        styleScores.append(styleScore);
    }

    // 按分数排序风格
    std::sort(styleScores.begin(), styleScores.end(), [](const StyleScore& a, const StyleScore& b) {
        return a.score > b.score;
        });

    // 清空likeListWidget并添加前3个风格名称
    ui->likeListWidget->clear();

    int topN = 3;
    for (int i = 0; i < std::min(topN, styleScores.size()); ++i) {
        int styleId = styleScores[i].styleId;

        // 根据styleId查找风格名称
        query.prepare("SELECT style_name FROM styles WHERE id = ?");
        query.addBindValue(styleId);
        if (!query.exec() || !query.next()) {
            QMessageBox::warning(this, "Error", "Failed to retrieve style name.");
            continue;
        }

        QString styleName = query.value(0).toString();
        ui->likeListWidget->addItem(QString("Rank %1: %2").arg(i + 1).arg(styleName));
    }
}

void Widget::on_myPageButton_clicked()
{
    updateLikeListWidget(userId); // 更新 likeListWidget 的内容
}
