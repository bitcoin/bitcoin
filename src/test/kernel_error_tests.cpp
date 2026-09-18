// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/error.h>
#include <node/kernel_notifications.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/translation.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

namespace {
const fs::path DIR{"/tmp/chainstate_snapshot"};

//! Every fatal error, so that the tests below cover all of them.
const std::vector<kernel::FatalError> ALL_FATAL_ERRORS{
    kernel::ActivateBestChainsFailed{.error = "Chainstate [ibd] @ height 1 (0000) Failed to connect best block (reason)"},
    kernel::AssumeutxoDataNotFound{.blockhash = uint256::ONE},
    kernel::BlockDisconnectFailed{},
    kernel::BlockFileCloseFailed{},
    kernel::BlockReadFailed{},
    kernel::BlockWriteFailed{},
    kernel::CorruptBlockFound{},
    kernel::DiskSpaceTooLow{},
    kernel::FailedToStartIndexes{},
    kernel::SnapshotChainstateDirRemovalFailed{.dir = DIR},
    kernel::SnapshotChainstateRenameFailed{.old_path = DIR, .new_path = DIR / "invalid"},
    kernel::SnapshotValidationFailed{.height_from = 200, .height_to = 100},
    kernel::SystemErrorWhileFlushing{.what = "what"},
    kernel::SystemErrorWhileLoadingExternalBlockFile{.what = "what"},
    kernel::SystemErrorWhileSavingBlock{.what = "what"},
    kernel::UndoDataWriteFailed{},
    kernel::UndoFileCloseFailed{},
};

//! The errors that reach BlockValidationState::Error(), whose reject reason is
//! surfaced by submitblock and submitheader.
const std::vector<kernel::FatalError> REJECT_REASON_ERRORS{
    kernel::BlockDisconnectFailed{},
    kernel::BlockReadFailed{},
    kernel::CorruptBlockFound{},
    kernel::DiskSpaceTooLow{},
    kernel::SystemErrorWhileFlushing{.what = "what"},
    kernel::SystemErrorWhileSavingBlock{.what = "what"},
    kernel::UndoDataWriteFailed{},
    kernel::UndoFileCloseFailed{},
};
} // namespace

BOOST_AUTO_TEST_SUITE(kernel_error_tests)

//! The kernel carries no user-facing text; the node must have a message for
//! every error it can be notified about.
BOOST_AUTO_TEST_CASE(error_message)
{
    for (const auto error : {kernel::FlushError::BLOCK_FILE_FLUSH_FAILED, kernel::FlushError::UNDO_FILE_FLUSH_FAILED}) {
        BOOST_CHECK(!node::FlushErrorMessage(error).original.empty());
    }
    for (const auto& error : ALL_FATAL_ERRORS) {
        BOOST_CHECK(!node::FatalErrorMessage(error).original.empty());
    }
}

//! Those reject reasons are the only text left in the kernel, and they have to
//! keep matching the node's wording.
BOOST_AUTO_TEST_CASE(reject_reason_matches_node_message)
{
    for (const auto& error : REJECT_REASON_ERRORS) {
        BOOST_CHECK_EQUAL(FatalErrorString(error), node::FatalErrorMessage(error).original);
    }
}

//! The only error whose fields the node expands beyond a plain substitution.
BOOST_AUTO_TEST_CASE(snapshot_validation_failed_message)
{
    const kernel::SnapshotValidationFailed error{.height_from = 200, .height_to = 100, .rename_error = "rename failed"};
    const std::string message{node::FatalErrorMessage(error).original};
    BOOST_CHECK_NE(message.find("from 200 to 100"), std::string::npos);
    BOOST_CHECK_NE(message.find("\nrename failed"), std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
