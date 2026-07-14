While `w0vncserver` is a good VNC server to [run in your user session](https://github.com/qwerty12/dumb-kde-hacks/tree/master/tigervnc-w0vncserver-no-permission-prompt), it will not work to make the Plasma Login display manager available over VNC. So you can't use it to login remotely. One of the stated goals of Plasma Login is "Remote (VNC/RDP) support from startup" so hopefully this won't be needed in the future.

[ReFrame](https://github.com/AlynxZhou/reframe) is a VNC server that is DRM/KMS-based, so it avoids Wayland's imposed restrictions. It can even render VTs over VNC...
It works well for letting you login, but I find it's very glitchy when KWin is active, and probably other Wayland compositors too.

The [ReFrame installation steps] are easy enough to follow, I won't repeat them. My only suggestions are to set a password and use the following in your configuration file:

```ini
cursor=false
ip=127.0.0.1
```

Binding to localhost is important because only an SSH tunnel should be used to access the server in order to provide encryption. In this case, make sure sshd is configured to run on startup too.

If you are using a non-US keyboard layout, run

```shell
sudo systemctl edit reframe-server@
```

(Add your connector after the @)

and add the following:

```ini
[Service]
Environment=XKB_DEFAULT_LAYOUT=gb
```

replacing _gb_ with your keyboard layout, of course.

Remember to run the following to start it now and make it run on startup:

```shell
systemctl enable --now reframe-server@
```

(Add your connector after the @)

To stop ReFrame when logging in (so that `w0vncserver` is the only running VNC server), and make it start again when logging out, do the following (credits to Claude):

1. Edit on-session-change.sh to add your connector after the reframe-server@

2. Install the script:

```shell
sudo install -D -m 755 on-session-change.sh /usr/local/sbin/on-session-change.sh
```

3. Make Plasma Login run it when logging in and out.

If /etc/pam.d/plasmalogin doesn't exist, `cp /usr/lib/pam.d/plasmalogin /etc/pam.d/plasmalogin`

Add the following to the end of /etc/pam.d/plasmalogin:

```
-session optional pam_exec.so /usr/local/sbin/on-session-change.sh
```

Note: this probably doesn't handle multiple sessions/users well.
