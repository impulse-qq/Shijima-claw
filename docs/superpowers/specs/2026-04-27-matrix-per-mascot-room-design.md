# 每个 Mascot 接入不同 Matrix Room 设计

## 概述

允许每个桌面宠物（Mascot）连接到不同的 Matrix room，实现消息的独立收发。

## 数据模型

### ShijimaWidget
- 新增 `QString m_matrixRoomId` — 该 mascot 监听的 room ID
- 为空表示未分配（unassigned）

### ShijimaContextMenu
- 新增 "分配到房间..." 菜单项 → 弹出输入对话框，接收 room ID
- 新增 "取消房间分配" 菜单项（仅当已分配时显示）

## MatrixClient 改动

### sendMessage()
```
void sendMessage(const QString &text, const QString &roomId = "")
```
- `roomId` 为空时：使用默认 `m_roomId`（向后兼容）
- `roomId` 有值时：发送到指定 room

## 消息路由

Matrix 消息到达时（`messageReceived(sender, body, roomId)`）：

1. 遍历所有 mascots
2. **精确匹配**：如果 `mascot->matrixRoomId() == roomId` → 该 mascot 显示气泡
3. **Fallback**：如果 `mascot->matrixRoomId().isEmpty()` → 所有未分配的 mascot 都显示气泡

## 发送消息

`ShijimaWidget::sendMatrixMessage(text)`：

- `m_matrixRoomId` 为空：显示 `[未分配房间]` 气泡，不发送
- `m_matrixRoomId` 有值：调用 `MatrixClient::sendMessage(text, m_matrixRoomId)`

## 用户交互流程

1. 右键点击 mascot → "分配到房间..." → 弹出对话框
2. 用户粘贴/输入 room ID → 确认
3. mascot 的 `m_matrixRoomId` 被设置，开始监听该 room
4. 收到消息时，按路由规则显示气泡

## 向后兼容

- 现有 `matrix.json` 配置的 room ID 继续作为默认 room
- 未分配 room 的 mascot 行为与之前一致（收到所有消息）
- 已连接的 Matrix 客户端保持不变