# Shijima-Qt

![Shijima-Qt running on Fedora 41](.images/Shijima-Qt-Fedora.jpg)

Cross-platform shimeji desktop pet simulator. Built with Qt6. Supports macOS, Linux and Windows.

- [Download the latest release](https://github.com/pixelomer/Shijima-Qt/releases/latest)
- [See all releases](https://github.com/pixelomer/Shijima-Qt/releases)
- [Report a bug or make a feature request](https://github.com/pixelomer/Shijima-Qt/issues)
- [Shijima homepage](https://getshijima.app)

If you'd like to support the development of Shijima, consider becoming a [sponsor on GitHub](https://github.com/sponsors/pixelomer) or [buy me a coffee](https://buymeacoffee.com/pixelomer).

## Quick Start

```bash
# Initialize submodules
git submodule update --init --recursive

# Build
CONFIG=release make -j8

# Run
./run.sh
```

## Building

If you have any problems with building Shijima-Qt, see the GitHub workflows in this repository.

### macOS

1. Install MacPorts.
2. Install build dependencies.

```bash
sudo port install qt6-qtbase qt6-qtmultimedia pkgconfig libarchive
```

3. Build.

```bash
CONFIG=release make -j8
```

### Linux

```bash
CONFIG=release make -j8
```

### Windows

A Docker image is provided to build Shijima-Qt.

```bash
docker build -t shijima-qt-dev docker-dev
docker run -e CONFIG=release --rm -v "$(pwd)":/work shijima-qt-dev bash -c 'mingw64-make -j8'
```

## Platform Notes

### macOS

Shijima-Qt needs the Accessibility permission to access the frontmost window.

### Linux

Shijima-Qt supports KDE Plasma 6 and GNOME 46 in both Wayland and X11. To get the frontmost window, Shijima-Qt automatically installs and enables a shell plugin when started.  
- On KDE, this is transparent to the user.
- On GNOME, the shell needs to be restarted on the first run. This can be done by logging out and logging back in. Shijima-Qt will exit with an appropriate error message if this is required.
- On other desktop environments, window tracking will not be available.

### Windows

Only tested on Windows 11. May also work on Windows 10. Window tracking is supported and no extra actions should be necessary to run Shijima-Qt.

## Matrix 集成

Shijima-Qt 支持 Matrix 消息集成。配置完成后，你的桌面宠物可以：

- **接收消息** — 从 Matrix 房间接收消息并显示为气泡
- **发送消息** — 通过右键菜单向 Matrix 房间发送消息
- **多房间支持** — 每个宠物可以分配到不同的 Matrix 房间

### 配置 matrix.json

Create the config file at `~/.config/shijima-qt/matrix.json`:

```json
{
  "homeserver": "https://matrix.org",
  "userId": "@yourusername:matrix.org",
  "username": "yourusername",
  "password": "yourpassword",
  "roomId": "!xxxxxxxxxxxxxxxxxxx:matrix.org"
}
```

| 字段 | 描述 |
|------|------|
| `homeserver` | Matrix 服务器地址（如 `https://matrix.org`） |
| `userId` | 你的 Matrix 用户 ID（如 `@alice:matrix.org`） |
| `username` | Matrix 用户名 |
| `password` | Matrix 密码 |
| `roomId` | 默认房间 ID（如 `!abc123:matrix.org`） |

### 每个宠物独立房间

可以为每个桌面宠物分配不同的 Matrix 房间，实现消息的独立收发：

1. **右键点击宠物** → **分配到房间...**
2. 输入 Room ID（如 `!xxxxxxxxxxxx:matrix.org`）
3. 该宠物将只接收分配房间的消息，并从该房间发送消息

**消息路由规则：**
- 如果有宠物被分配到消息所在的房间 → 该宠物显示气泡
- 如果没有宠物被分配到该房间 → 所有未分配房间的宠物都显示气泡（fallback）

**发送消息：**
- 右键点击宠物 → **发送消息...** → 消息会发送到该宠物分配的房间
- 如果宠物未分配房间，会显示 `[未分配房间]` 提示

**取消分配：**
- 右键点击宠物 → **分配到房间...** → 清空输入框后确认

### 获取 Access Token

**通过 Element（网页/桌面）：**
1. 打开 Element 并登录
2. 进入 **设置** → **帮助与关于** → **高级**
3. 点击 **Access Token** → 复制令牌

**通过 curl：**
```bash
curl -XPOST -d '{"type":"m.login.password", "user":"yourusername", "password":"yourpassword"}' \
  "https://matrix.org/_matrix/client/r0/login"
```

### 查找 Room ID

在 Element 中，打开房间 → **房间设置** → **高级** → 复制 **内部 Room ID**。

### 完整配置示例

```bash
# 1. 创建配置目录
mkdir -p ~/.config/shijima-qt

# 2. 写入配置（替换为你自己的值）
cat > ~/.config/shijima-qt/matrix.json << 'EOF'
{
  "homeserver": "https://matrix.org",
  "userId": "@shijima_demo:matrix.org",
  "username": "yourusername",
  "password": "yourpassword",
  "roomId": "!xxxxxxxxxxxx:matrix.org"
}
EOF

# 3. 运行 Shijima-Qt（Linux 示例）
LD_LIBRARY_PATH=libshimejifinder/build/unarr:$LD_LIBRARY_PATH ./shijima-qt
```

现在右键点击你的宠物，选择 **发送消息** 来通过 Matrix 聊天吧！
