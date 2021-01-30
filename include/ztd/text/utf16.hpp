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
// =============================================================================

#pragma once

#ifndef ZTD_TEXT_UTF16_HPP
#define ZTD_TEXT_UTF16_HPP

#include <ztd/text/version.hpp>

#include <ztd/text/code_point.hpp>
#include <ztd/text/code_unit.hpp>
#include <ztd/text/encode_result.hpp>
#include <ztd/text/decode_result.hpp>
#include <ztd/text/error_handler.hpp>
#include <ztd/text/forward.hpp>
#include <ztd/text/is_ignorable_error_handler.hpp>

#include <ztd/text/detail/empty_state.hpp>
#include <ztd/text/detail/range.hpp>

namespace ztd { namespace text {
	ZTD_TEXT_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __impl {
		//////
		/// @brief Internal tag for detecting a ztd::text-derved UTF-16 type.
		///
		/// @internal
		//////
		class __utf16_tag { };

		//////
		/// @brief An internal type meant to provide the bulk of the UTF-16 functionality.
		///
		/// @internal
		///
		/// @remarks Relies on CRTP.
		//////
		template <typename _Derived = void, typename _CodeUnit = char16_t, bool __surrogates_allowed = false>
		class __utf16_with : public __utf16_tag {
		public:
			//////
			/// @brief The state that can be used between calls to the encoder and decoder. It is an empty struct
			/// because there is no shift state to preserve between complete units of encoded information.
			//////
			using state = __detail::__empty_state;
			//////
			/// @brief The individual units that result from an encode operation or are used as input to a decode
			/// operation. For UTF-16 formats, this is usually char16_t, but this can change (see @ref
			/// ztd::text::basic_utf16).
			//////
			using code_unit = _CodeUnit;
			//////
			/// @brief The individual units that result from a decode operation or as used as input to an encode
			/// operation. For most encodings, this is going to be a Unicode Code Point or a Unicode Scalar Value.
			//////
			using code_point = unicode_code_point;
			//////
			/// @brief Whether or not the decode operation can process all forms of input into code point values.
			/// Thsi is true for all Unicode Transformation Formats (UTFs), which can encode and decode without a
			/// loss of information from a valid collection of code units.
			//////
			using is_decode_injective = ::std::true_type;
			//////
			/// @brief Whether or not the encode operation can process all forms of input into code unit values.
			/// This is true for all Unicode Transformation Formats (UTFs), which can encode and decode without loss
			/// of information from a valid input code point.
			//////
			using is_encode_injective = ::std::true_type;
			//////
			/// @brief The maximum number of code points a single complete operation of decoding can produce. This is
			/// 1 for all Unicode Transformation Format (UTF) encodings.
			//////
			inline static constexpr ::std::size_t max_code_points = 1;
			//////
			/// @brief The maximum code units a single complete operation of encoding can produce.
			//////
			inline static constexpr ::std::size_t max_code_units = 2;

		private:
			using __self_t = ::std::conditional_t<::std::is_void_v<_Derived>, __utf16_with, _Derived>;

		public:
			//////
			/// @brief Decodes a single complete unit of information as code points and produces a result with the
			/// input and output ranges moved past what was successfully read and written; or, produces an error and
			/// returns the input and output ranges untouched.
			///
			/// @param[in] __input The input view to read code uunits from.
			/// @param[in] __output The output view to write code points into.
			/// @param[in] __error_handler The error handler to invoke if encoding fails.
			/// @param[in, out] __s The necessary state information. For this encoding, the state is empty and means
			/// very little.
			///
			/// @returns A @ref ztd::text::decode_result object that contains the reconstructed input range,
			/// reconstructed output range, error handler, and a reference to the passed-in state.
			///
			/// @remarks To the best ability of the implementation, the iterators will be returned untouched (e.g.,
			/// the input models at least a view and a forward_range). If it is not possible, returned ranges may be
			/// incremented even if an error occurs due to the semantics of any view that models an input_range.
			//////
			template <typename _InputRange, typename _OutputRange, typename _ErrorHandler>
			static constexpr auto decode_one(
				_InputRange&& __input, _OutputRange&& __output, _ErrorHandler&& __error_handler, state& __s) {
				using _UInputRange   = __detail::__remove_cvref_t<_InputRange>;
				using _UOutputRange  = __detail::__remove_cvref_t<_OutputRange>;
				using _UErrorHandler = __detail::__remove_cvref_t<_ErrorHandler>;
				using _Result        = __detail::__reconstruct_decode_result_t<_UInputRange, _UOutputRange, state>;
				constexpr bool __call_error_handler = !is_ignorable_error_handler_v<_UErrorHandler>;

				auto __init   = __detail::__adl::__adl_cbegin(__input);
				auto __inlast = __detail::__adl::__adl_cend(__input);
				if constexpr (__call_error_handler) {
					if (__init == __inlast) {
						// an exhausted sequence is fine
						return _Result(::std::forward<_InputRange>(__input),
							::std::forward<_OutputRange>(__output), __s, encoding_error::ok);
					}
				}
				else {
					(void)__inlast;
				}

				auto __outit   = __detail::__adl::__adl_begin(__output);
				auto __outlast = __detail::__adl::__adl_end(__output);

				if constexpr (__call_error_handler) {
					if (__outit == __outlast) {
						__self_t __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::insufficient_output_space),
							::std::span<code_unit, 0>());
					}
				}
				else {
					(void)__outlast;
				}

				::std::array<code_unit, 2> __units {};
				__units[0]              = __detail::__dereference(__init);
				const code_unit& __lead = __units[0];
				__init                  = __detail::__next(__init);

				if (!__detail::__is_surrogate(__lead)) {
					__detail::__dereference(__outit) = static_cast<code_point>(__lead);
					__outit                          = __detail::__next(__outit);
					return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
						__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
						encoding_error::ok);
				}
				if constexpr (__call_error_handler) {
					if (!__detail::__is_lead_surrogate(__lead)) {
						__self_t __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::invalid_sequence),
							::std::span<code_unit, 1>(__units.data(), 1));
					}
				}
				if constexpr (__call_error_handler) {
					if (__init == __inlast) {
						__self_t __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::incomplete_sequence),
							::std::span<code_unit, 1>(__units.data(), 1));
					}
				}

				__units[1]               = __detail::__dereference(__init);
				const code_unit& __trail = __units[1];
				__init                   = __detail::__next(__init);
				if constexpr (__call_error_handler) {
					if (!__detail::__is_trail_surrogate(__trail)) {
						__self_t __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::invalid_sequence),
							::std::span<code_unit, 2>(__units.data(), 2));
					}
				}
				__detail::__dereference(__outit) = static_cast<code_point>(__detail::__utf16_combine_surrogates(
					static_cast<char16_t>(__lead), static_cast<char16_t>(__trail)));
				__outit                          = __detail::__next(__outit);

				return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
					__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
					encoding_error::ok);
			}

			//////
			/// @brief Encodes a single complete unit of information as code units and produces a result with the
			/// input and output ranges moved past what was successfully read and written; or, produces an error and
			/// returns the input and output ranges untouched.
			///
			/// @param[in] __input The input view to read code points from.
			/// @param[in] __output The output view to write code units into.
			/// @param[in] __error_handler The error handler to invoke if encoding fails.
			/// @param[in, out] __s The necessary state information. For this encoding, the state is empty and means
			/// very little.
			///
			/// @returns A @ref ztd::text::encode_result object that contains the reconstructed input range,
			/// reconstructed output range, error handler, and a reference to the passed-in state.
			///
			/// @remarks To the best ability of the implementation, the iterators will be returned untouched (e.g.,
			/// the input models at least a view and a forward_range). If it is not possible, returned ranges may be
			/// incremented even if an error occurs due to the semantics of any view that models an input_range.
			//////
			template <typename _InputRange, typename _OutputRange, typename _ErrorHandler>
			static constexpr auto encode_one(
				_InputRange&& __input, _OutputRange&& __output, _ErrorHandler&& __error_handler, state& __s) {
				using _UInputRange   = __detail::__remove_cvref_t<_InputRange>;
				using _UOutputRange  = __detail::__remove_cvref_t<_OutputRange>;
				using _UErrorHandler = __detail::__remove_cvref_t<_ErrorHandler>;
				using _Result        = __detail::__reconstruct_encode_result_t<_UInputRange, _UOutputRange, state>;
				constexpr bool __call_error_handler = !is_ignorable_error_handler_v<_UErrorHandler>;

				auto __init   = __detail::__adl::__adl_cbegin(__input);
				auto __inlast = __detail::__adl::__adl_cend(__input);
				if (__init == __inlast) {
					// an exhausted sequence is fine
					return _Result(__detail::__reconstruct(
						               ::std::in_place_type<_UInputRange>, ::std::forward<_InputRange>(__input)),
						__detail::__reconstruct(
						     ::std::in_place_type<_UOutputRange>, ::std::forward<_OutputRange>(__output)),
						__s, encoding_error::ok);
				}

				auto __outit   = __detail::__adl::__adl_begin(__output);
				auto __outlast = __detail::__adl::__adl_end(__output);

				if constexpr (__call_error_handler) {
					if (__outit == __outlast) {
						__self_t __self {};
						return __error_handler(__self,
							_Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>,
							             ::std::move(__init), ::std::move(__inlast)),
							     __detail::__reconstruct(::std::in_place_type<_UOutputRange>,
							          ::std::move(__outit), ::std::move(__outlast)),
							     __s, encoding_error::insufficient_output_space),
							::std::span<code_point, 0>());
					}
				}
				else {
					(void)__outlast;
				}


				code_point __points[1] {};
				__points[0]               = __detail::__dereference(__init);
				const code_point& __point = __points[0];
				__init                    = __detail::__next(__init);

				if constexpr (__call_error_handler) {
					if (__point > __detail::__last_code_point) {
						__self_t __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::invalid_sequence),
							::std::span<code_point, 1>(::std::addressof(__points[0]), 1));
					}
				}

				if (__point <= __detail::__last_bmp_value) {
					__detail::__dereference(__outit) = static_cast<char16_t>(__point);
					__outit                          = __detail::__next(__outit);
				}
				else {
					auto normal = __point - __detail::__normalizing_value;
					auto lead   = __detail::__first_lead_surrogate
						+ ((normal & __detail::__lead_surrogate_bitmask) >> __detail::__lead_shifted_bits);
					auto trail
						= __detail::__first_trail_surrogate + (normal & __detail::__trail_surrogate_bitmask);

					__detail::__dereference(__outit) = static_cast<char16_t>(lead);
					__outit                          = __detail::__next(__outit);

					if constexpr (__call_error_handler) {
						if (__outit == __outlast) {
							__self_t __self {};
							return __error_handler(__self,
								_Result(__detail::__reconstruct(
								             ::std::in_place_type<_UInputRange>, __init, __inlast),
								     __detail::__reconstruct(
								          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
								     __s, encoding_error::insufficient_output_space),
								::std::span<code_point, 1>(::std::addressof(__points[0]), 1));
						}
					}
					__detail::__dereference(__outit) = static_cast<char16_t>(trail);
					__outit                          = __detail::__next(__outit);
				}

				return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
					__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
					encoding_error::ok);
			}
		};
	} // namespace __impl

	//////
	/// @addtogroup ztd_text_encodings Encodings
	/// @{
	//////

	//////
	/// @brief A UTF-16 Encoding that traffics in, specifically, the desired code unit type provided as a template
	/// argument.
	///
	/// @tparam _Type The code unit type to use.
	///
	/// @remarks This is a strict UTF-16 implementation that does not allow lone, unpaired surrogates either in or out.
	//////
	template <typename _Type>
	class basic_utf16 : public __impl::__utf16_with<basic_utf16<_Type>, _Type> { };

	//////
	/// @brief A UTF-16 Encoding that traffics in char16_t. See @ref ztd::text::basic_utf16 for more details.
	///
	//////
	using utf16 = basic_utf16<char16_t>;

	//////
	/// @}
	//////



	ZTD_TEXT_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::text

#endif // ZTD_TEXT_UTF16_HPP
