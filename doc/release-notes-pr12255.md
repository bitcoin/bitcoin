systemd init file
=========

The systemd init file (`contrib/init/dashd.service`) has been changed to use
`/var/lib/dashd` as the data directory instead of `~dash/.dash`. This
change makes Dash Core more consistent with other services, and makes the
systemd init config more consistent with existing Upstart and OpenRC configs.

The configuration, PID, and data directories are now completely managed by
systemd, which will take care of their creation, permissions, etc. See
[`systemd.exec (5)`](https://www.freedesktop.org/software/systemd/man/systemd.exec.html#RuntimeDirectory=)
for more details.

When using the provided init files under `contrib/init`, overriding the
`datadir` option in `/etc/dash/dash.conf` will have no effect. This is
because the command line arguments specified in the init files take precedence
over the options specified in `/etc/dash/dash.conf`.
