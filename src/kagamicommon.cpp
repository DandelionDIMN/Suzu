#include "kagamicommon.h"

namespace kagami {
  void Message::SetObject(Object &object, string id) {
    this->object = make_shared<Object>(object);
    this->code = kCodeObject;
    this->detail = id;
  }

  Object Message::GetObj() const {
    return *static_pointer_cast<Object>(this->object);
  }

  bool Kit::IsGenericToken(string target) {
    const auto head = target.front();
    if (!IsAlpha(head) && head != '_') return false;
    for (auto &unit : target) {
      if (!IsDigit(unit) && !IsAlpha(unit) && unit != '_') return false;
    }
    return true;
  }

  bool Kit::IsInteger(string target) {
    const auto head = target.front();
    if (head == '-' && target.size() == 1) return false;
    if (!IsDigit(head) && head != '-')     return false;
    for (size_t i = 1;i < target.size();++i) {
      if (!IsDigit(target[i])) return false;
    }
    return true;
  }

  bool Kit::IsDouble(string target) {
    const auto head = target.front();
    if (head == '-' && target.size() == 1) return false;
    if (!IsDigit(head) && head != '-')     return false;
    for(size_t i = 1;i < target.size();++i) {
      if (!IsDigit(target[i]) && target[i] != '.')       return false;
      if (i == target.size() - 1 && !IsDigit(target[i])) return false;
    }
    return true;
  }

  bool Kit::IsBlank(string target) {
    for (auto &unit : target) {
      if (unit != ' ' && unit != '\t' && unit != '\r' && unit != '\n') return false;
    }
    return true;
  }

  TokenTypeEnum Kit::GetTokenType(string src) {
    TokenTypeEnum type = TokenTypeEnum::T_NUL;
    if (src == kStrNull || src.empty())             type = TokenTypeEnum::T_NUL;
    else if (src == kStrTrue || src == kStrFalse)   type = TokenTypeEnum::T_BOOLEAN;
    else if (IsGenericToken(src))                   type = TokenTypeEnum::T_GENERIC;
    else if (IsInteger(src))                        type = TokenTypeEnum::T_INTEGER;
    else if (IsDouble(src))                         type = TokenTypeEnum::T_DOUBLE;
    else if (std::regex_match(src, kPatternSymbol)) type = TokenTypeEnum::T_SYMBOL;
    else if (IsBlank(src))                          type = TokenTypeEnum::T_BLANK;
    else if (IsString(src))                         type = TokenTypeEnum::T_STRING;
    return type;
  }

  bool Kit::FindInStringGroup(string target, string source) {
    bool result = false;
    auto methods = this->BuildStringVector(source);
    for (auto &unit : methods) {
      if (unit == target) {
        result = true;
        break;
      }
    }
    return result;
  }

  vector<string> Kit::BuildStringVector(string source) {
    vector<string> result;
    string temp;
    for (auto unit : source) {
      if (unit == '|') {
        result.push_back(temp);
        temp.clear();
        continue;
      }
      temp.append(1, unit);
    }
    if (temp != kStrEmpty) result.push_back(temp);
    return result;
  }

  char Kit::ConvertChar(char target) {
    char result;
    switch (target) {
    case 't':result = '\t'; break;
    case 'n':result = '\n'; break;
    case 'r':result = '\r'; break;
    default:result = target; break;
    }
    return result;
  }

  wchar_t Kit::ConvertWideChar(wchar_t target) {
    wchar_t result;
    switch (target) {
    case L't':result = L'\t'; break;
    case L'n':result = L'\n'; break;
    case L'r':result = L'\r'; break;
    default:result = target; break;
    }
    return result;
  }

  bool Kit::IsWideString(string target) {
    auto result = false;
    for (auto &unit : target) {
      if (unit < 0 || unit>127) {
        result = true;
        break;
      }
    }
    return result;
  }
}