#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

class MusicPlayer : public QObject {
    Q_OBJECT

public:
    explicit MusicPlayer(QObject* parent = nullptr);
    ~MusicPlayer();

    void playSongById(int songId, const QString& songPath);
    void pausePlayback();
    //void setSliderPlayProgress();
    //void setMusicPosition(int sliderPlayValue);
    void setMusicVolume(int volume);
    bool isToplay();
    void stopPlayback();
    //qint64 getCurrentPosition() const;
signals:
    void calculateFinished(int);
    void playPosition(int);
    void playDuration(int);

private slots:
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);

private:
    QMediaPlayer* player;
    QSqlDatabase m_db;  // 本地 SQLite 数据库连接

    // 根据 songId 查询数据库获取本地文件路径
    QString queryLocalFilePath(int songId);
    // 初始化数据库连接
    bool initDatabaseConnection();
};

#endif // MUSICPLAYER_H
