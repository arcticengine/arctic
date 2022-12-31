// The MIT License (MIT)
//
// Copyright (c) 2021 Huldra
// Copyright (c) 2020 Luke de Oliveira <lukedeo@ldo.io>
// Copyright (c) 2017 Romain Sylvian
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//

/*
Intended use example:

int main(int argc, char const *argv[]) {
  OptionParser p;
  p.AddOption("--help", "-h")
      .Help("Display this help message and exit.");
  p.AddOption("--number", "-n")
      .Help("A number to do something with")
      .DefaultValue(42)
      .ArgumentType<int>()
      .SingleArgument();
  p.AddOption("--file")
      .Help("pass a list of files to load.")
      .MultipleArguments()
      .Required();
  p.AddOption("--save", "-s")
      .Help("Pass a file to save.")
      .Dest("")
      .SingleArgument();
  bool is_ok = p.ParseArgcArgv(argc, argv);
  if (!is_ok) {
    std::cout << p.GetLastError() << std::endl;
    return 1;
  }
  if (p.HasValue("help")) {
    std::cout << p.Help() << std::endl;
    return 0;
  }
  if (!p.CheckForMissingArgs()) {
    std::cout << p.GetLastError() << std::endl;
    return 1;
  }
  if (p.HasValue("number")) {
    int number_passed = p.GetValue<int>("number", 0);
  }
  if (p.HasValue("file")) {
    std::vector<std::string> filenames = p.GetValues<std::string>("file", std::string());
  }
  return 0;
}

After you p.AddOption("--foo", "-f"), you can chain additional statements.
These include:

.DefaultValue(...), to set a sensible default.
.ArgumentType<T>(), to set argument type.
.Dest(...), to set the metavar (i.e., the key to retrieve the value)
.Help(...), to set a help string for that argument.
.Required(), make a specific command line flag required for valid invocation.

  One of:
.NoArguments()
.SingleArgument()
.MultipleArguments(), can't be followed by short options like -s

*/

#ifndef ENGINE_OPTIONPARSER_H_
#define ENGINE_OPTIONPARSER_H_

#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>
#include <vector>

#include "engine/arctic_platform_fatal.h"

namespace arctic {

// The utils::* namespace contains general utilities not necessarily useful
// outside the main scope of the library
namespace utils {

std::vector<std::string> SplitStr(std::string s,
    const std::string &delimiter = " ") {
  size_t pos = 0;
  size_t delimiter_length = delimiter.length();
  std::vector<std::string> vals;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    vals.push_back(s.substr(0, pos));
    s.erase(0, pos + delimiter_length);
  }
  vals.push_back(s);
  return vals;
}

std::string StitchStr(const std::vector<std::string> &text,
    unsigned max_per_line, const std::string &leading_str) {
  std::vector<std::string> result;
  std::string line_value;
  for (const std::string &token : text) {
    if (line_value.empty()) {
      line_value = (leading_str + token);
      continue;
    }
    std::string hypothetical_line = line_value;
    hypothetical_line.append(" " + token);

    if (hypothetical_line.size() > max_per_line) {
      // In this case, we were better off before
      result.emplace_back(line_value);
      line_value = (leading_str + token);
    } else {
      line_value = hypothetical_line;
    }
  }
  // Collect the last line since we don't track indices in the loop proper.
  result.emplace_back(line_value);
  return std::accumulate(
      result.begin() + 1, result.end(), result.at(0),
      [](std::string &s, const std::string &piece) -> std::string {
        return s + "\n" + piece;
      });
}

}  // namespace utils

struct ValueChecker {
  virtual bool Check(const std::string &value) const {
    return true;
  }
};

template<typename T>
struct ValueTypeChecker : public ValueChecker {
  bool Check(const std::string &value) const override {
    std::stringstream ss(value);
    T res;
    ss >> res;
    if (ss.fail()) {
      return false;
    }
    if (ss.peek() == std::iostream::traits_type::eof()) {
      return true;
    }
    return false;
  }
};

// Enums for Option config
enum StorageMode { STORE_TRUE = 0, STORE_VALUE, STORE_MULT_VALUES };
enum OptionType { LONG_OPT = 0, SHORT_OPT, POSITIONAL_OPT, EMPTY_OPT };

// Option class definition
class Option {
private:
  bool found_ = false;
  bool required_ = false;
  StorageMode mode_ = STORE_TRUE;
  std::string help_ = "";
  std::string dest_ = "";
  std::string default_value_ = "";
  std::string metavar_ = "";

  std::string short_flag_ = "";
  std::string long_flag_ = "";
  std::string pos_flag_ = "";

  std::string last_error_ = "";
  std::shared_ptr<ValueChecker> value_checker_ = nullptr;

public:
  Option() = default;

  std::string HelpDoc();

  std::string &ShortFlag() { return short_flag_; }
  std::string &LongFlag() { return long_flag_; }
  std::string &PosFlag() { return pos_flag_; }

  bool GetFound() { return found_; }
  Option &SetFound(bool found) {
    found_ = found;
    return *this;
  }

  StorageMode GetMode() { return mode_; }
  Option &NoArguments() {
    mode_ = StorageMode::STORE_TRUE;
    return *this;
  }
  Option &SingleArgument() {
    mode_ = StorageMode::STORE_VALUE;
    return *this;
  }
  Option &MultipleArguments() {
    mode_ = StorageMode::STORE_MULT_VALUES;
    return *this;
  }

  bool CheckValue(const std::string &value) {
    if (!value_checker_) {
      return true;
    }
    return value_checker_->Check(value);
  }

  template<typename T>
  Option &ArgumentType() {
    value_checker_ = std::make_shared<ValueTypeChecker<T>>();
    return *this;
  }

  bool GetRequired() { return required_; }
  Option &Required() {
    required_ = true;
    return *this;
  }

  std::string GetMetavar() {
    std::string formatted_metavar;
    if (!metavar_.empty()) {
      if (mode_ == STORE_TRUE) {
        return "";
      }
      formatted_metavar = metavar_;
    } else {
      for (const auto &cand :
           {dest_, pos_flag_, long_flag_, std::string("ARG")}) {
        if (!cand.empty()) {
          formatted_metavar = cand.substr(cand.find_first_not_of('-'));
          std::transform(formatted_metavar.begin(), formatted_metavar.end(),
                         formatted_metavar.begin(), ::toupper);
          break;
        }
      }
    }
    if (mode_ == STORE_MULT_VALUES) {
      formatted_metavar = (formatted_metavar + "1 [" + formatted_metavar +
                           "2, " + formatted_metavar + "3, ...]");
    }
    return formatted_metavar;
  }

  Option &Metavar(const std::string &mvar) {
    metavar_ = mvar;
    return *this;
  }

  std::string GetHelp() { return help_; }
  Option &Help(const std::string &help) {
    help_ = help;
    return *this;
  }
  std::string GetDest() { return dest_; }
  Option &Dest(const std::string &dest) {
    dest_ = dest;
    return *this;
  }
  std::string GetDefaultValue() { return default_value_; }
  Option &DefaultValue(const std::string &default_value) {
    default_value_ = default_value;
    return *this;
  }
  Option &DefaultValue(const char *default_value) {
    default_value_ = std::string(default_value);
    return *this;
  }
  template <typename T> Option &DefaultValue(const T &default_value) {
    default_value_ = std::to_string(default_value);
    return *this;
  }

  static OptionType GetType(std::string opt);
  static bool GetDestination(const std::string &first_option,
      const std::string &second_option, std::string *out_dest);
  static bool ValidateOptionTypes(const OptionType &first_option_type,
      const OptionType &second_option_type, std::string *out_last_error);
};

// Non-inline definitions for Option methods
std::string Option::HelpDoc() {
  std::string h = "    ";
  if (!long_flag_.empty()) {
    h += long_flag_;
    if (!short_flag_.empty()) {
      h += ", ";
    }
  }
  if (!short_flag_.empty()) {
    h += short_flag_;
  }
  if (!pos_flag_.empty()) {
    h += pos_flag_;
  }

  auto arg_buf = std::max(h.length() + 1, static_cast<unsigned long>(25));
  auto help_str = utils::StitchStr(utils::SplitStr(help_), (Ui32)arg_buf + 50,
                                    std::string(arg_buf, ' '));
  size_t char_buf_size = h.length() + help_str.length() + 100;
  char char_buf[char_buf_size];
  snprintf(char_buf, char_buf_size, ("%-" + std::to_string(arg_buf) + "s%s\n").c_str(),
          h.c_str(), help_str.substr(arg_buf).c_str());
  return std::string(char_buf);
}

OptionType Option::GetType(std::string opt) {
  if (opt.empty()) {
    return OptionType::EMPTY_OPT;
  }
  if (opt.size() == 2) {
    if (opt[0] == '-') {
      return OptionType::SHORT_OPT;
    }
  }
  if (opt.size() > 2) {
    if (opt[0] == '-' && opt[1] == '-') {
      return OptionType::LONG_OPT;
    }
  }
  return OptionType::POSITIONAL_OPT;
}

bool Option::ValidateOptionTypes(const OptionType &first_option_type,
    const OptionType &second_option_type, std::string *out_last_error) {

  if (first_option_type == OptionType::EMPTY_OPT) {
    *out_last_error = "Parser inconsistency: Cannot have first option be empty.";
    return false;
  }
  if (first_option_type == OptionType::POSITIONAL_OPT &&
      second_option_type != OptionType::EMPTY_OPT) {
    *out_last_error = "Parser inconsistency: Positional arguments can only have"
      " one option, found non-empty second option.";
    return false;
  }
  if (second_option_type == OptionType::POSITIONAL_OPT) {
    *out_last_error = "Parser inconsistency: Cannot have second option be a positional option.";
    return false;
  }
  return true;
}

bool Option::GetDestination(const std::string &first_option,
    const std::string &second_option, std::string *out_dest) {
  std::string dest;
  OptionType first_opt_type = Option::GetType(first_option);
  OptionType second_opt_type = Option::GetType(second_option);

  std::string last_error;
  bool is_ok = ValidateOptionTypes(first_opt_type, second_opt_type, &last_error);
  if (!is_ok) {
    *out_dest = last_error;
    return false;
  }

  if (first_opt_type == OptionType::LONG_OPT) {
    dest = first_option.substr(2);
  } else if (second_opt_type == OptionType::LONG_OPT) {
    dest = second_option.substr(2);
  } else {
    if (first_opt_type == OptionType::SHORT_OPT) {
      dest = first_option.substr(1) + "_option";
    } else if (second_opt_type == OptionType::SHORT_OPT) {
      dest = second_option.substr(1) + "_option";
    } else {
      if (first_opt_type == OptionType::POSITIONAL_OPT &&
          second_opt_type == OptionType::EMPTY_OPT) {
        dest = first_option;
      } else {
        *out_dest = "Parser inconsistency error.";
        return false;
      }
    }
  }

  *out_dest = dest;
  return true;
}

// OptionParser class definition
class OptionParser {
public:
  explicit OptionParser(std::string description = "")
      : options_(0), description_(std::move(description)),
        pos_arg_count_(1) {
  }

  ~OptionParser() = default;

  bool ParseArgcArgv(Ui32 argc, char const *argv[]);

  Option &AddOption(const std::string &first_option,
      const std::string &second_option = "") {
    return AddOptionInternal(first_option, second_option);
  }
  
  bool HasValue(const std::string &key) {
    auto it = option_idx_.find(key);
    Check(it != option_idx_.end(),
        "Value can not be checked for the undefined flag: ", key.c_str());
    return options_[it->second].GetFound();
  }

  // Provide all template specializations for get_value<T>(keyName)

  template<typename T>
    const T GetValue(const std::string &key, T default_value) {
      auto it = values_.find(key);
      Check(it != values_.end(),
          "Value can not be obtained for the undefined flag: ", key.c_str());
      Check(it->second.size(),
          "No value specified for flag: ", key.c_str());
      std::stringstream ss(it->second[0]);
      T res;
      ss >> res;
      if (ss.fail()) {
        return default_value;
      }
      if (ss.peek() == std::iostream::traits_type::eof()) {
        return res;
      }
      return default_value;
    }

  template<typename T>
  std::vector<T> GetValues(const std::string &key, T default_value) {
    auto it = values_.find(key);
    Check(it != values_.end(),
        "Value can not be obtained for the undefined flag: ", key.c_str());
    std::vector<T> v;
    for (auto &entry : it->second) {
      std::stringstream ss(entry);
      T res;
      ss >> res;
      if (ss.fail()) {
        v.push_back(default_value);
      } else {
        if (ss.peek() == std::iostream::traits_type::eof()) {
          v.push_back(res);
        } else {
          v.push_back(default_value);
        }
      }
    }
    return std::move(v);
  }

  std::string Help();

  bool CheckForMissingArgs();
  std::string get_last_error() { return last_error_; }
private:
  Option &AddOptionInternal(const std::string &first_option,
      const std::string &second_option);

  bool GetValueArg(std::vector<std::string> &arguments, Ui32 &arg,
      Option &opt, std::string &flag);

  bool TryToGetOpt(std::vector<std::string> &arguments, Ui32 &arg,
      Option &option, std::string &flag);

  std::map<std::string, std::vector<std::string>> values_;
  int pos_arg_count_;
  std::vector<Option> options_;
  std::string prog_name_;
  std::string description_;
  std::vector<std::string> pos_option_names_;
  std::map<std::string, Ui32> option_idx_;
  bool is_ok_ = true;
  std::string last_error_ = "";
};


Option &OptionParser::AddOptionInternal(const std::string &first_option,
    const std::string &second_option) {
  options_.resize(options_.size() + 1);
  Option &opt = options_.back();
  OptionType first_option_type = Option::GetType(first_option);
  OptionType second_option_type = Option::GetType(second_option);

  std::string dest;
  bool is_ok = Option::GetDestination(first_option, second_option, &dest);
  if (!is_ok) {
    is_ok_ = false;
    last_error_ = dest;
    return opt;
  }
  opt.Dest(dest);

  if (first_option_type == OptionType::LONG_OPT) {
    opt.LongFlag() = first_option;
  } else if (second_option_type == OptionType::LONG_OPT) {
    opt.LongFlag() = second_option;
  }

  if (first_option_type == OptionType::SHORT_OPT) {
    opt.ShortFlag() = first_option;
  } else if (second_option_type == OptionType::SHORT_OPT) {
    opt.ShortFlag() = second_option;
  }
  if (first_option_type == OptionType::POSITIONAL_OPT) {
    opt.PosFlag() = first_option;
    pos_arg_count_ += 1;
    pos_option_names_.push_back(first_option);
  }
  return opt;
}

bool OptionParser::GetValueArg(std::vector<std::string> &arguments,
    Ui32 &arg, Option &opt, std::string &flag) {
  std::string val;
  values_[opt.GetDest()].clear();

  if (arguments[arg].size() > flag.size()) {
    auto search_pt = arguments[arg].find_first_of(' ');
    if (search_pt == std::string::npos) {
      std::stringstream str;
      str << "Error, long options (" << flag 
        << ") require a space before a value.";
      last_error_ = str.str();
      is_ok_ = false;
      return false;
    }
    std::vector<std::string> vals = utils::SplitStr(
        arguments[arg].substr(search_pt + 1));
    for (const std::string &v : vals) {
      if (opt.CheckValue(v)) {
        values_[opt.GetDest()].push_back(v);
      } else {
        std::stringstream str;
        str << "Error, option '" << flag 
          << "' value '" << v << "' could not be parsed.";
        last_error_ = str.str();
        is_ok_ = false;
        return false;
      }
    }
  } else {
    if (arg + 1 >= arguments.size()) {
      if (opt.GetDefaultValue().empty()) {
        std::stringstream str;
        str << "error, flag '" << flag
          << "' requires an argument.";
        last_error_ = str.str();
        is_ok_ = false;
        return false;
      }
      if (values_[opt.GetDest()].empty()) {
        val = opt.GetDefaultValue();
      }
    } else {
      if (arguments[arg + 1][0] == '-' && arguments[arg + 1][1] == '-') {
        if (opt.GetDefaultValue().empty()) {
          std::stringstream str;
          str << "error, flag '" << flag
            << "' requires an argument, but is followed by another flag.";
          last_error_ = str.str();
          is_ok_ = false;
          return false;
        }
        if (values_[opt.GetDest()].empty()) {
          val = opt.GetDefaultValue();
        }
      }
    }
  }

  if (!val.empty()) {
    if (opt.CheckValue(val)) {
      values_[opt.GetDest()].push_back(val);
    } else {
      std::stringstream str;
      str << "Error, option '" << flag 
        << "' value '" << val << "' could not be parsed.";
      last_error_ = str.str();
      is_ok_ = false;
      return false;
    }
    return true;
  }
  int arg_distance = 0;
  while (!(arguments[arg + 1][0] == '-' && arguments[arg + 1][1] == '-')) {
    arg++;
    if (arg_distance && (opt.GetMode() != StorageMode::STORE_MULT_VALUES)) {
      break;
    }
    arg_distance++;
    if (opt.CheckValue(arguments[arg])) {
      values_[opt.GetDest()].push_back(arguments[arg]);
    } else {
      std::stringstream str;
      str << "Error, option '" << flag 
        << "' value '" << arguments[arg] << "' could not be parsed.";
      last_error_ = str.str();
      is_ok_ = false;
      return false;
    }
    if (arg + 1 >= arguments.size()) {
      break;
    }
  }

  return true;
}

bool OptionParser::TryToGetOpt(std::vector<std::string> &arguments,
    Ui32 &arg, Option &option, std::string &flag) {
  if (flag.empty()) {
    return false;
  }

  if (arguments[arg] != flag) {
    return false;
  }

  if (!option.PosFlag().empty()) {
    values_[option.GetDest()].push_back(option.PosFlag());
    option.SetFound(true);
    return true;
  }

  if (option.GetMode() == STORE_TRUE) {
    option.SetFound(true);
    return true;
  }

  if (((option.GetMode() == STORE_VALUE) ||
       (option.GetMode() == STORE_MULT_VALUES)) &&
      !option.GetFound()) {
    if (GetValueArg(arguments, arg, option, flag)) {
      option.SetFound(true);
      return true;
    }
  }

  return false;
}

bool OptionParser::CheckForMissingArgs() {
  std::vector<std::string> missing;
  for (Option &opt : options_) {
    if ((opt.GetRequired()) && (!opt.GetFound())) {
      missing.push_back(opt.GetDest());
    } else if ((!opt.GetDefaultValue().empty()) && (!opt.GetFound())) {
      values_[opt.GetDest()].push_back(opt.GetDefaultValue());
      opt.SetFound(true);
    }
  }
  if (!missing.empty()) {
    std::string msg = "Missing required flags: " +
      std::accumulate(
          missing.begin() + 1, missing.end(), missing.at(0),
          [](std::string &s, const std::string &piece) -> std::string {
            return s + ", " + piece;
          }) +
      ".";
    last_error_ = msg;
    return false;
  }
  return true;
}

bool OptionParser::ParseArgcArgv(Ui32 argc, char const *argv[]) {
  Ui32 idx_ctr = 0;
  for (Option &opt : options_) {
    option_idx_[opt.GetDest()] = idx_ctr;
    idx_ctr++;
  }

  const std::string args_end = "-- ";
  prog_name_ = argv[0];
  std::vector<std::string> arguments(argv + 1, argv + argc);

  // dummy way to solve problem with last arg of
  arguments.emplace_back(args_end);

  // for each argument cluster
  int pos_args = 1;
  for (Ui32 arg = 0; arg < arguments.size(); ++arg) {
    bool match_found = false;
    // for each option sets
    for (Option &option : options_) {
      match_found = TryToGetOpt(arguments, arg, option, option.LongFlag());
      if (!is_ok_) {
        return false;
      }
      if (match_found) {
        break;
      }

      match_found = TryToGetOpt(arguments, arg, option, option.ShortFlag());
      if (!is_ok_) {
        return false;
      }
      if (match_found) {
        break;
      }
    }

    if (!match_found) {
      if (arguments[arg] != args_end) {
        if (pos_arg_count_ > pos_args) {
          options_[option_idx_.at(pos_option_names_[pos_args - 1])].SetFound(true);
          values_[pos_option_names_[pos_args - 1]].push_back(arguments[arg]);
          pos_args++;
        } else
          is_ok_ = false;
          std::stringstream str;
          str << "Unrecognized flag/option '" << arguments[arg] << "'";
          last_error_ = str.str();
          is_ok_ = false;
          return false;
      }
    }
  }
  return true;
}

std::string OptionParser::Help() {
  std::stringstream str;
  auto split = prog_name_.find_last_of('/');
  std::stringstream leading;
  leading << "usage: " << prog_name_.substr(split + 1) << " ";
  std::string usage_str = leading.str();
  str << usage_str;

  std::vector<std::string> option_usage;
  for (Option &option : options_) {
    std::stringstream optss;
    optss << (option.GetRequired() ? "" : "[");
    if (!option.ShortFlag().empty()) {
      optss << option.ShortFlag();
    } else if (!option.LongFlag().empty()) {
      optss << option.LongFlag();
    }
    if (option.GetMode() != StorageMode::STORE_TRUE) {
      optss << (option.PosFlag().empty() ? " " : "") << option.GetMetavar();
    }
    optss << (option.GetRequired() ? " " : "] ");
    option_usage.emplace_back(optss.str());
  }
  str << utils::StitchStr(option_usage, 80,
      std::string(usage_str.size(), ' ')).substr(usage_str.size())
    << std::endl;

  if (!description_.empty()) {
    str << "\n" << description_ << "\n" << std::endl;
  }

  std::vector<Option> pos_opts;
  std::vector<Option> reg_opts;
  std::copy_if(options_.begin(), options_.end(), std::back_inserter(pos_opts),
      [](Option &o) { return !o.PosFlag().empty(); });
  std::copy_if(options_.begin(), options_.end(), std::back_inserter(reg_opts),
      [](Option &o) { return o.PosFlag().empty(); });
  if (!pos_opts.empty()) {
    str << "\nPositional Arguments:" << std::endl;
    for (Option &o: pos_opts) {
      str << o.HelpDoc();
    }
  }
  if (!reg_opts.empty()) {
    str << "\nOptions:" << std::endl;
    for (Option &o: reg_opts) {
      str << o.HelpDoc();
    }
  }
  return str.str();
}

}  // namespace arctic

#endif  // ENGINE_OPTIONPARSER_H_
