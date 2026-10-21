// [Experimental] Autosave: a stub until the feature is written (docs/experimental-build.md).
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD
void exp_autosave_init() {}
void exp_autosave_frame() {}
void exp_autosave_level_loaded(bool ok) { (void)ok; }
#endif
