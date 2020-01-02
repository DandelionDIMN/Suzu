#pragma once
#include "lexical.h"
#include "object.h"

namespace kagami {
  /* Message state level for Message class */
  enum StateLevel {
    kStateNormal,
    kStateError,
    kStateWarning
  };

  enum ParameterPattern {
    kParamAutoSize,
    kParamAutoFill,
    kParamNormal
  };

  enum StateCode {
    kCodeSuccess = 0,
    kCodeIllegalParam = -1,
    kCodeIllegalCall = -2,
    kCodeIllegalSymbol = -3,
    kCodeBadStream = -4,
    kCodeBadExpression = -5
  };

  class Message {
  private:
    bool invoking_msg_;
    StateLevel level_;
    string detail_;
    StateCode code_;
    shared_ptr<void> object_;
    size_t idx_;

  public:
    Message() :
      invoking_msg_(false),
      level_(kStateNormal), 
      detail_(""), 
      code_(kCodeSuccess), 
      idx_(0) {}

    Message(const Message &msg) :
      invoking_msg_(msg.invoking_msg_),
      level_(msg.level_),
      detail_(msg.detail_),
      code_(msg.code_),
      object_(msg.object_),
      idx_(msg.idx_) {}

    Message(const Message &&msg) :
      Message(msg) {}

    Message(StateCode code, string detail, StateLevel level = kStateNormal) :
      invoking_msg_(false),
      level_(level), 
      detail_(detail), 
      code_(code), 
      idx_(0) {}

    Message(string detail) :
      invoking_msg_(false),
      level_(kStateNormal), 
      code_(kCodeSuccess), 
      detail_(""), 
      object_(make_shared<Object>(detail)),
      idx_(0) {}

    Message &operator=(Message &msg) {
      invoking_msg_ = msg.invoking_msg_;
      level_ = msg.level_;
      detail_ = msg.detail_;
      code_ = msg.code_;
      object_ = msg.object_;
      idx_ = msg.idx_;
      return *this;
    }

    Message &operator=(Message &&msg) {
      return this->operator=(msg);
    }

    StateLevel GetLevel() const { return level_; }
    StateCode GetCode() const { return code_; }
    string GetDetail() const { return detail_; }
    size_t GetIndex() const { return idx_; }
    bool HasObject() const { return object_ != nullptr; }
    bool IsInvokingMsg() const { return invoking_msg_; }

    Object GetObj() const {
      if (object_ == nullptr) return Object();
      return *static_pointer_cast<Object>(object_);
    }

    Message &SetObject(Object &object) {
      object_ = make_shared<Object>(object);
      return *this;
    }

    Message &SetObject(bool value) {
      object_ = make_shared<Object>(
        make_shared<bool>(value), kTypeIdBool
      );
      return *this;
    }

    Message &SetObject(int64_t value) {
      object_ = make_shared<Object>(
        make_shared<int64_t>(value), kTypeIdInt
        );
      return *this;
    }

    Message &SetObject(double value) {
      object_ = make_shared<Object>(
        make_shared<double>(value), kTypeIdFloat
        );
      return *this;
    }

    Message &SetObject(string value) {
      object_ = make_shared<Object>(
        make_shared<string>(value), kTypeIdString
        );
      return *this;
    }

    Message &SetObject(Object &&object) {
      return this->SetObject(object);
    }

    Message &SetLevel(StateLevel level) {
      level_ = level;
      return *this;
    }

    Message &SetCode(StateCode code) {
      code_ = code;
      return *this;
    }

    Message &SetDetail(const string &detail) {
      detail_ = detail;
      return *this;
    }

    Message &SetIndex(const size_t index) {
      idx_ = index;
      return *this;
    }

    Message &SetInvokingSign() {
      invoking_msg_ = true;
      return *this;
    }

    void Clear() {
      level_ = kStateNormal;
      detail_.clear();
      detail_.shrink_to_fit();
      code_ = kCodeSuccess;
      object_.reset();
      idx_ = 0;
    }
  };
}