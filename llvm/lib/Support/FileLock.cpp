//===--- FileLock.cpp - File-level Locking Utility--------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/FileLock.h"
#include "llvm/Support/FileSystem.h"
#include <system_error>

using namespace llvm::sys::fs;

namespace llvm {

FileLock::FileLock(const StringRef& FileName)
  : FileName(FileName)
{

  // NB: it's critical that no one concurrently deletes the lock file.
  int OpenedFD;
  if (std::error_code EC = openFileForReadWrite(this->FileName + ".lock",
                                                OpenedFD,
                                                CreationDisposition::CD_OpenAlways,
                                                OpenFlags::OF_None))
    report_fatal_error("Failed to create or open " + this->FileName + ".lock");
  this->FD = OpenedFD;

}

FileLock::~FileLock() {
  if (this->FD)
  {
    closeFile(*this->FD);
  }
}

bool FileLock::TryLock() {
  std::error_code lockEC = tryLockFile(*FD);
  if (lockEC && lockEC != std::errc::no_lock_available)
    report_fatal_error("Unexpected failure attempting to lock " + FileName + ".lock");

  return !lockEC;
}

void FileLock::Lock() {

  // tryLock with a timeout will keep trying the lock at 1ms intervals until the timeout. We usually have the
  // CPU to ourselves anyway. Err on the side of unreasably high timeouts - humans are expected to get annoyed
  // and kill clang, it's more for psyduck.
  //
  std::error_code lockEC = tryLockFile(*FD, std::chrono::minutes(10));
  if (lockEC && lockEC != std::errc::no_lock_available)
    report_fatal_error("Unexpected failure attempting to lock " + FileName + ".lock");
  if (lockEC == std::errc::no_lock_available)
    report_fatal_error("Timed out after waiting 10 minutes for " + FileName + ".lock. "
                       "This is probably deadlock due to circular module/header dependency.");
}

}
