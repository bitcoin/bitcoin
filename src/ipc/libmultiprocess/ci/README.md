### CI quick-reference

All CI is just bash and nix.

* **Workflow**:
  -  `.github/workflows/ci.yml` – lists the jobs (`default`, `llvm`, …).
* **Scripts**:
  - `ci/scripts/run.sh` – spins up the Nix shell then calls…
  - `ci/scripts/ci.sh` – …to configure, build, and test.
* **Configuration**:
  - `ci/configs/*.sh` – defines flags for each job.
  - `shell.nix` – defines build environment (compilers, tools, libraries).
* **Build directories**:
  - `build-*/` – separate build directories (like `build-default`, `build-llvm`) will be created for each job.

To run jobs locally:

```bash
CI_CONFIG=ci/configs/default.bash  ci/scripts/run.sh
CI_CONFIG=ci/configs/llvm.bash     ci/scripts/run.sh
CI_CONFIG=ci/configs/gnu32.bash    ci/scripts/run.sh
CI_CONFIG=ci/configs/sanitize.bash ci/scripts/run.sh
CI_CONFIG=ci/configs/olddeps.bash  ci/scripts/run.sh
```

By default CI jobs will reuse their build directories. `CI_CLEAN=1` can be specified to delete them before running instead.

### Running workflows with `act`

You can run either the entire workflow or a single matrix entry locally. On
macOS or Linux:

1. Install [`act`](https://github.com/nektos/act) and either Docker or
  Podman.
2. Inside the Podman VM, create a named volume for the Nix store (ext4,
  case-sensitive) so builds persist across runs. Recreate it any time you want
  a clean cache:
  ```bash
  podman volume create libmultiprocess-nix
  ```
3. From the repo root, launch the workflow. The example below targets the
  sanitize matrix entry; drop the `--matrix` flag to run every configuration.
  ```bash
  act \
    --reuse \
    -P ubuntu-latest=ghcr.io/catthehacker/ubuntu:act-24.04 \
    --container-options "-v libmultiprocess-nix:/nix" \
    -j build \
    --matrix config:sanitize
  ```


