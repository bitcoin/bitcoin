Updated RPCs
------------

- `createwalletdescriptor` no longer requires the `hdkey` argument when the
  wallet has no active descriptors but does have exactly one HD key added with
  `addhdkey`. Previously the user had to look the key up with `listdescriptors`
  and pass it in. This makes it easier to set up a blank wallet that only has
  the descriptors it actually needs, which `doc/multisig-tutorial.md` now
  demonstrates. (#32861)
