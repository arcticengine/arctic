A sample of the chosen code style (mostly https://google.github.io/styleguide/cppguide.html but not exactly)

Differences:
* Use ```Si32```, ```Ui32```, etc. instead of ```int```, ```uint32_t```, etc.
* Prefix global variables with ```g_```.

Header file:
~~~cpp
// The MIT License (MIT)
//
// Copyright (c) 2019 Huldra
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

#ifndef FOO_BAR_BAZ_H_
#define FOO_BAR_BAZ_H_

namespace outer {

const int kDaysInAWeek = 7;
const int kAndroid8_0_0 = 24;

struct UrlTableProperties {
  string name;
  int num_entries;
  static Pool<UrlTableProperties>* pool;
}

using PropertiesMap = hash_map<UrlTableProperties *, string>;

enum UrlTableErrors {
  kOK = 0,
  kErrorOutOfMemory,
  kErrorMalformedInput,
};

// All declarations are within the namespace scope.
// Notice the lack of indentation.
class MyClass {
 public:
  void Foo(double y, const string &in, string *out) noexcept;
  void AddTableEntry() noexcept;

 protected:
  void DeleteUrl() noexcept;
  void OpenFileOrDie() noexcept;

 private:
  string table_name_;  // underscore at end.
  static Pool<TableInfo>* pool_;
};

}  // namespace outer

#endif  // FOO_BAR_BAZ_H_
~~~

Cpp file:
~~~cpp
// The MIT License (MIT)
//
// Copyright (c) 2019 Huldra
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

#include "Header for this .cpp file"

#include <C system files>
#include <C++ system files>

#include <Other libraries' .h files>
#include <Your project's .h files>

#define SOME_MACRO 42

namespace outer {

using ::foo::bar;

// Definition of functions is within scope of the namespace.
// Consider std::string_view.
void MyClass::Foo(double double_value, const string &in, string *out) noexcept {
  int j = Goo();
  std::vector<int> v = {1, 2};

  float f = static_cast<float>(double_value);
  int64 y = int64{1} << 42
}

}  // namespace outer
~~~