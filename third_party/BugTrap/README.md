# BugTrap (vendored)

Crash reporter from [bchavez/BugTrap](https://github.com/bchavez/BugTrap) v1.4.9 (MIT).

This project is **MultiByte / x64**, so we link and ship:

- `bin/BugTrap-x64.dll` + `lib/BugTrap-x64.lib` (Release)
- `bin/BugTrapD-x64.dll` + `lib/BugTrapD-x64.lib` (Debug)

Integration lives in `source/src/util/ErrorHandler.cpp` (`BT_InstallSehFilter`).
Reports are written under the process log directory in a `crashes` folder.
`release/` is the full upstream package and is not required at build time.
