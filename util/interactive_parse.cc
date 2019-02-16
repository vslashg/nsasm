#include <iostream>

#include "nsasm/error.h"
#include "nsasm/parse.h"
#include "nsasm/token.h"

using nsasm::Directive;
using nsasm::Instruction;
using nsasm::Location;
using nsasm::Parse;
using nsasm::Tokenize;
using nsasm::TokenSpan;

int main(int argc, char** argv) {
  while (std::cin) {
    std::string line;
    std::cout << "> " << std::flush;
    std::getline(std::cin, line);

    // try to tokenize
    auto tokens = Tokenize(line, Location());
    if (!tokens.ok()) {
      std::cout << tokens.error().ToString() << "\n";
      continue;
    }

    // try to assemble
    auto assembly = Parse(*tokens);
    if (!assembly.ok()) {
      std::cout << assembly.error().ToString() << "\n";
      continue;
    }

    for (const auto& line : *assembly) {
      if (absl::holds_alternative<std::string>(line)) {
        std::cout << absl::get<std::string>(line) << ":\n";
      }
      if (absl::holds_alternative<Instruction>(line)) {
        const Instruction& ins = absl::get<Instruction>(line);
        std::cout << "    " << ins.ToString() << "\n";
      }
      if (absl::holds_alternative<Directive>(line)) {
        const Directive& dir = absl::get<Directive>(line);
        std::cout << "    " << dir.ToString() << "\n";
      }
    }
  }
  return 0;
}