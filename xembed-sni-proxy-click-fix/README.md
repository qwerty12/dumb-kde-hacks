For at least a decade, KDE's `xembed-sni-proxy` has been broken with regards to JDownloader.

Konrad Materka's patch from last year, [Draft: Revert "xembed-sni-proxy: Check if descendant windows want button events"](https://invent.kde.org/plasma/plasma-workspace/-/merge_requests/5383), largely fixes this, but the PR was closed and JD's tray icon remains broken to this day with KDE.

This is a crude `LD_PRELOAD`able implementation of said patch, so you don't need to recompile xembed-sni-proxy with it on every upgrade.

This hack relies on xembed-sni-proxy not changing *too* much.

Build and install first:

From this directory:
```shell
cmake -B build -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX="$HOME/.local/share/"
cmake --build build --target install/strip
```

Run `systemctl --user edit plasma-xembedsniproxy.service`

Add the following and save:

```ini
[Service]
Environment=LD_PRELOAD=%h/.local/share/xembedsniproxyclickfix/libxembedsniproxyclickfix.so
```

Run `systemctl --user daemon-reload && systemctl --user restart plasma-xembedsniproxy.service`

---

In JDownloader, I would recommend setting the tray icon settings thusly:

* Toggle window status with single click ☑️

* Show tooltip ◻️

There are some KWin Window Rules here in this repo you can import that _may_ make the JDownloader tray context icon a bit less annoying to work with.
