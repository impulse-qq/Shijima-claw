# Matrix Client Integration Design

**Date:** 2026-04-25
**Status:** Approved

## Overview

Integrate Matrix protocol support into Shijima-Qt, transforming the mascot simulator into a minimal Matrix client where mascots display and notify messages. Mascots act as the primary UI for messages through bubble notifications and animated responses.

## Architecture

```
ShijimaQt
├── ShijimaManager            # Core manager (unchanged)
├── MatrixClient              # NEW: Standalone Matrix client class
│   ├── loadConfig()         # Load JSON config (account, room)
│   ├── login()              # POST /login
│   ├── startSyncLoop()     # Periodic /sync polling
│   ├── sendMessage()        # PUT /rooms/{roomId}/send/m.room.message
│   └── handleEvent()        # Process incoming events
├── ShijimaHttpApi           # Existing HTTP API (unchanged)
├── ShijimaWidget            # Mascot + message bubble rendering
└── ShijimaContextMenu       # Right-click menu with Matrix options
```

## Config Format

`~/.config/shijima-qt/matrix.json`:

```json
{
  "homeserver": "https://matrix.org",
  "userId": "@user:matrix.org",
  "accessToken": "...",
  "roomId": "!roomId:matrix.org"
}
```

Flow: startup → loadConfig → login → startSyncLoop → handle incoming events

## Features

### Login
- On startup, load config from JSON file
- Auto-login if accessToken valid (token-based auth)
- If no token, show error in logs; user must configure valid credentials

### Sync Loop
- Poll `/sync?timeout=30000` every ~30 seconds
- Detect new messages in configured room
- On new message event → trigger mascot animation + bubble display

### Message Display
- Message appears as bubble tooltip near mascot
- Bubble fades after 5 seconds
- On new message: mascot plays "notify" animation (waving or special behavior)
- Only messages from configured room are displayed

### User Interaction
- **Right-click menu**: "Send message" → popup input dialog
- **Hotkey**: configurable shortcut (default: Ctrl+Shift+M) → popup input dialog
- Input: text field → Enter to send or Escape to cancel

### Send Message
- POST to `/rooms/{roomId}/send/m.room.message` with access token
- Text message only (m.text)

### Error Handling
- Connection failure: retry with exponential backoff (1s, 2s, 4s, max 60s)
- Invalid token: log error; mascot shows "disconnected" state
- Send failure: log error; do not retry automatically

## Tech Stack

- **HTTP Client**: cpp-httplib (existing, via submodule)
- **JSON**: nlohmann/json (existing, embedded or via dependency)
- **No E2EE**: plaintext room messages only
- **No WebEngine**: pure C++ implementation

## Implementation Notes

- MatrixClient is a standalone class, decoupled from ShijimaManager
- Sync loop runs in a separate thread to avoid blocking UI
- Incoming messages posted to main thread via Qt signal
- Bubble rendering reuses existing notification infrastructure in ShijimaWidget
- Right-click menu extended via ShijimaContextMenu

## Files to Create/Modify

### New Files
- `MatrixClient.hh` — Header
- `MatrixClient.cc` — Implementation

### Modified Files
- `ShijimaWidget.cc/.hh` — Add message bubble rendering
- `ShijimaContextMenu.cc/.hh` — Add "Send Message" menu item
- `Makefile` or `common.mk` — Add MatrixClient.o to build
- `shijima-qt.ico` — Unchanged
- `AGENTS.md` — Update with Matrix integration docs

## Out of Scope

- Room management / joining new rooms
- User search / invite
- Media / attachments
- End-to-end encryption
- Multiple room support
- Notifications outside the configured room