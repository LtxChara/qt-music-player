#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QObject>
#include <QThread>

/*
1. 在Qt中，信号和槽之间的参数传递是通过复制进行的。由于信号和槽机制中的参数传递是
通过复制进行的，所以参数类型必须是const的，以防止在传递过程中被修改。

2. 如果参数不是const的，那么当信号被发出时，参数的值会被复制并传递给槽。如果在这
个过程中修改了参数的值，那么原始参数的值也会被修改，然后可能报错。

3. 另外，参数类型通常是引用类型，因为这样可以避免复制大型对象，从而提高性能。如果
参数类型是值类型，那么在传递过程中会复制该值。如果该值是一个大型对象，那么复制可能
会花费相当多的时间和资源。通过使用引用类型，可以避免这种复制，从而提高程序的效率。
*/

class MyThread : public QObject
{
    Q_OBJECT

private:
    QString time1;  //当前播放时间
    int time = 0;   //得到传送过来的总时间秒数，如果和上一次不相等则更新、否则不操作

public:
    explicit MyThread(QObject *parent = nullptr);
    ~MyThread();

signals:
    void completePosition(const QString &time);
    void completeDuration(const QString &time);

public slots:
    void handlePlayPosition(const int &musicTime);
    void handlePlayDuration(const int &musicTime);

};

#endif // MYTHREAD_H
