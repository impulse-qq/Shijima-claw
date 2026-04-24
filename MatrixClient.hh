#pragma once

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

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <QObject>
#include <QString>
#include <QJsonObject>

class MatrixClient : public QObject {
    Q_OBJECT
public:
    struct Config {
        QString homeserver;
        QString userId;
        QString accessToken;
        QString roomId;
    };

    explicit MatrixClient(QObject *parent = nullptr);
    ~MatrixClient() {
        if (m_syncThread) {
            m_running.store(false);
            if (m_syncThread->joinable()) {
                m_syncThread->join();
            }
            delete m_syncThread;
        }
    }

    bool loadConfig(const QString &path);
    void login();
    void startSyncLoop();
    void stopSyncLoop();
    void sendMessage(const QString &text);

    bool isConnected() const { return m_connected.load(); }
    QString lastError() const { return m_lastError; }

signals:
    void messageReceived(const QString &sender, const QString &body, const QString &roomId);
    void connectedChanged(bool connected);
    void errorOccurred(const QString &error);

private:
    Q_DISABLE_COPY(MatrixClient)

    void syncLoop();
    long long parseSyncNextBatch(const QJsonObject &root);
    QString extractRoomId(const QJsonObject &event);
    QString extractSender(const QJsonObject &event);
    QString extractBody(const QJsonObject &event);
    bool isValidEvent(const QJsonObject &event);

    std::thread *m_syncThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_connected;
    QString m_accessToken;
    QString m_roomId;
    QString m_homeserver;
    QString m_userId;
    QString m_lastError;
    long long m_nextBatch;
    int m_retryCount;
};