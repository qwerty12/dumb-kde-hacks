Hack to make the sleep button in KDE Plasma's kickoff applet and the logoff screen ignore inhibitors and sleep anyway

(GNOME does this, which makes sense to me)

This relies on the Arch Linux packaging system to install a hook to automatically run [`patchelf`](https://github.com/NixOS/patchelf) to add the kforcesuspend hook library to `plasmashell` and `ksmserver-logout-greeter` on install of this package and upgrades of `plasma-workspace`. Adapt as necessary for your distro.

The hook _should_ refuse to add the hook library if the versions of QtDBus mismatch; a recompile of kforcesuspend will be necessary.

1. `git clone https://github.com/qwerty12/dumb-kde-hacks.git`

2. `cd dumb-kde-hacks/kforcesuspend && makepkg -si`

3. Log out and log in again

This installs a Polkit rule to allow suspending with inhibitors active without requiring you to enter your password - for users in the `wheel` group. This was done because when pressing the log out button on the full-screen logoff window, the password prompt appears behind it and is impossible to focus.

If you run a multi-user system where some KDE users aren't part of the wheel group, you might want to enable the `POLKIT_CHECK_AUTH` CMake option. With it enabled, when kforcesuspend is loaded into `ksmserver-logout-greeter`, it will check if the caller can suspend while bypassing inhibitors without needing to enter a password. If this isn't the case, the hook won't call the inhibitor-bypassing suspend method to avoid the problem mentioned above.

Of course, if you disable /usr/share/polkit-1/rules.d/kforcesuspend-ignore-inhibit.rules entirely, you might just find it better to make the hook/install script simply not patch `ksmserver-logout-greeter`.

### Why a hook? KDE Plasma is open source.

This is small enough to trivially patch, and you don't run into the hassle of needing to rebuild plasma-workspace from source every single time for something so basic.

To undo this, just uninstall like normal, and the dependency on libkforcesuspend.so will be removed from `plasmashell`. You can run `sudo pacman -S plasma-workspace` to restore the original binaries afterwards if you so wish.

## Physical buttons

Using the sleep button on your keyboard will still obey inhibitors; this doesn't attempt to change anything there.

In KDE, it is Powerdevil that handles the buttons' events. However, it is also the component that puts your PC to sleep when idle - patching it with kforcesuspend would just mean all inhibitors are ignored.

For the sleep button on your keyboard, the simple workaround is this:

* from System Settings -> Shortcuts, unbind Sleep from the default Power Management action

* add a custom shortcut: `/usr/bin/gdbus call -y -i -d org.freedesktop.login1 -o /org/freedesktop/login1 -m org.freedesktop.login1.Manager.SuspendWithFlags 16`

* bind your keyboard's sleep button to that

For the power button physically on the PC itself, it's not quite as easy. Powerdevil asks logind if it can handle the power button events itself, and logind obliges, so the settings available in logind.conf have no effect.

The workarounds seem to be:

* in KDE's System Settings, make pressing the power button show the logout screen - with kforcesuspend, the sleep button there will ignore inhibitors

* install [acpid](https://wiki.archlinux.org/title/Acpid) and make your power button run the `gdbus` command above. Make sure you have the power button set to do nothing in KDE's settings

Thanks to [`kubo/plthook`](https://github.com/kubo/plthook) for the hooking library used.
