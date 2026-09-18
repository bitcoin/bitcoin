// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/kernel_notifications.h>

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <chain.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/system.h>
#include <kernel/context.h>
#include <kernel/error.h>
#include <kernel/warning.h>
#include <node/abort.h>
#include <node/interface_ui.h>
#include <node/warnings.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/log.h>
#include <util/overloaded.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <thread>
#include <variant>

using util::ReplaceAll;

static void AlertNotify(const std::string& strMessage)
{
#if HAVE_SYSTEM
    std::string strCmd = gArgs.GetArg("-alertnotify", "");
    if (strCmd.empty()) return;

    // Alert text should be plain ascii coming from a trusted source, but to
    // be safe we first strip anything not in safeChars, then add single quotes around
    // the whole string before passing it to the shell:
    std::string singleQuote("'");
    std::string safeStatus = SanitizeString(strMessage);
    safeStatus = singleQuote+safeStatus+singleQuote;
    ReplaceAll(strCmd, "%s", safeStatus);

    std::thread t(runCommand, strCmd);
    t.detach(); // thread runs free
#endif
}

namespace node {

kernel::InterruptResult KernelNotifications::blockTip(SynchronizationState state, const CBlockIndex& index, double verification_progress)
{
    {
        LOCK(m_tip_block_mutex);
        Assume(index.GetBlockHash() != uint256::ZERO);
        m_state.tip_block = index.GetBlockHash();
        m_tip_block_cv.notify_all();
    }

    uiInterface.NotifyBlockTip(state, index, verification_progress);
    if (m_stop_at_height && index.nHeight >= m_stop_at_height) {
        if (!m_shutdown_request()) {
            LogError("Failed to send shutdown signal after reaching stop height\n");
        }
        return kernel::Interrupted{};
    }
    return {};
}

void KernelNotifications::headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync)
{
    uiInterface.NotifyHeaderTip(state, height, timestamp, presync);
}

void KernelNotifications::progress(const bilingual_str& title, int progress_percent, bool resume_possible)
{
    uiInterface.ShowProgress(title.translated, progress_percent, resume_possible);
}

void KernelNotifications::warningSet(kernel::Warning id, const bilingual_str& message)
{
    if (m_warnings.Set(id, message)) {
        AlertNotify(message.original);
    }
}

void KernelNotifications::warningUnset(kernel::Warning id)
{
    m_warnings.Unset(id);
}

// The kernel does not translate its errors; this is where translation happens.
bilingual_str FlushErrorMessage(kernel::FlushError error)
{
    switch (error) {
    case kernel::FlushError::BLOCK_FILE_FLUSH_FAILED:
        return _("Flushing block file to disk failed. This is likely the result of an I/O error.");
    case kernel::FlushError::UNDO_FILE_FLUSH_FAILED:
        return _("Flushing undo file to disk failed. This is likely the result of an I/O error.");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

void KernelNotifications::flushError(kernel::FlushError error)
{
    AbortNode(m_shutdown_request, m_exit_status, FlushErrorMessage(error), &m_warnings);
}

bilingual_str FatalErrorMessage(const kernel::FatalError& error)
{
    return std::visit(util::Overloaded{
        [](const kernel::ActivateBestChainsFailed& e) { return Untranslated(e.error); },
        [](const kernel::AssumeutxoDataNotFound& e) { return strprintf(_("Assumeutxo data not found for the given blockhash '%s'."), e.blockhash.ToString()); },
        [](const kernel::BlockDisconnectFailed&) -> bilingual_str { return _("Failed to disconnect block."); },
        [](const kernel::BlockFileCloseFailed&) -> bilingual_str { return _("Failed to close file when writing block."); },
        [](const kernel::BlockReadFailed&) -> bilingual_str { return _("Failed to read block."); },
        [](const kernel::BlockWriteFailed&) -> bilingual_str { return _("Failed to write block."); },
        [](const kernel::CorruptBlockFound&) -> bilingual_str { return _("Corrupt block found indicating potential hardware failure."); },
        [](const kernel::DiskSpaceTooLow&) -> bilingual_str { return _("Disk space is too low!"); },
        [](const kernel::FailedToStartIndexes&) -> bilingual_str { return _("Failed to start indexes, shutting down…"); },
        [](const kernel::SnapshotChainstateDirRemovalFailed& e) {
            return strprintf(_("Failed to remove snapshot chainstate dir (%s). "
                               "Manually remove it before restarting.\n"), fs::PathToString(e.dir));
        },
        [](const kernel::SnapshotChainstateRenameFailed& e) {
            return strprintf(_("Rename of '%s' -> '%s' failed. "
                               "Cannot clean up the background chainstate leveldb directory."),
                             fs::PathToString(e.old_path), fs::PathToString(e.new_path));
        },
        [](const kernel::SnapshotValidationFailed& e) {
            bilingual_str message = strprintf(_(
                "%s failed to validate the -assumeutxo snapshot state. "
                "This indicates a hardware problem, or a bug in the software, or a "
                "bad software modification that allowed an invalid snapshot to be "
                "loaded. As a result of this, the node will shut down and stop using any "
                "state that was built on the snapshot, resetting the chain height "
                "from %d to %d. On the next "
                "restart, the node will resume syncing from %d "
                "without using any snapshot data. "
                "Please report this incident to %s, including how you obtained the snapshot. "
                "The invalid snapshot chainstate will be left on disk in case it is "
                "helpful in diagnosing the issue that caused this error."),
                CLIENT_NAME, e.height_from, e.height_to, e.height_to, CLIENT_BUGREPORT);
            if (e.rename_error) message += Untranslated("\n" + *e.rename_error);
            return message;
        },
        [](const kernel::SystemErrorWhileFlushing& e) { return strprintf(_("System error while flushing: %s"), e.what); },
        [](const kernel::SystemErrorWhileLoadingExternalBlockFile& e) { return strprintf(_("System error while loading external block file: %s"), e.what); },
        [](const kernel::SystemErrorWhileSavingBlock& e) { return strprintf(_("System error while saving block to disk: %s"), e.what); },
        [](const kernel::UndoDataWriteFailed&) -> bilingual_str { return _("Failed to write undo data."); },
        [](const kernel::UndoFileCloseFailed&) -> bilingual_str { return _("Failed to close block undo file."); },
    }, error);
}

void KernelNotifications::fatalError(const kernel::FatalError& error)
{
    node::AbortNode(m_shutdown_on_fatal_error ? m_shutdown_request : nullptr,
                    m_exit_status, FatalErrorMessage(error), &m_warnings);
}

std::optional<uint256> KernelNotifications::TipBlock()
{
    AssertLockHeld(m_tip_block_mutex);
    return m_state.tip_block;
};


void ReadNotificationArgs(const ArgsManager& args, KernelNotifications& notifications)
{
    if (auto value{args.GetIntArg("-stopatheight")}) notifications.m_stop_at_height = *value;
}

} // namespace node
