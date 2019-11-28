//==- TriCoreAsmParser.cpp - Parse TriCore assembly to MCInst instructions -==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InstPrinter/TriCoreInstPrinter.h"
#include "MCTargetDesc/TriCoreMCExpr.h"
#include "MCTargetDesc/TriCoreMCTargetDesc.h"
#include "TargetInfo/TriCoreTargetInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

namespace {
struct TriCoreOperand;

class TriCoreAsmParser : public MCTargetAsmParser {
  SMLoc getLoc() const { return getParser().getTok().getLoc(); }

  bool generateImmOutOfRangeError(OperandVector &Operands, uint64_t ErrorInfo,
                                  int64_t Lower, int64_t Upper, Twine Msg);

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;

  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;

  bool ParseDirective(AsmToken DirectiveID) override;

// Auto-generated instruction matching functions
#define GET_ASSEMBLER_HEADER
#include "TriCoreGenAsmMatcher.inc"

  OperandMatchResultTy parseImmediate(OperandVector &Operands);
  OperandMatchResultTy parseRegister(OperandVector &Operands);
  OperandMatchResultTy parseMemOpBaseReg(OperandVector &Operands);
  OperandMatchResultTy parseOperandWithModifier(OperandVector &Operands);

  bool parseOperand(OperandVector &Operands);

  void setFeatureBits(uint64_t Feature, StringRef FeatureString) {
    if (!(getSTI().getFeatureBits()[Feature])) {
      MCSubtargetInfo &STI = copySTI();
      setAvailableFeatures(
          ComputeAvailableFeatures(STI.ToggleFeature(FeatureString)));
    }
  }

  void clearFeatureBits(uint64_t Feature, StringRef FeatureString) {
    if (getSTI().getFeatureBits()[Feature]) {
      MCSubtargetInfo &STI = copySTI();
      setAvailableFeatures(
          ComputeAvailableFeatures(STI.ToggleFeature(FeatureString)));
    }
  }

public:
  enum TriCoreMatchResultTy {
    Match_Dummy = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "TriCoreGenAsmMatcher.inc"
#undef GET_OPERAND_DIAGNOSTIC_TYPES
  };

  static bool classifySymbolRef(const MCExpr *Expr,
                                TriCoreMCExpr::VariantKind &Kind,
                                int64_t &Addend);

  TriCoreAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                   const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }
};

/// TriCoreOperand - Instances of this class represent a parsed machine
/// instruction
struct TriCoreOperand : public MCParsedAsmOperand {

  enum KindTy {
    Token,
    Register,
    Immediate,
  } Kind;

  struct RegOp {
    unsigned RegNum;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  SMLoc StartLoc, EndLoc;
  union {
    StringRef Tok;
    RegOp Reg;
    ImmOp Imm;
  };

  TriCoreOperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

public:
  TriCoreOperand(const TriCoreOperand &o) : MCParsedAsmOperand() {
    Kind = o.Kind;
    StartLoc = o.StartLoc;
    EndLoc = o.EndLoc;
    switch (Kind) {
    case Register:
      Reg = o.Reg;
      break;
    case Immediate:
      Imm = o.Imm;
      break;
    case Token:
      Tok = o.Tok;
      break;
    }
  }

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return false; }

  bool evaluateConstantImm(int64_t &Imm, TriCoreMCExpr::VariantKind &VK) const {
    const MCExpr *Val = getImm();
    bool Ret = false;

    if (auto *RE = dyn_cast<TriCoreMCExpr>(Val)) {
      Ret = RE->evaluateAsConstant(Imm);
      VK = RE->getKind();
    } else if (auto CE = dyn_cast<MCConstantExpr>(Val)) {
      Ret = true;
      VK = TriCoreMCExpr::VK_TRICORE_None;
      Imm = CE->getValue();
    }
    return Ret;
  }

  template <int N> bool isSImmN() const { return isImmN<N, true>(); }

  template <int N> bool isUImmN() const { return isImmN<N, false>(); }

  template <int N, bool SIGNED> bool isImmN() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = false; // symbols for this operand type is not allowed yet
    else
      IsValid = SIGNED ? isInt<N>(Imm) : isUInt<N>(Imm);

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  bool isSImm16_BOL() const { return isImm16<true, false>(); }

  bool isSImm16_RLC() const { return isImm16<true, true>(); }

  bool isUImm16_RLC() const { return isImm16<false, true>(); }

  // templated function for RLC and BOL format, they need special handling
  // since some of the modifiers can only be used with one of the formats
  // but not with the other one e.g.: HI is only for RLC, SM is only for BOL
  template <bool SIGNED, bool isRLC> bool isImm16() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;
    bool isVKCorrect = false;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = TriCoreAsmParser::classifySymbolRef(getImm(), VK, Imm);
    else
      IsValid = SIGNED ? isInt<16>(Imm) : isUInt<16>(Imm);

    isVKCorrect = (VK == TriCoreMCExpr::VK_TRICORE_None) ||
                  (VK == TriCoreMCExpr::VK_TRICORE_LO);

    if (isRLC)
      isVKCorrect = isVKCorrect || (VK == TriCoreMCExpr::VK_TRICORE_HI);
    else
      isVKCorrect = isVKCorrect || (VK == TriCoreMCExpr::VK_TRICORE_SM);

    return IsValid && isVKCorrect;
  }

  template <int WIDTH, int SHIFT, bool SIGNED> bool isScaledImm() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    // only allow symbols when the operand is: simm15_lsb0 or simm24_lsb0
    // which are corresponds to _15REL and _24REL relocations
    if (!IsConstantImm)
      IsValid = (WIDTH == 15 || WIDTH == 24) && SIGNED
                    ? TriCoreAsmParser::classifySymbolRef(getImm(), VK, Imm)
                    : false;
    else
      IsValid = SIGNED ? isShiftedInt<WIDTH, SHIFT>(Imm)
                       : isShiftedUInt<WIDTH, SHIFT>(Imm);

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  // checking against "PC + {27b’111111111111111111111111111, disp4, 0}"
  bool isSImm4_1() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = false; // symbols for this operand type is not allowed yet
    else
      IsValid = Imm >= -32 && Imm <= -2 && (Imm % 2 == 0);

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  // checking against {off18[17:14], 14'b0, off18[13:0]} form
  bool isOff18Abs() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = TriCoreAsmParser::classifySymbolRef(getImm(), VK, Imm);
    else
      IsValid = (Imm & ~0xf0003fff) == 0;

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  // checking against {off18[17:0], 14'b0} form
  bool isOff18AbsV2() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = false; // symbols for this operand type is not allowed yet
    else
      IsValid = (Imm & ~0xffffc000) == 0;

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  // checking against zero_ext(disp4 + 16) * 2 form
  bool isDisp4_16() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = false; // symbols for this operand type is not allowed yet
    else
      IsValid = Imm >= 32 && Imm <= 62 && (Imm % 2 == 0);

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  // checking against {disp24[23:20], 7'b0, disp24[19:0], 1'b0} form
  bool isDisp24Abs() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = TriCoreAsmParser::classifySymbolRef(getImm(), VK, Imm);
    else
      IsValid = (Imm & ~0xf01ffffe) == 0;

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  // checking if in the range of 6 bit signed immediate
  bool isSImm9Shift() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = false; // symbols for this operand type is not allowed yet
    else
      IsValid = (Imm >= -32 && Imm <= 31);

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  // checking if in the range of 5 bit signed immediate
  bool isSImm9Shift5() const {
    int64_t Imm;
    TriCoreMCExpr::VariantKind VK = TriCoreMCExpr::VK_TRICORE_None;
    bool IsValid;

    if (!isImm())
      return false;

    bool IsConstantImm = evaluateConstantImm(Imm, VK);

    if (!IsConstantImm)
      IsValid = false; // symbols for this operand type is not allowed yet
    else
      IsValid = (Imm >= -16 && Imm <= 15);

    return IsValid && VK == TriCoreMCExpr::VK_TRICORE_None;
  }

  /// getStartLoc - Gets location of the first token of this operand
  SMLoc getStartLoc() const override { return StartLoc; }
  /// getEndLoc - Gets location of the last token of this operand
  SMLoc getEndLoc() const override { return EndLoc; }

  unsigned getReg() const override {
    assert(Kind == Register && "Invalid type access!");
    return Reg.RegNum;
  }

  const MCExpr *getImm() const {
    assert(Kind == Immediate && "Invalid type access!");
    return Imm.Val;
  }

  StringRef getToken() const {
    assert(Kind == Token && "Invalid type access!");
    return Tok;
  }

  void print(raw_ostream &OS) const override {
    switch (Kind) {
    case Immediate:
      OS << *getImm();
      break;
    case Register:
      OS << "<register " << llvm::TriCoreInstPrinter::getRegisterName(getReg())
         << ">";
      break;
    case Token:
      OS << "'" << getToken() << "'";
      break;
    }
  }

  static std::unique_ptr<TriCoreOperand> createToken(StringRef Str, SMLoc S) {
    auto Op = std::make_unique<TriCoreOperand>(Token);
    Op->Tok = Str;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static std::unique_ptr<TriCoreOperand> createReg(unsigned RegNo, SMLoc S,
                                                   SMLoc E) {
    auto Op = std::make_unique<TriCoreOperand>(Register);
    Op->Reg.RegNum = RegNo;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<TriCoreOperand> createImm(const MCExpr *Val, SMLoc S,
                                                   SMLoc E) {
    auto Op = std::make_unique<TriCoreOperand>(Immediate);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const {
    assert(Expr && "Expr shouldn't be null!");
    int64_t Imm = 0;
    bool IsConstant = false;

    // if the expression is a TriCoreMCExpr type then evaluate it with
    // custom function
    if (auto *RE = dyn_cast<TriCoreMCExpr>(Expr)) {
      IsConstant = RE->evaluateAsConstant(Imm);
      // if plain constant expression evaluate it the usual way
    } else if (auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
      IsConstant = true;
      Imm = CE->getValue();
    }

    // create the operand depending on if it is a constant or not
    if (IsConstant)
      Inst.addOperand(MCOperand::createImm(Imm));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  // Used by the TableGen Code
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    addExpr(Inst, getImm());
  }
};
} // end anonymous namespace.

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "TriCoreGenAsmMatcher.inc"

bool TriCoreAsmParser::generateImmOutOfRangeError(
    OperandVector &Operands, uint64_t ErrorInfo, int64_t Lower, int64_t Upper,
    Twine Msg = "immediate must be an integer in the range") {
  SMLoc ErrorLoc = ((TriCoreOperand &)*Operands[ErrorInfo]).getStartLoc();
  return Error(ErrorLoc, Msg + " [" + Twine(Lower) + ", " + Twine(Upper) + "]");
}

bool TriCoreAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                               OperandVector &Operands,
                                               MCStreamer &Out,
                                               uint64_t &ErrorInfo,
                                               bool MatchingInlineAsm) {
  MCInst Inst;
  SMLoc ErrorLoc;

  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
  default:
    break;
  case Match_Success:
    Inst.setLoc(IDLoc);
    Out.EmitInstruction(Inst, getSTI());
    if (!(getSTI().getFeatureBits()[TriCore::Only32BitInstructions])) {
      setFeatureBits(TriCore::Allow16BitInstructions, "allow-16bit");
      setFeatureBits(TriCore::Allow32BitInstructions, "allow-32bit");
    }
    return false;
  case Match_MissingFeature:
    return Error(IDLoc, "instruction use requires an option to be enabled");
  case Match_MnemonicFail:
    return Error(IDLoc, "unrecognized instruction mnemonic");
  case Match_InvalidOperand:
    ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(ErrorLoc, "too few operands for instruction");

      ErrorLoc = ((TriCoreOperand &)*Operands[ErrorInfo]).getStartLoc();
      if (ErrorLoc == SMLoc())
        ErrorLoc = IDLoc;
    }
    return Error(ErrorLoc, "invalid operand for instruction");
  case Match_InvalidUImm2:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 2) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidSImm4:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 3), (1 << 3) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidSImm4_1:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -32, -2,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be even number in the integer range");
  case Match_InvalidUImm4:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 4) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidUImm5:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 5) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidUImm8:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 8) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidSImm9:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 8), (1 << 8) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidUImm9:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 9) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidSImm9Shift:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 5), (1 << 5) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidSImm9Shift5:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 4), (1 << 4) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidSImm10:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 9), (1 << 9) - 1,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be in the integer range");
  case Match_InvalidSImm16_BOL:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 15), (1 << 15) - 1,
        "Operand must be a valid symbol with optional operand prefix lo or sm "
        "OR it must be an the integer in the range");
  case Match_InvalidSImm16_RLC:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 15), (1 << 15) - 1,
        "Operand must be a valid symbol with optional operand prefix lo or hi "
        "OR it must be an the integer in the range");
  case Match_InvalidUImm16_RLC:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 16) - 1,
        "Operand must be a valid symbol with optional operand prefix lo or hi "
        "OR it must be an the integer in the range");
  case Match_InvalidUImm4_Lsb1:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, ((1 << 4) - 1) * 2,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be even number in the integer range");
  case Match_InvalidUImm4_Lsb2:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, ((1 << 4) - 1) * 4,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be divisible by 4 and in the integer range");
  case Match_InvalidSImm8_Lsb1:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 8), (1 << 8) - 2,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be even number in the integer range");
  case Match_InvalidUImm8_Lsb2:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, ((1 << 8) - 1) * 4,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be divisible by 4 and in the integer range");
  case Match_InvalidSImm15_Lsb1:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 15), (1 << 15) - 2,
        "Operand must be a valid symbol expression or an even integer and "
        "in the range");
  case Match_InvalidSImm24_Lsb1:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 24), (1 << 24) - 2,
        "Operand must be a valid symbol expression or an even integer and "
        "in the range");
  case Match_InvalidOff18Abs:
    return Error(((TriCoreOperand &)*Operands[ErrorInfo]).getStartLoc(),
                 "Operand must be a valid symbol expression or a 32 bit "
                 "address with bit 27-14 set to 0");
  case Match_InvalidOff18AbsV2:
    return Error(((TriCoreOperand &)*Operands[ErrorInfo]).getStartLoc(),
                 "Operand must be a valid symbol expression or a 32 bit "
                 "address with bit 13-0 set to 0");
  case Match_InvalidDisp4_16:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 16 * 2, (15 + 16) * 2,
        "Operand prefixes and symbol expressions are not allowed for this "
        "operand and it must be even number in the integer range");
  case Match_InvalidDisp24Abs:
    ErrorLoc = ((TriCoreOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc,
                 "Operand must be a valid symbol expression or a 32 bit "
                 "address with bit 27-21 and 0 set to 0");
  }

  llvm_unreachable("Unknown match type detected!");
}

bool TriCoreAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                     SMLoc &EndLoc) {
  const AsmToken &Tok = getParser().getTok();
  StartLoc = Tok.getLoc();
  EndLoc = Tok.getEndLoc();
  RegNo = 0;
  StringRef Name = getLexer().getTok().getIdentifier();

  RegNo = MatchRegisterName(Name);
  if (RegNo == 0)
    RegNo = MatchRegisterAltName(Name);
  if (RegNo == 0)
    return Error(StartLoc, "invalid register name");

  getParser().Lex(); // Eat identifier token.
  return false;
}

OperandMatchResultTy TriCoreAsmParser::parseRegister(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::Percent:
  case AsmToken::Identifier:
    if (getLexer().is(AsmToken::Percent))
      getLexer().Lex();
    else
      return MatchOperand_NoMatch;

    StringRef Name = getLexer().getTok().getIdentifier();
    unsigned RegNo = MatchRegisterName(Name);
    if (RegNo == 0) {
      RegNo = MatchRegisterAltName(Name);
      if (RegNo == 0)
        return MatchOperand_NoMatch;
    }
    getLexer().Lex();
    Operands.push_back(TriCoreOperand::createReg(RegNo, S, E));
  }
  return MatchOperand_Success;
}

OperandMatchResultTy TriCoreAsmParser::parseImmediate(OperandVector &Operands) {
  const MCExpr *IdVal;
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::LParen:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::Integer:
  case AsmToken::String:
    break;
  case AsmToken::Identifier:
    if (!parseOperandWithModifier(Operands))
      return MatchOperand_Success;
    break;
  }

  if (getParser().parseExpression(IdVal))
    return MatchOperand_ParseFail;

  Operands.push_back(TriCoreOperand::createImm(IdVal, S, E));
  return MatchOperand_Success;
}

// Parsing operand in the forms:
// [<reg>], [+<reg>], [<reg>+], [<reg>+r], [<reg>+c]
OperandMatchResultTy
TriCoreAsmParser::parseMemOpBaseReg(OperandVector &Operands) {
  bool PlusFound = false;

  if (getLexer().isNot(AsmToken::LBrac))
    return MatchOperand_NoMatch;

  // creating '[' token operand
  Operands.push_back(TriCoreOperand::createToken("[", getLoc()));
  getParser().Lex(); // eat '['

  if (getLexer().is(AsmToken::Plus)) {
    PlusFound = true;
    Operands.push_back(TriCoreOperand::createToken("+", getLoc()));
    getParser().Lex(); // eat '+'
  }

  // parse the address register
  if (parseRegister(Operands) != MatchOperand_Success)
    return MatchOperand_ParseFail;

  // cant have 2 '+'
  if (getLexer().is(AsmToken::Plus) && PlusFound)
    return MatchOperand_ParseFail;

  // parse '+', "+r", "+c"
  if ((getLexer().is(AsmToken::Plus) && !PlusFound)) {
    // creating '+' token operand
    Operands.push_back(TriCoreOperand::createToken("+", getLoc()));
    getParser().Lex(); // eat '+'

    if (getLexer().getKind() == AsmToken::Identifier) {
      StringRef Identifier = getParser().getTok().getIdentifier();

      if (!Identifier.equals("r") && !Identifier.equals("c"))
        return MatchOperand_ParseFail;
      else {
        // creating 'c' or 'r' token operand
        Operands.push_back(TriCoreOperand::createToken(Identifier, getLoc()));
        getParser().Lex(); // eat 'Identifier'
      }
    }
  }

  // check if next token is ']'
  if (getLexer().isNot(AsmToken::RBrac))
    return MatchOperand_ParseFail;

  // creating ']' token operand
  Operands.push_back(TriCoreOperand::createToken("]", getLoc()));
  getParser().Lex(); // eat ']'

  return MatchOperand_Success;
}

// Parsing operand in the forms:
// <modifier>:<constant>, <modifier>:<symbol>
// <modifier>:<symbol><+|-><constant>
// where modifers: sm, lo, hi (case insensitive)
OperandMatchResultTy
TriCoreAsmParser::parseOperandWithModifier(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);

  // check for the ':' which should follow the modifier ex.: lo:foo
  if (getLexer().peekTok().getKind() != AsmToken::Colon)
    return MatchOperand_NoMatch;

  // parse the identifier and determine the variantkind based on it
  StringRef Identifier = getParser().getTok().getIdentifier();
  TriCoreMCExpr::VariantKind VK =
      TriCoreMCExpr::getVariantKindForName(Identifier);

  // only proceed further if the kind is not invalid
  if (VK == TriCoreMCExpr::VK_TRICORE_Invalid)
    return MatchOperand_NoMatch;

  // only parse the indentifier if colon also found, this way letting hi, lo, sm
  // to be usable as symbol names also
  getParser().Lex(); // Eat the identifier
  getParser().Lex(); // Eat ':'

  // sm modifier can not be applied to constant expressions
  if (VK == TriCoreMCExpr::VK_TRICORE_SM &&
      (getLexer().getKind() != AsmToken::Identifier &&
       getLexer().getKind() != AsmToken::String)) {
    Error(getLexer().getLoc(), "Illegal prefix for constant expression");
    return MatchOperand_ParseFail;
  }

  // parse the expression ex.: lo:foo -> foo
  const MCExpr *SubExpr;
  if (getParser().parseExpression(SubExpr))
    return MatchOperand_ParseFail;

  // create the operand
  const MCExpr *ModExpr = TriCoreMCExpr::create(SubExpr, VK, getContext());
  Operands.push_back(TriCoreOperand::createImm(ModExpr, S, E));

  return MatchOperand_Success;
}

/// Looks at a token type and creates the relevant operand
/// from this information, adding to Operands.
/// If operand was parsed, returns false, else true.
bool TriCoreAsmParser::parseOperand(OperandVector &Operands) {
  // Attempt to parse token as register
  if (parseRegister(Operands) == MatchOperand_Success)
    return false;

  // Attempt to parse token as an immediate
  if (parseImmediate(Operands) == MatchOperand_Success)
    return false;

  // Attempt to parse token as a memory operand
  if (parseMemOpBaseReg(Operands) == MatchOperand_Success) {
    // parse offset
    if (getLexer().isNot(AsmToken::EndOfStatement) &&
        getLexer().isNot(AsmToken::Comma))
      return parseImmediate(Operands) != MatchOperand_Success;

    return false;
  }

  // Finally we have exhausted all options and must declare defeat.
  Error(getLoc(), "unknown operand");
  return true;
}

bool TriCoreAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                        StringRef Name, SMLoc NameLoc,
                                        OperandVector &Operands) {
  // First operand is token for instruction
  Operands.push_back(TriCoreOperand::createToken(Name, NameLoc));

  // If there are no more operands, then finish
  if (getLexer().is(AsmToken::EndOfStatement))
    return false;

  // Parse first operand
  if (parseOperand(Operands))
    return true;

  // Parse until end of statement, consuming commas between operands
  while (getLexer().is(AsmToken::Comma)) {
    // Consume comma token
    getLexer().Lex();

    // Parse next operand
    if (parseOperand(Operands))
      return true;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }

  getParser().Lex(); // Consume the EndOfStatement.
  return false;
}

// Returns false on succes
bool TriCoreAsmParser::ParseDirective(AsmToken DirectiveID) {
  StringRef IDVal = DirectiveID.getIdentifier();
  if (IDVal == ".code16") {
    if (!(getSTI().getFeatureBits()[TriCore::Only32BitInstructions]))
      clearFeatureBits(TriCore::Allow32BitInstructions, "allow-32bit");
    getParser().getStreamer().EmitAssemblerFlag(MCAF_Code16);
    return false;
  }
  if (IDVal == ".code32") {
    clearFeatureBits(TriCore::Allow16BitInstructions, "allow-16bit");
    getParser().getStreamer().EmitAssemblerFlag(MCAF_Code32);
    return false;
  }
  return true;
}

// classifying the expression, determining it's VK_TRICORE_* kind,
// also checks if it is a valid expression like foo, foo+<constant> etc.
bool TriCoreAsmParser::classifySymbolRef(const MCExpr *Expr,
                                         TriCoreMCExpr::VariantKind &Kind,
                                         int64_t &Addend) {
  Kind = TriCoreMCExpr::VK_TRICORE_None;
  Addend = 0;

  if (const TriCoreMCExpr *RE = dyn_cast<TriCoreMCExpr>(Expr)) {
    Kind = RE->getKind();
    Expr = RE->getSubExpr();
  }

  // It's a simple symbol reference or constant with no addend.
  if (isa<MCConstantExpr>(Expr) || isa<MCSymbolRefExpr>(Expr))
    return true;

  const MCBinaryExpr *BE = dyn_cast<MCBinaryExpr>(Expr);
  if (!BE)
    return false;

  // TODO: adding support for <constant>+<symbolref>
  if (!isa<MCSymbolRefExpr>(BE->getLHS()))
    return false;

  if (BE->getOpcode() != MCBinaryExpr::Add &&
      BE->getOpcode() != MCBinaryExpr::Sub)
    return false;

  // not letting the right hand side of the bin op to be a symbol
  if (isa<MCSymbolRefExpr>(BE->getRHS()))
    return true;

  // See if the addend is is a constant, otherwise there's more going
  // on here than we can deal with.
  auto AddendExpr = dyn_cast<MCConstantExpr>(BE->getRHS());
  if (!AddendExpr)
    return false;

  Addend = AddendExpr->getValue();
  if (BE->getOpcode() == MCBinaryExpr::Sub)
    Addend = -Addend;

  // It's some symbol reference + a constant addend
  return Kind != TriCoreMCExpr::VK_TRICORE_Invalid;
}

extern "C" void LLVMInitializeTriCoreAsmParser() {
  RegisterMCAsmParser<TriCoreAsmParser> X(getTheTriCoreTarget());
}
