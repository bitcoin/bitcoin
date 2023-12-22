Docker Build System
-------------------

Modified the Dockerfile to exclude the `dash-qt` graphical wallet interface from the set of binaries copied to
`/usr/local/bin` during the Docker image build process. This change streamlines the Docker image, making it more
suitable for server environments and headless applications, where the graphical user interface of `dash-qt` is not
required. The update enhances the Docker image's efficiency by reducing its size and minimizing unnecessary components.
