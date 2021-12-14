//===--- FileLock.h - File-level locking utility ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_SUPPORT_LOCKFILEMANAGER_H
#define LLVM_SUPPORT_LOCKFILEMANAGER_H

#include "llvm/Support/FileSystem.h"

namespace llvm {
  
class FileLock {
private:
  Optional<int> FD;
  StringRef FileName;

  FileLock(const FileLock&) = delete;

public:

  FileLock(const StringRef& FileName);
  ~FileLock();

  bool TryLock();
  void Lock();
};

} // end namespace llvm

#endif // LLVM_SUPPORT_LOCKFILEMANAGER_H
