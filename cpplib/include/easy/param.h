#ifndef PARAM
#define PARAM

#include <easy/runtime/Context.h>
#include <easy/function_wrapper.h>
#include <easy/options.h>
#include <easy/meta.h>

namespace easy {

namespace  {

// special types
template<bool special>
struct set_parameter_helper {

  template<class Param, class FunctionWrapper>
  struct function_wrapper_specialization_is_possible {

    template<class Param_>
    static std::true_type can_assign_fun_pointer(std::remove_pointer_t<Param_>);

    template<class Param_>
    static std::false_type can_assign_fun_pointer (...);

    using  type = decltype(can_assign_fun_pointer<Param>(
                             *std::declval<FunctionWrapper>().getFunctionPointer()));

    static constexpr bool value { type::value };
  };

  template<class _, class Arg>
  static void set_param(Context &C, size_t idx,
                        std::enable_if_t<(bool)std::is_placeholder<std::decay_t<Arg>>::value, Arg>) {
    C.setParameterIndex(idx, std::is_placeholder<std::decay_t<Arg>>::value-1);
  }

  template<class Param, class Arg> // TODO use param to perform type checking!
  static void set_param(Context &C, size_t idx,
                        std::enable_if_t<easy::is_function_wrapper<Arg>::value, Arg> &&arg) {
    static_assert(function_wrapper_specialization_is_possible<Param, Arg>::value,
                  "easy::jit composition is not possible. Incompatible types.");
    C.setParameterModule(idx, arg.getFunction());
  }
};

template<>
struct set_parameter_helper<false> {

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        std::enable_if_t<std::is_integral<Param>::value, Arg> &&arg) {
    C.setParameterInt(idx, std::forward<Arg>(arg));
  }

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        std::enable_if_t<std::is_floating_point<Param>::value, Arg> &&arg) {
    C.setParameterFloat(idx, std::forward<Arg>(arg));
  }

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        std::enable_if_t<std::is_pointer<Param>::value, Arg> &&arg) {
    C.setParameterPtr(idx, std::forward<Arg>(arg));
  }

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        std::enable_if_t<std::is_reference<Param>::value, Arg> &&arg) {
    C.setParameterPtr(idx, std::addressof(arg));
  }

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        std::enable_if_t<std::is_class<Param>::value, Arg> &&arg) {
    C.setParameterStruct(idx, std::addressof(arg));
  }
};

template<class Param, class Arg>
struct set_parameter {

  static constexpr bool is_ph = std::is_placeholder<std::decay_t<Arg>>::value;
  static constexpr bool is_fw = easy::is_function_wrapper<Arg>::value;
  static constexpr bool is_special = is_ph || is_fw;

  using help = set_parameter_helper<is_special>;
};

}

template<class ... NoOptions>
void set_options(Context &, NoOptions&& ...) {
  static_assert(meta::type_list<NoOptions...>::empty, "Remaining options to be processed!");
}

template<class Option0, class ... Options>
void set_options(Context &C, Option0&& Opt, Options&& ... Opts) {
  using OptTy = std::decay_t<Option0>;
  OptTy& OptRef = std::ref<OptTy>(Opt);
  static_assert(options::is_option<OptTy>::value, "An easy::jit option is expected");

  OptRef.handle(C);
  set_options(C, std::forward<Options>(Opts)...);
}

template<class ParameterList, class ... Options>
std::enable_if_t<ParameterList::empty>
set_parameters(ParameterList,
               Context& C, size_t, Options&& ... opts) {
  set_options<Options...>(C, std::forward<Options>(opts)...);
}

template<class ParameterList, class Arg0, class ... Args>
std::enable_if_t<!ParameterList::empty>
set_parameters(ParameterList,
               Context &C, size_t idx, Arg0 &&arg0, Args&& ... args) {
  using Param0 = typename ParameterList::head;
  using ParametersTail = typename ParameterList::tail;

  set_parameter<Param0, Arg0>::help::template set_param<Param0, Arg0>(C, idx, std::forward<Arg0>(arg0));
  set_parameters<ParametersTail, Args&&...>(ParametersTail(), C, idx+1, std::forward<Args>(args)...);
}

}

#endif // PARAM
