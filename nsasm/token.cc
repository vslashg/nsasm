#include "nsasm/token.h"

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "nsasm/mnemonic.h"

namespace nsasm {

namespace {

bool IsHexDigit(char ch) {
  return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch <= 'F');
}

bool IsDecimalDigit(char ch) { return (ch >= '0' && ch <= '9'); }

bool IsIdentifierFirstChar(char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

bool IsIdentifierChar(char ch) {
  return IsDecimalDigit(ch) || IsIdentifierFirstChar(ch);
}

}  // namespace

std::string Token::ToString() const {
  if (EndOfLine()) {
    return "end of line";
  }
  auto mnemonic = Mnemonic();
  if (mnemonic) {
    return absl::StrCat("mnemonic ", nsasm::ToString(*mnemonic));
  }
  auto suffix = Suffix();
  if (suffix) {
    return absl::StrCat("suffix ", nsasm::ToString(*suffix));
  }
  auto literal = Literal();
  if (literal) {
    return absl::StrCat("literal ", *literal);
  }
  auto identifier = Identifier();
  if (identifier) {
    return absl::StrCat("identifier ", *identifier);
  }
  auto punctuation = Punctuation();
  if (punctuation) {
    std::string spelling = nsasm::ToString(*punctuation);
    if (spelling.size() > 3) {
      // This is a keyword, not an operator
      return absl::StrCat("keyword `", spelling, "`");
    }
    if (spelling.size() == 1 && spelling[0] >= 'A' && spelling[0] <= 'Z') {
      // A letter represents a register
      return absl::StrCat("register ", spelling);
    }
    return absl::StrCat("symbol `", spelling, "`");
  }
  return "logic error?";
}

ErrorOr<std::vector<Token>> Tokenize(absl::string_view sv, Location loc) {
  std::vector<Token> result;
  while (true) {
    sv = absl::StripLeadingAsciiWhitespace(sv);
    if (sv.empty() || sv[0] == ';') {
      result.emplace_back(EndOfLine(), loc);
      return result;
    }
    int remain = sv.size();

    // punctuation
    if (sv.size() >= 3) {
      absl::string_view next = sv.substr(0, 3);
      Punctuation found = P_none;
      if (next == "+++") {
        found = P_plusplusplus;
      } else if (next == "---") {
        found = P_minusminusminus;
      }
      if (found != P_none) {
        sv.remove_prefix(3);
        result.emplace_back(found, loc);
        continue;
      }
    }
    if (sv.size() >= 2) {
      absl::string_view next = sv.substr(0, 2);
      Punctuation found = P_none;
      if (next == "::") {
        found = P_scope;
      } else if (next == "++") {
        found = P_plusplus;
      } else if (next == "--") {
        found = P_minusminus;
      }
      if (found != P_none) {
        sv.remove_prefix(2);
        result.emplace_back(found, loc);
        continue;
      }
    }
    char next = sv[0];
    if (next == '(' || next == ')' || next == '[' || next == ']' ||
        next == ',' || next == ':' || next == ',' || next == '#' ||
        next == '+' || next == '-' || next == '*' || next == '/' ||
        next == '@' || next == '{' || next == '}' || next == '>' ||
        next == '<' || next == '^') {
      sv.remove_prefix(1);
      result.emplace_back(next, loc);
      continue;
    }

    // hexdecimal literal
    bool hex_prefix = false;
    if (remain >= 2 && sv[0] == '$' && IsHexDigit(sv[1])) {
      hex_prefix = true;
      sv.remove_prefix(1);
    } else if (remain >= 3 && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X') &&
               IsHexDigit(sv[2])) {
      hex_prefix = true;
      sv.remove_prefix(2);
    }
    if (hex_prefix) {
      // Consume hex digits from the string.
      // This should be absl::from_chars<int> except nobody has written that
      // yet. Grumble, grumble...
      std::string hex_digits;
      while (!sv.empty() && IsHexDigit(sv[0])) {
        hex_digits.push_back(sv[0]);
        sv.remove_prefix(1);
      }
      int value = strtol(hex_digits.c_str(), nullptr, 16);
      // For hex constants, deduce the type from the number of characters.
      // ("$00" is a byte and "$0000" is a word, for example.)
      NumericType type = T_long;
      if (hex_digits.size() <= 2) {
        type = T_byte;
      } else if (hex_digits.size() <= 4) {
        type = T_word;
      }
      result.emplace_back(value, loc, type);
      continue;
    }

    // decimal literal
    if (IsDecimalDigit(sv[0])) {
      std::string decimal_digits;
      while (!sv.empty() && IsDecimalDigit(sv[0])) {
        decimal_digits.push_back(sv[0]);
        sv.remove_prefix(1);
      }
      int value = strtol(decimal_digits.c_str(), nullptr, 10);
      result.emplace_back(value, loc);
      continue;
    }

    // directives
    if (sv[0] == '.') {
      std::string identifier = ".";
      sv.remove_prefix(1);
      while (!sv.empty() && IsIdentifierChar(sv[0])) {
        identifier.push_back(sv[0]);
        sv.remove_prefix(1);
      }
      // directive?
      auto directive = ToDirectiveName(identifier);
      if (directive.has_value()) {
        result.emplace_back(*directive, loc);
        continue;
      }
      // suffix?
      auto suffix = ToSuffix(identifier);
      if (suffix.has_value()) {
        result.emplace_back(*suffix, loc);
        continue;
      }
      return Error("Unrecognized dotted name '%s' in input", identifier)
          .SetLocation(loc);
    }

    // identifiers and keywords
    if (IsIdentifierFirstChar(sv[0])) {
      std::string identifier;
      identifier.push_back(sv[0]);
      sv.remove_prefix(1);
      while (!sv.empty() && IsIdentifierChar(sv[0])) {
        identifier.push_back(sv[0]);
        sv.remove_prefix(1);
      }
      // mnemonic?
      auto mnemonic = ToMnemonic(identifier);
      if (mnemonic.has_value()) {
        result.emplace_back(*mnemonic, loc);
        continue;
      }
      // register name?
      if (identifier.size() == 1) {
        char next = absl::ascii_toupper(identifier[0]);
        if (next == 'A' || next == 'S' || next == 'X' || next == 'Y') {
          result.emplace_back(next, loc);
          continue;
        }
      }
      // keyword?
      if (absl::AsciiStrToLower(identifier) == "export") {
        result.emplace_back(P_export, loc);
        continue;
      } else if (absl::AsciiStrToLower(identifier) == "noreturn") {
        result.emplace_back(P_noreturn, loc);
        continue;
      } else if (absl::AsciiStrToLower(identifier) == "yields") {
        result.emplace_back(P_yields, loc);
        continue;
      }
      // Not a reserved word, so it's an identifier
      result.emplace_back(identifier, loc);
      continue;
    }

    // none of the above
    return Error("Unexpected character '%c' in input", sv[0]).SetLocation(loc);
  }
}

std::string ToString(Punctuation p) {
  switch (p) {
    case P_scope:
      return "::";
    case P_export:
      return "export";
    case P_noreturn:
      return "noreturn";
    case P_yields:
      return "yields";
    case P_plusplus:
      return "++";
    case P_plusplusplus:
      return "+++";
    case P_minusminus:
      return "--";
    case P_minusminusminus:
      return "---";
    default:
      return absl::StrFormat("%c", p);
  }
}

}  // namespace nsasm
