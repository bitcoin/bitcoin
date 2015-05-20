#include "omnicore_init.h"

#include "chainparams.h"
#include "util.h"

#include <string>

#include <QMessageBox>
#include <QSettings>

namespace OmniCore {

/**
 * Creates a message box with general warnings and potential risks
 *
 * @return True if the user acknowledged the warnings
 */
static bool getDisclaimerDialogResult()
{
    QString qstrTitle("WARNING - Experimental Software");

    QString qstrText(
    "This software is EXPERIMENTAL software. USE ON MAINNET AT YOUR OWN RISK.");

    QString qstrInformativeText(
    "By default this software will use your existing Bitcoin wallet, including spending "
    "bitcoins contained therein (for example for transaction fees or trading).\n\n"
    "The protocol and transaction processing rules for the Omni Layer are still under "
    "active development and are subject to change in future.\n\n"
    "Omni Core should be considered an alpha-level product, and you use it at your "
    "own risk. Neither the Omni Foundation nor the Omni Core developers assumes "
    "any responsibility for funds misplaced, mishandled, lost, or misallocated.\n\n"
    "Further, please note that this installation of Omni Core should be viewed as "
    "EXPERIMENTAL. Your wallet data, bitcoins and Omni Layer tokens may be lost, "
    "deleted, or corrupted, with or without warning due to bugs or glitches. "
    "Please take caution.\n\n"
    "This software is provided open-source at no cost. You are responsible for "
    "knowing the law in your country and determining if your use of this software "
    "contravenes any local laws.\n\n"
    "PLEASE DO NOT use wallet(s) with significant amounts of bitcoins or Omni Layer "
    "tokens while testing!");

    QMessageBox msgBoxDisclaimer;
    msgBoxDisclaimer.setIcon(QMessageBox::Warning);
    msgBoxDisclaimer.setWindowTitle(qstrTitle);
    msgBoxDisclaimer.setText(qstrText);
    msgBoxDisclaimer.setInformativeText(qstrInformativeText);
    msgBoxDisclaimer.setStandardButtons(QMessageBox::Abort|QMessageBox::Ok);
    msgBoxDisclaimer.setDefaultButton(QMessageBox::Ok);
    msgBoxDisclaimer.setButtonText(QMessageBox::Abort, "&Exit");
    msgBoxDisclaimer.setButtonText(QMessageBox::Ok, "Acknowledge + &Continue");

    return msgBoxDisclaimer.exec() == QMessageBox::Ok;
}

/**
 * Determines if a disclaimer about the use of this software should be shown
 *
 * @see AskUserToAcknowledgeRisks()
 * @return True if the dialog should be shown
 */
static bool showDisclaimer()
{
    // Don't skip, if disclaimer was requested explicitly
    if (GetBoolArg("-disclaimer", false))
        return true;

    // Skip when starting minimized
    if (GetBoolArg("-min", false))
        return false;

    // Only mainnet users face the risk of monetary loss
    if (Params().NetworkIDString() != "main")
        return false;

    QSettings settings;

    // Check whether the disclaimer was acknowledged before
    return settings.value("OmniCore/fShowDisclaimer", true).toBool();
}

/**
 * Shows an user dialog with general warnings and potential risks
 *
 * The user is asked to acknowledge the risks related to the use of
 * this software. The decision of the user is stored and can be used
 * to skip the dialog during initialization.
 *
 * @see getDisclaimerDialogResult()
 * @see Initialize()
 * @return True if the user acknowledged the warnings
 */
bool AskUserToAcknowledgeRisks()
{
    QSettings settings;

    // Show message box
    bool fAccepted = getDisclaimerDialogResult();

    // Store decision and show next time, if declined
    settings.setValue("OmniCore/fShowDisclaimer", !fAccepted);

    return fAccepted;
}

/**
 * Setup and initialization related to Omni Core Qt
 *
 * @return True if the initialization was successful
 */
bool Initialize()
{
    if (showDisclaimer())
        return AskUserToAcknowledgeRisks();

    return true;
}

} // namespace OmniCore
