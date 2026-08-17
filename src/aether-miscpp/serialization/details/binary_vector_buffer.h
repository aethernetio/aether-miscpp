/*
 * Copyright 2026 Aethernet Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AETHER_SERIALIZATION_DETAILS_BINARY_VECTOR_BUFFER_H_
#define AETHER_SERIALIZATION_DETAILS_BINARY_VECTOR_BUFFER_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <utility>
#include <vector>

#include "aether-miscpp/serialization/details/serialization_result.h"
#include "aether-miscpp/serialization/details/tags.h"

namespace ae::seri {

template <typename SizeType = std::uint32_t>
class BinaryVectorBuffer {
 public:
  explicit BinaryVectorBuffer(std::vector<std::uint8_t>& bytes) noexcept
      : bytes_(bytes) {}

  SeriResult Write(SizeWriteTag tag) {
    auto const size = static_cast<SizeType>(tag.size);
    return Write(DataWriteTag{&size, sizeof(size)});
  }

  SeriResult Write(DataWriteTag tag) {
    if (tag.size == 0) {
      return Ok{good};
    }
    auto const* ptr = reinterpret_cast<std::uint8_t const*>(tag.data);
    bytes_.insert(bytes_.end(), ptr, ptr + tag.size);
    return Ok{good};
  }

  SeriResult Read(SizeReadTag tag) {
    SizeType size{};
    TRY_RESULT(Read(DataReadTag{&size, sizeof(size)}));
    tag.size = static_cast<std::size_t>(size);
    return Ok{good};
  }

  SeriResult Read(DataReadTag tag) {
    if (read_position_ + tag.size > bytes_.size()) {
      return Error{read_eof};
    }
    std::memcpy(tag.data, bytes_.data() + read_position_, tag.size);
    read_position_ += tag.size;
    return Ok{good};
  }

  std::size_t read_position() const noexcept { return read_position_; }

  void set_read_position(std::size_t read_position) noexcept {
    read_position_ = read_position;
  }

 protected:
  std::vector<std::uint8_t>& bytes_;
  std::size_t read_position_{};
};

template <typename SizeType = std::uint32_t>
class LimitedVectorBuffer : public BinaryVectorBuffer<SizeType> {
 public:
  explicit LimitedVectorBuffer(std::vector<std::uint8_t>& bytes) noexcept
      : BinaryVectorBuffer<SizeType>(bytes) {}

  using BinaryVectorBuffer<SizeType>::Read;
  using BinaryVectorBuffer<SizeType>::read_position;
  using BinaryVectorBuffer<SizeType>::set_read_position;

  bool eof() const noexcept { return eof_; }

  SeriResult Write(SizeWriteTag tag) {
    auto const size = static_cast<SizeType>(tag.size);
    return Write(DataWriteTag{&size, sizeof(size)});
  }

  SeriResult Write(DataWriteTag tag) {
    return WriteImpl(tag.size, [this, tag]() {
      auto const* ptr = reinterpret_cast<std::uint8_t const*>(tag.data);
      this->bytes_.insert(this->bytes_.end(), ptr, ptr + tag.size);
    });
  }

 private:
  template <typename Writer>
  SeriResult WriteImpl(std::size_t size, Writer&& writer) {
    if (eof_) {
      return Error{write_eof};
    }
    if (size == 0) {
      return Ok{good};
    }
    if (size > this->bytes_.capacity() - this->bytes_.size()) {
      eof_ = true;
      return Error{write_eof};
    }
    auto const new_size = this->bytes_.size() + size;
    std::forward<Writer>(writer)();
    if (new_size == this->bytes_.capacity()) {
      eof_ = true;
    }
    return Ok{good};
  }

  bool eof_{};
};

}  // namespace ae::seri

#endif  // AETHER_SERIALIZATION_DETAILS_BINARY_VECTOR_BUFFER_H_
