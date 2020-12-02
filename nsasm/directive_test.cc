#include "nsasm/directive.h"

#include "absl/strings/ascii.h"
#include "gtest/gtest.h"

namespace nsasm {
namespace {

TEST(Directive, directive_types) {
  // Directives that take no arguments
  for (DirectiveName name : {D_begin, D_end, D_halt}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_no_arg);
  }

  // Directives that take single values
  for (DirectiveName name : {D_equ}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_single_arg);
  }

  // Directives that take constant values
  for (DirectiveName name : {D_org}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_constant_arg);
  }

  // Directives that take a flag state name
  for (DirectiveName name : {D_mode}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_flag_arg);
  }

  // Directives that take one or two flag state names
  for (DirectiveName name : {D_entry}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_calling_convention_arg);
  }

  // Directives that take lists of values
  for (DirectiveName name : {D_db, D_dl, D_dw}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_list_arg);
  }

  // Directives that take a simple name
  for (DirectiveName name : {D_module}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_name_arg);
  }

  // Bespoke syntax for .remote
  for (DirectiveName name : {D_remote}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_remote_arg);
  }
}

}  // namespace
}  // namespace nsasm
