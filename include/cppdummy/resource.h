#pragma once

#include <concepts>
#include <functional>
#include <utility>

namespace cppdummy {

template <typename T>
concept ReleasableResource = requires(T value) {
  {
    value == T {}
    } -> std::convertible_to<bool>;
};

template <ReleasableResource T, T kInvalid = T{}>
class Resource {
public:
  Resource() noexcept = default;

  Resource(T handle, std::function<void(T)> deleter) noexcept
      : handle_{std::move(handle)}, deleter_{std::move(deleter)} {}

  Resource(const Resource&) = delete;
  Resource& operator=(const Resource&) = delete;

  Resource(Resource&& other) noexcept
      : handle_{std::exchange(other.handle_, kInvalid)}, deleter_{std::move(other.deleter_)} {}

  Resource& operator=(Resource&& other) noexcept {
    if (this != &other) {
      Reset();
      handle_ = std::exchange(other.handle_, kInvalid);
      deleter_ = std::move(other.deleter_);
    }
    return *this;
  }

  ~Resource() { Reset(); }

  void Reset() noexcept {
    if (handle_ != kInvalid && deleter_) {
      deleter_(handle_);
      handle_ = kInvalid;
    }
  }

  void Reset(T handle, std::function<void(T)> deleter) noexcept {
    Reset();
    handle_ = std::move(handle);
    deleter_ = std::move(deleter);
  }

  [[nodiscard]] T Release() noexcept { return std::exchange(handle_, kInvalid); }

  [[nodiscard]] T Get() const noexcept { return handle_; }

  [[nodiscard]] bool IsValid() const noexcept { return handle_ != kInvalid && deleter_ != nullptr; }

  explicit operator bool() const noexcept { return IsValid(); }

  void Swap(Resource& other) noexcept {
    std::swap(handle_, other.handle_);
    std::swap(deleter_, other.deleter_);
  }

private:
  T handle_{kInvalid};
  std::function<void(T)> deleter_{nullptr};
};

template <ReleasableResource T, T kInvalid = T{}>
void Swap(Resource<T, kInvalid>& lhs, Resource<T, kInvalid>& rhs) noexcept {
  lhs.Swap(rhs);
}

template <ReleasableResource T, T kInvalid = T{}, typename Deleter>
[[nodiscard]] Resource<T, kInvalid> MakeResource(T handle, Deleter&& deleter) {
  return Resource<T, kInvalid>(std::move(handle), std::forward<Deleter>(deleter));
}

}  // namespace cppdummy