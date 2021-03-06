# =============================================================================
#
# ztd.text
# Copyright © 2021 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
# Contact: opensource@soasis.org
#
# Commercial License Usage
# Licensees holding valid commercial ztd.text licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Shepherd's Oasis, LLC.
# For licensing terms and conditions see your agreement. For
# further information contact opensource@soasis.org.
#
# Apache License Version 2 Usage
# Alternatively, this file may be used under the terms of Apache License
# Version 2.0 (the "License") for non-commercial use; you may not use this
# file except in compliance with the License. You may obtain a copy of the
# License at
#
#		http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============================================================================>

# # Boost.Text barely builds on Windows or MinGW, and you can't even
# # include <boost/text/unencoded_rope.hpp> without graphemes somehow getting involved...
# # So! We will do this later.

#[[
file(GLOB ztd.text.examples.boost_text.sources source/*.cpp)

add_executable(ztd.text.examples.boost_text ${ztd.text.examples.boost_text.sources})
target_compile_definitions(ztd.text.examples.boost_text
	PRIVATE
	ZTD_TEXT_COMPILE_TIME_ENCODING_NAME="UTF-8"
)
if (MSVC)
	target_compile_options(ztd.text.examples.boost_text
		PRIVATE /std:c++latest /utf-8 /permissive-)
else()
	target_compile_options(ztd.text.examples.boost_text
		PRIVATE -std=c++2a -Wall -Werror -Wpedantic -fexec-charset=UTF-8)
endif()
target_include_directories(ztd.text.examples.boost_text
	PRIVATE
	${Boost_INCLUDE_DIRS}
	${CMAKE_CURRENT_BINARY_DIR}/Boost.Text-prefix/src/Boost.Text/include/
)
add_dependencies(ztd.text.examples.boost_text
	Boost.Text
)
target_link_libraries(ztd.text.examples.boost_text
	PRIVATE
	ztd::text
	${CMAKE_DL_LIBS}
)
add_test(NAME ztd.text.examples.boost_text COMMAND ztd.text.examples.boost_text)

]]
