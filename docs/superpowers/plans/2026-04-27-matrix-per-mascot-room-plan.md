# Per-Mascot Matrix Room 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让每个 mascot 可以分配到不同的 Matrix room，实现消息的独立收发。

**Architecture:**
- `ShijimaWidget` 新增 `m_matrixRoomId` 成员存储分配的房间
- `MatrixClient::sendMessage` 增加可选的 `roomId` 参数
- `ShijimaManager` 中的消息路由改为按 room 匹配 + fallback 到未分配的 mascot

**Tech Stack:** C++, Qt, Matrix Client API

---

## 文件结构

```
ShijimaWidget.hpp     - 添加 m_matrixRoomId 成员和 getter
ShijimaWidget.cc      - 修改 sendMatrixMessage 使用 m_matrixRoomId
MatrixClient.hh       - sendMessage 增加 roomId 参数
MatrixClient.cc      - 实现 roomId 参数逻辑
ShijimaContextMenu.cc - 添加 "分配到房间..." 和 "取消房间分配" 菜单项
ShijimaManager.cc     - 修改消息路由逻辑
```

---

## Task 1: ShijimaWidget 添加 roomId 成员

**Files:**
- Modify: `ShijimaWidget.hpp:100-109`

- [ ] **Step 1: 在 ShijimaWidget.hpp 中添加成员和 getter**

在 `m_currentBubbleText` 下方添加：

```cpp
QString m_matrixRoomId;
public:
QString matrixRoomId() const { return m_matrixRoomId; }
void setMatrixRoomId(const QString &roomId) { m_matrixRoomId = roomId; }
```

- [ ] **Step 2: Commit**

```bash
git add ShijimaWidget.hpp && git commit -m "feat(matrix): add m_matrixRoomId to ShijimaWidget"
```

---

## Task 2: MatrixClient 支持指定 roomId 发送

**Files:**
- Modify: `MatrixClient.hh:46` (sendMessage 签名)
- Modify: `MatrixClient.cc:188-224` (sendMessage 实现)

- [ ] **Step 1: 修改 MatrixClient.hh 的 sendMessage 签名**

将 `void sendMessage(const QString &text);` 改为：

```cpp
void sendMessage(const QString &text, const QString &roomId = "");
```

- [ ] **Step 2: 修改 MatrixClient.cc 的 sendMessage 实现**

在 `sendMessage` 函数中，将 room 构建逻辑从：
```cpp
std::string path = "/_matrix/client/r0/rooms/" + m_roomId.toStdString()
    + "/send/m.room.message/" + std::to_string(txnId);
```
改为：
```cpp
QString targetRoom = roomId.isEmpty() ? m_roomId : roomId;
std::string path = "/_matrix/client/r0/rooms/" + targetRoom.toStdString()
    + "/send/m.room.message/" + std::to_string(txnId);
```

- [ ] **Step 3: Commit**

```bash
git add MatrixClient.hh MatrixClient.cc && git commit -m "feat(matrix): add roomId parameter to sendMessage"
```

---

## Task 3: ShijimaWidget 的 sendMatrixMessage 使用 roomId

**Files:**
- Modify: `ShijimaWidget.cc:345-371`

- [ ] **Step 1: 修改 sendMatrixMessage**

找到 `sendMatrixMessage` 函数，修改发送逻辑：

```cpp
void ShijimaWidget::sendMatrixMessage(const QString &text) {
    debugLog(QString("sendMatrixMessage: %1").arg(text));

    ShijimaManager *manager = ShijimaManager::defaultManager();
    MatrixClient *client = manager ? manager->matrixClient() : nullptr;

    if (!client) {
        debugLog("sendMatrixMessage: no MatrixClient available");
        showMessageBubble("[Error: Matrix not configured]");
        return;
    }

    if (!client->isConnected()) {
        debugLog("sendMatrixMessage: not connected to Matrix server");
        showMessageBubble("[Error: Not connected to Matrix]");
        return;
    }

    // Check if this mascot has a room assigned
    if (m_matrixRoomId.isEmpty()) {
        debugLog("sendMatrixMessage: no room assigned to this mascot");
        showMessageBubble("[未分配房间]");
        return;
    }

    // Wire up error signal to bubble display
    QObject::connect(client, &MatrixClient::errorOccurred, this,
        [this](const QString &error) {
            debugLog(QString("Matrix send error: %1").arg(error));
            showMessageBubble(QString("[Error: %1]").arg(error));
        }, Qt::SingleShotConnection);

    client->sendMessage(text, m_matrixRoomId);
}
```

- [ ] **Step 2: Commit**

```bash
git add ShijimaWidget.cc && git commit -m "feat(matrix): use m_matrixRoomId in sendMatrixMessage"
```

---

## Task 4: ShijimaContextMenu 添加房间分配菜单

**Files:**
- Modify: `ShijimaContextMenu.cc:86-93` (在 Send message 后添加新菜单项)
- Modify: `ShijimaContextMenu.cc:110-117` (添加 showAssignRoomDialog 函数)

- [ ] **Step 1: 在 Send message 后添加新菜单项**

在 `action = addAction("Send message");` 后（第88行）添加：

```cpp
action = addAction("分配到房间...");
connect(action, &QAction::triggered, this, &ShijimaContextMenu::showAssignRoomDialog);
```

- [ ] **Step 2: 添加 showAssignRoomDialog 函数**

在 `showSendMessageDialog` 函数后添加：

```cpp
void ShijimaContextMenu::showAssignRoomDialog() {
    bool ok;
    QString roomId = QInputDialog::getText(parentWidget(),
        "分配到房间", "输入 Room ID:", QLineEdit::Normal,
        shijimaParent()->m_matrixRoomId, &ok);
    if (ok && !roomId.isEmpty()) {
        shijimaParent()->setMatrixRoomId(roomId);
    }
}
```

在文件顶部添加 `#include <QLineEdit>` 如果还没有。

- [ ] **Step 3: 提交**

```bash
git add ShijimaContextMenu.cc && git commit -m "feat(matrix): add assign room menu item"
```

---

## Task 5: ShijimaManager 修改消息路由逻辑

**Files:**
- Modify: `ShijimaManager.cc:774-787` (当前的 messageReceived 连接)

- [ ] **Step 1: 修改 messageReceived 连接逻辑**

当前的连接是绑定到 `m_mascots.front()`。需要改为遍历所有 mascots 进行路由。

找到这段代码（约在 774-787 行）：
```cpp
if (m_matrixClient) {
    connect(m_matrixClient, &MatrixClient::messageReceived,
        [this](const QString &sender, const QString &body, const QString &roomId) {
            if (connected && !m_matrixMessageConnected && !m_mascots.empty()) {
                ShijimaWidget *firstMascot = m_mascots.front();
                connect(m_matrixClient, &MatrixClient::messageReceived,
                    [firstMascot](const QString &sender, const QString &body, const QString &){
                        std::cerr << "[ShijimaManager] messageReceived: sender=" << sender.toStdString() << " body=" << body.toStdString() << std::endl;
                        firstMascot->showMessageBubble(body);
                    });
                m_matrixMessageConnected = true;
            }
        });
}
```

替换为新的路由逻辑：

```cpp
if (m_matrixClient) {
    connect(m_matrixClient, &MatrixClient::messageReceived,
        [this](const QString &sender, const QString &body, const QString &roomId) {
            std::cerr << "[ShijimaManager] messageReceived: sender=" << sender.toStdString()
                << " body=" << body.toStdString() << " roomId=" << roomId.toStdString() << std::endl;

            bool delivered = false;
            for (auto mascot : m_mascots) {
                if (mascot->matrixRoomId() == roomId) {
                    mascot->showMessageBubble(body);
                    delivered = true;
                }
            }
            if (!delivered) {
                // Fallback: deliver to all unassigned mascots
                for (auto mascot : m_mascots) {
                    if (mascot->matrixRoomId().isEmpty()) {
                        mascot->showMessageBubble(body);
                    }
                }
            }
        });
    m_matrixMessageConnected = true;
}
```

- [ ] **Step 2: 同样修改 spawn 中的 messageReceived 连接**

找到 `spawn` 函数中的 messageReceived 连接（约在 1131-1139 行），应用同样的路由逻辑。

- [ ] **Step 3: 提交**

```bash
git add ShijimaManager.cc && git commit -m "feat(matrix): implement per-mascot room routing"
```

---

## Self-Review 检查清单

1. **Spec coverage:** 所有 spec 中的需求都有对应的 task 吗？
   - [x] ShijimaWidget m_matrixRoomId 成员 → Task 1
   - [x] MatrixClient sendMessage roomId 参数 → Task 2
   - [x] sendMatrixMessage 使用 roomId → Task 3
   - [x] 菜单添加分配房间选项 → Task 4
   - [x] 消息路由逻辑 → Task 5

2. **Placeholder scan:** 没有 TBD/TODO，未实现的步骤，或模糊的描述

3. **Type consistency:**
   - `m_matrixRoomId` 在 ShijimaWidget 中定义
   - `matrixRoomId()` getter 返回 `QString`
   - `setMatrixRoomId(const QString &)` setter 存在
   - `MatrixClient::sendMessage(const QString &, const QString &roomId = "")` 签名一致