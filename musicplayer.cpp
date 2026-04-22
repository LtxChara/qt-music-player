#include "musicplayer.h"
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QMediaPlayer>
#include <QTime>
#include <QUuid>

MusicPlayer::MusicPlayer(QObject* parent) : QObject(parent)
{
    player = new QMediaPlayer(this);

    // 连接播放器进度信号
    connect(player, &QMediaPlayer::positionChanged, this, &MusicPlayer::onPositionChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &MusicPlayer::onDurationChanged);

    // 初始化本地 SQLite 数据库连接
    if (!initDatabaseConnection()) {
        qCritical() << "MusicPlayer 数据库连接初始化失败，播放功能将不可用。";
    }
}

MusicPlayer::~MusicPlayer()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    // 移除该实例独有的数据库连接，避免连接名冲突
    QString connName = m_db.connectionName();
    if (!connName.isEmpty()) {
        QSqlDatabase::removeDatabase(connName);
    }
}

bool MusicPlayer::initDatabaseConnection()
{
    // 使用唯一连接名，避免与主程序或其他实例冲突
    QString connName = QString("musicplayer_conn_%1").arg(QUuid::createUuid().toString().remove('{').remove('}'));
    m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    m_db.setDatabaseName("music.db");

    if (!m_db.open()) {
        qCritical() << "无法打开本地数据库 music.db:" << m_db.lastError().text();
        return false;
    }
    return true;
}

QString MusicPlayer::queryLocalFilePath(int songId)
{
    if (!m_db.isOpen()) {
        qCritical() << "数据库未打开，无法查询歌曲路径。";
        return QString();
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT file_path FROM songs WHERE id = :id");
    query.bindValue(":id", songId);

    if (!query.exec()) {
        qCritical() << "查询歌曲路径失败:" << query.lastError().text();
        return QString();
    }

    if (query.next()) {
        return query.value(0).toString();
    }

    qWarning() << "数据库中未找到 songId 为" << songId << "的歌曲记录。";
    return QString();
}

void MusicPlayer::playSongById(int songId, const QString& songPath)
{
    qDebug() << "尝试播放歌曲，ID:" << songId << "传入路径:" << songPath;

    // 优先使用传入的 songPath；若为空，则通过 songId 查询数据库
    QString localPath = songPath;
    if (localPath.isEmpty()) {
        localPath = queryLocalFilePath(songId);
    }

    if (localPath.isEmpty()) {
        qCritical() << "无法获取歌曲文件路径（songId:" << songId << "）。";
        QMessageBox::critical(nullptr, "播放失败", "无法获取歌曲文件路径，请检查数据库中是否存在该歌曲记录。");
        return;
    }

    // 校验本地文件是否存在
    if (!QFile::exists(localPath)) {
        qCritical() << "本地音乐文件不存在:" << localPath;
        QMessageBox::critical(nullptr, "播放失败", QString("本地音乐文件不存在:\n%1").arg(localPath));
        return;
    }

    // 设置媒体源并播放
    player->setMedia(QUrl::fromLocalFile(localPath));
    player->play();
    qDebug() << "开始播放本地文件:" << localPath;
}

void MusicPlayer::setMusicVolume(int volume)
{
    player->setVolume(volume);
}

bool MusicPlayer::isToplay()
{
    return (QMediaPlayer::PlayingState == player->state());
}

void MusicPlayer::stopPlayback()
{
    if (player->state() == QMediaPlayer::PlayingState || player->state() == QMediaPlayer::PausedState) {
        player->stop();
        qDebug() << "Playback stopped.";
    }
}

void MusicPlayer::pausePlayback()
{
    if (player->state() == QMediaPlayer::PlayingState) {
        player->pause();
        qDebug() << "Playback paused.";
    }
    else if (player->state() == QMediaPlayer::PausedState) {
        player->play();
        qDebug() << "Playback resumed.";
    }
}

void MusicPlayer::onPositionChanged(qint64 position)
{
    qint64 duration = player->duration();
    if (duration > 0) {
        int sliderNumber = static_cast<int>(position * 100 / duration);
        emit calculateFinished(sliderNumber);
    }
    emit playPosition(static_cast<int>(position / 1000));
}

void MusicPlayer::onDurationChanged(qint64 duration)
{
    emit playDuration(static_cast<int>(duration / 1000));
}
