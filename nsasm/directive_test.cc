#include "nsasm/directive.h"

#include "absl/strings/ascii.h"
#include "gtest/gtest.h"

namespace nsasm {
namespace {

// Helper to make conversion tests; intended to be invoked by macro.
void CheckToString(DirectiveName d, std::string s) {
  SCOPED_TRACE(s);
  std::string upper = s;
  absl::AsciiStrToUpper(&upper);
  EXPECT_EQ(ToString(d), upper);
  EXPECT_TRUE(ToDirectiveName(upper).has_value());
  EXPECT_EQ(*ToDirectiveName(upper), d);

  // lowercase strings should convert to mnemonics as well
  EXPECT_TRUE(ToDirectiveName(s).has_value());
  EXPECT_EQ(*ToDirectiveName(s), d);
}

#define CHECK_DIRECTIVE_NAME(name) CheckToString(D_##name, "." #name)

TEST(Directive, directive_names) {
  CHECK_DIRECTIVE_NAME(begin);
  CHECK_DIRECTIVE_NAME(db);
  CHECK_DIRECTIVE_NAME(dw);
  CHECK_DIRECTIVE_NAME(dl);
  CHECK_DIRECTIVE_NAME(end);
  CHECK_DIRECTIVE_NAME(entry);
  CHECK_DIRECTIVE_NAME(equ);
  CHECK_DIRECTIVE_NAME(mode);
  CHECK_DIRECTIVE_NAME(module);
  CHECK_DIRECTIVE_NAME(org);

  // Invalid strings should not be converted
  EXPECT_FALSE(ToDirectiveName("").has_value());
  EXPECT_FALSE(ToDirectiveName(".HCF").has_value());
}

TEST(Directive, directive_types) {
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

  // Directives that take lists of values
  for (DirectiveName name : {D_db, D_dl, D_dw}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_list_arg);
  }

  // Directives that take a flag state name
  for (DirectiveName name : {D_mode}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_flag_arg);
  }

  // Directives that take one or two flag state names
  for (DirectiveName name : {D_entry}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_flag_or_flags_arg);
  }

  // Directives that take a simple name
  for (DirectiveName name : {D_module}) {
    SCOPED_TRACE(ToString(name));
    EXPECT_EQ(DirectiveTypeByName(name), DT_name_arg);
  }
}

}  // namespace
}  // namespace nsasm
