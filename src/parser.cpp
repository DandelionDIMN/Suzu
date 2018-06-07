//BSD 2 - Clause License
//
//Copyright(c) 2017 - 2018, Suzu Nakamura
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions are met :
//
//*Redistributions of source code must retain the above copyright notice, this
//list of conditions and the following disclaimer.
//
//* Redistributions in binary form must reproduce the above copyright notice,
//this list of conditions and the following disclaimer in the documentation
//and/or other materials provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//  OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//#include <iostream>
#include <ctime>
#include "parser.h"

namespace kagami {
  namespace trace {
    vector<log_t> &GetLogger() {
      static vector<log_t> base;
      return base;
    }

    void Log(Message msg) {
      auto now = time(nullptr);
#if defined(_WIN32)
      char nowtime[30] = { ' ' };
      ctime_s(nowtime, sizeof(nowtime), &now);
      GetLogger().push_back(log_t(string(nowtime), msg));
#else
      string nowtime(ctime(&now));
      GetLogger().push_back(log_t(nowtime, msg));
#endif
    }

    bool IsEmpty() {
      return GetLogger().empty();
    }
  }

  namespace entry {
    vector<EntryProvider> &GetEntryBase() {
      static vector<EntryProvider> base;
      return base;
    }

    void Inject(EntryProvider provider) {
      GetEntryBase().emplace_back(provider);
    }

    EntryProvider Order(string id,string type = kTypeIdNull,int size = -1) {
      EntryProvider result;
      vector<EntryProvider> &base = GetEntryBase();
      string spectype = kTypeIdNull;
      //size_t argsize = 0;

      if (id == "+" || id == "-" || id == "*" || id == "/"
        || id == "==" || id == "<=" || id == ">=" || id == "!="
        || id == ">" || id == "<") {
        result = Order("binexp");
      }
      else if (id == "=") {
        result = Order(kStrSet);
      }
      else {
        for (auto &unit : base) {
          if (unit.GetId() == id) {
            if (type == unit.GetSpecificType() && (size == -1 || size == unit.GetArgumentSize())) {
              result = unit;
              break;
            }
          }
        }
      }
      return result;
    }

    int GetRequiredCount(string target) {
      auto result = -1;
      if (target == "+" || target == "-" || target == "*" || target == "/"
        || target == "==" || target == "<=" || target == ">=" || target == "!=") {
        result = Order("binexp").GetArgumentSize();
      }
      else {
        auto provider = Order(target);
        switch (provider.Good()) {
        case true:result = provider.GetArgumentSize(); break;
        case false:result = kCodeIllegalArgs; break;
        }
      }
      return result;
    }

    void RemoveByTemplate(ActivityTemplate temp) {
      auto &base = GetEntryBase();
      vector<EntryProvider>::iterator it;

      for (it = base.begin(); it != base.end(); ++it) {
        if (*it == temp) {
          break;
        }
      }
      if (it != base.end()) {
        base.erase(it);
      }
    }
  }

  ScriptProvider::ScriptProvider(const char *target) {
    string temp;
    end = false;
    health = false;
    current = 0;

    stream.open(target, std::ios::in);
    if (stream.good()) {
      while (!stream.eof()) {
        std::getline(stream, temp);
        if (!IsBlankStr(temp) && temp.front() != '#') {
          storage.push_back(Processor().Reset().Build(temp));
          if (!storage.back().IsHealth()) {
            //this->errorString = storage.back().getErrorString();
          }
        }
      }
      if (!storage.empty()) health = true;
    }
    stream.close();
  }

  Message ScriptProvider::Run(deque<string> res) {
    //using namespace entry;
    Message result;
    int code;
    stack<size_t> cycleNestStack;
    stack<size_t> cycleTailStack;
    stack<bool> conditionStack;
    stack<size_t> modeStack;
    size_t currentMode = kModeNormal;
    size_t nestHeadCount = 0;
    auto value = kStrEmpty;
    auto health = true;
    //auto modeStackLock = false;

    if (storage.empty()) {
      return result;
    }

    entry::CreateManager();

    //TODO:add custom function support
    if (!res.empty()) {
      if (res.size() != parameters.size()) {
        result.combo(kStrFatalError, kCodeIllegalCall, "wrong parameter count.");
        return result;
      }
      for (size_t count = 0; count < parameters.size(); count++) {
        //CreateObject(parameter.at(i), res.at(i), false);
      }
    }

    //Main state machine
    while (current < storage.size()) {
      if (!health) break;
      result = storage.at(current).Start(currentMode);
      value = result.GetValue();
      code = result.GetCode();

      if (value == kStrFatalError) {
        trace::Log(result);
        break;
      }
      if (value == kStrWarning) {
        trace::Log(result);
      }
      //TODO:return

      switch (code) {
      case kCodeConditionRoot:
        modeStack.push(currentMode);
        if (value == kStrTrue) {
          currentMode = kModeCondition;
          conditionStack.push(true);
        }
        else if (value == kStrFalse) {
          currentMode = kModeNextCondition;
          conditionStack.push(false);
        }
        else {
          health = false;
        }
        break;
      case kCodeConditionBranch:
        if (nestHeadCount > 0) break;
        if (!conditionStack.empty()) {
          if (conditionStack.top() == false && currentMode == kModeNextCondition
            && value == kStrTrue) {
            currentMode = kModeCondition;
            conditionStack.top() = true;
          }
        }
        else {
          //msg.combo
          health = false;
        }
        break;
      case kCodeConditionLeaf:
        if (nestHeadCount > 0) break;
        if (!conditionStack.empty()) {
          switch (conditionStack.top()) {
          case true:
            currentMode = kModeNextCondition;
            break;
          case false:
            conditionStack.top() = true;
            currentMode = kModeCondition;
            break;
          }
        }
        else {
          //msg.combo
          health = false;
        }
        break;
      case kCodeHeadSign:
        if (cycleNestStack.empty()) {
          modeStack.push(currentMode);
        }
        else {
          if (cycleNestStack.top() != current - 1) {
            modeStack.push(currentMode);
          }
        }
        if (value == kStrTrue) {
          currentMode = kModeCycle;
          if (cycleNestStack.empty()) {
            cycleNestStack.push(current - 1);
          }
          else if (cycleNestStack.top() != current - 1) {
            cycleNestStack.push(current - 1);
          }
        }
        else if (value == kStrFalse) {
          currentMode = kModeCycleJump;
          if (!cycleTailStack.empty()) {
            current = cycleTailStack.top();
          }
        }
        else {
          health = false;
        }
        break;
      case kCodeFillingSign:
        nestHeadCount++;
        break;
      case kCodeTailSign:
        if (nestHeadCount > 0) {
          nestHeadCount--;
          break;
        }
        if (currentMode == kModeCondition || currentMode == kModeNextCondition) {
          conditionStack.pop();
          currentMode = modeStack.top();
          modeStack.pop();
        }
        if (currentMode == kModeCycle || currentMode == kModeCycleJump) {
          switch (currentMode) {
          case kModeCycle:
            if (cycleTailStack.empty()) cycleTailStack.push(current - 1);
            current = cycleNestStack.top();
            break;
          case kModeCycleJump:
            currentMode = modeStack.top();
            modeStack.pop();
            cycleNestStack.pop();
            cycleTailStack.pop();
            break;
          default:break;
          }
        }
      default:break;
      }
      ++current;
    }

    entry::DisposeManager();

    return result;
  }

  Processor &Processor::Build(string target) {
    Kit kit;
    string current, data, forward;
    auto exemptBlankChar = true;
    auto stringProcessing = false;
    char currentChar;
    auto forwardChar = ' ';
    vector<string> origin, output;
    size_t count, head = 0, tail = 0, nest = 0;

    const auto toString = [](char t) ->string {return string().append(1, t); };

    health = true;

    if (target.front() == '#') return *this;

    //PreProcessing
    for (count = 0; count < target.size(); ++count) {
      currentChar = target[count];
      if (kit.GetDataType(toString(currentChar)) != kTypeBlank
        && exemptBlankChar) {
        head = count;
        exemptBlankChar = false;
      }
      if (currentChar == '\'' && forwardChar != '\\') stringProcessing = !stringProcessing;
      if (!stringProcessing && currentChar == '#') {
        tail = count;
        break;
      }
      forwardChar = target.at(count);
    }

    if (tail > head) data = target.substr(head, tail - head);
    else data = target.substr(head, target.size() - head);

    while (kit.GetDataType(toString(data.back())) == kTypeBlank) data.pop_back();
    
    //Spilt
    forwardChar = 0;
    for (count = 0; count < data.size(); ++count) {
      currentChar = data[count];
      if (currentChar == '\'' && forwardChar != '\\') {
        if (kit.GetDataType(current) == kTypeBlank) {
          current.clear();
        }
        stringProcessing = !stringProcessing;
      }
      current.append(1, currentChar);
      if (kit.GetDataType(current) != kTypeNull) {
        forwardChar = data[count];
        continue;
      }
      else {
        current = current.substr(0, current.size() - 1);
        if (kit.GetDataType(current) == kTypeBlank) {
          if (stringProcessing) origin.push_back(current);
          current.clear();
          current.append(1, currentChar);
        }
        else {
          origin.push_back(current);
          current.clear();
          current.append(1, currentChar);
        }
      }
      forwardChar = data[count];
    }

    auto type = kit.GetDataType(current);
    if (type != kTypeNull && type != kTypeBlank) origin.push_back(current);
    current.clear();

    //third cycle
    stringProcessing = false;
    //type = kTypeNull;
    for (count = 0; count < origin.size(); ++count) {
      current = origin.at(count);
      if (!stringProcessing) {
        //Analyzing
        if (current == "(" || current == "[") nest++;
        if (current == ")" || current == "]") nest--;
        if (current == "[") {
          if (kit.GetDataType(forward) != kGenericToken && forward != "]" && forward != ")") {
            errorString = "Illegal subscript operation.";
            nest = 0;
            health = false;
            break;
          }
        }
        if (current == ",") {
          if (kit.GetDataType(forward) == kTypeSymbol &&
            forward != "]" && forward != ")" &&
            forward != "++" && forward != "--" &&
            forward != "'") {
            errorString = "Illegal comma location.";
            health = false;
            break;
          }
        }
      }
      if (current == "'" && forward != "\\") {
        if (stringProcessing) {
          output.back().append(current);
        }
        else {
          output.push_back(kStrEmpty);
          output.back().append(current);
        }
        stringProcessing = !stringProcessing;
      }
      else {
        if (stringProcessing) {
          output.back().append(current);
        }
        else {
          if ((current == "+" || current == "-") && !output.empty()) {
            if (output.back() == current) {
              output.back().append(current);
            }
            else {
              output.push_back(current);
            }
          }
          else if (current == "=") {
            if (output.back() == "<" || output.back() == ">" || output.back() == "=" || output.back() == "!") {
              output.back().append(current);
            }
            else {
              output.push_back(current);
            }
          }
          else {
            output.push_back(current);
          }
        }
      }

      forward = origin.at(count);
    }

    if (stringProcessing == true) {
      errorString = "Quotation mark is missing.";
      health = false;
    }
    if (nest > 0) {
      errorString = "Bracket/Square Bracket is missing";
      health = false;
    }

    raw = output;

    return *this;
  }

  // ReSharper disable CppMemberFunctionMayBeStatic
  int Processor::GetPriority(const string target) const {
    // ReSharper restore CppMemberFunctionMayBeStatic
    int priority;
    if (target == "=" || target == "var") {
      priority = 0;
    }
    else if (target == "==" || target == ">=" || target == "<="
      || target == "!=" || target == "<" || target == ">") {
      priority = 1;
    }
    else if (target == "+" || target == "-") {
      priority = 2;
    }
    else if (target == "*" || target == "/" || target == "\\") {
      priority = 3;
    }
    else {
      priority = 4;
    }
    return priority;
  }

  Object *Processor::GetObj(string name) {
    Object *result = new Object();
    if (name.substr(0, 2) == "__") {
      const auto it = lambdamap.find(name);
      if (it != lambdamap.end()) result = &(it->second);
    }
    else {
      const auto ptr = entry::FindObject(name);
      if (ptr != nullptr) result = ptr;
    }
    return result;
  }

  vector<string> Processor::Spilt(string target) {
    auto temp = kStrEmpty;
    auto start = false;
    for (auto &unit : target) {
      if (unit == ':') {
        start = true;
        continue;
      }
      if (start) {
        temp.append(1, unit);
      }
    }
    auto result = Kit().BuildStringVector(temp);
    return result;
  }

  string Processor::GetHead(string target) {
    string result = kStrEmpty;
    for (auto &unit : target) {
      if (unit == ':') break;
      else result.append(1, unit);
    }
    return result;
  }

  bool Processor::Assemble(Message &msg) {
    Kit kit;
    const Attribute strAttr(type::GetTemplate(kTypeIdRawString)->GetMethods(), false);
    Attribute attribute;
    Object temp, *origin;
    size_t size, count;
    auto providerSize = -1;
    int argMode, priority;
    auto health = true;
    const auto id = GetHead(symbol.back());
    auto providerType = kTypeIdNull;
    auto tags = Spilt(symbol.back());
    vector<string> args;
    deque<string> tokens;
    ObjectMap map;

    const auto getName = [](string target) ->string {
      if (target.front() == '&' || target.front() == '%')
        return target.substr(1, target.size() - 1);
      else return target;
    };

    if (id == kStrNop) return true;

    if (!tags.empty()) {
      providerType = tags.front();
      if (tags.size() > 1) {
        providerSize = stoi(tags.at(1));
      }
      else {
        providerSize = -1;
      }
    }

    auto provider = entry::Order(id, providerType, providerSize);

    if (!provider.Good()) {
      msg.combo(kStrFatalError, kCodeIllegalCall, "Activity not found.");
      return false;
    }
    else {
      size = provider.GetArgumentSize();
      argMode = provider.GetArgumentMode();
      priority = provider.GetPriority();
      args = provider.GetArguments();
    }


    if (!disable_set_entry) {
      count = size;
      while (count > 0 && !item.empty() && item.back() != "(") {
        tokens.push_front(item.back());
        item.pop_back();
        count--;
      }
      if (!item.empty() && item.back() == "(") item.pop_back();
    }
    else {
      while (item.back() != "," && !item.empty()) {
        tokens.push_front(item.back());
        item.pop_back();
      }
    }

    if (argMode == kCodeNormalArgs && (tokens.size() < size || tokens.size() > size)) {
      msg.combo(kStrFatalError, kCodeIllegalArgs, "Parameter doesn't match expected count.(01)");
      health = false;
    }
    else if (argMode == kCodeAutoFill && tokens.size() < 1) {
      msg.combo(kStrFatalError, kCodeIllegalArgs, "You should provide at least one parameter.(02)");
      health = false;
    }
    else {
      for (count = 0; count < size; count++) {
        if (tokens.size() - 1 < count) {
          map.insert(pair<string, Object>(getName(args.at(count)), Object()));
        }
        else {
          auto tokenType = kit.GetDataType(tokens.at(count));
          switch (tokenType) {
          case kGenericToken:
            if (args.at(count).front() == '&') {
              origin = GetObj(tokens.at(count));
              temp.Ref(*origin);
            }
            else if (args.at(count).front() == '%') {
              temp.Manage(tokens.at(count), kTypeIdRawString, kit.BuildAttrStr(strAttr));
            }
            else {
              temp = *GetObj(tokens.at(count));
            }
            break;
          default:
            temp.Manage(tokens.at(count), kTypeIdRawString, kit.BuildAttrStr(strAttr));
            break;
          }
          map.insert(pair<string,Object>(getName(args.at(count)), temp));
          temp.Clear();
        }
      }

      switch (priority) {
      case kFlagOperatorEntry:
        map.insert(pair<string, Object>(kStrOperator, Object()
          .Manage(symbol.back(), kTypeIdRawString, kit.BuildAttrStr(strAttr))));
        break;
      case kFlagMethod:
        origin = GetObj(item.back());
        map.insert(pair<string, Object>(kStrObject, Object()
          .Ref(*origin)));
        item.pop_back();
        break;
      default:
        break;
      }
    }
    
    if (health) {
      switch (mode) {
      case kModeCycleJump:
        if (id == kStrEnd) msg = provider.Start(map);
        else if (symbol.front() == kStrIf || symbol.front() == kStrWhile) {
          msg.combo(kStrRedirect, kCodeFillingSign, kStrTrue);
        }
        break;
      case kModeNextCondition:
        if (symbol.front() == kStrIf || symbol.front() == kStrWhile) {
          msg.combo(kStrRedirect, kCodeFillingSign, kStrTrue);
        }
        else if (id == kStrElse || id == kStrEnd) msg = provider.Start(map);
        else if (symbol.front() == kStrElif) msg = provider.Start(map);
        else msg.combo(kStrEmpty, kCodeSuccess, kStrEmpty);
        break;
      case kModeNormal:
      case kModeCondition:
      default:
        msg = provider.Start(map);
        break;
      }

      if (msg.GetCode() == kCodeObject) {
        temp = msg.GetObj();
        auto tempId = msg.GetDetail() + to_string(lambdaObjectCount); //detail start with "__"
        item.push_back(tempId); 
        lambdamap.insert(pair<string, Object>(tempId, temp));
        ++lambdaObjectCount;
      }
      else if (msg.GetValue() == kStrRedirect && (msg.GetCode() == kCodeSuccess || msg.GetCode() == kCodeFillingSign)) {
        item.push_back(msg.GetDetail());
      }

      health = (msg.GetValue() != kStrFatalError && msg.GetValue() != kStrWarning);
      symbol.pop_back();
    }
    return health;
  }

  void Processor::EqualMark() {
    switch (symbol.empty()) {
    case true:
      symbol.push_back(currentToken); 
      break;
    case false:
      switch (define_line) {
      case true:define_line = false; break;
      case false:symbol.push_back(currentToken); break;
      }
      break;
    }
  }

  void Processor::Comma() {
    if (symbol.back() == kStrVar) {
      disable_set_entry = true;
    }
    if (disable_set_entry) {
      symbol.push_back(kStrVar);
      item.push_back(currentToken);
    }
  }

  bool Processor::LeftBracket(Message &msg) {
    bool result = true;
    if (symbol.back() == kStrVar) {
      msg.combo(kStrFatalError, kCodeIllegalCall, "Illegal pattern of definition.");
      result = false;
    }
    else {
      symbol.push_back(currentToken);
      item.push_back(currentToken);
    }
    return result;
  }

  bool Processor::RightBracket(Message &msg) {
    bool result = true;
    while (symbol.back() != "(" && !symbol.empty()) {
      result = Assemble(msg);
      if (!result) break;
    }

    if (result) {
      if (symbol.back() == "(") symbol.pop_back();
      result = Assemble(msg);
    }

    return result;
  }

  void Processor::LeftSquareBracket() {
    if (item.back().substr(0, 2) == "__") {
      operatorTargetType = lambdamap.find(item.back())->second.GetTypeId();
    }
    else {
      operatorTargetType = entry::FindObject(item.back())->GetTypeId();
    }
    item.push_back(currentToken);
    symbol.push_back(currentToken);
    subscript_processing = true;
  }

  bool Processor::RightSquareBracket(Message &msg) {
    bool result;
    deque<string> container;
    if (!subscript_processing) {
      //msg.combo
      result = false;
    }
    else {
      subscript_processing = false;
      while (symbol.back() != "[" && !symbol.empty()) {
        result = Assemble(msg);
        if (!result) break;
      }
      if (symbol.back() == "[") symbol.pop_back();
      while (item.back() != "[" && !item.empty()) {
        container.push_back(item.back());
        item.pop_back();
      }
      if (item.back() == "[") item.pop_back();
      if (!container.empty()) {
        switch (container.size()) {
        case 1:symbol.push_back("at:" + operatorTargetType + "|1"); break;
        case 2:symbol.push_back("at:" + operatorTargetType + "|2"); break;
        default:break;
        }

        while (!container.empty()) {
          item.push_back(container.back());
          container.pop_back();
        }

        result = Assemble(msg);
      }
      else {
        //msg.combo
        result = false;
      }
    }

    Kit().CleanupDeque(container);
    return result;
  }

  void Processor::Dot() {
    dot_operator = true;
  }

  void Processor::OtherSymbols() {
    if (symbol.empty()) {
      symbol.push_back(currentToken);
    }
    else if (GetPriority(currentToken) < GetPriority(symbol.back()) && symbol.back() != "(" && symbol.back() != "[") {
      auto j = symbol.size() - 1;
      auto k = item.size();
      while (symbol.at(j) != "(" && symbol.back() != "[" && GetPriority(currentToken) < GetPriority(symbol.at(j))) {
        if (k == item.size()) { k -= entry::GetRequiredCount(symbol.at(j)); }
        else { k -= entry::GetRequiredCount(symbol.at(j)) - 1; }
        --j;
      }
      symbol.insert(symbol.begin() + j + 1, currentToken);
      nextInsertSubscript = k;
      insert_btn_symbols = true;

      // ReSharper disable once CppAssignedValueIsNeverUsed
      j = 0;
      // ReSharper disable once CppAssignedValueIsNeverUsed
      k = 0;
    }
    else {
      symbol.push_back(currentToken);
    }
  }

  bool Processor::FunctionAndObject(Message &msg) {
    Kit kit;
    bool result = true;
    bool function = false;

    if (currentToken == kStrVar) define_line = true;
    if (currentToken == kStrDef) function_line = true;

    if (dot_operator) {
      auto id = entry::GetTypeId(item.back());
      if (kit.FindInStringVector(currentToken, type::GetTemplate(id)->GetMethods())) {
        auto provider = entry::Order(currentToken, id);
        if (provider.Good()) {
          symbol.push_back(currentToken + ':' + id);
          function = true;
        }
        else {
          //TODO:???
        }
        
        dot_operator = false;
      }
      else {
        msg.combo(kStrFatalError, kCodeIllegalCall, "No such method/member in " + id + ".");
        result = false;
      }
    }
    else {
      switch (function_line) {
      case true:
        item.push_back(currentToken);
        break;
      case false:
        switch (entry::Order(currentToken).Good()) {
        case true:symbol.push_back(currentToken); function = true; break;
        case false:item.push_back(currentToken); break;
        }
        break;
      }
    }

    if (!define_line && function && nextToken != "(" && currentToken != kStrEnd) {
      errorString = "Bracket after function is missing";
      this->health = false;
      result = false;
    }
    if (function_line && forwardToken == kStrDef && nextToken != "(" && currentToken != kStrEnd) {
      errorString = "Illegal declare of function.";
      this->health = false;
      result = false;
    }

    return result;
  }

  void Processor::OtherTokens() {
    switch (insert_btn_symbols) {
    case true:
      item.insert(item.begin() + nextInsertSubscript, currentToken);
      insert_btn_symbols = false;
      break;
    case false:
      item.push_back(currentToken);
      break;
    }
  }

  void Processor::FinalProcessing(Message &msg) {
    while (symbol.empty() != true) {
      if (symbol.back() == "(" || symbol.back() == ")") {
        msg.combo(kStrFatalError, kCodeIllegalSymbol, "Another bracket expected.");
        break;
      }
      Assemble(msg);
    }
  }

  Message Processor::Start(size_t mode) {
    using namespace entry;
    const auto size = raw.size();
    int unitType;
    Kit kit;
    Message result;
    auto state = true;

    if (this->health == false) {
      result.combo(kStrFatalError, kCodeBadExpression, errorString);
      return result;
    }

    lambdaObjectCount = 0;
    nextInsertSubscript = 0;
    this->mode = mode;
    operatorTargetType = kTypeIdNull;
    comma_exp_func = false,
    insert_btn_symbols = false,
    disable_set_entry = false,
    dot_operator = false,
    subscript_processing = false;
    function_line = false;
    define_line = false;

    for (size_t i = 0; i < size; ++i) {
      if (!this->health) {
        result.combo(kStrFatalError, kCodeBadExpression, errorString);
        break;
      }
      if(!state) break;
      currentToken = raw.at(i);
      if (i < size - 1) nextToken = raw.at(i + 1);
      else nextToken = kStrNull;
      unitType = kit.GetDataType(raw.at(i));
      result.combo(kStrEmpty, kCodeSuccess, kStrEmpty);
      if (unitType == kTypeSymbol) {
        if (raw[i] == "=") EqualMark();
        else if (raw[i] == ",") Comma();
        else if (raw[i] == "[") LeftSquareBracket();
        else if (raw[i] == ".") Dot();
        else if (raw[i] == "(") state = LeftBracket(result);
        else if (raw[i] == "]") state = RightSquareBracket(result);
        else if (raw[i] == ")") state = RightBracket(result);
        else if (raw[i] == "++"); //
        else if (raw[i] == "--"); //
        else OtherSymbols();
      }
      else if (unitType == kGenericToken) state = FunctionAndObject(result);
      else if (unitType == kTypeNull) {
        result.combo(kStrFatalError, kCodeIllegalArgs, "Illegal token.");
        state = false;
      }
      else OtherTokens();
      forwardToken = currentToken;
    }

    if (state == true) FinalProcessing(result);

    kit.CleanupDeque(item).CleanupDeque(symbol);
    lambdamap.clear();


    return result;
  }

  Message EntryProvider::Start(ObjectMap &map) const {
    Message result;
    switch (this->Good()) {
    case true:result = activity(map); break;
    case false:result.combo(kStrFatalError, kCodeIllegalCall, "Illegal entry.");; break;
    }
    return result;
  }
}

