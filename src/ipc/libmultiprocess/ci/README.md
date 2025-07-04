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
```

By default CI jobs will reuse their build directories. `CI_CLEAN=1` can be specified to delete them before running instead.
