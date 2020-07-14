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

//
// This is a C++ implementation for the queue from
// http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
//

#ifndef ENGINE_MTQ_MPSC_TAIL_SWAP_H_
#define ENGINE_MTQ_MPSC_TAIL_SWAP_H_

#include "engine/mtq_base_common.h"

#include <utility>
#include <type_traits>
#include <cstddef>

namespace arctic {

namespace dtl {


template<typename TargetType>
static bool constexpr DeleteTheObjectIsNoexcept =
  noexcept(delete reinterpret_cast<TargetType *>(0xAFAFAF));


template<typename PayloadType>
struct Node {
  std::atomic<Node *> next = {nullptr};
  PayloadType payload;
};


template<typename PayloadType>
class MPSC_TailSwap_Impl {
public:
  explicit
  MPSC_TailSwap_Impl() noexcept(noexcept(new Node<PayloadType>)) = default;

  MPSC_TailSwap_Impl(const MPSC_TailSwap_Impl &) = delete;
  void operator=(const MPSC_TailSwap_Impl &) = delete;

  MPSC_TailSwap_Impl(MPSC_TailSwap_Impl &&move) noexcept
    : head(move.head)
    , tail(move.tail) {
    move.head = nullptr;
  }

  MPSC_TailSwap_Impl &operator=(MPSC_TailSwap_Impl &&move) noexcept {
    delete_list();
    head = move.head;
    tail.store(head, MO_RELAXED);
    move.head = nullptr;
  }

  ~MPSC_TailSwap_Impl()
  noexcept(noexcept(DeleteTheObjectIsNoexcept<Node<PayloadType>>)) {
    delete_list();
  }

  void enqueue(PayloadType &&item) noexcept(noexcept(new Node<PayloadType>)) {
    Node<PayloadType> *new_node = new Node<PayloadType>;
    new_node->payload = std::move(item);
    Node<PayloadType> *old_tail = tail.exchange(new_node, MO_RELAXED);
    old_tail->next.store(new_node, MO_RELEASE);
  }

  bool dequeue(PayloadType &item)
  noexcept(DeleteTheObjectIsNoexcept<Node<PayloadType>>) {
    Node<PayloadType> *next_head = head->next.load(MO_RELAXED);
    if (next_head == nullptr) {
      return false;
    }
    std::atomic_thread_fence(MO_ACQUIRE);
    delete head;
    head = next_head;
    item = std::move(next_head->payload);
    return true;
  }

protected:
  void delete_list()
  noexcept(DeleteTheObjectIsNoexcept<Node<PayloadType>>) {
    std::atomic_thread_fence(MO_ACQUIRE);
    for (Node<PayloadType> *link = head; link;) {
      Node<PayloadType> *next = link->next.load(MO_RELAXED);
      delete link;
      link = next;
    }
  }


  Node<PayloadType> *head = new Node<PayloadType>;
  std::atomic<Node<PayloadType> *> tail = {head};
};


template<size_t SIZE>
struct JustArrayInside {
  char data[SIZE];
};


} // namespace dtl


template<typename PayloadType,
  bool TRIVIAL =
  std::is_trivially_move_assignable<PayloadType>::value &&
    std::is_trivially_destructible<PayloadType>::value>
class MPSC_TailSwap;


template<typename PayloadType>
class MPSC_TailSwap<PayloadType, true> {
public:
  void enqueue(PayloadType &&item)
  noexcept(noexcept(new dtl::Node<PayloadType>)) {
    impl.enqueue(reinterpret_cast<ImplItemType &&>(item));
  }

  bool dequeue(PayloadType &item)
  noexcept(dtl::DeleteTheObjectIsNoexcept<dtl::Node<PayloadType>>) {
    return impl.dequeue(reinterpret_cast<ImplItemType &>(item));
  }

protected:
  using ImplItemType = dtl::JustArrayInside<sizeof(PayloadType)>;

  dtl::MPSC_TailSwap_Impl<ImplItemType> impl;
};


template<typename PayloadType>
class MPSC_TailSwap<PayloadType, false>
  : public dtl::MPSC_TailSwap_Impl<PayloadType> {
};


} // namespace arctic

#endif  // ENGINE_MTQ_MPSC_TAIL_SWAP_H_
