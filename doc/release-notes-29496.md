Mempool Policy Changes
----------------------

- Transactions with version number set to 3 are now treated as standard on all networks (#29496),
  subject to Opt-in Topologically Restricted Until Confirmation (TRUC) Transactions policy as
  described in [BIP 431](https://github.com/bitcoin/bips/blob/master/bip-0431.mediawiki).  The
  policy includes limits on spending unconfirmed outputs (#28948), eviction of a previous descendant
  if a more incentive-compatible one is submitted (#29306), and a maximum transaction size of 10,000vB
  (#29873). These restrictions simplify the assessment of incentive compatibility of accepting or
  replacing TRUC transactions, thus ensuring any replacements are more profitable for the node and
  making fee-bumping more reliable.
