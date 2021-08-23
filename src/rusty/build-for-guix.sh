#! /bin/sh

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

rm -rf ${SCRIPT_DIR}/target
mkdir ${SCRIPT_DIR}/target
mkdir ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/sv2-ffi/Cargo.toml --allow-dirty
mv ${SCRIPT_DIR}/sv2-ffi/target/package/* ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/binary-sv2/binary-sv2/Cargo.toml --allow-dirty
mv ${SCRIPT_DIR}/binary-sv2/binary-sv2/target/package/* ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/binary-sv2/no-serde-sv2/codec/Cargo.toml --allow-dirty
mv ${SCRIPT_DIR}/binary-sv2/no-serde-sv2/codec/target/package/* ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/binary-sv2/no-serde-sv2/derive_codec/Cargo.toml --allow-dirty 
mv ${SCRIPT_DIR}/binary-sv2/no-serde-sv2/derive_codec/target/package/* ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/framing-sv2/Cargo.toml --allow-dirty
mv ${SCRIPT_DIR}/framing-sv2/target/package/* ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/const-sv2/Cargo.toml --allow-dirty
mv ${SCRIPT_DIR}/const-sv2/target/package/* ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/codec-sv2/Cargo.toml --allow-dirty --no-verify
mv ${SCRIPT_DIR}/codec-sv2/target/package/* ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/subprotocols/common-messages/Cargo.toml --allow-dirty
mv ${SCRIPT_DIR}/subprotocols/common-messages/target/package/* ${SCRIPT_DIR}/target/package

cargo package --manifest-path ${SCRIPT_DIR}/subprotocols/template-distribution/Cargo.toml --allow-dirty
mv ${SCRIPT_DIR}/subprotocols/template-distribution/target/package/* ${SCRIPT_DIR}/target/package


