#! /bin/sh

ROOT=$1
OUT_DIR=$2

DEPS="./deps"

rm -rf $DEPS

mkdir $DEPS

rustc \
        --crate-name binary_codec_sv2 \
        --edition=2018 \
        $ROOT/binary-sv2/no-serde-sv2/codec/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi \
        --crate-type lib \
        --emit=dep-info,metadata,link \
        -C embed-bitcode=no \
        -C debug-assertions=off \
        --out-dir $DEPS \
        -L dependency=$DEPS

rustc \
        --crate-name binary_codec_sv2 \
        --edition=2018 \
        $ROOT/binary-sv2/no-serde-sv2/codec/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi,artifacts \
        --crate-type lib \
        --emit=dep-info,metadata,link \
        -C opt-level=3 \
        -C embed-bitcode=no \
        --out-dir $DEPS \
        -L dependency=$DEPS

rustc \
        --crate-name const_sv2 \
        --edition=2018 \
        $ROOT/const-sv2/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi,artifacts \
        --crate-type lib \
        --emit=dep-info,metadata,link \
        -C opt-level=3 \
        -C embed-bitcode=no \
        --out-dir $DEPS \
        -L dependency=$DEPS

rustc \
        --crate-name derive_codec_sv2 \
        --edition=2018 \
        $ROOT/binary-sv2/no-serde-sv2/derive_codec/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi \
        --crate-type proc-macro \
        --emit=dep-info,link \
        -C prefer-dynamic \
        -C embed-bitcode=no \
        -C debug-assertions=off \
        --out-dir $DEPS \
        -L dependency=$DEPS \
        --extern binary_codec_sv2=$DEPS/libbinary_codec_sv2.rlib \
        --extern proc_macro

rustc \
        --crate-name binary_sv2 \
        --edition=2018 \
        $ROOT/binary-sv2/binary-sv2/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi,artifacts \
        --crate-type lib \
        --emit=dep-info,metadata,link \
        -C opt-level=3 \
        -C embed-bitcode=no \
        --cfg 'feature="binary_codec_sv2"' \
        --cfg 'feature="core"' \
        --cfg 'feature="default"' \
        --cfg 'feature="derive_codec_sv2"' \
        --out-dir $DEPS \
        -L dependency=$DEPS \
        --extern binary_codec_sv2=$DEPS/libbinary_codec_sv2.rmeta \
        --extern derive_codec_sv2=$DEPS/libderive_codec_sv2.so

rustc \
        --crate-name framing_sv2 \
        --edition=2018 \
        $ROOT/framing-sv2/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi,artifacts \
        --crate-type lib \
        --emit=dep-info,metadata,link \
        -C opt-level=3 \
        -C embed-bitcode=no \
        --out-dir $DEPS \
        -L dependency=$DEPS \
        --extern binary_sv2=$DEPS/libbinary_sv2.rmeta \
        --extern const_sv2=$DEPS/libconst_sv2.rmeta

rustc \
        --crate-name common_messages_sv2 \
        --edition=2018 \
        $ROOT/subprotocols/common-messages/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi \
        --crate-type lib \
        --emit=dep-info,metadata,link \
        -C opt-level=3 \
        -C embed-bitcode=no \
        --out-dir $DEPS \
        -L dependency=$DEPS \
        --extern binary_sv2=$DEPS/libbinary_sv2.rmeta \
        --extern const_sv2=$DEPS/libconst_sv2.rmeta

rustc \
        --crate-name template_distribution_sv2 \
        --edition=2018 \
        $ROOT/subprotocols/template-distribution/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi \
        --crate-type lib \
        --emit=dep-info,metadata,link \
        -C opt-level=3 \
        -C embed-bitcode=no \
        --out-dir $DEPS \
        -L dependency=$DEPS \
        --extern binary_sv2=$DEPS/libbinary_sv2.rmeta \
        --extern const_sv2=$DEPS/libconst_sv2.rmeta

rustc \
        --crate-name codec_sv2 \
        --edition=2018 \
        $ROOT/codec-sv2/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi \
        --crate-type lib \
        --emit=dep-info,metadata,link \
        -C opt-level=3 \
        -C embed-bitcode=no \
        --out-dir $DEPS \
        -L dependency=$DEPS \
        --extern binary_sv2=$DEPS/libbinary_sv2.rmeta \
        --extern const_sv2=$DEPS/libconst_sv2.rmeta \
        --extern framing_sv2=$DEPS/libframing_sv2.rmeta

rustc \
        --crate-name sv2_ffi \
        --edition=2018 \
        $ROOT/sv2-ffi/src/lib.rs \
        --error-format=json \
        --json=diagnostic-rendered-ansi \
        --crate-type staticlib \
        -C opt-level=3 \
        -C embed-bitcode=no \
        --out-dir $OUT_DIR  \
        -L dependency=$DEPS \
        --extern binary_sv2=$DEPS/libbinary_sv2.rlib \
        --extern codec_sv2=$DEPS/libcodec_sv2.rlib \
        --extern common_messages_sv2=$DEPS/libcommon_messages_sv2.rlib \
        --extern const_sv2=$DEPS/libconst_sv2.rlib \
        --extern template_distribution_sv2=$DEPS/libtemplate_distribution_sv2.rlib

