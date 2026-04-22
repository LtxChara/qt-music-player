#pragma warning (disable:4819)
#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMediaPlaylist>
#include <QMenu>
#include <QInputDialog>
#include <QSystemTrayIcon>
#include <QtSql/QSqlDatabase> // 连接数据库
#include <QtSql/QSqlError>    // 数据库连接失败打印报错语句
#include <QtSql/QSqlQuery> // 数据库操作（增删改查）
#include <QSqlQueryModel>
#include <QDebug>
#include <QMessageBox>
#include <QVariantList> // 泛型链表，可以存储任何类型的数据
#include <QFileDialog>
#include <QListWidgetItem>
#include <QTimer>
#include <QMouseEvent>
#include <QMovie>
#include <algorithm>
#include <random>
#include <QScrollBar>
#include "mymusicplayer.h"
#include "mythread.h"
#include"musicplayer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

struct StyleScore {
    int styleId;
    int score;
};

struct StyleSongCount {
    int styleId;
    int songCount;
};


class Widget : public QWidget
{
    Q_OBJECT

private:
    Ui::Widget* ui;
    QPoint offset;           //窗口拖动时记录的起始点 
    QMovie* movie;           //背景动画gif
  
    myMusicPlayer* myMediaPlayer = nullptr;  // 本地音乐播放器
    MusicPlayer* musicPlayer = nullptr;      // 服务器音乐播放器

    int userId = -1;           // 添加用户ID成员变量
    QString userName;          // 添加用户名成员变量
    int currentSongId = -1;    // 当前播放的歌曲ID（-1 表示无歌曲正在播放）
    int currentStyleId = -1;   // 当前播放的歌曲风格ID（-1 表示未设置）
    int musiclist_index = -1;  // 用于标识现在展示的是哪个歌单
    bool useSSHPlayer = false;

    MyThread* myT = nullptr;        // 本地音乐-主线程
    QThread* thread = nullptr;      // 本地音乐-子线程
    QTimer* timer = nullptr;        // 本地音乐-读取播放器进度条的值
    int sliderPlayValue = 0;        // 本地音乐-记录读取进度条的值
    QString mediaPath;              // 本地音乐-存储当前音乐文件的目录+文件名
    QHash<QString, int> musicFileIndex;  // 本地音乐-添加一个成员变量来保存音乐文件路径和索引的映射
    int valueindex = 0;             // 本地音乐-用于判断当前上一曲或下一曲的索引value值
    int countFiles = 0;             // 本地音乐-hash key的键值value 计算添加的文件个数
    int playtime = 0;               // 本地音乐-显示当前歌曲进度时间

public:
    Widget(QWidget* parent = nullptr, int userId = -1, const QString& userName = QString());
    ~Widget();
    
    void mousePressEvent(QMouseEvent* event);    // 鼠标点击
    void mouseMoveEvent(QMouseEvent* event);     // 鼠标追踪
    void mouseReleaseEvent(QMouseEvent* event);  // 鼠标释放
    
    void init_UI();                     // UI组件额外的一些处理
    void movieToLoad();                 // 加载gif

    void init_play();                   // 播放初始化
    void setPreOrNextPlay();            // 本地音乐-当上一曲或者下一曲播放时调用
    
    void adjustUserFavoriteStyle(int userId, int styleId, int adjustment);
    void recommendSongs(int userId);    // 推荐歌曲
    void updateLikeListWidget(int userId);
    void printPlaylist(QSqlQueryModel* model);  //打印歌单
    
public slots:
    void on_closeButton_clicked();      // 点击按钮-关闭页面
    void on_minimizeButton_clicked();   // 点击按钮-最小化页面

    void on_likeButton_clicked();       // 点击按钮-喜欢正在播放的歌曲
    void on_dislikeButton_clicked();    // 点击按钮-不喜欢正在播放的歌曲

    void on_playButton_clicked();       // 点击按钮-播放歌曲

    void on_volumeButton_clicked();     // 点击按钮-音量条隐藏/出现
    void on_volumeSlider_valueChanged(int value);  // 将音量值返回给播放器

    void on_lastButton_clicked();    // 本地音乐-点击按钮-上一首歌曲
    void on_nextButton_clicked();    // 本地音乐-点击按钮-下一首歌曲
    void on_clearButton1_clicked();  // 本地音乐-清除歌曲列表

    void sethSliderValue(const int& number);  // 本地音乐
    void showMusicPosition(QString time);     // 本地音乐-显示音乐当前播放时间
    void showMusicDuration(QString time);     // 本地音乐-显示音乐总时间

    void on_addLocalMusicButton_clicked();    // 本地音乐-点击按钮-添加本地歌曲
    void on_localSongListWidget_itemClicked(QListWidgetItem* item);        // 本地音乐-单击选中歌曲
    void on_localSongListWidget_itemDoubleClicked(QListWidgetItem* item);  // 本地音乐-双击文件播放歌曲

    void on_musicSlider_valueChanged(int value);   // 本地音乐-拖动滑块改变音乐播放位置
    void on_musicSlider_sliderReleased();          // 本地音乐-释放滑块后把当前进度条值传递给播放器
    
    void on_rockButton_clicked();       // 风格歌单-打印摇滚歌曲  
    void on_popButton_clicked();        // 风格歌单-打印流行歌曲 
    void on_jazzButton_clicked();       // 风格歌单-打印爵士歌曲 
    void on_classicalButton_clicked();  // 风格歌单-打印古典歌曲 
    void on_hip_hopButton_clicked();    // 风格歌单-打印嘻哈歌曲 
    void on_electronicButton_clicked(); // 风格歌单-打印电子歌曲 
    void on_countryButton_clicked();    // 风格歌单-打印乡村歌曲 
    void on_folkButton_clicked();       // 风格歌单-打印民间歌曲 
    void on_bluesButton_clicked();      // 风格歌单-打印布鲁斯歌曲 

    void playSelectedSong(const QModelIndex& index);  // 曲库音乐-播放选中歌曲

    void loadUserPlaylists();                     // 加载用户的歌单  
    void showSongContextMenu(const QPoint& pos);  //显示当前用户的所有歌单  
    void on_addSongListButton_clicked();          // 点击按钮-增加新歌单    
    void showContextMenu(const QPoint& pos);      // 显示 “重命名” 与 “删除” 选项
    void renamePlaylist();  // 重命名歌单
    void deletePlaylist();  // 删除歌单    
    void addToPlaylist();   // 添加歌曲到歌单     
    void on_nameListWidget_itemDoubleClicked(QListWidgetItem* item);  // 打开我的歌单
    void on_recommendListWidget_itemClicked(QListWidgetItem* item);   //播放推荐歌单中的歌曲
    void showRecommendSongContextMenu(const QPoint& pos);  //在推荐歌单中显示当前用户的所有歌单
    void addToPlaylistFromRecommend();  //添加推荐歌单中的歌曲到歌单
    
    void on_searchButton_clicked();     // 搜索功能
    
    void on_recommendButton_clicked();  // 推荐功能
       
    void on_vipButton_clicked();        // vip功能
    
    void on_myPageButton_clicked();     // 显示“我喜欢”
};

#endif