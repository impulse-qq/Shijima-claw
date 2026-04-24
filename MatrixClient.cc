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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <iostream>

using namespace httplib;

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
    m_accessToken = obj.value("accessToken").toString();
    m_roomId = obj.value("roomId").toString();

    if (m_homeserver.isEmpty() || m_userId.isEmpty() ||
        m_accessToken.isEmpty() || m_roomId.isEmpty()) {
        m_lastError = "Config missing required fields (homeserver, userId, accessToken, roomId)";
        return false;
    }

    return true;
}

void MatrixClient::login() {
    if (m_accessToken.isEmpty()) {
        m_lastError = "No access token configured";
        emit errorOccurred(m_lastError);
        return;
    }
    m_connected = true;
    emit connectedChanged(true);
}

void MatrixClient::startSyncLoop() {
    if (m_running.load()) {
        return;
    }
    m_running = true;
    m_syncThread = new std::thread([this]() { syncLoop(); });
}

void MatrixClient::stopSyncLoop() {
    m_running = false;
    if (m_syncThread != nullptr) {
        m_syncThread->join();
        delete m_syncThread;
        m_syncThread = nullptr;
    }
}

void MatrixClient::sendMessage(const QString &text) {
    if (!m_connected.load() || m_accessToken.isEmpty()) {
        emit errorOccurred("Not connected");
        return;
    }

    static long long txnId = 0;
    txnId++;

    Client cli(m_homeserver.toStdString());
    std::string path = "/_matrix/client/r0/rooms/" + m_roomId.toStdString()
        + "/send/m.room.message/" + std::to_string(txnId) + "?access_token=" + m_accessToken.toStdString();

    QJsonObject content;
    content["msgtype"] = "m.text";
    content["body"] = text;

    QJsonObject message;
    message["content"] = content;
    message["type"] = "m.room.message";

    QJsonDocument doc(message);
    std::string body = doc.toJson(QJsonDocument::Compact).toStdString();

    auto res = cli.Put(path.c_str(), body, "application/json");
    if (!res || res->status != 200) {
        std::string err = res ? res->body : "Connection failed";
        m_lastError = QString::fromStdString(err);
        emit errorOccurred(m_lastError);
    }
}

void MatrixClient::syncLoop() {
    Client cli(m_homeserver.toStdString());

    while (m_running.load()) {
        if (!m_connected.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        std::string path = "/_matrix/client/r0/sync?timeout=30000";
        if (!m_nextBatch.isEmpty()) {
            path += "&since=" + m_nextBatch.toStdString();
        }
        path += "&access_token=" + m_accessToken.toStdString();

        auto res = cli.Get(path.c_str());
        if (!res || res->status != 200) {
            m_connected = false;
            emit connectedChanged(false);
            m_retryCount++;
            int delay = std::min(60, 1 << m_retryCount);
            std::this_thread::sleep_for(std::chrono::seconds(delay));
            continue;
        }

        m_retryCount = 0;
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(
            QByteArray::fromStdString(res->body), &error);
        if (error.error != QJsonParseError::NoError) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        QJsonObject root = doc.object();
        m_nextBatch = parseSyncNextBatch(root);

        QJsonObject room = root.value("rooms").toObject();
        QJsonObject join = room.value("join").toObject();
        if (join.contains(m_roomId)) {
            QJsonObject roomData = join.value(m_roomId).toObject();
            QJsonArray events = roomData.value("timeline").toObject()
                .value("events").toArray();
            for (const QJsonValue &ev : events) {
                QJsonObject event = ev.toObject();
                if (isValidEvent(event)) {
                    QString sender = extractSender(event);
                    QString body = extractBody(event);
                    if (!sender.isEmpty() && !body.isEmpty()) {
                        QMetaObject::invokeMethod(this, [this, sender, body]() {
                            emit messageReceived(sender, body, m_roomId);
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
    if (event.value("type").toString() != "m.room.message") {
        return false;
    }
    QJsonObject content = event.value("content").toObject();
    if (content.value("msgtype").toString() != "m.text") {
        return false;
    }
    if (extractBody(event).isEmpty()) {
        return false;
    }
    return true;
}