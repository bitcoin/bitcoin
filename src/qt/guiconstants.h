#ifndef GUICONSTANTS_H
#define GUICONSTANTS_H

/* Milliseconds between model updates */
static const int MODEL_UPDATE_DELAY = 500;

/* Maximum  passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 16;

/* Invalid field background style */
#define STYLE_INVALID "background:#FF8080"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(128, 128, 128)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(255, 0, 0)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(140, 140, 140)

/* Colors for minting tab for each coin age group */
#define COLOR_MINT_YOUNG QColor(127, 127, 240)
#define COLOR_MINT_MATURE QColor(127, 240, 127)
#define COLOR_MINT_OLD QColor(240, 127, 127)


#endif // GUICONSTANTS_H
