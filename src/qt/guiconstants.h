#ifndef GUICONSTANTS_H
#define GUICONSTANTS_H

/* Milliseconds between model updates */
static const int MODEL_UPDATE_DELAY = 250;

/* AskPassphraseDialog -- Maximum passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* BitcoinGUI -- Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 16;

/* Invalid field background style */
#define STYLE_INVALID "background:#FF8080"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(140, 140, 140)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(255, 58, 66)
/* Table List -- negative amount */
#define COLOR_NEGATIVE_TABLE QColor(255, 58, 66)
/* Transaction list -- positive amount */
#define COLOR_POSITIVE QColor(60, 176, 84)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(171, 171, 171)

/* Tooltips longer than this (in characters) are converted into rich text,
   so that they can be word-wrapped.
 */
static const int TOOLTIP_WRAP_THRESHOLD = 80;

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* QRCodeDialog -- size of exported QR Code image */
#define EXPORT_IMAGE_SIZE 256

/* Colors for minting tab for each coin age group */
#define COLOR_MINT_YOUNG QColor(255, 224, 226)
#define COLOR_MINT_MATURE QColor(204, 255, 207)
#define COLOR_MINT_OLD QColor(111, 252, 141)

#endif // GUICONSTANTS_H
