#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

template <typename Y, typename T> Y *ye_down_cast(T *t) {
  return reinterpret_cast<Y *>(reinterpret_cast<uint8_t *>(t) + sizeof(T));
}

template <typename Y, typename T> Y *ye_up_cast(T *t) {
  return reinterpret_cast<Y *>(reinterpret_cast<uint8_t *>(t) - sizeof(T));
}

template <typename T>
concept ClassConcept = std::is_class_v<T>;

template <typename T>
concept PodConcept = std::is_standard_layout_v<T>;

template <ClassConcept T, PodConcept Y, typename... Args>
auto re_constory(Args &&...args) -> T * {
  auto root_obj = reinterpret_cast<T *>(malloc(sizeof(T) + sizeof(Y)));
  new (root_obj) T(args...);
  if constexpr (!std::is_default_constructible_v<T> or
                std::is_trivially_default_constructible_v<T> or
                std::is_nothrow_default_constructible_v<T> and
                    std::is_class_v<T>) {
    new (ye_down_cast<Y *>(root_obj)) Y{};
  }
  return root_obj;
}

class YiMin {

public:
  struct UserInfo {
    std::string name;
    uint8_t age;
  };
  static auto yimin_ye_constory() -> YiMin * {
    return re_constory<YiMin, UserInfo>();
  }
public:
  auto printInfo() noexcept -> void {
    std::printf("%d", ye_down_cast<UserInfo>(this)->age);
    std::printf("%s", ye_down_cast<UserInfo>(this)->name.data());
  }
};
int main(int argc, char const *argv[]) {
  auto user = YiMin::yimin_ye_constory();

  ye_down_cast<YiMin::UserInfo>(user)->age = 31;
  user->printInfo();
  return 0;
}
