Indexes
-------

- The implementation of coinstatsindex was changed to prevent an overflow bug that could already be observed on the default Signet. The new version of the index will need to be synced from scratch when starting the upgraded node for the first time. The new version is stored in `/indexes/coinstatsindex/` in contrast to the old version which was stored at `/indexes/coinstats/`. The old version of the index is not deleted by the upgraded node in case the user chooses to downgrade their node in the future. If the user does not plan to downgrade it is safe for them to remove `/indexes/coinstats/` from their datadir. A future release of Bitcoin Core may remove the old version of the index automatically.
