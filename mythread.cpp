#include "mythread.h"
#include <QString>
#include <QDebug>

MyThread::MyThread(QObject *parent) : QObject(parent)
{
    qDebug() << "Slave Thread_Constructor function: " << QThread::currentThread();
}

MyThread::~MyThread(){
    qDebug() << "Slave Thread_Destructor function: " << QThread::currentThread();
}

// 计算当前播放位置的时间并返回字符串
void MyThread::handlePlayPosition(const int &musicTime)
{
    int h,m,s;
    h = musicTime / 3600;
    m = (musicTime - h * 3600) / 60;
    s = musicTime - h * 3600 - m * 60;
    time1 = QStringLiteral("%2:%3").arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
    emit completePosition(time1);
}

// 计算总时间然后返回（每一次播放音乐文件的总时间只需要接收一次即可）
void MyThread::handlePlayDuration(const int &musicTime)
{
    if(musicTime != time)
    {
        time = musicTime;
        int h,m,s;
        h = musicTime / 3600;
        m = (musicTime - h * 3600) / 60;
        s = musicTime - h * 3600 - m * 60;
        QString time2 = QStringLiteral("%2:%3").arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));   //计算总时间
        qDebug() << "Slave Thread_handlePlayDuration: " << QThread::currentThread();
        emit completeDuration(time2);
    }
}

