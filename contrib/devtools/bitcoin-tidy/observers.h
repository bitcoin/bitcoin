#pragma once

#include "observers-base.h"

class TxObservers : public ObserversBase
{
public:
  TxObservers(
    clang::StringRef CheckName, clang::tidy::ClangTidyContext* Context);
};
