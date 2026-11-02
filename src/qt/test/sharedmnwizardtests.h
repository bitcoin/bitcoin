// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TEST_SHAREDMNWIZARDTESTS_H
#define BITCOIN_QT_TEST_SHAREDMNWIZARDTESTS_H

#include <QObject>

namespace interfaces {
class Node;
} // namespace interfaces

//! Page-level tests for SharedMnCreateDialog: the order the pages come in, how
//! a pasted message decides which page the user lands on, and what the status
//! board says after replies are absorbed. They drive the dialog through the
//! same seams the walkthrough screenshots use - goToPage(), currentPage(),
//! handleImportedText() and copySession(). The cases about what the wallet can
//! sign for, or must refuse to sign, build a wallet; the rest need none.
class SharedMnWizardTests : public QObject
{
    Q_OBJECT

public:
    explicit SharedMnWizardTests(interfaces::Node& node) : m_node(node) {}

private Q_SLOTS:
    void coordinatorPageOrder();
    void participantPagesAndRoleInference();
    void participantCannotChooseATakenShare();
    void coordinatorResumesSavedSession();
    void coordinatorResumesFullySignedSession();
    void unauthorisedSignedInputsAreDetected();
    void refusesToSignCoinsOutsideOwnContribution();
    void refusesToSignShortChangedOwnContribution();
    void savingWaitsForTheOperatorKeyBackup();
    void coordinatorCanRetryOwnApproval();
    void participantsPageGating();
    void statusBoardAbsorbsRepliesInAnyOrder();
    void pastedMessageRouting();
    void copyPutsFingerprintedEnvelopeOnClipboard();
    void unlockingDiscardsApprovals();

private:
    interfaces::Node& m_node;
};

#endif // BITCOIN_QT_TEST_SHAREDMNWIZARDTESTS_H
