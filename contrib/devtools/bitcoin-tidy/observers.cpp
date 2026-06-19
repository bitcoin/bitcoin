#include "observers.h"

TxObservers::TxObservers(
  clang::StringRef CheckName, clang::tidy::ClangTidyContext* Context)
  : ObserversBase(CheckName, Context)
{
  ClassName = "CTransaction";
  MemberToAccessor["vin"] = "GetInputs()";
  MemberToAccessor["vout"] = "GetOutputs()";
  MemberToAccessor["version"] = "GetVersion()";
  MemberToAccessor["nLockTime"] = "GetLockTime()";
}
