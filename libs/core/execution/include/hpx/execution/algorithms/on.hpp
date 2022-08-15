  /////////////////////////////////////////////////////////////////////////////
  // [execution.senders.adaptors.on]
  namespace __on {
    namespace __impl {
      template <class _SchedulerId, class _SenderId, class _ReceiverId>
        struct __operation;

      template <class _SchedulerId, class _SenderId, class _ReceiverId>
        struct __receiver_ref
          : receiver_adaptor<__receiver_ref<_SchedulerId, _SenderId, _ReceiverId>> {
          using _Scheduler = __t<_SchedulerId>;
          using _Sender = __t<_SenderId>;
          using _Receiver = __t<_ReceiverId>;
          __operation<_SchedulerId, _SenderId, _ReceiverId>* __op_state_;
          _Receiver&& base() && noexcept {
            return (_Receiver&&) __op_state_->__rcvr_;
          }
          const _Receiver& base() const & noexcept {
            return __op_state_->__rcvr_;
          }
          auto get_env() const
            -> make_env_t<env_of_t<_Receiver>, with_t<get_scheduler_t, _Scheduler>> {
            return make_env(
              execution::get_env(this->base()),
              with(get_scheduler, __op_state_->__scheduler_));
          }
        };

      template <class _SchedulerId, class _SenderId, class _ReceiverId>
        struct __receiver
          : receiver_adaptor<__receiver<_SchedulerId, _SenderId, _ReceiverId>> {
          using _Scheduler = __t<_SchedulerId>;
          using _Sender = __t<_SenderId>;
          using _Receiver = __t<_ReceiverId>;
          using __receiver_ref_t =
            __receiver_ref<_SchedulerId, _SenderId, _ReceiverId>;
          __operation<_SchedulerId, _SenderId, _ReceiverId>* __op_state_;
          _Receiver&& base() && noexcept {
            return (_Receiver&&) __op_state_->__rcvr_;
          }
          const _Receiver& base() const & noexcept {
            return __op_state_->__rcvr_;
          }

          void set_value() && noexcept {
            // cache this locally since *this is going bye-bye.
            auto* __op_state = __op_state_;
            try {
              // This line will invalidate *this:
              start(__op_state->__data_.template emplace<1>(__conv{
                [__op_state] {
                  return connect((_Sender&&) __op_state->__sndr_,
                                  __receiver_ref_t{{}, __op_state});
                }
              }));
            } catch(...) {
              set_error((_Receiver&&) __op_state->__rcvr_,
                        current_exception());
            }
          }
        };

      template <class _SchedulerId, class _SenderId, class _ReceiverId>
        struct __operation {
          using _Scheduler = __t<_SchedulerId>;
          using _Sender = __t<_SenderId>;
          using _Receiver = __t<_ReceiverId>;
          using __receiver_t = __receiver<_SchedulerId, _SenderId, _ReceiverId>;
          using __receiver_ref_t = __receiver_ref<_SchedulerId, _SenderId, _ReceiverId>;

          friend void tag_invoke(start_t, __operation& __self) noexcept {
            start(std::get<0>(__self.__data_));
          }

          template <class _Sender2, class _Receiver2>
          __operation(_Scheduler __sched, _Sender2&& __sndr, _Receiver2&& __rcvr)
            : __data_{in_place_index<0>, __conv{[&, this]{
                return connect(schedule(__sched),
                                __receiver_t{{}, this});
              }}}
            , __scheduler_((_Scheduler&&) __sched)
            , __sndr_((_Sender2&&) __sndr)
            , __rcvr_((_Receiver2&&) __rcvr) {}
          __operation(__operation&&) = delete;

          variant<
              connect_result_t<schedule_result_t<_Scheduler>, __receiver_t>,
              connect_result_t<_Sender, __receiver_ref_t>> __data_;
          _Scheduler __scheduler_;
          _Sender __sndr_;
          _Receiver __rcvr_;
        };

      template <class _SchedulerId, class _SenderId>
        struct __sender {
          using _Scheduler = __t<_SchedulerId>;
          using _Sender = __t<_SenderId>;
          template <class _ReceiverId>
            using __receiver_ref_t =
              __receiver_ref<_SchedulerId, _SenderId, _ReceiverId>;
          template <class _ReceiverId>
            using __receiver_t =
              __receiver<_SchedulerId, _SenderId, _ReceiverId>;
          template <class _ReceiverId>
            using __operation_t =
              __operation<_SchedulerId, _SenderId, _ReceiverId>;

          _Scheduler __scheduler_;
          _Sender __sndr_;

          template <__decays_to<__sender> _Self, receiver _Receiver>
            requires constructible_from<_Sender, __member_t<_Self, _Sender>> &&
              sender_to<schedule_result_t<_Scheduler>,
                        __receiver_t<__x<decay_t<_Receiver>>>> &&
              sender_to<_Sender, __receiver_ref_t<__x<decay_t<_Receiver>>>>
          friend auto tag_invoke(connect_t, _Self&& __self, _Receiver&& __rcvr)
            -> __operation_t<__x<decay_t<_Receiver>>> {
            return {((_Self&&) __self).__scheduler_,
                    ((_Self&&) __self).__sndr_,
                    (_Receiver&&) __rcvr};
          }

          template <__sender_queries::__sender_query _Tag, class... _As>
            requires __callable<_Tag, const _Sender&, _As...>
          friend auto tag_invoke(_Tag __tag, const __sender& __self, _As&&... __as)
            noexcept(__nothrow_callable<_Tag, const _Sender&, _As...>)
            -> __call_result_if_t<__sender_queries::__sender_query<_Tag>, _Tag, const _Sender&, _As...> {
            return ((_Tag&&) __tag)(__self.__sndr_, (_As&&) __as...);
          }

          template <class...>
            using __value_t = completion_signatures<>;

          template <__decays_to<__sender> _Self, class _Env>
          friend auto tag_invoke(get_completion_signatures_t, _Self&&, _Env) ->
            make_completion_signatures<
              schedule_result_t<_Scheduler>,
              _Env,
              make_completion_signatures<
                __member_t<_Self, _Sender>,
                make_env_t<_Env, with_t<get_scheduler_t, _Scheduler>>,
                completion_signatures<set_error_t(exception_ptr)>>,
              __value_t>;
        };
    } // namespace __impl

    struct on_t {
      template <scheduler _Scheduler, sender _Sender>
        requires tag_invocable<on_t, _Scheduler, _Sender>
      auto operator()(_Scheduler&& __sched, _Sender&& __sndr) const
        noexcept(nothrow_tag_invocable<on_t, _Scheduler, _Sender>)
        -> tag_invoke_result_t<on_t, _Scheduler, _Sender> {
        return tag_invoke(*this, (_Scheduler&&) __sched, (_Sender&&) __sndr);
      }

      template <scheduler _Scheduler, sender _Sender>
      auto operator()(_Scheduler&& __sched, _Sender&& __sndr) const
        -> __impl::__sender<__x<decay_t<_Scheduler>>,
                            __x<decay_t<_Sender>>> {
        return {(_Scheduler&&) __sched, (_Sender&&) __sndr};
      }
    };
  } // namespace __on
