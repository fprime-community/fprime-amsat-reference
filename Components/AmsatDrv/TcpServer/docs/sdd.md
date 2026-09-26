# AmsatDrv::TcpServer

A TCP server byte stream driver: F´'s `Drv::TcpServer` (from the F´ version this project uses, nasa/fprime `52412392`) with two fixes. CDHDeployment uses it as `comDriver`, the GDS link. It has the same ports, `configure`/`start`/`join` API, and behavior while a GDS is connected. Replace it with `Drv.TcpServer` after moving to an F´ release with equivalent fixes (upstream fixed the shutdown hang in `e3fd78f28` and reworked reconnection in `bae7d2573`).

## Fixes

| Problem in `Drv::TcpServer` | Effect on CDHDeployment | Fix |
|---|---|---|
| `send` reconnects when no GDS is connected. It calls `accept()` on the sending thread (ComQueue's) while holding the component's port lock. | While no GDS is connected, the telemetry thread blocks until one connects. On SIGTERM, shutdown waits on it forever. | Sends without a connection return the buffer with `OTHER_ERROR` at once. Only the read task accepts connections. |
| If the read task is already in `accept()`, that reconnect reports success and `send` asserts on the invalid socket (`IpSocket.cpp: FW_ASSERT(socketDescriptor.fd != -1)`). | After a GDS disconnects, SIGTERM crashes the application with a core dump. | As above: a send never reconnects. |
| `stop()` and `terminate()` do not wake a read task blocked in `accept()`. Closing a socket does not interrupt `accept()` on Linux. | On SIGTERM before any GDS has connected, joining the read task hangs, so `systemctl stop` waits for its timeout. | `terminate()` stops the read task, then shuts down and closes the listening socket (backported from `e3fd78f28`). Teardown calls `terminate()` instead of `stop()`. |

A send that finds the connection closed underneath it closes the connection only if the read task has not already replaced it with a new one.

## Usage

```cpp
comDriver.configure(hostname, port);
comDriver.start(name, priority, stackSize);  // read task: listens and accepts connections
...
comDriver.terminate();                       // at teardown, instead of stop()
(void)comDriver.join();
```

## Unit Tests

The 7 unit tests of `Drv::TcpServer` are carried over unchanged (messaging, receive thread, reconnection, auto-connect, buffer deallocation). New tests cover the fixes. Each would hang or crash with `Drv::TcpServer`, so each runs its blocking calls under a time limit and fails within seconds instead.

| Name | Description |
|---|---|
| Shutdown.SendWithoutConnection | With no client and no read task, a send returns `OTHER_ERROR` within 500 ms |
| Shutdown.SendWhileReadTaskAccepts | While the read task waits in `accept()`, a send returns `OTHER_ERROR` within 500 ms without asserting |
| Shutdown.TerminateWakesAccept | `terminate()` and `join()` finish within 5 s while the read task waits in `accept()` |

`CDHDeployment/test/host/test_shutdown.py` checks the whole deployment: SIGTERM exits with status 0 within 5 s when a GDS was never connected, has disconnected, or is connected.

Run the unit tests from this directory:

```sh
fprime-util generate --ut
fprime-util check
```
