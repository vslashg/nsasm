#include "nsasm/mnemonic.h"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"

namespace nsasm {
namespace {

constexpr absl::string_view mnemonic_names[] = {
    "adc", "and", "asl", "bit", "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec",
    "dex", "dey", "eor", "inc", "inx", "iny", "lda", "ldx", "ldy", "lsr", "mvn",
    "mvp", "nop", "ora", "pea", "pei", "per", "pha", "phb", "phd", "phk", "phx",
    "phy", "pla", "plb", "pld", "plx", "ply", "rol", "ror", "sbc", "sed", "sei",
    "sta", "stp", "stx", "sty", "stz", "tax", "tay", "tcd", "tcs", "tdc", "trb",
    "tsb", "tsc", "tsx", "txa", "txs", "txy", "tya", "tyx", "wai", "wdm", "xba",
    "bcc", "bcs", "beq", "bmi", "bne", "bpl", "bra", "brk", "brl", "bvc", "bvs",
    "cop", "jmp", "jsl", "jsr", "rti", "rtl", "rts", "clc", "php", "plp", "rep",
    "sec", "sep", "xce", "add", "sub",
};

constexpr absl::string_view suffix_names[] = {".b", ".w"};

}  // namespace

absl::string_view ToString(Mnemonic m) {
  if (m < M_adc || m > PM_sub) {
    return "";
  }
  return mnemonic_names[m];
}

absl::string_view ToString(Suffix m) {
  if (m < S_b || m > S_w) {
    return "";
  }
  return suffix_names[m];
}

absl::optional<Mnemonic> ToMnemonic(std::string s) {
  static auto lookup = new absl::flat_hash_map<absl::string_view, Mnemonic>{
      {"adc", M_adc},  {"and", M_and}, {"asl", M_asl}, {"bit", M_bit},
      {"cld", M_cld},  {"cli", M_cli}, {"clv", M_clv}, {"cmp", M_cmp},
      {"cpx", M_cpx},  {"cpy", M_cpy}, {"dec", M_dec}, {"dex", M_dex},
      {"dey", M_dey},  {"eor", M_eor}, {"inc", M_inc}, {"inx", M_inx},
      {"iny", M_iny},  {"lda", M_lda}, {"ldx", M_ldx}, {"ldy", M_ldy},
      {"lsr", M_lsr},  {"mvn", M_mvn}, {"mvp", M_mvp}, {"nop", M_nop},
      {"ora", M_ora},  {"pea", M_pea}, {"pei", M_pei}, {"per", M_per},
      {"pha", M_pha},  {"phb", M_phb}, {"phd", M_phd}, {"phx", M_phx},
      {"phy", M_phy},  {"phk", M_phk}, {"pla", M_pla}, {"plb", M_plb},
      {"pld", M_pld},  {"plx", M_plx}, {"ply", M_ply}, {"rol", M_rol},
      {"ror", M_ror},  {"sbc", M_sbc}, {"sed", M_sed}, {"sei", M_sei},
      {"sta", M_sta},  {"stp", M_stp}, {"stx", M_stx}, {"sty", M_sty},
      {"stz", M_stz},  {"tax", M_tax}, {"tay", M_tay}, {"tcd", M_tcd},
      {"tcs", M_tcs},  {"tdc", M_tdc}, {"trb", M_trb}, {"tsb", M_tsb},
      {"tsc", M_tsc},  {"tsx", M_tsx}, {"txa", M_txa}, {"txs", M_txs},
      {"txy", M_txy},  {"tya", M_tya}, {"tyx", M_tyx}, {"wai", M_wai},
      {"wdm", M_wdm},  {"xba", M_xba}, {"bcc", M_bcc}, {"bcs", M_bcs},
      {"beq", M_beq},  {"bmi", M_bmi}, {"bne", M_bne}, {"bpl", M_bpl},
      {"bra", M_bra},  {"brk", M_brk}, {"brl", M_brl}, {"bvc", M_bvc},
      {"bvs", M_bvs},  {"cop", M_cop}, {"jmp", M_jmp}, {"jsl", M_jsl},
      {"jsr", M_jsr},  {"rti", M_rti}, {"rtl", M_rtl}, {"rts", M_rts},
      {"clc", M_clc},  {"php", M_php}, {"plp", M_plp}, {"rep", M_rep},
      {"sec", M_sec},  {"sep", M_sep}, {"xce", M_xce}, {"add", PM_add},
      {"sub", PM_sub},
  };
  absl::AsciiStrToLower(&s);
  auto iter = lookup->find(s);
  if (iter == lookup->end()) {
    return absl::nullopt;
  }
  return iter->second;
}

absl::optional<Suffix> ToSuffix(std::string s) {
  static auto lookup = new absl::flat_hash_map<absl::string_view, Suffix>{
      {".b", S_b}, {".w", S_w}};
  absl::AsciiStrToLower(&s);
  auto iter = lookup->find(s);
  if (iter == lookup->end()) {
    return absl::nullopt;
  }
  return iter->second;
}

namespace {
std::vector<Mnemonic>* MakeAllMnemonics() {
  auto result = new std::vector<Mnemonic>;
  for (int mnemonic_index = M_adc; mnemonic_index <= PM_sub; ++mnemonic_index) {
    result->push_back(static_cast<Mnemonic>(mnemonic_index));
  }
  return result;
}

std::vector<Suffix>* MakeAllSuffixes() {
  auto result = new std::vector<Suffix>{S_b, S_w};
  return result;
}

}  // namespace

const std::vector<Mnemonic>& AllMnemonics() {
  static std::vector<Mnemonic>* all_mnemonics = MakeAllMnemonics();
  return *all_mnemonics;
}

const std::vector<Suffix>& AllSuffixes() {
  static std::vector<Suffix>* all_suffixes = MakeAllSuffixes();
  return *all_suffixes;
}

}  // namespace nsasm
