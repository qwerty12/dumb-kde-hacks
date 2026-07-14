TigerVNC's [`w0vncserver`](https://tigervnc.org/doc/w0vncserver.html) is probably the only VNC server that supports KDE/the Wayland protocols KWin provides.

It works well, except every time you login, you're asked if you want to allow it to view and control the screen. The option to remember your choice lasts for one session. Apparently, the `RememberDisplayChoice` option is meant to solve this, but it doesn't work for me, so it's set to `Never` in the service file.

KDE supports a documented [XDG Portal Pre-Authorization](https://develop.kde.org/docs/administration/portal-permissions/) system. With this, you can have authorisation set up in advance, but it requires the app to have assigned itself an `app_id` (apparently, you can authorise every application without an app_id but that sounds a little extreme).

The steps here will give `w0vncserver` an app ID of `org.tigervnc.w0vncserver` and allow remote control without prompting you.

1. Set up a VNC password:

```shell
vncpasswd
```

2. Install a desktop file (based on KRdp's):

```shell
install -D -m 644 org.tigervnc.w0vncserver.desktop ~/.local/share/applications/org.tigervnc.w0vncserver.desktop
```

3. Install the systemd user service (one way to register the app id if unsandboxed, apparently), register `w0vncserver` for autostart and run it now on port 5150:

```shell
install -D -m 644 app-org.tigervnc.w0vncserver.service ~/.config/systemd/user/app-org.tigervnc.w0vncserver.service && systemctl --user enable --now app-org.tigervnc.w0vncserver.service
```

4. Pre-authorise `w0vncserver` (thanks [miso](https://askubuntu.com/a/1472853a) for an alternative to installing `flatpak`):

```shell
dbus-send --print-reply \
    --dest=org.freedesktop.impl.portal.PermissionStore \
    /org/freedesktop/impl/portal/PermissionStore \
    org.freedesktop.impl.portal.PermissionStore.SetPermission \
    string:'kde-authorized' \
    boolean:'true' \
    string:'remote-desktop' \
    string:'org.tigervnc.w0vncserver' \
    array:string:'yes'
```

If you actually look at what you're installing, you'll see `w0vncserver` binds to localhost only. This is because you should be using an SSH tunnel to be accessing your VNC servers. VNC by itself is unencrypted, so your VNC password is visible over the network, the contents of your computer screen are sent back unencrypted, and what you type and send to your computer is also unencrypted, including your passwords.

The open-source [AVNC](https://github.com/gujjwal00/avnc) Android VNC client makes it very easy to connect with an SSH tunnel:

Host: 127.0.0.1

Port: 5150

✅ Use SSH tunnel

SSH host: <your remote IP>
