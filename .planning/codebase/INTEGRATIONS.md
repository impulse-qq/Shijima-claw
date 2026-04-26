# External Integrations

**Analysis Date:** 2026/04/26

## APIs & External Services

**Local HTTP API (Shijima-Qt):**
- REST API on `127.0.0.1:32456`
- Implementation: `cpp-httplib` v0.19.0 (header-only)
- Auth: None (loopback-only)
- Base URL: `http://127.0.0.1:32456/shijima/api/v1`
- Single-instance enforcement: `main.cc` pings `/ping` before starting; throws if already running

**Endpoints (from `ShijimaHttpApi.cc`):**

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/shijima/api/v1/mascots` | List active mascots |
| POST | `/shijima/api/v1/mascots` | Spawn new mascot |
| GET | `/shijima/api/v1/mascots/:id` | Get single mascot data |
| PUT | `/shijima/api/v1/mascots/:id` | Alter mascot position/behavior |
| DELETE | `/shijima/api/v1/mascots/:id` | Remove specific mascot |
| DELETE | `/shijima/api/v1/mascots` | Remove all mascots |
| GET | `/shijima/api/v1/loadedMascots` | List available mascots |
| GET | `/shijima/api/v1/loadedMascots/:id` | Get mascot info |
| GET | `/shijima/api/v1/loadedMascots/:id/preview.png` | Get mascot preview image |
| GET | `/shijima/api/v1/ping` | Health check |

**Selector Support:** GET `/mascots?selector=<js_expr>` filters mascots via Duktape JavaScript evaluation

**Matrix Integration:**
- Implementation: `MatrixClient.cc` / `MatrixClient.hh`
- Protocol: Matrix Client-Server API (r0)
- Auth: Access token-based
- Config file: `~/.config/shijima-qt/matrix.json`

**Matrix Config Schema:**
```json
{
  "homeserver": "https://matrix.org",
  "userId": "@username:matrix.org",
  "accessToken": "syt_xxxxx",
  "roomId": "!roomid:matrix.org"
}
```

**Matrix Endpoints Used (from `MatrixClient.cc`):**
- `/_matrix/client/r0/sync?timeout=30000` - Long-poll for incoming messages
- `/_matrix/client/r0/rooms/:roomId/send/m.room.message/:txnId` - Send message

**Matrix Message Format:**
```json
{
  "type": "m.room.message",
  "content": {
    "msgtype": "m.text",
    "body": "<message text>"
  }
}
```

**Matrix Retry Logic:**
- Exponential backoff on failures (1s, 2s, 4s, ... up to 60s max)
- Auto-reconnect on connection loss

## Data Storage

**File Storage:**
- **Mascot assets** - Directories with image frames + XML configs
  - Default mascot: `DefaultMascot/` (46 PNG frames + `behaviors.xml` + `actions.xml`)
  - User mascots: Imported from `.zip` archives via `libshimejifinder`
- **Settings:** `QSettings` (platform-native: `~/.config/shijima-qt/` on Linux)
- **Matrix config:** `~/.config/shijima-qt/matrix.json`

**Archive Extraction:**
- `libshimejifinder` - Extracts mascot archives
- Engine: `unarr` (RAR/7z/ZIP) + `libarchive` (additional formats)

## Authentication & Identity

**Matrix:**
- Access token stored in config file
- No password-based login
- Token acquired via Element web client or `curl` login API

**Desktop Permissions:**
- **macOS:** Accessibility permission required for frontmost window detection
- **Linux (GNOME):** Shell extension auto-installed; requires restart on first run
- **Linux (KDE):** KWin script auto-loaded via DBus

## Desktop Environment Integrations

**Linux - KDE Plasma:**
- `KWin.cc` / `KWin.hpp` - KWin DBus API (`org.kde.KWin.Scripting`)
- `KDEWindowObserverBackend.cc` - Window tracking for KDE
- Script: `kwin_script.js` embedded as C array

**Linux - GNOME:**
- `GNOME.cc` / `GNOME.hpp` - GNOME Shell extension management
- `GNOMEWindowObserverBackend.cc` - Window tracking for GNOME
- Extension: `shijima-helper@pixelomer.github.io` at `Platform/Linux/gnome_script/`
- Auto-installed to `~/.local/share/gnome-shell/extensions/`
- Requires logout/login after first install

**Linux - DBus:**
- `DBus.cc` / `DBus.hpp` - Generic DBus communication
- Used for KWin script loading and inter-process communication

**macOS:**
- AppKit.framework for window management
- ApplicationServices.framework for accessibility APIs

**Windows:**
- Win32 API for window tracking
- ws2_32 (Winsock2) for HTTP API

## Environment Configuration

**Required env vars:** None at runtime

**Build-time vars:**
- `CONFIG` — `release` or `debug`
- `SHIJIMA_USE_QTMULTIMEDIA` — `1` or `0`
- `PREFIX` — Install prefix (default `/usr/local`)

## Submodule Dependencies

| Submodule | URL | Purpose |
|-----------|-----|---------|
| `libshijima/` | `git@github.com:pixelomer/libshijima` | Mascot behavior engine (Duktape scripting, RapidXML) |
| `libshimejifinder/` | `git@github.com:pixelomer/libshimejifinder` | Archive extraction (unarr + libarchive) |
| `cpp-httplib/` | `https://github.com/yhirose/cpp-httplib` | HTTP server/client (header-only, v0.19.0) |

---

*Integration audit: 2026/04/26*
