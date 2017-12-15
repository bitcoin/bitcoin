#ifndef OMNICORE_PERSISTENCE_H
#define OMNICORE_PERSISTENCE_H

#include <boost/filesystem.hpp>

class CBlockIndex;

/** Indicates whether persistence is enabled and the state is stored. */
bool IsPersistenceEnabled(int blockHeight);

/** Stores the in-memory state in files. */
int PersistInMemoryState(const CBlockIndex* pBlockIndex);

/** Loads and retrieves state from a file. */
int RestoreInMemoryState(const std::string& filename, int what, bool verifyHash = false);

/** Loads and restores the latest state. Returns -1 if reparse is required. */
int LoadMostRelevantInMemoryState();


#endif // OMNICORE_PERSISTENCE_H
