Updated settings
----------------

- IPC listeners configured with `-ipcbind` now reserve and accept
  up to 16 client connections by default. The limit can be set per
  listener by appending `,max-connections=<n>` to the bind address,
  for example `-ipcbind=unix:/path_to_socket_file.sock,max-connections=8`.
