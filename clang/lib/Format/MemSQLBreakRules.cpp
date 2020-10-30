#include "TokenAnnotator.h"
#include "FormatToken.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/Debug.h"

namespace memsql {

using namespace clang;
using namespace clang::format;

bool mustBreakBefore(const AnnotatedLine &Line, const FormatToken &Right) {
  FormatToken *Left = Right.Previous;

  if (Left->is(clang::format::TT_CtorInitializerColon)) {
    // Go back to scan parameter list on Ctor to make wrap decision.
    bool ShouldWrapParam = false;
    for (FormatToken *Search = Line.First; Search != Left;
         Search = Search->Next) {
      if (Search->is(tok::l_paren)) {
        ShouldWrapParam = Search->ParameterCount > 1;
      }
      if (Search->is(tok::comma)) {
        Search->Next->MustBreakBefore = ShouldWrapParam;
      }
    }
    Right.Previous->MustBreakBefore = false;
    return true;
  }

  if (Left->is(TT_CtorInitializerComma)) {
    Left->Previous->MustBreakBefore = false;
    return true;
  }

  if (Left->is(TT_TemplateCloser)) {
    Left->MustBreakBefore = false;
    // Don't return here since we dont know if we want to break before Right
    // token yet.
  }

  // We do not want to break ":" inside a for stmt for(e:list).
  // but the way to avoid it is very hacky.
  // Since in ContinuationIndenter::mustBreak() it will enforce a line break
  // for TT_InlineASMColon type. So I cheat the type.
  if (Right.is(TT_InlineASMColon)) {
    Left->Next->Type = TT_Unknown;
    return false;
  }

  if (Left->is(tok::l_brace) && Left->BlockKind == BK_Block &&
      Left->Children.size() > 0) {
    return true;
  }

  if (Right.is(tok::kw_while) && Left->is(tok::r_brace)) {
    return true;
  }

  if (Right.is(tok::l_brace)) {
    // Always insert newline between ') {' or if "{" is a lambda brace.
    if (Left->is(tok::r_paren) || Right.is(TT_LambdaLBrace)) {
      return true;
    }

    // Look ahead to see if it is a brace initializer:
    // https://en.cppreference.com/w/cpp/language/aggregate_initialization
    // T object = {arg1, arg2, ...};	(1)
    // T object {arg1, arg2, ... };	(2)	(since C++11)
    if (Right.BlockKind != BK_Block) { // Either BK_Unknown or BK_BracedInit
      if (Right.ParameterCount > 2) {
        Right.Next->MustBreakBefore = true;
        for (FormatToken *Search = Right.Next;
             Search && Search != Right.MatchingParen; Search = Search->Next) {
          if (Search->is(tok::comma)) {
            Search->Next->MustBreakBefore = true;
          }
        }
        Right.MatchingParen->MustBreakBefore = true;
        return true;
      }
    }
  }

  if (Line.First->TokenText.equals("Auto") && Left->is(tok::semi) &&
      Line.Level == 0) {
    return true;
  }

  if (Left->is(tok::l_paren) && Line.MustBeDeclaration &&
      Line.MightBeFunctionDecl) {
    FormatToken *FuncName = nullptr;
    for (FuncName = Left; FuncName; FuncName = FuncName->Previous) {
      if (FuncName && FuncName->Type == TT_FunctionDeclarationName) {
        FuncName->MustBreakBefore = false;
        for (FormatToken *Search = Left;
             Search && Search != Left->MatchingParen; Search = Search->Next) {
          if (Search->is(tok::r_paren) && Search != Left->MatchingParen) {
            Search->Next->MustBreakBefore = true;
          }
        }
        if (Left->ParameterCount > 1) {
          FuncName->MustBreakBefore = true;
          return true;
        }
        break;
      }
    }
  }

  return false;
}
} // namespace memsql
