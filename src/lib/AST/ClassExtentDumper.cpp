#include "clang/AST/ClassExtentDumper.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/LLVM.h"
#include "llvm/Support/Path.h"
#include "llvm/ADT/SmallVector.h"

using namespace clang;

// Constructor
ClassExtentDumper::ClassExtentDumper(CompilerInstance &CI, llvm::raw_ostream &OS)
    : CI(CI), OS(OS) {}

// Entry point
void ClassExtentDumper::HandleTranslationUnit(ASTContext &Context) {
  llvm::errs() << "======================== Class Extent Report ========================\n\n";
  for (auto D : Context.getTranslationUnitDecl()->decls()) {
    dumpIfClassLike(D);
  }
  llvm::errs() << "=====================================================================\n";
}

// Recursively visit decls and dump class/struct/union details
void ClassExtentDumper::dumpIfClassLike(const Decl *D) {
  if (const auto *RD = dyn_cast<CXXRecordDecl>(D)) {
    if (!RD->isCompleteDefinition() || RD->isImplicit())
      return;

    SourceManager &SM = CI.getSourceManager();
    SourceLocation BeginLoc = RD->getBeginLoc();
    SourceLocation EndLoc = RD->getEndLoc();

    if (BeginLoc.isInvalid() || EndLoc.isInvalid() || !SM.isInMainFile(BeginLoc))
      return;

    PresumedLoc BeginPresumed = SM.getPresumedLoc(BeginLoc);
    PresumedLoc EndPresumed = SM.getPresumedLoc(EndLoc);
    if (BeginPresumed.isInvalid() || EndPresumed.isInvalid())
      return;

    std::string Kind = RD->isUnion() ? "Union" :
                       RD->isStruct() ? "Struct" :
                       RD->getDescribedClassTemplate() ? "Template Class" :
                                                         "Class";

    std::string QualifiedName = RD->getQualifiedNameAsString();

    llvm::errs() << "[+] " << Kind << ":" << std::string(15 - Kind.length(), ' ')
                 << QualifiedName << "\n"
                 << "    File:            " << BeginPresumed.getFilename() << "\n"
                 << "    Line Range:      "
                 << BeginPresumed.getLine() << "-"
                 << EndPresumed.getLine() << "\n\n";
  }

  if (const auto *DC = dyn_cast<DeclContext>(D)) {
    for (auto *SubDecl : DC->decls())
      dumpIfClassLike(SubDecl);
  }
}

// Visit C-style records
bool ClassExtentDumper::VisitRecordDecl(RecordDecl *D) {
  if (D->isThisDeclarationADefinition() && !isa<CXXRecordDecl>(D))
    dumpClassExtent(D);
  return true;
}

// Core logic for dumping extent of a RecordDecl
void ClassExtentDumper::dumpClassExtent(const RecordDecl *D) {
  SourceManager &SM = CI.getSourceManager();
  SourceLocation B = D->getBeginLoc();
  SourceLocation E = D->getEndLoc();

  if (B.isInvalid() || E.isInvalid() || SM.isInSystemHeader(B))
    return;

  unsigned StartLine = SM.getSpellingLineNumber(B);
  unsigned EndLine   = SM.getSpellingLineNumber(E);
  std::string File   = getFileName(B);
  std::string Name   = getFullyQualifiedName(D);

  OS << Name << ":" << File << ":" 
     << StartLine << "-" << EndLine << "\n";
}

// Resolve full qualified name
std::string ClassExtentDumper::getFullyQualifiedName(const RecordDecl *D) {
  SmallVector<const DeclContext *, 8> Contexts;
  const DeclContext *Ctx = D->getDeclContext();
  while (Ctx && !Ctx->isTranslationUnit()) {
    if (auto *ND = dyn_cast<NamedDecl>(Ctx))
      Contexts.push_back(Ctx);
    Ctx = Ctx->getParent();
  }

  std::string Qualified;
  for (auto It = Contexts.rbegin(), End = Contexts.rend(); It != End; ++It) {
    if (auto *ND = dyn_cast<NamedDecl>(*It)) {
      if (!Qualified.empty())
        Qualified += "::";
      if (auto *NS = dyn_cast<NamespaceDecl>(ND)) {
        Qualified += NS->isAnonymousNamespace()
                     ? "<anonymous namespace>"
                     : NS->getNameAsString();
      } else {
        Qualified += ND->getNameAsString();
      }
    }
  }

  if (!Qualified.empty())
    Qualified += "::";

  if (D->getIdentifier())
    Qualified += D->getNameAsString();
  else if (auto *TD = D->getTypedefNameForAnonDecl())
    Qualified += TD->getNameAsString();
  else
    Qualified += "<anonymous>";

  return Qualified;
}

// Get source file name from a location
std::string ClassExtentDumper::getFileName(SourceLocation Loc) {
  SourceManager &SM = CI.getSourceManager();
  PresumedLoc P = SM.getPresumedLoc(Loc);
  if (P.isValid())
    return std::string(P.getFilename());
  return "<unknown>";
}
