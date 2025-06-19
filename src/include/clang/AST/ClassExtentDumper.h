//===--- ClassExtentDumper.h - Dump class extents ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
//
// This file declares ClassExtentDumper, which visits each RecordDecl
// and prints its fully qualified name, source file path, and line range.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_CLASSEXTENTDUMPER_H
#define LLVM_CLANG_AST_CLASSEXTENTDUMPER_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

namespace clang {

class ClassExtentDumper 
    : public ASTConsumer,
      public RecursiveASTVisitor<ClassExtentDumper> {
  CompilerInstance &CI;
  llvm::raw_ostream &OS;

public:
  explicit ClassExtentDumper(CompilerInstance &CI, llvm::raw_ostream &OS);

  void HandleTranslationUnit(ASTContext &Context) override;

  // Visit C-style structs/unions (non-CXXRecordDecl)
  bool VisitRecordDecl(RecordDecl *D);

private:
  void dumpIfClassLike(const Decl *D);
  void dumpClassExtent(const RecordDecl *D);
  std::string getFullyQualifiedName(const RecordDecl *D);
  std::string getFileName(SourceLocation Loc);
};

} // namespace clang

#endif // LLVM_CLANG_AST_CLASSEXTENTDUMPER_H
