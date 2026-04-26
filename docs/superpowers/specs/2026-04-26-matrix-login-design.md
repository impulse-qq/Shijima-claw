# Matrix Login & Auto-Reconnect Design

## Overview

Rework MatrixClient authentication to use username/password login instead of a pre-configured static token. This enables automatic token refresh on 401 responses, resolving the "can't receive messages" issue caused by token expiration.

## Configuration Change

**File:** `matrix.json`

| Field | Required | Description |
|-------|----------|-------------|
| `homeserver` | Yes | Matrix homeserver URL |
| `userId` | Yes | Full Matrix user ID (e.g. `@tuanzhang:matrix.iladmin.cn`) |
| `username` | Yes | Username for login |
| `password` | Yes | Password for login |
| `roomId` | Yes | Target room ID |

`accessToken` field is removed — it is now obtained dynamically via login.

## Login Flow

### MatrixClient::login()

1. Read `username` + `password` from current config
2. POST `/_matrix/client/r0/login` with:
   ```json
   {
     "type": "m.login.password",
     "identifier": { "type": "m.id.user", "user": "<username>" },
     "password": "<password>"
   }
   ```
3. On 200: extract `access_token` from response, save to `m_accessToken`, set `m_connected = true`, emit `connectedChanged(true)`
4. On failure: emit `errorOccurred`, `m_connected` stays false

### loadConfig()

Add `username` and `password` to config fields. Validate these are non-empty.

## 401 Auto-Refresh

In `syncLoop()`, when HTTP response is 401:

1. Set `m_connected = false`
2. Log the 401
3. Call `login()` to refresh token
4. If login succeeds, retry the sync request once
5. If retry fails or login fails, enter existing exponential backoff retry loop

## sendMessage JSON Fix

**Current (broken):**
```json
{"body":"...","msgtype":"m.text"}
```

**Correct format:**
```json
{"content":{"body":"...","msgtype":"m.text"},"type":"m.room.message"}
```

## Logging Additions

| Event | Level | Log Message |
|-------|-------|-------------|
| login() called | INFO | `login() username=<username>` |
| login() success | INFO | `login() succeeded, token obtained` |
| login() failure | ERROR | `login() failed: <error>` |
| 401 received in sync | INFO | `syncLoop() got 401, refreshing token` |
| token refresh success | INFO | `syncLoop() token refreshed, retrying` |
| token refresh failure | INFO | `syncLoop() token refresh failed, retry in Xs` |

## Files to Modify

- `MatrixClient.cc` — login(), loadConfig(), syncLoop() 401 handling, sendMessage() JSON format
- `MatrixClient.hh` — no changes needed (same public API)
- `ShijimaManager.cc` — matrix.json path comment update (optional)
