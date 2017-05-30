Sample init scripts and service configuration for flowd
==========================================================

Sample scripts and configuration files for systemd, Upstart and OpenRC
can be found in the contrib/init folder.

    contrib/init/flowd.service:    systemd service unit configuration
    contrib/init/flowd.openrc:     OpenRC compatible SysV style init script
    contrib/init/flowd.openrcconf: OpenRC conf.d file
    contrib/init/flowd.conf:       Upstart service configuration file
    contrib/init/flowd.init:       CentOS compatible SysV style init script

1. Service User
---------------------------------

All three Linux startup configurations assume the existence of a "flow" user
and group.  They must be created before attempting to use these scripts.
The OS X configuration assumes flowd will be set up for the current user.

2. Configuration
---------------------------------

At a bare minimum, flowd requires that the rpcpassword setting be set
when running as a daemon.  If the configuration file does not exist or this
setting is not set, flowd will shutdown promptly after startup.

This password does not have to be remembered or typed as it is mostly used
as a fixed token that flowd and client programs read from the configuration
file, however it is recommended that a strong and secure password be used
as this password is security critical to securing the wallet should the
wallet be enabled.

If flowd is run with the "-server" flag (set by default), and no rpcpassword is set,
it will use a special cookie file for authentication. The cookie is generated with random
content when the daemon starts, and deleted when it exits. Read access to this file
controls who can access it through RPC.

By default the cookie is stored in the data directory, but it's location can be overridden
with the option '-rpccookiefile'.

This allows for running flowd without having to do any manual configuration.

`conf`, `pid`, and `wallet` accept relative paths which are interpreted as
relative to the data directory. `wallet` *only* supports relative paths.

For an example configuration file that describes the configuration settings,
see `contrib/debian/examples/flow.conf`.

3. Paths
---------------------------------

3a) Linux

All three configurations assume several paths that might need to be adjusted.

Binary:              `/usr/bin/flowd`  
Configuration file:  `/etc/flow/flow.conf`  
Data directory:      `/var/lib/flowd`  
PID file:            `/var/run/flowd/flowd.pid` (OpenRC and Upstart) or `/var/lib/flowd/flowd.pid` (systemd)  
Lock file:           `/var/lock/subsys/flowd` (CentOS)  

The configuration file, PID directory (if applicable) and data directory
should all be owned by the flow user and group.  It is advised for security
reasons to make the configuration file and data directory only readable by the
flow user and group.  Access to flow-cli and other flowd rpc clients
can then be controlled by group membership.

3b) Mac OS X

Binary:              `/usr/local/bin/flowd`  
Configuration file:  `~/Library/Application Support/Flow/flow.conf`  
Data directory:      `~/Library/Application Support/Flow`
Lock file:           `~/Library/Application Support/Flow/.lock`

4. Installing Service Configuration
-----------------------------------

4a) systemd

Installing this .service file consists of just copying it to
/usr/lib/systemd/system directory, followed by the command
`systemctl daemon-reload` in order to update running systemd configuration.

To test, run `systemctl start flowd` and to enable for system startup run
`systemctl enable flowd`

4b) OpenRC

Rename flowd.openrc to flowd and drop it in /etc/init.d.  Double
check ownership and permissions and make it executable.  Test it with
`/etc/init.d/flowd start` and configure it to run on startup with
`rc-update add flowd`

4c) Upstart (for Debian/Ubuntu based distributions)

Drop flowd.conf in /etc/init.  Test by running `service flowd start`
it will automatically start on reboot.

NOTE: This script is incompatible with CentOS 5 and Amazon Linux 2014 as they
use old versions of Upstart and do not supply the start-stop-daemon utility.

4d) CentOS

Copy flowd.init to /etc/init.d/flowd. Test by running `service flowd start`.

Using this script, you can adjust the path and flags to the flowd program by
setting the FLOWD and FLAGS environment variables in the file
/etc/sysconfig/flowd. You can also use the DAEMONOPTS environment variable here.

4e) Mac OS X

Copy org.flow.flowd.plist into ~/Library/LaunchAgents. Load the launch agent by
running `launchctl load ~/Library/LaunchAgents/org.flow.flowd.plist`.

This Launch Agent will cause flowd to start whenever the user logs in.

NOTE: This approach is intended for those wanting to run flowd as the current user.
You will need to modify org.flow.flowd.plist if you intend to use it as a
Launch Daemon with a dedicated flow user.

5. Auto-respawn
-----------------------------------

Auto respawning is currently only configured for Upstart and systemd.
Reasonable defaults have been chosen but YMMV.
