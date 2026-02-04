#include "SimpleCompEditor.h"

#define LOCTEXT_NAMESPACE "FSimpleCompEditorModule"

void FSimpleCompEditorModule::StartupModule() {
  // This code will execute after your module is loaded into memory
}

void FSimpleCompEditorModule::ShutdownModule() {
  // This function may be called during shutdown to clean up your module
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSimpleCompEditorModule, SimpleCompEditor)
