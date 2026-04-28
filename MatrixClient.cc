//
// Shijima-Qt - Cross-platform shimeji simulation app for desktop
// Copyright (C) 2025 pixelomer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "MatrixClient.hh"
#include <httplib.h>
#include <chrono>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <iostream>

using namespace httplib;

static QString timestamp() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

static void matrixLog(const QString& level, const QString& msg) {
    static QFile file("/tmp/shijima-debug.log");
    if (!file.isOpen()) {
        file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
    QTextStream out(&file);
    out << "[" << timestamp() << "] [" << level << "] " << msg << "\n";
    out.flush();
}

#define MATRIX_LOG(msg) matrixLog("INFO", msg)
#define MATRIX_ERR(msg) matrixLog("ERROR", msg)

MatrixClient::MatrixClient(QObject *parent)
    : QObject(parent)
    , m_syncThread(nullptr)
    , m_running(false)
    , m_connected(false)
    , m_nextBatch()
    , m_retryCount(0)
{
}

MatrixClient::~MatrixClient() {
    stopSyncLoop();
}

bool MatrixClient::loadConfig(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Failed to open config: " + path;
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        m_lastError = "JSON parse error: " + error.errorString();
        return false;
    }
    if (!doc.isObject()) {
        m_lastError = "Config must be a JSON object";
        return false;
    }

    QJsonObject obj = doc.object();
    m_homeserver = obj.value("homeserver").toString();
    m_userId = obj.value("userId").toString();
    m_username = obj.value("username").toString();
    m_password = obj.value("password").toString();
    m_roomId = obj.value("roomId").toString();
    m_accessToken = obj.value("accessToken").toString(); // backward compat, may be empty

    if (m_homeserver.isEmpty() || m_userId.isEmpty() ||
        m_username.isEmpty() || m_password.isEmpty() || m_roomId.isEmpty()) {
        m_lastError = "Config missing required fields (homeserver, userId, username, password, roomId)";
        std::cerr << "DEBUG loadConfig() FAIL: missing fields hs=" << m_homeserver.isEmpty() << " uid=" << m_userId.isEmpty() << " user=" << m_username.isEmpty() << " pass=" << m_password.isEmpty() << " room=" << m_roomId.isEmpty() << std::endl;
        return false;
    }

    std::cerr << "DEBUG loadConfig() OK username=" << m_username.toStdString() << " password_len=" << m_password.length() << std::endl;
    return true;
}

void MatrixClient::login() {
    std::cerr << "DEBUG login() ENTER username=" << m_username.toStdString() << std::endl;
    MATRIX_LOG(QString("login() called, username=") + m_username);
    if (m_username.isEmpty() || m_password.isEmpty()) {
        m_lastError = "No username or password configured";
        MATRIX_ERR("Login failed: " + m_lastError);
        emit errorOccurred(m_lastError);
        return;
    }

    Client cli(m_homeserver.toStdString());
    std::string path = "/_matrix/client/r0/login";

    QJsonObject identifier;
    identifier["type"] = "m.id.user";
    identifier["user"] = m_username;

    QJsonObject req;
    req["type"] = "m.login.password";
    req["identifier"] = identifier;
    req["password"] = m_password;

    QJsonDocument doc(req);
    std::string body = doc.toJson(QJsonDocument::Compact).toStdString();

    MATRIX_LOG(QString("login() POST ") + QString::fromStdString(path));
    auto res = cli.Post(path.c_str(), body, "application/json");
    if (!res || res->status != 200) {
        std::string err = res ? res->body : "Connection failed";
        m_lastError = QString::fromStdString(err);
        MATRIX_ERR("login() failed: " + m_lastError);
        emit errorOccurred(m_lastError);
        m_connected = false;
        emit connectedChanged(false);
        return;
    }

    QJsonParseError parseError;
    QJsonDocument respDoc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = "Login response JSON parse error: " + parseError.errorString();
        MATRIX_ERR("login() failed: " + m_lastError);
        emit errorOccurred(m_lastError);
        m_connected = false;
        emit connectedChanged(false);
        return;
    }

    QJsonObject resp = respDoc.object();
    m_accessToken = resp.value("access_token").toString();
    if (m_accessToken.isEmpty()) {
        m_lastError = "Login response missing access_token";
        MATRIX_ERR("login() failed: " + m_lastError);
        emit errorOccurred(m_lastError);
        m_connected = false;
        emit connectedChanged(false);
        return;
    }

    m_connected = true;
    MATRIX_LOG("login() succeeded, token obtained");
    std::cerr << "DEBUG login() EXIT connected=" << m_connected.load() << " token_len=" << m_accessToken.length() << std::endl;
    emit connectedChanged(true);
}

void MatrixClient::startSyncLoop() {
    if (m_running.load()) {
        MATRIX_LOG("startSyncLoop() called but already running");
        return;
    }
    MATRIX_LOG("startSyncLoop() starting sync thread");
    std::cerr << "DEBUG startSyncLoop() creating thread..." << std::endl;
    m_running = true;
    m_syncThread = new std::thread([this]() { syncLoop(); });
    std::cerr << "DEBUG startSyncLoop() thread created" << std::endl;
}

void MatrixClient::stopSyncLoop() {
    m_running = false;
    if (m_syncThread != nullptr) {
        m_syncThread->join();
        delete m_syncThread;
        m_syncThread = nullptr;
    }
}

void MatrixClient::sendMessage(const QString &text, const QString &roomId) {
    MATRIX_LOG(QString("sendMessage() called, text=\"" + text + "\" roomId=\"" + roomId + "\""));
    if (m_accessToken.isEmpty()) {
        MATRIX_ERR("sendMessage() failed: No access token");
        emit errorOccurred("No access token");
        return;
    }

    static long long txnIdBase = std::chrono::steady_clock::now().time_since_epoch().count() % 100000;
    static long long txnIdCounter = 0;
    long long txnId = txnIdBase + (txnIdCounter++ % 100000);

    QString targetRoom = roomId.isEmpty() ? m_roomId : roomId;
    Client cli(m_homeserver.toStdString());
    std::string path = "/_matrix/client/r0/rooms/" + targetRoom.toStdString()
        + "/send/m.room.message/" + std::to_string(txnId);

    Headers headers;
    headers.emplace("Authorization", "Bearer " + m_accessToken.toStdString());

    QJsonObject content;
    content["msgtype"] = "m.text";
    content["body"] = text;

    QJsonDocument doc(content);
    std::string body = doc.toJson(QJsonDocument::Compact).toStdString();

    MATRIX_LOG(QString("sendMessage() PUT ") + QString::fromStdString(path) + " body=" + QString::fromStdString(body));
    auto res = cli.Put(path.c_str(), headers, body, "application/json");
    if (!res || res->status != 200) {
        std::string err = res ? res->body : "Connection failed";
        MATRIX_ERR("sendMessage() failed: " + QString::fromStdString(err));
        m_lastError = QString::fromStdString(err);
        emit errorOccurred(m_lastError);
    } else {
        MATRIX_LOG("sendMessage() success, event_id=" + QString::fromStdString(res->body));
    }
}

void MatrixClient::syncLoop() {
    std::cerr << "DEBUG syncLoop() ENTER token_len=" << m_accessToken.length() << std::endl;
    MATRIX_LOG(QString("syncLoop() started, homeserver=") + m_homeserver + " roomId=" + m_roomId);
    Client cli(m_homeserver.toStdString());

    while (m_running.load()) {
        if (!m_connected.load()) {
            MATRIX_LOG("syncLoop() waiting (m_connected=false)");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        std::string path = "/_matrix/client/r0/sync?timeout=30000";
        if (!m_nextBatch.isEmpty()) {
            path += "&since=" + m_nextBatch.toStdString();
        }

        // Refresh headers each time since token may have been updated by login()
        Headers headers;
        headers.emplace("Authorization", "Bearer " + m_accessToken.toStdString());
        std::cerr << "DEBUG syncLoop() token len=" << m_accessToken.length() << " connected=" << m_connected.load() << std::endl;
        MATRIX_LOG(QString("syncLoop() Authorization: Bearer length=") + QString::number(m_accessToken.length()));

        MATRIX_LOG(QString("syncLoop() GET ") + QString::fromStdString(path));
        auto res = cli.Get(path.c_str(), headers);
        if (!res || res->status == 401) {
            if (res && res->status == 401) {
                MATRIX_LOG("syncLoop() got 401, refreshing token");
            } else {
                std::string err = res ? ("HTTP " + std::to_string(res->status)) : "no response";
                MATRIX_ERR(QString("syncLoop() failed: ") + QString::fromStdString(err));
            }
            m_connected = false;
            emit connectedChanged(false);

            // Attempt token refresh
            login();
            if (!m_connected.load()) {
                m_retryCount++;
                int delay = std::min(60, 1 << m_retryCount);
                MATRIX_LOG(QString("syncLoop() token refresh failed, retry in ") + QString::number(delay) + "s (retry #" + QString::number(m_retryCount) + ")");
                std::this_thread::sleep_for(std::chrono::seconds(delay));
                continue;
            }

            // Retry with new token
            Headers retryHeaders;
            retryHeaders.emplace("Authorization", "Bearer " + m_accessToken.toStdString());
            MATRIX_LOG("syncLoop() token refreshed, retrying sync");
            auto retryRes = cli.Get(path.c_str(), retryHeaders);
            if (!retryRes || retryRes->status != 200) {
                std::string err = retryRes ? ("HTTP " + std::to_string(retryRes->status)) : "no response";
                MATRIX_ERR(QString("syncLoop() retry failed: ") + QString::fromStdString(err));
                m_connected = false;
                emit connectedChanged(false);
                m_retryCount++;
                int delay = std::min(60, 1 << m_retryCount);
                std::this_thread::sleep_for(std::chrono::seconds(delay));
                continue;
            }
            std::swap(res, retryRes);
            m_retryCount = 0;
            MATRIX_LOG(QString("syncLoop() got response after token refresh, status=200, body_len=") + QString::number(res->body.size()));
        } else if (res->status != 200) {
            std::string err = "HTTP " + std::to_string(res->status);
            MATRIX_ERR(QString("syncLoop() failed: ") + QString::fromStdString(err));
            m_connected = false;
            emit connectedChanged(false);
            m_retryCount++;
            int delay = std::min(60, 1 << m_retryCount);
            MATRIX_LOG(QString("syncLoop() retry in ") + QString::number(delay) + "s (retry #" + QString::number(m_retryCount) + ")");
            std::this_thread::sleep_for(std::chrono::seconds(delay));
            continue;
        }

        m_retryCount = 0;
        MATRIX_LOG(QString("syncLoop() got response, status=200, body_len=") + QString::number(res->body.size()));
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(
            QByteArray::fromStdString(res->body), &error);
        if (error.error != QJsonParseError::NoError) {
            MATRIX_ERR(QString("syncLoop() JSON parse error: ") + error.errorString());
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        QJsonObject root = doc.object();
        m_nextBatch = parseSyncNextBatch(root);

        QJsonObject room = root.value("rooms").toObject();
        QJsonObject join = room.value("join").toObject();
        MATRIX_LOG(QString("syncLoop() rooms.join count=") + QString::number(join.size()) + " my roomId=[" + m_roomId + "]");
        for (auto it = join.begin(); it != join.end(); ++it) {
            QString actualRoomId = it.key();
            MATRIX_LOG(QString("syncLoop() room ID in response: [") + actualRoomId + "]");

            QJsonObject roomData = it.value().toObject();
            QJsonArray events = roomData.value("timeline").toObject()
                .value("events").toArray();
            MATRIX_LOG(QString("syncLoop() room=") + actualRoomId + " timeline events count=" + QString::number(events.size()));
            if (events.isEmpty()) {
                continue;
            }
            for (const QJsonValue &ev : events) {
                QJsonObject event = ev.toObject();
                QString evType = event.value("type").toString();
                QString evSender = event.value("sender").toString();
                MATRIX_LOG(QString("syncLoop() event: type=") + evType + " sender=" + evSender);
                bool valid = isValidEvent(event);
                MATRIX_LOG(QString("syncLoop() isValidEvent() returned: ") + (valid ? "true" : "false"));
                if (valid) {
                    QString sender = extractSender(event);
                    QString body = extractBody(event);
                    MATRIX_LOG(QString("syncLoop() valid event: sender=") + sender + " body=" + body);
                    if (!sender.isEmpty() && !body.isEmpty()) {
                        MATRIX_LOG(QString("syncLoop() emitting messageReceived: sender=") + sender + " body=" + body + " room=" + actualRoomId);
                        QMetaObject::invokeMethod(this, [this, sender, body, actualRoomId]() {
                            emit messageReceived(sender, body, actualRoomId);
                        }, Qt::QueuedConnection);
                    }
                }
            }
        }

        m_connected = true;
        emit connectedChanged(true);
    }
}

QString MatrixClient::parseSyncNextBatch(const QJsonObject &root) {
    return root.value("next_batch").toString();
}

QString MatrixClient::extractRoomId(const QJsonObject &event) {
    return event.value("room_id").toString();
}

QString MatrixClient::extractSender(const QJsonObject &event) {
    return event.value("sender").toString();
}

QString MatrixClient::extractBody(const QJsonObject &event) {
    QJsonObject content = event.value("content").toObject();
    return content.value("body").toString();
}

bool MatrixClient::isValidEvent(const QJsonObject &event) {
    QString evType = event.value("type").toString();
    MATRIX_LOG(QString("isValidEvent() checking: type=") + evType);
    if (evType != "m.room.message") {
        MATRIX_LOG(QString("isValidEvent() rejecting: not m.room.message, is ") + evType);
        return false;
    }
    QJsonObject content = event.value("content").toObject();
    QString msgtype = content.value("msgtype").toString();
    MATRIX_LOG(QString("isValidEvent() msgtype=") + msgtype);
    if (msgtype != "m.text") {
        MATRIX_LOG(QString("isValidEvent() rejecting: not m.text, is ") + msgtype);
        return false;
    }
    QString body = extractBody(event);
    MATRIX_LOG(QString("isValidEvent() body length=") + QString::number(body.length()));
    if (body.isEmpty()) {
        MATRIX_LOG("isValidEvent() rejecting: body is empty");
        return false;
    }
    MATRIX_LOG("isValidEvent() accepting");
    return true;
}

#include "MatrixClient.moc"