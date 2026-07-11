Pastes the chosen clipboard contents when selecting something in KDE's clipboard manager, Klipper, under Wayland

When pressing ~~Win~~Meta+v and selecting from the list, whether it's the top-most item, whether it's via keyboard or mouse, when the focus is back on the first non-Klipper-popup-window, it will be pasted, like in Windows.

Why is Shift+Insert used for pasting? Shift+Insert seems to work in most places. Ctrl+V isn't supported in most terminals by default; Ctrl+Shift+V doesn't work in the Kate editor.

### Limitations:

* This is only designed to work with the Klipper popup window that appears when pressing Meta+v, not its system tray icon etc.

* A KWin script must be installed for klipper-paste-on-select to get notified about the active window. Blame it on Wayland and its fragmentation

* This is inherently prone to race conditions. It relies on signals from KWin to know if the Klipper popup is/is not the active window, and for a signal from Klipper that's sent when the stored clipboard contents are changed - even if done so from outside of Klipper. If the Klipper popup-window-closed signal is not received by klipper-paste-on-select within a second of receiving the clipboard-contents-changed one, to avoid erroneous pastes, klipper-paste-on-select will not send the paste key combination

* KWin jumps the gun a little and signals that a window is active before, apparently, it's ready to receive input. This program arbitrarily waits 150ms before pressing paste... There's no point in polling the Wayland compositor when said Wayland compositor already notified me

* This uses /dev/uinput to send the key combo, like every other application out there claiming to support Wayland

* Both the KWin script and klipper-paste-on-select assume the Klipper popup window isn't the active window at the time of their starting. If it is, things may go out of sync

That said, all in all, this actually does work rather reliably on the whole. Maybe not if your system is under heavy load, but you have bigger problems at that point.

### Installation

1. Make sure your user can access /dev/uinput. On Arch Linux, the easiest way to do this is to `sudo pacman -S ydotool` and follow its instructions during install to add yourself to the `input` group. On other distros, try something like [this](https://github.com/bendahl/uinput#uinput-----). Remember to log out and log in again.

2. Install the KWin script and activate kpos Window Notifier

```shell
kpackagetool6 --type=KWin/Script -i kpos-window-notifier/ && kcmshell6 kcm_kwin_scripts
```

3. Build the program

```shell
CGO_ENABLED=0 go build -trimpath -gcflags='all=-C -dwarf=false' -ldflags='-s -w -buildid=' -buildvcs=false
```

4. Install the binary

```shell
install -D -m 755 klipper-paste-on-select ~/.local/bin/klipper-paste-on-select
```

5. Install the systemd user service, register klipper-paste-on-select for autostart and run it now

```shell
install -D -m 644 klipper-paste-on-select.service ~/.config/systemd/user/klipper-paste-on-select.service && systemctl --user enable --now klipper-paste-on-select.service
```

### Credits:

* [defue/kde-klipper-paste-on-click](https://github.com/defue/kde-klipper-paste-on-click) - a lot of the concepts here come from this project

* [rvaiya/keyd/scripts/keyd-application-mapper](https://github.com/rvaiya/keyd/blob/master/scripts/keyd-application-mapper) - for demonstrating it's possible to get notified by KWin via D-Bus
