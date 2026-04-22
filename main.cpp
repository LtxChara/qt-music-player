#include "widget.h"
#include "userlogin.h"
#include "userregister.h"
#include "adminlogin.h"

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

/**
 * @brief 扫描项目 music/ 目录下的默认歌曲，并导入到数据库中。
 * 仅在 songs 表为空时执行一次，防止重复插入。
 * 同时更新 styles 表的 song_list 字段，供推荐算法使用。
 */
void scanAndLoadDefaultSongs()
{
    QSqlQuery query;
    query.exec("SELECT COUNT(*) FROM songs");
    if (query.next() && query.value(0).toInt() > 0)
    {
        qDebug() << "songs 表已有数据，跳过默认歌曲加载。";
        return;
    }

    // 兼容两种启动场景：直接运行 exe（位于 release/ 或 debug/）或从项目根目录启动
    QDir appDir(QCoreApplication::applicationDirPath());
    QStringList possiblePaths;
    possiblePaths << appDir.absoluteFilePath("music")
                  << QDir(appDir.absoluteFilePath("..")).absoluteFilePath("music");

    QString musicPath;
    for (const QString &path : possiblePaths)
    {
        if (QDir(path).exists())
        {
            musicPath = path;
            break;
        }
    }

    if (musicPath.isEmpty())
    {
        qWarning() << "未找到默认音乐目录 music/，跳过默认歌曲加载。";
        return;
    }
    qDebug() << "扫描默认音乐目录:" << musicPath;

    // 目录名到 styles 表 style_name 的映射（hiphop 目录对应 hip-hop 风格）
    QMap<QString, QString> dirToStyle;
    dirToStyle["rock"] = "rock";
    dirToStyle["pop"] = "pop";
    dirToStyle["jazz"] = "jazz";
    dirToStyle["classical"] = "classical";
    dirToStyle["hiphop"] = "hip-hop";
    dirToStyle["electronic"] = "electronic";
    dirToStyle["country"] = "country";
    dirToStyle["folk"] = "folk";
    dirToStyle["blues"] = "blues";

    QDir musicDir(musicPath);
    QStringList subDirs = musicDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    // 按风格缓存新插入的歌曲 ID，用于批量更新 styles.song_list
    QMap<QString, QList<int>> styleSongIds;

    for (const QString &subDir : subDirs)
    {
        if (!dirToStyle.contains(subDir))
        {
            continue;
        }
        QString styleName = dirToStyle[subDir];

        QDir songDir(musicDir.absoluteFilePath(subDir));
        QStringList filters;
        filters << "*.mp3" << "*.wav" << "*.wma" << "*.flac" << "*.aac" << "*.ogg";
        songDir.setNameFilters(filters);
        QStringList files = songDir.entryList(QDir::Files);

        for (const QString &file : files)
        {
            QFileInfo fileInfo(songDir.absoluteFilePath(file));
            QString absPath = fileInfo.absoluteFilePath();
            QString title = fileInfo.baseName(); // 不含扩展名，作为歌曲标题

            query.prepare("INSERT INTO songs (title, style, file_path, vip) VALUES (:title, :style, :path, 0)");
            query.bindValue(":title", title);
            query.bindValue(":style", styleName);
            query.bindValue(":path", absPath);
            if (!query.exec())
            {
                qWarning() << "插入歌曲失败:" << query.lastError().text() << title;
                continue;
            }

            int songId = query.lastInsertId().toInt();
            styleSongIds[styleName].append(songId);
            qDebug() << "已加载默认歌曲:" << title
                     << "风格:" << styleName
                     << "路径:" << absPath;
        }
    }

    // 将新歌曲 ID 更新到 styles 表的 song_list 中
    for (auto it = styleSongIds.begin(); it != styleSongIds.end(); ++it)
    {
        QString styleName = it.key();
        const QList<int> &ids = it.value();
        if (ids.isEmpty())
            continue;

        query.prepare("SELECT id, song_list FROM styles WHERE style_name = :name");
        query.bindValue(":name", styleName);
        if (!query.exec() || !query.next())
        {
            qWarning() << "未找到风格记录:" << styleName;
            continue;
        }

        int styleId = query.value(0).toInt();
        QString existing = query.value(1).toString();
        QStringList idStrs;
        for (int sid : ids)
        {
            idStrs.append(QString::number(sid));
        }

        QString newList = existing.isEmpty() ? idStrs.join(",")
                                             : existing + "," + idStrs.join(",");

        query.prepare("UPDATE styles SET song_list = :list WHERE id = :id");
        query.bindValue(":list", newList);
        query.bindValue(":id", styleId);
        if (!query.exec())
        {
            qWarning() << "更新风格 song_list 失败:" << styleName << query.lastError().text();
        }
        else
        {
            qDebug() << "已更新风格" << styleName << "歌曲列表:" << newList;
        }
    }
}

/**
 * @brief 初始化本地 SQLite 数据库
 * 若数据库或表不存在，则自动创建，确保程序可独立运行。
 */
bool initDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("music.db");

    if (!db.open())
    {
        QMessageBox::critical(nullptr, "数据库错误",
                              QString("无法打开本地数据库 music.db:\n%1").arg(db.lastError().text()));
        qCritical() << "数据库打开失败:" << db.lastError().text();
        return false;
    }

    qDebug() << "成功连接本地 SQLite 数据库 music.db";

    QSqlQuery query;

    // 创建用户表
    query.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL UNIQUE, "
        "password TEXT NOT NULL, "
        "vip INTEGER DEFAULT 0, "
        "play_list TEXT, "
        "favorite_styles TEXT DEFAULT '0,0,0,0,0,0,0,0,0'"
        ")");

    // 创建歌曲表（file_path 存储本地音乐文件的绝对路径）
    query.exec(
        "CREATE TABLE IF NOT EXISTS songs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "title TEXT NOT NULL, "
        "style TEXT, "
        "file_path TEXT NOT NULL, "
        "vip INTEGER DEFAULT 0"
        ")");

    // 创建歌单表
    query.exec(
        "CREATE TABLE IF NOT EXISTS playlists ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "user_id INTEGER, "
        "song_list TEXT"
        ")");

    // 创建管理员表
    query.exec(
        "CREATE TABLE IF NOT EXISTS admins ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL UNIQUE, "
        "password TEXT NOT NULL"
        ")");

    // 创建风格表（用于推荐算法）
    query.exec(
        "CREATE TABLE IF NOT EXISTS styles ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "style_name TEXT NOT NULL, "
        "song_list TEXT"
        ")");

    // 初始化风格数据（id 从 1 开始，与推荐算法中的 styleId 对应）
    QStringList styleNames = {
        "rock", "pop", "jazz", "classical", "hip-hop",
        "electronic", "country", "folk", "blues"};
    for (int i = 0; i < styleNames.size(); ++i)
    {
        int styleId = i + 1;
        query.prepare("SELECT id FROM styles WHERE id = :id");
        query.bindValue(":id", styleId);
        query.exec();
        if (!query.next())
        {
            query.prepare("INSERT INTO styles (id, style_name, song_list) VALUES (:id, :name, '')");
            query.bindValue(":id", styleId);
            query.bindValue(":name", styleNames[i]);
            query.exec();
        }
    }

    // 若用户表为空，插入一个默认测试用户（用户名 user，密码 123456），方便首次验证
    query.exec("SELECT COUNT(*) FROM users");
    if (query.next() && query.value(0).toInt() == 0)
    {
        query.prepare("INSERT INTO users (name, password, vip, favorite_styles) "
                      "VALUES ('user', '123456', 0, '0,0,0,0,0,0,0,0,0,0')");
        query.exec();
        qDebug() << "已插入默认测试用户：user / 123456";
    }

    // 若管理员表为空，插入一个默认管理员（用户名 admin，密码 admin），方便后台管理验证
    query.exec("SELECT COUNT(*) FROM admins");
    if (query.next() && query.value(0).toInt() == 0)
    {
        query.prepare("INSERT INTO admins (name, password) VALUES ('admin', 'admin')");
        query.exec();
        qDebug() << "已插入默认管理员：admin / admin";
    }

    // 扫描并加载默认歌曲到数据库（仅在 songs 表为空时执行）
    scanAndLoadDefaultSongs();

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    // 初始化本地 SQLite 数据库
    if (!initDatabase())
    {
        return -1;
    }

    userLogin loginDialog;
    QObject::connect(&loginDialog, &userLogin::loginSuccessful, [&](int userId, const QString &userName)
                     {
         Widget* w = new Widget(nullptr, userId, userName);
         w->recommendSongs(userId);
         w->updateLikeListWidget(userId);
         w->show();
         loginDialog.close(); });

    loginDialog.show();

    return a.exec();
}
