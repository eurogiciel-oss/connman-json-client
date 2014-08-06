# connman-json-client

This project aim to provide a simple ncurses interface to control ConnMan through
dbus.

It's based on connmanctl, the official command line client for ConnMan.
ConnMan's home page: https://connman.net

## Build

Building the project is as straight forward as running `run-me.sh`.
You will need development packages for dbus, ncurses and libjson and of course
autotools.

## Ncurses UI sneak peek

Main screen, technologies available:
```text
 Connman ncurses UI                       State: online      OfflineMode: false
┌──────────────────────────────────────────────────────────────────────────────┐
│ Technologies:                                                                │
│                                                                              │
│ p2p      P2P (p2p)            Powered true           Connected false         │
│ ethernet Wired (ethernet)     Powered true           Connected true          │
│ wifi     WiFi (wifi)          Powered true           Connected false         │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
 [INFO] 'd' to disconnect, 'Return' for details, 'F5' to force refresh
 [INFO] ^C to quit
```

Wifi networks you can connect to:
```text
 Connman ncurses UI                       State: online      OfflineMode: false
┌──────────────────────────────────────────────────────────────────────────────┐
│ Choose a network to connect to:                                              │
│                                                                              │
│ eSSID                            State            Security    Signal Strength│
│ XXXXXXXXX                        idle             [ "psk" ]          49%     │
│ XXX_XXXXXXXX                     idle             [ "wep" ]          48%     │
│ XXX_XXX                          idle             [ "psk" ]          48%     │
│ XXX_XXXX                         idle             [ "none" ]         47%     │
│ XXX_XXX_XXXXXX                   idle             [ "wep" ]          47%     │
│ XXX_XXX_XXXX                     idle             [ "psk" ]          47%     │
│ XXX_XXXX                         idle             [ "psk" ]          46%     │
│ XX-XXXXX-XX-XXXXXXXXXXXXXX       idle             [ "none" ]         39%     │
│ XXXX XXXXXXXXXX                  idle             [ "psk", "wps" ]   37%     │
│ XXXXXXX-XXXX                     idle             [ "psk", "wps" ]   35%     │
│ XXXXXX                           idle             [ "none" ]         34%     │
│ XXX_XXXX                         idle             [ "none" ]         27%     │
│ XXX_XXX                          idle             [ "psk" ]          27%     │
│ XXX_XXXX                         idle             [ "psk" ]          27%     │
│ XXXXXXXXXXX                      idle             [ "psk" ]          27%     │
│ XXX_XXX_XXXX                     idle             [ "wep" ]          26%     │
└──────────────────────────────────────────────────────────────────────────────┘
 [INFO] 'F5' to refresh network list, 'F6' to force a scan
 [INFO] 'Esc' to get back
```
