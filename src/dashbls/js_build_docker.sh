DOCKER_IMAGE=emscripten/emsdk

git submodule update --init --recursive
mkdir js_build

docker run \
  --rm \
  -v $(pwd):/src \
  -w="/src/js_build" \
  $DOCKER_IMAGE \
  emcmake cmake /src

docker run \
  --rm \
  -v $(pwd):/src \
  -w="/src/js_build" \
  $DOCKER_IMAGE \
  cmake --build . -- -j10
