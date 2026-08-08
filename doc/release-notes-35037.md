Updated settings
----------------

- IPC listeners configured with `-ipcbind` now reserve and accept
  up to 16 client connections by default. The limit can be set per
  listener by appending `,max-connections=<n>` to the bind address,
  for example `-ipcbind=unix:,max-connections=8`.
