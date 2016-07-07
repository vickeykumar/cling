
#include "cling/Interpreter/ClingTabCompletion.h"

#include "cling/Interpreter/Interpreter.h"
#include "clang/Sema/Sema.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/CompilerInstance.h"
#include "cling/Interpreter/InterpreterCallbacks.h"

namespace cling {

  bool ClingTabCompletion::Complete(const std::string& Line /*in+out*/,
                size_t& Cursor /*in+out*/,
                std::vector<std::string>& DisplayCompletions /*out*/) {
    //Get the results
    const char * const argV = "cling";
    cling::Interpreter CodeCompletionInterp(ParentInterp, 1, &argV);

    // Create the CodeCompleteConsumer with InterpreterCallbacks
    // from the parent interpreter and set the consumer for the child
    // interpreter
    // Yuck! But I need the const/non-const to be fixed somehow.
    
    const InterpreterCallbacks* callbacks = ParentInterp.getCallbacks();
    callbacks->CreateCodeCompleteConsumer(&CodeCompletionInterp); 

    clang::CompilerInstance* codeCompletionCI = CodeCompletionInterp.getCI();
    clang::Sema& codeCompletionSemaRef = codeCompletionCI->getSema();
    // Ignore diagnostics when we tab complete
    clang::IgnoringDiagConsumer* ignoringDiagConsumer = new clang::IgnoringDiagConsumer();
    codeCompletionSemaRef.getDiagnostics().setClient(ignoringDiagConsumer, true);

    auto Owner = ParentInterp.getCI()->getSema().getDiagnostics().takeClient();
    auto Client = ParentInterp.getCI()->getSema().getDiagnostics().getClient();
    ParentInterp.getCI()->getSema().getDiagnostics().setClient(ignoringDiagConsumer, false);
    CodeCompletionInterp.codeComplete(Line, Cursor);
  
    callbacks->GetCompletionResults(&CodeCompletionInterp, DisplayCompletions);
    // Restore the original diag client for parent interpreter
    ParentInterp.getCI()->getSema().getDiagnostics().setClient(Client, Owner.release() != nullptr);
    // FIX-ME : Change it in the Incremental Parser
    // It does not work even if I call unload in IncrementalParser, I think
    // it would be to early.
    CodeCompletionInterp.unload(1);
    
  return true;
  }
}
