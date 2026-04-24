# Shijima-Qt

![Shijima-Qt running on Fedora 41](.images/Shijima-Qt-Fedora.jpg)

Cross-platform shimeji desktop pet simulator. Built with Qt6. Supports macOS, Linux and Windows.

- [Download the latest release](https://github.com/pixelomer/Shijima-Qt/releases/latest)
- [See all releases](https://github.com/pixelomer/Shijima-Qt/releases)
- [Report a bug or make a feature request](https://github.com/pixelomer/Shijima-Qt/issues)
- [Shijima homepage](https://getshijima.app)

If you'd like to support the development of Shijima, consider becoming a [sponsor on GitHub](https://github.com/sponsors/pixelomer) or [buy me a coffee](https://buymeacoffee.com/pixelomer).

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

## Matrix Integration

Shijima-Qt supports Matrix messaging integration. When configured, your desktop mascot can:

- **Receive messages** from a Matrix room and display them as speech bubbles
- **Send messages** to a Matrix room via the right-click context menu

### Configuration

Create the config file at `~/.config/shijima-qt/matrix.json`:

```json
{
  "homeserver": "https://matrix.org",
  "userId": "@yourusername:matrix.org",
  "accessToken": "syt_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
  "roomId": "!xxxxxxxxxxxxxxxxxxx:matrix.org"
}
```

| Field | Description |
|-------|-------------|
| `homeserver` | Your Matrix homeserver URL (e.g. `https://matrix.org`) |
| `userId` | Your full Matrix user ID (e.g. `@alice:matrix.org`) |
| `accessToken` | Your Matrix access token for authentication |
| `roomId` | The room ID to join (e.g. `!abc123:matrix.org`) |

### Getting Your Access Token

**Via Element (Web/Desktop):**
1. Open Element and log in
2. Go to **Settings** → **Help & About** → **Advanced**
3. Click **Access Token** → copy the token

**Via curl:**
```bash
curl -XPOST -d '{"type":"m.login.password", "user":"yourusername", "password":"yourpassword"}' \
  "https://matrix.org/_matrix/client/r0/login"
```

### Finding Room ID

In Element, open the room → **Room Settings** → **Advanced** → copy the **Internal room ID**.

### Example: Complete Setup

```bash
# 1. Create config directory
mkdir -p ~/.config/shijima-qt

# 2. Write config (replace with your values)
cat > ~/.config/shijima-qt/matrix.json << 'EOF'
{
  "homeserver": "https://matrix.org",
  "userId": "@shijima_demo:matrix.org",
  "accessToken": "syt_xxxxxxxx_xxxxxxxx",
  "roomId": "!xxxxxxxxxxxx:matrix.org"
}
EOF

# 3. Run Shijima-Qt (Linux example)
LD_LIBRARY_PATH=libshimejifinder/build/unarr:$LD_LIBRARY_PATH ./shijima-qt
```

Now right-click your mascot and select **Send Message** to chat through Matrix!
