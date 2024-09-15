// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2018 Vitaliy Manushkin
// Copyright (c) 2020 Huldra
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

#ifndef ENGINE_TEMPLATE_TUNE_H_
#define ENGINE_TEMPLATE_TUNE_H_

namespace arctic {

/// @brief A template struct for fusing multiple parameter types.
/// @tparam BaseType The base type to apply the fused parameters to.
/// @tparam FuseType The first parameter type to fuse.
/// @tparam MoreFuseTypes Additional parameter types to fuse.
template<typename BaseType,
  typename FuseType,
  typename ... MoreFuseTypes>
struct FuseParams
  : public FuseParams<
    typename FuseType::template ApplyTemplate<BaseType>,
    MoreFuseTypes...> {
};

/// @brief A template struct for fusing a single parameter type.
/// @tparam BaseType The base type to apply the fused parameter to.
/// @tparam FuseType The parameter type to fuse.
template<typename BaseType, typename FuseType>
struct FuseParams<BaseType, FuseType>
  : public FuseType::template ApplyTemplate<BaseType> {
};

/// @brief Macro to declare a tunable value parameter.
/// @param TemplateParamName The name of the template parameter.
/// @param ParamType The type of the parameter.
/// @param InStructParamName The name of the parameter inside the struct.
#define                                                                       \
    DECLARE_TUNE_VALUE_PARAM(TemplateParamName, ParamType, InStructParamName) \
    template <ParamType VALUE>                                                \
    struct TemplateParamName {                                                \
        template <typename BaseType>                                          \
        struct ApplyTemplate: public BaseType {                               \
            static constexpr ParamType InStructParamName = VALUE;             \
        };                                                                    \
    }

/// @brief Macro to declare a tunable value parameter with a default value.
/// @param TemplateParamName The name of the template parameter.
/// @param ParamType The type of the parameter.
/// @param DefaultValue The default value for the parameter.
/// @param InStructParamName The name of the parameter inside the struct.
#define                                                           \
    DECLARE_TUNE_VALUE_PARAM_DEFVALUE(                            \
        TemplateParamName,                                        \
        ParamType,                                                \
        DefaultValue,                                             \
        InStructParamName)                                        \
    template <ParamType VALUE = DefaultValue>                     \
    struct TemplateParamName {                                    \
        template <typename BaseType>                              \
        struct ApplyTemplate: public BaseType {                   \
            static constexpr ParamType InStructParamName = VALUE; \
        };                                                        \
    }

/// @brief Macro to declare a tunable type parameter.
/// @param TemplateParamName The name of the template parameter.
/// @param InStructParamName The name of the parameter inside the struct.
#define DECLARE_TUNE_TYPE_PARAM(TemplateParamName, InStructParamName) \
    template <typename Value>                                         \
    struct TemplateParamName {                                        \
        template <typename BaseType>                                  \
        struct ApplyTemplate: public BaseType {                       \
            typedef Value InStructParamName;                          \
        };                                                            \
    }

/// @brief Macro to declare a tunable type parameter with a default type.
/// @param TemplateParamName The name of the template parameter.
/// @param InStructParamName The name of the parameter inside the struct.
/// @param DefaultType The default type for the parameter.
#define DECLARE_TUNE_TYPE_PARAM_DEFTYPE(\
    TemplateParamName, InStructParamName, DefaultType) \
    template <typename Value = DefaultType>            \
    struct TemplateParamName {                         \
        template <typename BaseType>                   \
        struct ApplyTemplate: public BaseType {        \
            typedef Value InStructParamName;           \
        };                                             \
    }

}  // namespace arctic

#endif  // ENGINE_TEMPLATE_TUNE_H_
