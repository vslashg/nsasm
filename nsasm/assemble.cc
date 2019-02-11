#include "nsasm/assemble.h"

#include <tuple>

#include "nsasm/expression.h"
#include "nsasm/opcode_map.h"

namespace nsasm {
namespace {

// expr   -> term +- term +- term...
// term   -> factor */ factor */ factor...
// factor -> comp | -factor
// comp   -> literal | identifier | (expr)

ErrorOr<ExpressionOrNull> Expr(TokenSpan* pos);
ErrorOr<ExpressionOrNull> Term(TokenSpan* pos);
ErrorOr<ExpressionOrNull> Factor(TokenSpan* pos);
ErrorOr<ExpressionOrNull> Comp(TokenSpan* pos);

bool AtEnd(const TokenSpan* pos) {
  return pos->front().IsEndOfLine() || pos->front() == ':';
}

Location Loc(const TokenSpan* pos) { return pos->front().Location(); }

bool IsRegister(const Token& tok) {
  return tok == 'A' || tok == 'S' || tok == 'X' || tok == 'Y';
}

struct Nothing {};
ErrorOr<Nothing> Consume(TokenSpan* pos, char punct,
                         absl::string_view message) {
  if (pos->front() != punct) {
    return Error("Expected %s, found %s", message, pos->front().ToString())
        .SetLocation(pos->front().Location());
  }
  pos->remove_prefix(1);
  return Nothing();
}

ErrorOr<Nothing> ConfirmAtEnd(const TokenSpan* pos, absl::string_view message) {
  if (!AtEnd(pos)) {
    return Error("Unexpected %s %s", pos->front().ToString(), message)
        .SetLocation(pos->front().Location());
  }
  return Nothing();
}

ErrorOr<Nothing> ConfirmLegalRegister(const TokenSpan* pos,
                                      absl::string_view allowed,
                                      absl::string_view message) {
  if (IsRegister(pos->front())) {
    char reg = *pos->front().Punctuation();
    if (allowed.find(reg) == absl::string_view::npos) {
      return Error("Register %c cannot be used %s", reg, message)
          .SetLocation(pos->front().Location());
    }
  }
  return Nothing();
}

ErrorOr<ExpressionOrNull> Expr(TokenSpan* pos) {
  auto term_or_error = Term(pos);
  NSASM_RETURN_IF_ERROR(term_or_error);
  ExpressionOrNull term = std::move(*term_or_error);

  while (!AtEnd(pos)) {
    BinaryOp oper;
    if (pos->front() == '+') {
      oper = plus_op;
    } else if (pos->front() == '-') {
      oper = minus_op;
    } else {
      break;
    }
    pos->remove_prefix(1);
    auto rhs = Term(pos);
    NSASM_RETURN_IF_ERROR(rhs);
    ExpressionOrNull combined = absl::make_unique<BinaryExpression>(
        std::move(term), std::move(*rhs), oper);
    term = std::move(combined);
  }
  return std::move(term);
}

ErrorOr<ExpressionOrNull> Term(TokenSpan* pos) {
  auto factor_or_error = Factor(pos);
  NSASM_RETURN_IF_ERROR(factor_or_error);
  ExpressionOrNull factor = std::move(*factor_or_error);

  while (!AtEnd(pos)) {
    BinaryOp oper;
    if (pos->front() == '*') {
      oper = multiply_op;
    } else if (pos->front() == '/') {
      oper = divide_op;
    } else {
      break;
    }
    pos->remove_prefix(1);
    auto rhs = Factor(pos);
    NSASM_RETURN_IF_ERROR(rhs);
    ExpressionOrNull combined = absl::make_unique<BinaryExpression>(
        std::move(factor), std::move(*rhs), oper);
    factor = std::move(combined);
  }
  return std::move(factor);
}

ErrorOr<ExpressionOrNull> Factor(TokenSpan* pos) {
  UnaryOp oper;
  if (pos->front() == '-') {
    oper = negate_op;
  }
  if (oper) {
    pos->remove_prefix(1);
    auto arg = Factor(pos);
    NSASM_RETURN_IF_ERROR(arg);
    return {absl::make_unique<UnaryExpression>(std::move(*arg), oper)};
  }
  return Comp(pos);
}

ErrorOr<ExpressionOrNull> Comp(TokenSpan* pos) {
  if (pos->front().IsLiteral()) {
    ExpressionOrNull literal = absl::make_unique<Literal>(
        *pos->front().Literal(), pos->front().Type());
    pos->remove_prefix(1);
    return std::move(literal);
  }
  if (pos->front().IsIdentifier()) {
    ExpressionOrNull identifier =
        absl::make_unique<Identifier>(*pos->front().Identifier());
    pos->remove_prefix(1);
    return std::move(identifier);
  }
  if (pos->front() == '(') {
    pos->remove_prefix(1);
    auto parenthesized = Expr(pos);
    NSASM_RETURN_IF_ERROR(parenthesized);
    NSASM_RETURN_IF_ERROR(Consume(pos, ')', "close parenthesis"));
    return parenthesized;
  }
  return Error("Expected expression, found %s", pos->front().ToString())
      .SetLocation(Loc(pos));
}

ErrorOr<Instruction> CreateInstruction(
    Mnemonic mnemonic, SyntacticAddressingMode sam, Location location,
    ExpressionOrNull arg1 = ExpressionOrNull(),
    ExpressionOrNull arg2 = ExpressionOrNull()) {
  auto am = DeduceMode(mnemonic, sam, arg1, arg2);
  NSASM_RETURN_IF_ERROR_WITH_LOCATION(am, location);
  Instruction i;
  i.mnemonic = mnemonic;
  i.addressing_mode = *am;
  i.arg1 = std::move(arg1);
  i.arg2 = std::move(arg2);
  return std::move(i);
}

ErrorOr<Instruction> ParseInstruction(TokenSpan* pos) {
  if (AtEnd(pos) || !pos->front().IsMnemonic()) {
    return Error("logic error: ParseInstruction() called on non-mnemonic");
  }
  Mnemonic mnemonic = *pos->front().Mnemonic();
  pos->remove_prefix(1);

  if (AtEnd(pos)) {
    return CreateInstruction(mnemonic, SA_imp, Loc(pos));
  }

  NSASM_RETURN_IF_ERROR(ConfirmLegalRegister(pos, "A", "directly"));
  if (pos->front() == 'A') {
    pos->remove_prefix(1);
    NSASM_RETURN_IF_ERROR(ConfirmAtEnd(pos, "after A operand"));
    return CreateInstruction(mnemonic, SA_acc, Loc(pos));
  }

  if (pos->front() == '#') {
    pos->remove_prefix(1);
    auto arg1 = Expr(pos);
    NSASM_RETURN_IF_ERROR(arg1);
    if (AtEnd(pos)) {
      return CreateInstruction(mnemonic, SA_imm, Loc(pos), std::move(*arg1));
    }
    NSASM_RETURN_IF_ERROR(Consume(pos, ',', "comma or end of line"));
    NSASM_RETURN_IF_ERROR(Consume(pos, '#', "#"));
    auto arg2 = Expr(pos);
    NSASM_RETURN_IF_ERROR(arg2);
    NSASM_RETURN_IF_ERROR(ConfirmAtEnd(pos, "after immediate arguments"));
    return CreateInstruction(mnemonic, SA_mov, Loc(pos), std::move(*arg1),
                             std::move(*arg2));
  }

  if (pos->front() == '[') {
    pos->remove_prefix(1);
    auto arg1 = Expr(pos);
    NSASM_RETURN_IF_ERROR(arg1);
    NSASM_RETURN_IF_ERROR(Consume(pos, ']', "close bracket"));
    if (AtEnd(pos)) {
      return CreateInstruction(mnemonic, SA_lng, Loc(pos), std::move(*arg1));
    }
    NSASM_RETURN_IF_ERROR(Consume(pos, ',', "comma or end of line"));
    NSASM_RETURN_IF_ERROR(
        ConfirmLegalRegister(pos, "Y", "with indirect long indexing"));
    NSASM_RETURN_IF_ERROR(Consume(pos, 'Y', "register Y"));
    NSASM_RETURN_IF_ERROR(
        ConfirmAtEnd(pos, "after indirect long indexed argument"));
    return CreateInstruction(mnemonic, SA_lng_y, Loc(pos), std::move(*arg1));
  }

  // The one ambiguity in the grammar is how to deal with a '(' character at
  // the start of an argument.  This can either represent an indirect argument,
  // or a parenthetical subexpression.  The former is chosen if possible.  Here
  // we try both.
  //
  // Often, the error message returned by the latter attempt will be reasonable
  // for invalid input.  We return errors in the first pass when they are
  // indexing mode specific, and when we know that the token stream can't be a
  // valid expression either.

  if (pos->front() == '(') {
    // Remember where we were; we will set it back if we abandon this path
    TokenSpan backup_pos = *pos;

    pos->remove_prefix(1);
    auto arg1 = Expr(pos);
    // If we couldn't scan an indexing expression argument here, we wouldn't
    // succeed trying to parse it as a subexpression either.
    NSASM_RETURN_IF_ERROR(arg1);
    if (pos->front() == ',') {
      // If we found a comma inside the outermost parentheses, then this has
      // to be some manner of indexing syntax.
      pos->remove_prefix(1);
      // We have scanned "OPR (arg1,"
      // This is either "OPR (arg1,X)", "ORP (arg1,S),Y", or an error.
      NSASM_RETURN_IF_ERROR(
          ConfirmLegalRegister(pos, "XS", "with indexed indirect mode"));
      if (pos->front() == 'X') {
        pos->remove_prefix(1);
        NSASM_RETURN_IF_ERROR(Consume(pos, ')', "close parenthesis"));
        NSASM_RETURN_IF_ERROR(
            ConfirmAtEnd(pos, "after indexed indirect argument"));
        return CreateInstruction(mnemonic, SA_ind_x, Loc(pos),
                                 std::move(*arg1));
      } else {
        NSASM_RETURN_IF_ERROR(Consume(pos, 'S', "X or S register"));
        NSASM_RETURN_IF_ERROR(Consume(pos, ')', "close parenthesis"));
        NSASM_RETURN_IF_ERROR(
            Consume(pos, ',', "comma after stack relative indirect"));
        NSASM_RETURN_IF_ERROR(ConfirmLegalRegister(
            pos, "Y", "with stack relative indirect indexing"));
        NSASM_RETURN_IF_ERROR(Consume(pos, 'Y', "register Y"));
        NSASM_RETURN_IF_ERROR(ConfirmAtEnd(
            pos, "after stack relative indirect indexed argument"));
        return CreateInstruction(mnemonic, SA_stk_y, Loc(pos),
                                 std::move(*arg1));
      }
    }
    if (pos->front() == ')') {
      // We have scanned "OPR (arg1)".  This is legal on its own, or we may have
      // "OPR (arg1),Y".  Anything else we will attempt to parse as a direct
      // value below.
      pos->remove_prefix(1);
      if (AtEnd(pos)) {
        return CreateInstruction(mnemonic, SA_ind, Loc(pos), std::move(*arg1));
      }
      if (pos->front() == ',') {
        // A comma after parens means this must be indexing
        pos->remove_prefix(1);
        NSASM_RETURN_IF_ERROR(
            ConfirmLegalRegister(pos, "Y", "with indirect indexing"));
        NSASM_RETURN_IF_ERROR(Consume(pos, 'Y', "register Y"));
        NSASM_RETURN_IF_ERROR(
            ConfirmAtEnd(pos, "after indirect indexed argument"));
        return CreateInstruction(mnemonic, SA_ind_y, Loc(pos),
                                 std::move(*arg1));
      }
      // We have to abandon this attempt and go back to parsing this as an
      // expression.
      *pos = backup_pos;
    }
  }

  // We've tried everything else; now try a bare expression.
  auto arg1 = Expr(pos);
  NSASM_RETURN_IF_ERROR(arg1);
  if (AtEnd(pos)) {
    return CreateInstruction(mnemonic, SA_dir, Loc(pos), std::move(*arg1));
  }
  NSASM_RETURN_IF_ERROR(Consume(pos, ',', "comma or end of line"));
  NSASM_RETURN_IF_ERROR(
      ConfirmLegalRegister(pos, "XYS", "with direct indexing"));
  if (pos->front() == 'X') {
    pos->remove_prefix(1);
    NSASM_RETURN_IF_ERROR(ConfirmAtEnd(pos, "after indexed argument"));
    return CreateInstruction(mnemonic, SA_dir_x, Loc(pos), std::move(*arg1));
  } else if (pos->front() == 'Y') {
    pos->remove_prefix(1);
    NSASM_RETURN_IF_ERROR(ConfirmAtEnd(pos, "after indexed argument"));
    return CreateInstruction(mnemonic, SA_dir_y, Loc(pos), std::move(*arg1));
  } else {
    NSASM_RETURN_IF_ERROR(Consume(pos, 'S', "X, Y, or S register"));
    NSASM_RETURN_IF_ERROR(ConfirmAtEnd(pos, "after stack relative argument"));
    return CreateInstruction(mnemonic, SA_stk, Loc(pos), std::move(*arg1));
  }
}

ErrorOr<Directive> ParseDirective(TokenSpan* pos) {
  if (AtEnd(pos) || !pos->front().IsDirectiveName()) {
    return Error("logic error: ParseDirective() called on non-directive-name");
  }
  Directive directive;
  directive.name = *pos->front().DirectiveName();
  pos->remove_prefix(1);
  DirectiveType directive_type = DirectiveTypeByName(directive.name);
  switch (directive_type) {
    case DT_single_arg: {
      auto arg1 = Expr(pos);
      NSASM_RETURN_IF_ERROR(arg1);
      directive.argument = std::move(*arg1);
      NSASM_RETURN_IF_ERROR(ConfirmAtEnd(pos, "after directive argument"));
      return std::move(directive);
    }
    case DT_list_arg: {
      // Loop structured such that we must find at least one argument, but more
      // are ok.
      while (true) {
        auto arg = Expr(pos);
        NSASM_RETURN_IF_ERROR(arg);
        directive.list_argument.push_back(std::move(*arg));
        if (AtEnd(pos)) {
          return std::move(directive);
        }
        NSASM_RETURN_IF_ERROR(Consume(pos, ',', "comma or end of line"));
      }
      return std::move(directive);
    }
    case DT_flag_arg: {
      Location loc = pos->front().Location();
      if (!pos->front().IsIdentifier()) {
        return Error("Expected mode name, found %s", pos->front().ToString())
            .SetLocation(loc);
      }
      std::string flag_name = *pos->front().Identifier();
      pos->remove_prefix(1);
      auto flag_state = FlagState::FromName(flag_name);
      if (!flag_state.has_value()) {
        return Error("\"%s\" does not name a flag state", flag_name)
            .SetLocation(loc);
      }
      directive.flag_state_argument = *flag_state;
      NSASM_RETURN_IF_ERROR(ConfirmAtEnd(pos, "after flag state"));
      return std::move(directive);
    }
  }
}

}  // namespace

ErrorOr<std::vector<absl::variant<Instruction, Directive, std::string>>>
Assemble(absl::Span<const Token> tokens) {
  std::vector<absl::variant<Instruction, Directive, std::string>> result_vector;

  while (!tokens.empty()) {
    // An unexpected token at the beginning of the line is a label.
    // But don't allow multiples without a colon.
    //   foo adc #$12       ; okay
    //   foo: adc #$12      ; okay
    //   foo bar adc #$12   ; unexpected 'bar'
    //   foo: bar adc #$12  ; okay
    if (tokens.front().IsIdentifier()) {
      result_vector.push_back(*tokens.front().Identifier());
      tokens.remove_prefix(1);
      if (!tokens.empty() && tokens.front() == ':') {
        tokens.remove_prefix(1);
        continue;
      }
    }

    if (AtEnd(&tokens)) {
      return result_vector;
    }

    if (tokens.front().IsDirectiveName()) {
      auto directive = ParseDirective(&tokens);
      NSASM_RETURN_IF_ERROR(directive);
      if (!AtEnd(&tokens)) {
        return Error(
            "logic error: ParseDirective() did not read to a line end");
      }
      tokens.remove_prefix(1);
      result_vector.push_back(std::move(*directive));
      continue;
    }

    auto mnemonic = tokens.front().Mnemonic();
    Location mnemonic_location = tokens.front().Location();
    if (!mnemonic) {
      return Error("Expected mnemonic or directive but found %s",
                   tokens.front().ToString())
          .SetLocation(mnemonic_location);
    }
    auto instruction = ParseInstruction(&tokens);
    NSASM_RETURN_IF_ERROR(instruction);
    if (!AtEnd(&tokens)) {
      return Error(
          "logic error: ParseInstruction() did not read to a line end");
    }
    tokens.remove_prefix(1);
    result_vector.push_back(std::move(*instruction));
  }
  return result_vector;
}

}  // namespace nsasm