// =============================================================================
//
// ztd.text
// Copyright © 2021 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.text licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License") for non-commercial use; you may not use this
// file except in compliance with the License. You may obtain a copy of the
// License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ============================================================================>

#ifndef ZTD_TEXT_TRANSCODE_ITERATOR_HPP
#define ZTD_TEXT_TRANSCODE_ITERATOR_HPP

#include <ztd/text/version.hpp>

#include <ztd/text/error_handler.hpp>
#include <ztd/text/state.hpp>
#include <ztd/text/unbounded.hpp>
#include <ztd/text/subrange.hpp>
#include <ztd/text/is_ignorable_error_handler.hpp>

#include <ztd/text/detail/encoding_iterator.hpp>
#include <ztd/text/detail/blackhole_iterator.hpp>
#include <ztd/text/detail/encoding_iterator_storage.hpp>
#include <ztd/text/detail/encoding_range.hpp>
#include <ztd/text/detail/ebco.hpp>
#include <ztd/text/detail/adl.hpp>
#include <ztd/text/detail/range.hpp>
#include <ztd/text/detail/transcode_one.hpp>

#include <ztd/text/detail/prologue.hpp>

namespace ztd { namespace text {
	ZTD_TEXT_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @addtogroup ztd_text_ranges Ranges, Views, and Iterators
	/// @{
	//////

	//////
	/// @brief A sentinel type that can be used to compare with a ztd::text::transcode_iterator.
	///
	//////
	class transcode_sentinel { };

	//////
	/// @brief A transcoding iterator that takes an input of code units and provides an output over the code units of
	/// the desired @p _ToEncoding after converting from the @p _FromEncoding in a fashion that will never produce a
	/// ztd::text::encoding_error::insufficient_output error.
	///
	/// @tparam _FromEncoding The encoding to read the underlying range of code points as.
	/// @tparam _ToEncoding The encoding to read the underlying range of code points as.
	/// @tparam _Range The range of input that will be fed into the _FromEncoding's decode operation.
	/// @tparam _FromErrorHandler The error handler for any decode-step failures.
	/// @tparam _ToErrorHandler The error handler for any encode-step failures.
	/// @tparam _FromState The state type to use for the decode operations to intermediate code points.
	/// @tparam _ToState The state type to use for the encode operations to intermediate code points.
	///
	/// @remarks This type produces proxies as their reference type, and are only readable, not writable iterators. The
	/// type will also try many different shortcuts for decoding the input and encoding the intermediates,
	/// respectively, including invoking a few customization points for either @c decode_one or @c encode_one . It
	/// may also call @c transcode_one to bypass having to do the round-trip through two encodings, which an encoding
	/// pair that a developer is interested in can use to do the conversion more quickly. The view presents code units
	/// one at a time, regardless of how many code units are output by one decode operation. This means if, for
	/// example, one (1) UTF-16 code unit becomes two (2) UTF-8 code units, it will present each code unit one at a
	/// time. If you are looking to explicitly know each collection of characters, you will have to use lower-level
	/// interfaces.
	//////
	template <typename _FromEncoding, typename _ToEncoding, typename _Range, typename _FromErrorHandler,
		typename _ToErrorHandler, typename _FromState, typename _ToState>
	class transcode_iterator
	: private __txt_detail::__ebco<__txt_detail::__remove_cvref_t<_FromEncoding>, 0>,
	  private __txt_detail::__ebco<__txt_detail::__remove_cvref_t<_ToEncoding>, 1>,
	  private __txt_detail::__ebco<__txt_detail::__remove_cvref_t<_FromErrorHandler>, 2>,
	  private __txt_detail::__ebco<__txt_detail::__remove_cvref_t<_ToErrorHandler>, 3>,
	  private __txt_detail::__state_storage<__txt_detail::__remove_cvref_t<_FromEncoding>,
		  __txt_detail::__remove_cvref_t<_FromState>, 0>,
	  private __txt_detail::__state_storage<__txt_detail::__remove_cvref_t<_ToEncoding>,
		  __txt_detail::__remove_cvref_t<_ToState>, 1>,
	  private __txt_detail::__cursor_cache<
		  max_code_units_v<__txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_ToEncoding>>>,
		  __txt_detail::__is_range_input_or_output_range_v<
		       __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_Range>>>>,
	  private __txt_detail::__error_cache<
		  decode_error_handler_always_returns_ok_v<
		       __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_FromEncoding>>,
		       __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<
		            _FromErrorHandler>>> && encode_error_handler_always_returns_ok_v<__txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_ToEncoding>>, __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_ToErrorHandler>>>>,
	  private __txt_detail::__ebco<_Range, 4> {
	private:
		using _URange                = __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_Range>>;
		using _UFromEncoding         = __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_FromEncoding>>;
		using _UToEncoding           = __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_ToEncoding>>;
		using _UFromErrorHandler     = __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_FromErrorHandler>>;
		using _UToErrorHandler       = __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_ToErrorHandler>>;
		using _UFromState            = __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_FromState>>;
		using _UToState              = __txt_detail::__remove_cvref_t<__txt_detail::__unwrap_t<_ToState>>;
		using _BaseIterator          = __txt_detail::__range_iterator_t<_URange>;
		using _IntermediateCodePoint = code_point_t<_UToEncoding>;
		inline static constexpr ::std::size_t _MaxValues = max_code_units_v<_UToEncoding>;
		inline static constexpr bool _IsSingleValueType  = _MaxValues == 1;
		inline static constexpr bool _IsInputOrOutput    = __txt_detail::__is_range_input_or_output_range_v<_URange>;
		inline static constexpr bool _IsCursorless       = _IsSingleValueType && !_IsInputOrOutput;
		inline static constexpr bool _IsErrorless
			= decode_error_handler_always_returns_ok_v<_UFromEncoding,
			       _UFromErrorHandler> && encode_error_handler_always_returns_ok_v<_UToEncoding, _UToErrorHandler>;
		using __base_cursor_cache_t      = __txt_detail::__cursor_cache<_MaxValues, _IsInputOrOutput>;
		using __base_cursor_cache_size_t = typename __base_cursor_cache_t::_SizeType;
		using __base_error_cache_t       = __txt_detail::__error_cache<_IsErrorless>;
		using __base_from_encoding_t     = __txt_detail::__ebco<__txt_detail::__remove_cvref_t<_FromEncoding>, 0>;
		using __base_to_encoding_t       = __txt_detail::__ebco<__txt_detail::__remove_cvref_t<_ToEncoding>, 1>;
		using __base_from_error_handler_t
			= __txt_detail::__ebco<__txt_detail::__remove_cvref_t<_FromErrorHandler>, 2>;
		using __base_to_error_handler_t = __txt_detail::__ebco<__txt_detail::__remove_cvref_t<_ToErrorHandler>, 3>;
		using __base_from_state_t       = __txt_detail::__state_storage<__txt_detail::__remove_cvref_t<_FromEncoding>,
               __txt_detail::__remove_cvref_t<_FromState>, 0>;
		using __base_to_state_t         = __txt_detail::__state_storage<__txt_detail::__remove_cvref_t<_ToEncoding>,
               __txt_detail::__remove_cvref_t<_ToState>, 1>;
		using __base_range_t            = __txt_detail::__ebco<_Range, 4>;

		inline static constexpr bool _IsBackwards
			= __txt_detail::__is_detected_v<__txt_detail::__detect_object_encode_one_backwards, _UFromEncoding,
			     _URange, _UFromErrorHandler, _UFromState>;

	public:
		//////
		/// @brief The underlying range type.
		///
		//////
		using range_type = _Range;
		//////
		/// @brief The base iterator type.
		///
		//////
		using iterator_type = _BaseIterator;
		//////
		/// @brief The encoding type used for decoding to intermediate code point storage.
		///
		//////
		using from_encoding_type = _FromEncoding;
		//////
		/// @brief The encoding type used for encoding to the final code units storage.
		///
		//////
		using to_encoding_type = _ToEncoding;
		//////
		/// @brief The error handler when a decode operation fails.
		///
		//////
		using from_error_handler_type = _FromErrorHandler;
		//////
		/// @brief The error handler when an encode operation fails.
		///
		//////
		using to_error_handler_type = _ToErrorHandler;
		//////
		/// @brief The state type used for decode operations.
		///
		//////
		using from_state_type = __txt_detail::__remove_cvref_t<_FromState>;
		//////
		/// @brief The state type used for encode operations.
		///
		//////
		using to_state_type = __txt_detail::__remove_cvref_t<_ToState>;
		//////
		/// @brief The strength of the iterator category, as defined in relation to the base.
		///
		//////
		using iterator_category = ::std::conditional_t<
			__txt_detail::__is_iterator_concept_or_better_v<::std::bidirectional_iterator_tag, _BaseIterator>,
			::std::conditional_t<_IsBackwards, ::std::bidirectional_iterator_tag, ::std::forward_iterator_tag>,
			__txt_detail::__iterator_category_t<_BaseIterator>>;
		//////
		/// @brief The strength of the iterator concept, as defined in relation to the base.
		///
		//////
		using iterator_concept = ::std::conditional_t<
			__txt_detail::__is_iterator_concept_or_better_v<::std::bidirectional_iterator_tag, _BaseIterator>,
			::std::conditional_t<_IsBackwards, ::std::bidirectional_iterator_tag, ::std::forward_iterator_tag>,
			__txt_detail::__iterator_concept_t<_BaseIterator>>;
		//////
		/// @brief The object type that gets output on every dereference.
		///
		//////
		using value_type = code_unit_t<_ToEncoding>;
		//////
		/// @brief A pointer type to the value_type.
		///
		//////
		// TODO: is there such a thing?
		using pointer = value_type*;
		//////
		/// @brief The value returned from derefencing the iterator.
		///
		/// @remarks This is a proxy iterator, so the @c reference is a non-reference @c value_type.
		//////
		using reference = value_type;
		//////
		/// @brief The type returned when two of these pointers are subtracted from one another.
		///
		/// @remarks It's not a very useful type...
		//////
		using difference_type = __txt_detail::__iterator_difference_type_t<_BaseIterator>;

		//////
		/// @brief Does not allow for default construction of the transcode_iterator.
		///
		//////
		transcode_iterator() = delete;

		//////
		/// @brief Copy constructs a transcode_iterator.
		///
		//////
		constexpr transcode_iterator(const transcode_iterator&) = default;
		//////
		/// @brief Move constructs a transcode_iterator.
		///
		//////
		constexpr transcode_iterator(transcode_iterator&&) = default;

		//////
		/// @brief Constructs a transcode_iterator from the underlying range.
		///
		/// @param[in] __range The input range to wrap and iterate over.
		//////
		constexpr transcode_iterator(range_type __range)
		: transcode_iterator(::std::move(__range), to_encoding_type {}) {
		}

		//////
		/// @brief Constructs a transcode_iterator from the underlying range.
		///
		/// @param[in] __range The input range to wrap and iterate over.
		/// @param[in] __to_encoding The encoding object to call @c encode_one or equivalent functionality on.
		//////
		constexpr transcode_iterator(range_type __range, to_encoding_type __to_encoding)
		: transcode_iterator(::std::move(__range), from_encoding_type {}, ::std::move(__to_encoding)) {
		}

		//////
		/// @brief Constructs a transcode_iterator from the underlying range.
		///
		/// @param[in] __range The input range to wrap and iterate over.
		/// @param[in] __from_encoding The encoding object to call @c decode_one or equivalent functionality on.
		/// @param[in] __to_encoding The encoding object to call @c encode_one or equivalent functionality on.
		//////
		constexpr transcode_iterator(
			range_type __range, from_encoding_type __from_encoding, to_encoding_type __to_encoding)
		: transcode_iterator(::std::move(__range), ::std::move(__from_encoding), ::std::move(__to_encoding),
			from_error_handler_type {}, to_error_handler_type {}) {
		}

		//////
		/// @brief Constructs a transcode_iterator from the underlying range.
		///
		/// @param[in] __range The input range to wrap and iterate over.
		/// @param[in] __from_encoding The encoding object to call @c decode_one or equivalent functionality on.
		/// @param[in] __to_encoding The encoding object to call @c encode_one or equivalent functionality on.
		/// @param[in] __from_error_handler The error handler for decode operations to store in this view.
		/// @param[in] __to_error_handler The error handler for encode operations to store in this view.
		//////
		constexpr transcode_iterator(range_type __range, from_encoding_type __from_encoding,
			to_encoding_type __to_encoding, from_error_handler_type __from_error_handler,
			to_error_handler_type __to_error_handler)
		: transcode_iterator(::std::move(__range), ::std::move(__from_encoding), ::std::move(__to_encoding),
			::std::move(__from_error_handler), ::std::move(__to_error_handler), from_state_type {},
			to_state_type {}) {
		}

		//////
		/// @brief Constructs a transcode_iterator from the underlying range.
		///
		/// @param[in] __range The input range to wrap and iterate over.
		/// @param[in] __from_encoding The encoding object to call @c decode_one or equivalent functionality on.
		/// @param[in] __to_encoding The encoding object to call @c encode_one or equivalent functionality on.
		/// @param[in] __from_error_handler The error handler for decode operations to store in this view.
		/// @param[in] __to_error_handler The error handler for encode operations to store in this view.
		/// @param[in] __from_state The state to user for the decode operation.
		/// @param[in] __to_state The state to user for the decode operation.
		//////
		constexpr transcode_iterator(range_type __range, from_encoding_type __from_encoding,
			to_encoding_type __to_encoding, from_error_handler_type __from_error_handler,
			to_error_handler_type __to_error_handler, from_state_type __from_state, to_state_type __to_state)
		: __base_from_encoding_t(::std::move(__from_encoding))
		, __base_to_encoding_t(::std::move(__to_encoding))
		, __base_from_error_handler_t(::std::move(__from_error_handler))
		, __base_to_error_handler_t(::std::move(__to_error_handler))
		, __base_from_state_t(this->from_encoding(), ::std::move(__from_state))
		, __base_to_state_t(this->to_encoding(), ::std::move(__to_state))
		, __base_cursor_cache_t()
		, __base_range_t(::std::move(__range))
		, _M_cache() {
			this->_M_read_one();
		}

		//////
		/// @brief Copy assigns- a transcode_iterator.
		///
		//////
		constexpr transcode_iterator& operator=(const transcode_iterator&) = default;
		//////
		/// @brief Move assigns a transcode_iterator.
		///
		//////
		constexpr transcode_iterator& operator=(transcode_iterator&&) = default;

		// observers

		//////
		/// @brief The decoding ("from") encoding object.
		///
		/// @returns A const l-value reference to the encoding object used to construct this iterator.
		//////
		constexpr const from_encoding_type& from_encoding() const {
			return this->__base_from_encoding_t::__get_value();
		}

		//////
		/// @brief The decoding ("from") encoding object.
		///
		/// @returns An l-value reference to the encoding object used to construct this iterator.
		//////
		constexpr from_encoding_type& from_encoding() {
			return this->__base_from_encoding_t::__get_value();
		}

		//////
		/// @brief The encoding ("to") encoding object.
		///
		/// @returns A const l-value reference to the encoding object used to construct this iterator.
		//////
		constexpr const to_encoding_type& to_encoding() const {
			return this->__base_to_encoding_t::__get_value();
		}

		//////
		/// @brief The encoding ("to") encoding object.
		///
		/// @returns An l-value reference to the encoding object used to construct this iterator.
		//////
		constexpr to_encoding_type& to_encoding() {
			return this->__base_to_encoding_t::__get_value();
		}

		//////
		/// @brief The decoding ("from") state object.
		///
		//////
		constexpr const from_state_type& from_state() const {
			return this->__base_from_state_t::_M_get_state();
		}

		//////
		/// @brief The decoding ("from") state object.
		///
		//////
		constexpr from_state_type& from_state() {
			return this->__base_from_state_t::_M_get_state();
		}

		//////
		/// @brief The encoding ("to") state object.
		///
		//////
		constexpr const to_state_type& to_state() const {
			return this->__base_to_state_t::_M_get_state();
		}

		//////
		/// @brief The encoding ("to") state object.
		///
		//////
		constexpr to_state_type& to_state() {
			return this->__base_to_state_t::_M_get_state();
		}

		//////
		/// @brief The error handler object.
		///
		//////
		constexpr const from_error_handler_type& from_handler() const {
			return this->__base_from_error_handler_t::__get_value();
		}

		//////
		/// @brief The error handler object.
		///
		//////
		constexpr from_error_handler_type& from_handler() {
			return this->__base_from_error_handler_t::__get_value();
		}

		//////
		/// @brief The error handler object.
		///
		//////
		constexpr const to_error_handler_type& to_handler() const& noexcept {
			return this->__base_to_error_handler_t::__get_value();
		}

		//////
		/// @brief The error handler object.
		///
		//////
		constexpr to_error_handler_type& to_handler() & noexcept {
			return this->__base_to_error_handler_t::__get_value();
		}

		//////
		/// @brief The error handler object.
		///
		//////
		constexpr to_error_handler_type&& to_handler() && noexcept {
			return ::std::move(this->__base_to_error_handler_t::__get_value());
		}

		//////
		/// @brief The input range used to construct this object.
		///
		//////
		constexpr range_type range() & noexcept(::std::is_copy_constructible_v<range_type>
			     ? ::std::is_nothrow_copy_constructible_v<range_type>
			     : ::std::is_nothrow_move_constructible_v<range_type>) {
			if constexpr (::std::is_copy_constructible_v<range_type>) {
				return this->__base_range_t::__get_value();
			}
			else {
				return ::std::move(this->__base_range_t::__get_value());
			}
		}

		//////
		/// @brief The input range used to construct this object.
		///
		//////
		constexpr range_type range() const& noexcept(::std::is_nothrow_copy_constructible_v<range_type>) {
			return this->__base_range_t::__get_value();
		}

		//////
		/// @brief The input range used to construct this object.
		///
		//////
		constexpr range_type range() && noexcept(::std::is_nothrow_move_constructible_v<range_type>) {
			return ::std::move(this->__base_range_t::__get_value());
		}

		//////
		/// @brief Whether or not the contained range is empty.
		///
		//////
		constexpr bool empty() const noexcept {
			if constexpr (__txt_detail::__is_detected_v<__txt_detail::__detect_adl_empty, _Range>) {
				return __txt_detail::__adl::__adl_empty(this->__base_range_t::__get_value());
			}
			else {
				return __txt_detail::__adl::__adl_begin(this->__base_range_t::__get_value())
					== __txt_detail::__adl::__adl_end(this->__base_range_t::__get_value());
			}
		}

		// observers and modifiers: iteration

		//////
		/// @brief Increment a copy of the iterator.
		///
		/// @returns A copy to the incremented iterator.
		//////
		constexpr transcode_iterator operator++(int) {
			transcode_iterator __copy = this;
			++__copy;
			return __copy;
		}

		//////
		/// @brief Increment the iterator.
		///
		/// @returns A reference to *this, after incrementing the iterator.
		//////
		constexpr transcode_iterator& operator++() {
			if constexpr (_IsCursorless) {
				this->_M_read_one();
			}
			else {
				++this->_M_position;
				if (this->_M_position == this->__base_cursor_cache_t::_M_size) {
					this->_M_read_one();
				}
			}
			return *this;
		}

		//////
		/// @brief Dereference the iterator.
		///
		/// @returns A value_type (NOT a reference) of the iterator.
		///
		/// @remarks This is a proxy iterator, and therefore only returns a value_type object and not a reference
		/// object. Encoding iterators are only readable, not writable.
		//////
		constexpr value_type operator*() const {
			if constexpr (_IsSingleValueType) {
				return this->_M_cache[0];
			}
			else {
				return this->_M_cache[this->__base_cursor_cache_t::_M_position];
			}
		}

		// observers: comparison

		//////
		/// @brief Compares whether or not this iterator has truly reached the end.
		///
		//////
		friend constexpr bool operator==(const transcode_iterator& __it, const transcode_sentinel&) {
			return __it.empty();
		}

		//////
		/// @brief Compares whether or not this iterator has truly reached the end.
		///
		//////
		friend constexpr bool operator==(const transcode_sentinel& __sen, const transcode_iterator& __it) {
			return __it == __sen;
		}

		//////
		/// @brief Compares whether or not this iterator has truly reached the end.
		///
		//////
		friend constexpr bool operator!=(const transcode_iterator& __it, const transcode_sentinel& __sen) {
			return !(__it == __sen);
		}

		//////
		/// @brief Compares whether or not this iterator has truly reached the end.
		///
		//////
		friend constexpr bool operator!=(const transcode_sentinel& __sen, const transcode_iterator& __it) {
			return !(__sen == __it);
		}

	private:
		constexpr void _M_read_one() noexcept {
			auto& __this_input_range = this->_M_range();
			auto __this_cache_begin  = this->_M_cache.data();
			decltype(__this_cache_begin) __this_cache_end {};
			if constexpr (_IsInputOrOutput) {
				auto __result = __txt_detail::__basic_transcode_one<__txt_detail::__consume::__no>(
					::std::move(__this_input_range), this->from_encoding(), this->_M_cache, this->to_encoding(),
					this->from_handler(), this->to_handler(), this->from_state(), this->to_state());
				__this_cache_end
					= __txt_detail::__adl::__adl_to_address(__txt_detail::__adl::__adl_begin(__result.output));
				if constexpr (!_IsErrorless) {
					this->__base_error_cache_t::_M_error_code = __result.error_code;
				}
				this->__base_range_t::__get_value() = ::std::move(__result.input);
			}
			else {
				auto __result = __txt_detail::__basic_transcode_one<__txt_detail::__consume::__no>(
					__this_input_range, this->from_encoding(), this->_M_cache, this->to_encoding(),
					this->from_handler(), this->to_handler(), this->from_state(), this->to_state());
				__this_cache_end
					= __txt_detail::__adl::__adl_to_address(__txt_detail::__adl::__adl_begin(__result.output));
				if constexpr (!_IsErrorless) {
					this->__base_error_cache_t::_M_error_code = __result.error_code;
				}
				this->__base_range_t::__get_value() = ::std::move(__result.input);
			}
			if constexpr (!_IsCursorless) {
				__base_cursor_cache_size_t __data_size
					= static_cast<__base_cursor_cache_size_t>(__this_cache_end - __this_cache_begin);
				ZTD_TEXT_ASSERT_MESSAGE_I_("size of produced value can never be bigger thanthe cache",
					static_cast<::std::size_t>(__data_size) <= this->_M_cache.size());
				this->__base_cursor_cache_t::_M_position = static_cast<__base_cursor_cache_size_t>(0);
				this->__base_cursor_cache_t::_M_size     = __data_size;
			}
		}

		constexpr _URange& _M_range() noexcept {
			return this->__base_range_t::__get_value();
		}

		constexpr const _URange& _M_range() const noexcept {
			return this->__base_range_t::__get_value();
		}

		::std::array<value_type, _MaxValues> _M_cache;
	};

	//////
	/// @}
	//////

	ZTD_TEXT_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::text

#include <ztd/text/detail/epilogue.hpp>

#endif // ZTD_TEXT_TRANSCODE_ITERATOR_HPP
