#include "mymusicplayer.h"

myMusicPlayer::myMusicPlayer(QObject *parent) : QObject(parent)
{
    myPlayer = new QMediaPlayer(this);
}

myMusicPlayer::~myMusicPlayer()
{
    // 由析构函数统一释放 QMediaPlayer，避免外部悬空指针
}

void myMusicPlayer::playMusic(QString mediaPath)
{
    qDebug()<<"playMusic: myPlayer->state:"<< myPlayer->state();

    // 若当前播放器不是播放状态并且路径与当前播放路径不一致，则更新播放路径并设置媒体源
    if(QMediaPlayer:: PlayingState != myPlayer->state() && mediaPath != playpath )
    {
        playpath = mediaPath;
        myPlayer->setMedia(QUrl::fromLocalFile(playpath));
    }
    myPlayer->play();
}

void myMusicPlayer::pauseMusic()
{
    // 若当前播放器是播放状态，则暂停播放
    if(QMediaPlayer:: PlayingState == myPlayer->state())
    {
        qDebug()<<"pauseMusic: myPlayer->state:"<< myPlayer->state();
        myPlayer->pause();
    }
}

void myMusicPlayer::stopMusic()
{
    if (myPlayer)
    {
        myPlayer->stop(); // 停止播放
        qDebug() << "Music stopped.";
        // 删除职责交给析构函数，避免外部持有该实例时解引用空指针
    }
}

/*
 1.音乐在走的时候，进度条也跟随变换，把音乐当前的百分比值传递给进度条
 2.传递信号（音乐当前播放的时间位置 musicPosition），让子线程计算处理
 3.传递信号（音乐的总时间 musicDuration），让子线程处理 
 */
void myMusicPlayer::setSliderPlayProgress()
{
    // 防御除零：若总时长为 0，则跳过本次计算
    if (myPlayer->duration() <= 0) {
        return;
    }

    // 计算滑块相对于音乐当前播放时间的位置
    int sliderNumber = myPlayer->position() * 100 / myPlayer->duration();
    emit calculateFinished(sliderNumber);

    int musicPosition = myPlayer->position() / 1000;
    emit playPosition(musicPosition);

    int musicDuration = myPlayer->duration() / 1000;
    emit playDuration(musicDuration);
}

// 拖动进度条，改变滑块的值，传递给音乐，改变音乐的播放位置，然后播放
void myMusicPlayer::setMusicPosition(int sliderPlayValue)
{
    if (myPlayer->duration() <= 0) {
        return;
    }
    myPlayer->setPosition(myPlayer->duration() * sliderPlayValue / 100);
    myPlayer->play();
}

// 设置播放器音量
void myMusicPlayer::setMusicVolume(int volume)
{
    myPlayer->setVolume(volume);
}


// 判断当前是否播放
bool myMusicPlayer::isMyMusicPlayerPlaying()
{
    return (QMediaPlayer:: PlayingState == myPlayer->state());
}