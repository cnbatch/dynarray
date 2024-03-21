/** @copyright
BSD 3-Clause License

Copyright (c) 2020, cnbatch
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*!***************************************************************//*!
 * @file   dynarray_lite.hpp
 * @brief  VLA for C++
 *
 * @author cnbatch
 * @date   January 2021
 *********************************************************************/

 /*! \mainpage VLA for C++: dynarray
  *
  * \section sec1 Depencencies
  *
  * C++ 17
  * 
  * C++ Standard Library
  * 
  * \section sec2 Project on Github:
  * 
  * https://github.com/cnbatch/dynarray
  */

#pragma once
#ifndef DYNARRAY_LITE_HPP
#define DYNARRAY_LITE_HPP

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#define CPP20_DYNARRAY_CONSTEXPR constexpr
#define CPP20_DYNARRAY_NODISCARD [[nodiscard]]
#else
#define CPP20_DYNARRAY_CONSTEXPR
#define CPP20_DYNARRAY_NODISCARD
#endif

namespace vla
{
	template<typename T, template<typename U> typename _Allocator>
	class dynarray;

	namespace internal_impl
	{
		template <typename T, template<typename U> typename _Allocator>
		struct inner_type
		{
			using value_type = T;
			enum { nested_level = 0 };
		};

		template <typename T, template<typename U> typename _Allocator>
		struct inner_type<dynarray<T, _Allocator>, _Allocator>
		{
			using value_type = typename inner_type<T, _Allocator>::value_type;
			enum { nested_level = inner_type<T, _Allocator>::nested_level + 1 };
		};

		template<typename Skip> CPP20_DYNARRAY_CONSTEXPR
		std::size_t expand_parameters(std::size_t count, const Skip &skip) { return count; }

		template<typename Skip, typename ... Args> CPP20_DYNARRAY_CONSTEXPR
		std::size_t expand_parameters(std::size_t count, const Skip &skip, Args&& ... args) { return count * expand_parameters(std::forward<Args>(args)...); }

	}	// internal namespace

	template<typename T>
	class vla_iterator
	{
		using self_value_type = vla_iterator<T>;
		using self_reference = vla_iterator<T> &;
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		CPP20_DYNARRAY_CONSTEXPR explicit vla_iterator(pointer ptr = nullptr) : dynarray_ptr(ptr) {}
		CPP20_DYNARRAY_CONSTEXPR vla_iterator(const vla_iterator<T> &other_iterator) : dynarray_ptr(other_iterator.dynarray_ptr) {}

		// operators

		CPP20_DYNARRAY_CONSTEXPR reference operator*() const noexcept { return *dynarray_ptr; }

		CPP20_DYNARRAY_CONSTEXPR pointer operator->() const noexcept { return dynarray_ptr; }

		CPP20_DYNARRAY_CONSTEXPR reference operator[](difference_type offset) const noexcept { return dynarray_ptr[offset]; }

		CPP20_DYNARRAY_CONSTEXPR self_reference operator=(const self_value_type & right_iterator) noexcept { dynarray_ptr = right_iterator.dynarray_ptr; return *this; }

		CPP20_DYNARRAY_CONSTEXPR self_reference operator=(pointer ptr) noexcept { dynarray_ptr = ptr; return *this; }

		CPP20_DYNARRAY_CONSTEXPR self_reference operator++() noexcept { ++dynarray_ptr; return *this; }

		CPP20_DYNARRAY_CONSTEXPR self_value_type operator++(int) noexcept { return self_value_type(dynarray_ptr++); }

		CPP20_DYNARRAY_CONSTEXPR self_reference operator--() noexcept { --dynarray_ptr; return *this; }

		CPP20_DYNARRAY_CONSTEXPR self_value_type operator--(int) noexcept { return self_value_type(dynarray_ptr--); }

		CPP20_DYNARRAY_CONSTEXPR self_reference operator+=(difference_type offset) noexcept { dynarray_ptr += offset; return *this; }

		CPP20_DYNARRAY_CONSTEXPR self_reference operator-=(difference_type offset) noexcept { dynarray_ptr -= offset; return *this; }

		CPP20_DYNARRAY_CONSTEXPR self_value_type operator+(difference_type offset) const noexcept { return self_value_type(dynarray_ptr) += offset; }

		CPP20_DYNARRAY_CONSTEXPR self_value_type operator-(difference_type offset) const noexcept { return self_value_type(dynarray_ptr - offset); }

		CPP20_DYNARRAY_CONSTEXPR difference_type operator-(const self_value_type & right_iterator) const noexcept { return dynarray_ptr - right_iterator.dynarray_ptr; }

		CPP20_DYNARRAY_CONSTEXPR bool operator==(const self_value_type &right_iterator) const noexcept { return dynarray_ptr == right_iterator.dynarray_ptr; }

#ifdef DYNARRAY_USING_CPP20
		CPP20_DYNARRAY_CONSTEXPR auto operator<=>(const self_value_type &right_iterator) const noexcept = default;
#else
		CPP20_DYNARRAY_CONSTEXPR bool operator!=(const self_value_type &right_iterator) const noexcept { return dynarray_ptr != right_iterator.dynarray_ptr; }

		CPP20_DYNARRAY_CONSTEXPR bool operator<(const self_value_type &right_iterator) const noexcept { return dynarray_ptr < right_iterator.dynarray_ptr; }

		CPP20_DYNARRAY_CONSTEXPR bool operator>(const self_value_type &right_iterator) const noexcept { return dynarray_ptr > right_iterator.dynarray_ptr; }

		CPP20_DYNARRAY_CONSTEXPR bool operator<=(const self_value_type &right_iterator) const noexcept { return dynarray_ptr <= right_iterator.dynarray_ptr; }

		CPP20_DYNARRAY_CONSTEXPR bool operator>=(const self_value_type &right_iterator) const noexcept { return dynarray_ptr >= right_iterator.dynarray_ptr; }
#endif

		friend CPP20_DYNARRAY_CONSTEXPR self_value_type operator+(typename vla_iterator::difference_type offset, const self_value_type &other) noexcept { return self_value_type(other) += offset; }

	private:
		pointer dynarray_ptr;
	};


	template<typename T, template<typename U> typename _Allocator = std::allocator>
	class dynarray
	{
		friend class dynarray<dynarray<T, _Allocator>, _Allocator>;
		using internal_value_type = typename internal_impl::inner_type<T, _Allocator>::value_type;
		using internal_pointer_type = internal_value_type *;

	public:

		// Member types

		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type &;
		using const_reference = const value_type &;
		using pointer = value_type *;
		using const_pointer = const value_type *;

		using iterator = vla_iterator<T>;
		using const_iterator = vla_iterator<const T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		using allocator_type = _Allocator<T>;

		// Member functions

		/*!
		 * @brief Default Constructor.
		 * Create a zero-size array.
		 */
		CPP20_DYNARRAY_CONSTEXPR dynarray() noexcept : array_allocator(allocator_type())
		{
			initialise();
		}

		/*!
		 * @brief Construct by count.
		 * Create a one-dimensional array.
		 *
		 * @param count The size (length) of array
		 */
		CPP20_DYNARRAY_CONSTEXPR dynarray(size_type count) : array_allocator(allocator_type())
		{
			initialise();
			allocate_array(count);
		}

		/*!
		 * @brief Construct by count and use your custom allocator.
		 * Create a one-dimensional array.
		 * 
		 * @param count The size (length) of array
		 * @param other_allocator Your custom allocator
		 */
		template<typename _Alloc_t, typename = std::enable_if_t<std::is_same_v<std::decay_t<_Alloc_t>, allocator_type>>>
		CPP20_DYNARRAY_CONSTEXPR dynarray(size_type count, _Alloc_t &&other_allocator) : array_allocator(other_allocator)
		{
			initialise();
			allocate_array(count);
		}

		/*!
		 * @brief Usage 1: Construct by multiple 'count'.\n
		 * Create a multi-dimensional array.\n
		 * Example A: dynarray<dynarray<int>> my_array(10, 10);         // creates a 2D array (10 × 10), initialise with default value (zero)\n
		 * Example B: dynarray<dynarray<int>> my_array(10, 10, 20);     // creates a 2D array (10 × 10), initialise with value (20)
		 * 
		 * Usage 2: Construct by a 'count' and initialise the elements with 'args'\n
		 * Create a one-dimensional array, use 'args' to initialise the array's elements.\n
		 * Example A: dynarray<int> my_array(100);                      // creates an array (100 elements), initialise with default value (zero)\n
		 * Example B: dynarray<int> my_array(100, 20);                  // creates an array (100 elements), initialise with value (20)
		 *
		 * @param count The first dimention
		 * @param ...args If 'sizeof...(args)' is greater than the level of nested array, the rest of arg(s) will be used for initial array's elements.
		 */
		template<typename ... Args>
		CPP20_DYNARRAY_CONSTEXPR dynarray(size_type count, Args&& ... args) : array_allocator(allocator_type())
		{
			initialise();
			allocate_array(count, std::forward<Args>(args)...);
		}

		/*!
		 * @brief Construct by multiple 'count' and use your custom allocator.
		 * 
		 * @param count The first dimension
		 * @param other_allocator Your custom allocator
		 * @param ...args If 'sizeof...(args)' is greater than the level of nested array, the rest of arg(s) will be used for initial array's elements.
		 */
		template<typename _Alloc_t, typename = std::enable_if_t<std::is_same_v<std::decay_t<_Alloc_t>, allocator_type>>, typename ... Args>
		CPP20_DYNARRAY_CONSTEXPR dynarray(size_type count, _Alloc_t &&other_allocator, Args&& ... args) : array_allocator(other_allocator)
		{
			initialise();
			allocate_array(count, other_allocator, std::forward<Args>(args)...);
		}

		/*!
		 * @brief Duplicate an existing dynarray.
		 * 
		 * @param other Another array to be copied
		 */
		CPP20_DYNARRAY_CONSTEXPR dynarray(const dynarray &other) : array_allocator(allocator_type())
		{
			initialise();
			copy_array(other);
		}

		/*!
		 * @brief Initialise with rvalue.
		 *
		 * @param other Another array
		 */
		CPP20_DYNARRAY_CONSTEXPR dynarray(dynarray &&other) noexcept : array_allocator(allocator_type())
		{
			move_array(other);
		}

		/*!
		 * @brief Duplicate an existing dynarray and use your custom allocator.
		 * 
		 * @param other Another array to be copied
		 * @param other_allocator Allocator for the first level
		 * @param ...args Other dimention's allocator(s)
		 */
		template<typename ... Args>
		CPP20_DYNARRAY_CONSTEXPR dynarray(const dynarray &other, const allocator_type &other_allocator, Args&& ... args) : array_allocator(other_allocator)
		{
			initialise();
			copy_array(other, std::forward<Args>(args)...);
		}

		/*!
		 * @brief Duplicate an existing dynarray by using iterator.
		 * 
		 * @param other_begin begin(), cbegin() of iterator; or rbegin(), crbegin() of reverse iterator
		 * @param other_end end(), cend() of iterator; or rend(), crend() of reverse iterator
		 */

		template<typename InputIterator, typename = decltype(*std::declval<InputIterator&>(), ++std::declval<InputIterator&>(), void())>
		CPP20_DYNARRAY_CONSTEXPR dynarray(InputIterator other_begin, InputIterator other_end) : array_allocator(allocator_type())
		{
			initialise();
			copy_array(other_begin, other_end);
		}

		/*!
		 * @brief Create a one-dimensional array with initializer_list and use your allocator (if any)
		 * 
		 * @param input_list Your initializer_list
		 * @param other_allocator Your allocator (if any)
		 */
		template<typename ... Args>
		CPP20_DYNARRAY_CONSTEXPR dynarray(std::initializer_list<T> input_list, const allocator_type &other_allocator = allocator_type(), Args&& ...args)
			: array_allocator(other_allocator)
		{
			initialise();
			allocate_array(input_list, std::forward<Args>(args)...);
		}

		/*!
		 * @brief Copy an existing dynarray.
		 * 
		 * If using operator= in nested dynarray, the original structure will not change. Replace original values only.
		 * 
		 * @param other The right side of '='
		 * @return A copied dynarray
		 */
		CPP20_DYNARRAY_CONSTEXPR dynarray& operator=(const dynarray &other) noexcept
		{
			loop_copy(other);
			return *this;
		}

		/*!
		 * @brief Save an temporary created dynarray.
		 * 
		 * If using operator= in nested dynarray, the original structure will not change. Replace original values only.
		 * 
		 * Unless the size of current dynarray is zero.
		 * 
		 * @param other The right side of '='
		 * @return A new dynarray
		 */
		CPP20_DYNARRAY_CONSTEXPR dynarray& operator=(dynarray &&other) noexcept
		{
			if (current_dimension_array_size > 0 && current_dimension_array_size != other.current_dimension_array_size)
				move_values(other);
			else
			{
				deallocate_array();
				move_array(other);
			}
			return *this;
		}

		/*!
		 * @brief Erase existing data and construct a new dynarray with initializer_list.
		 * 
		 * @param input_list Your initializer_list
		 * @return A new dynarray
		 */
		CPP20_DYNARRAY_CONSTEXPR dynarray& operator=(std::initializer_list<T> input_list) noexcept
		{
			loop_copy(input_list);
			return *this;
		}

		/*!
		 * @brief Erase existing data and construct a new dynarray with initializer_list.
		 *
		 * @param input_list Your initializer_list
		 * @return A new dynarray
		 */
		template<typename Ty>
		CPP20_DYNARRAY_CONSTEXPR dynarray& operator=(std::initializer_list<std::initializer_list<Ty>> input_list) noexcept
		{
			loop_copy(input_list);
			return *this;
		}

		/*!
		 * @brief Deconstruct.
		 * 
		 */
		CPP20_DYNARRAY_CONSTEXPR ~dynarray()
		{
			deallocate_array();
		}

		// Element access

		/*!
		 * @brief Returns a reference to the element at specified location pos, with bounds checking.
		 * 
		 * If pos is not within the range of the container, an exception of type std::out_of_range is thrown.
		 * 
		 * @param pos Position of the element to return
		 * @return Reference to the requested element
		*/
		CPP20_DYNARRAY_CONSTEXPR reference at(size_type pos);

		/*!
		 * @brief Returns a const reference to the element at specified location pos, with bounds checking.
		 *
		 * If pos is not within the range of the container, an exception of type std::out_of_range is thrown.
		 *
		 * @param pos Position of the element to return
		 * @return Reference to the requested element
		*/
		CPP20_DYNARRAY_CONSTEXPR const_reference at(size_type pos) const;

		/*!
		 * Returns a reference to the element at specified location pos. No bounds checking is performed.
		 * 
		 * @param pos Position of the element to return
		 * @return Reference to the requested element
		*/
		CPP20_DYNARRAY_CONSTEXPR reference operator[](size_type pos);

		/*!
		 * Returns a const reference to the element at specified location pos. No bounds checking is performed.
		 *
		 * @param pos Position of the element to return
		 * @return Reference to the requested element
		*/
		CPP20_DYNARRAY_CONSTEXPR const_reference operator[](size_type pos) const;

		/*!
		 * @brief Returns a reference to the first element in the container.
		 * 
		 * Calling front() on an empty container should be undefined. But for everyone's convenience, nullptr is returned here.
		 * 
		 * @return Reference to the first element
		*/
		CPP20_DYNARRAY_CONSTEXPR reference front() { return (*this)[0]; }

		/*!
		 * @brief Returns a const reference to the first element in the container.
		 * 
		 * Calling front() on an empty container should be undefined. But for everyone's convenience, nullptr is returned here.
		 *
		 * @return Const reference to the first element
		*/
		CPP20_DYNARRAY_CONSTEXPR const_reference front() const { return (*this)[0]; }

		/*!
		 * @brief Returns a reference to the last element in the container.
		 * 
		 * Calling back() on an empty container should be undefined. But for everyone's convenience, nullptr is returned here.
		 *
		 * @return Reference to the last element
		*/
		CPP20_DYNARRAY_CONSTEXPR reference back();

		/*!
		 * @brief Returns a const reference to the first element in the container.
		 * 
		 * Calling back() on an empty container should be undefined. But for everyone's convenience, nullptr is returned here.
		 *
		 * @return Const reference to the first element
		*/
		CPP20_DYNARRAY_CONSTEXPR const_reference back() const;

		/*!
		 * @brief Returns pointer to the innermost underlying array serving as element storage.
		 * 
		 * For everyone's convenience, calling data() on an empty container will return nullptr.
		 *
		 * @return Pointer to the innermost underlying element storage.
		 * For non-empty containers, the returned pointer compares equal to the address of the first element.
		 * 
		*/
		CPP20_DYNARRAY_CONSTEXPR internal_pointer_type data() noexcept
		{
			if constexpr (std::is_same_v<T, internal_value_type>)
				return current_dimension_array_data;
			else return current_dimension_array_data->data();
		}

		/*!
		 * @brief Returns const pointer to the innermost underlying array serving as element storage.
		 * 
		 * For everyone's convenience, calling data() on an empty container will return nullptr.
		 *
		 * @return Const pointer to the innermost underlying element storage.
		 * For non-empty containers, the returned pointer compares equal to the address of the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const internal_pointer_type data() const noexcept
		{
			if constexpr (std::is_same_v<T, internal_value_type>)
				return current_dimension_array_data;
			else return current_dimension_array_data->data();
		}

		/*!
		 * @brief Returns pointer to the underlying array serving as element storage.
		 * 
		 * For everyone's convenience, calling data() on an empty container will return nullptr.
		 *
		 * @return Pointer to the underlying element storage.
		 * For non-empty containers, the returned pointer compares equal to the address of the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR pointer get() noexcept { return current_dimension_array_data; }

		/*!
		 * @brief Returns const pointer to the underlying array serving as element storage.
		 * 
		 * For everyone's convenience, calling data() on an empty container will return nullptr.
		 *
		 * @return Const pointer to the underlying element storage.
		 * For non-empty containers, the returned pointer compares equal to the address of the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const pointer get() const noexcept { return current_dimension_array_data; }

		/*!
		 * @brief Checks if the container has no elements.
		 * @return true if the container is empty, false otherwise
		*/
		CPP20_DYNARRAY_CONSTEXPR CPP20_DYNARRAY_NODISCARD bool empty() const noexcept { return !static_cast<bool>(size()); }

		/*!
		 * @brief Returns the number of elements in the container.
		 * @return The number of elements in the container.
		*/
		CPP20_DYNARRAY_CONSTEXPR size_type size() const noexcept { return current_dimension_array_size; }

		/*!
		 * @brief Returns the maximum number of elements the container is able to hold due to system or library implementation limitations.
		 * 
		 * This value typically reflects the theoretical limit on the size of the container, at most std::numeric_limits<difference_type>::max().
		 * At runtime, the size of the container may be limited to a value smaller than max_size() by the amount of RAM available.
		 * 
		 * @return Maximum number of elements.
		*/
		CPP20_DYNARRAY_CONSTEXPR size_type max_size() const noexcept { return std::numeric_limits<difference_type>::max(); }

		/*!
		 * @brief Exchanges the contents of the container with those of other.
		 * 
		 * Does not invoke any move, copy, or swap operations on individual elements.
		 * 
		 * All iterators and references remain valid. The past-the-end iterator is invalidated.
		 * 
		 * @param other dynarray to exchange the contents with
		*/
		CPP20_DYNARRAY_CONSTEXPR void swap(dynarray &other) noexcept;

		/*!
		 * @brief Assigns the given value value to all elements in the container.
		 * @param value The value to assign to the elements
		*/
		CPP20_DYNARRAY_CONSTEXPR void fill(const internal_value_type& value);


		// Iterators

		/*!
		 * @brief Returns an iterator to the first element of the vector.
		 * 
		 * If the vector is empty, the returned iterator will be equal to end().
		 * 
		 * @return Iterator to the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR iterator begin() noexcept { return iterator(current_dimension_array_data); }

		/*!
		 * @brief Returns an iterator to the first element of the vector.
		 *
		 * If the vector is empty, the returned iterator will be equal to end().
		 *
		 * @return Iterator to the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const_iterator begin() const noexcept { return const_iterator(current_dimension_array_data); }

		/*!
		 * @brief Returns an iterator to the first element of the vector.
		 *
		 * If the vector is empty, the returned iterator will be equal to end().
		 *
		 * @return Iterator to the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const_iterator cbegin() const noexcept { return begin(); }

		/*!
		 * @brief Returns an iterator to the element following the last element of the vector.
		 *
		 * This element acts as a placeholder; attempting to access it results in undefined behavior.
		 *
		 * @return Iterator to the element following the last element.
		*/
		CPP20_DYNARRAY_CONSTEXPR iterator end() noexcept { return iterator(current_dimension_array_data + current_dimension_array_size); }

		/*!
		 * @brief Returns an iterator to the element following the last element of the vector.
		 *
		 * This element acts as a placeholder; attempting to access it results in undefined behavior.
		 *
		 * @return Iterator to the element following the last element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const_iterator end() const noexcept { return const_iterator(current_dimension_array_data + current_dimension_array_size); }

		/*!
		 * @brief Returns an iterator to the element following the last element of the vector.
		 *
		 * This element acts as a placeholder; attempting to access it results in undefined behavior.
		 *
		 * @return Iterator to the element following the last element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const_iterator cend() const noexcept { return end(); }

		/*!
		 * @brief Returns a reverse iterator to the first element of the reversed vector.
		 *
		 * It corresponds to the last element of the non-reversed vector. If the vector is empty, the returned iterator is equal to rend().
		 *
		 * @return Reverse iterator to the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

		/*!
		 * @brief Returns a reverse iterator to the first element of the reversed vector.
		 *
		 * It corresponds to the last element of the non-reversed vector. If the vector is empty, the returned iterator is equal to rend().
		 *
		 * @return Reverse iterator to the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

		/*!
		 * @brief Returns a reverse iterator to the first element of the reversed vector.
		 *
		 * It corresponds to the last element of the non-reversed vector. If the vector is empty, the returned iterator is equal to rend().
		 *
		 * @return Reverse iterator to the first element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

		/*!
		 * @brief Returns a reverse iterator to the element following the last element of the reversed vector.
		 *
		 * It corresponds to the element preceding the first element of the non-reversed vector.
		 * This element acts as a placeholder, attempting to access it results in undefined behavior.
		 *
		 * @return Reverse iterator to the element following the last element.
		*/
		CPP20_DYNARRAY_CONSTEXPR reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

		/*!
		 * @brief Returns a reverse iterator to the element following the last element of the reversed vector.
		 *
		 * It corresponds to the element preceding the first element of the non-reversed vector.
		 * This element acts as a placeholder, attempting to access it results in undefined behavior.
		 *
		 * @return Reverse iterator to the element following the last element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

		/*!
		 * @brief Returns a reverse iterator to the element following the last element of the reversed vector.
		 *
		 * It corresponds to the element preceding the first element of the non-reversed vector.
		 * This element acts as a placeholder, attempting to access it results in undefined behavior.
		 *
		 * @return Reverse iterator to the element following the last element.
		*/
		CPP20_DYNARRAY_CONSTEXPR const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

	private:

		size_type current_dimension_array_size;
		pointer current_dimension_array_data;

		allocator_type array_allocator;


		CPP20_DYNARRAY_CONSTEXPR void initialise();

		template<typename Ty>
		static CPP20_DYNARRAY_CONSTEXPR size_type expand_list(std::initializer_list<Ty> init);

		template<typename ... Args>
		static CPP20_DYNARRAY_CONSTEXPR size_type expand_counts(size_type count, Args&& ... args);

		CPP20_DYNARRAY_CONSTEXPR void verify_size(size_type count);

		CPP20_DYNARRAY_CONSTEXPR void allocate_array(size_type count);

		template<typename ...Args>
		CPP20_DYNARRAY_CONSTEXPR void allocate_array(size_type count, Args&& ... args);

		template<typename _Alloc_t, typename = std::enable_if_t<std::is_same_v<std::decay_t<_Alloc_t>, allocator_type>>, typename ...Args>
		CPP20_DYNARRAY_CONSTEXPR void allocate_array(size_type count, _Alloc_t &&other_allocator, Args&& ... args);

		template<typename ... Args>
		CPP20_DYNARRAY_CONSTEXPR void allocate_array(std::initializer_list<T> input_list, Args&& ...args);

		CPP20_DYNARRAY_CONSTEXPR void deallocate_array();

		CPP20_DYNARRAY_CONSTEXPR void copy_array(const dynarray &other);

		template<typename ...Args>
		CPP20_DYNARRAY_CONSTEXPR void copy_array(const dynarray &other, Args&& ... args);

		template<typename InputIterator>
		CPP20_DYNARRAY_CONSTEXPR void copy_array(InputIterator other_begin, InputIterator other_end);

		CPP20_DYNARRAY_CONSTEXPR void loop_copy(const dynarray &other) noexcept;

		CPP20_DYNARRAY_CONSTEXPR void loop_copy(std::initializer_list<T> input_list) noexcept;

		template<typename Ty>
		CPP20_DYNARRAY_CONSTEXPR void loop_copy(std::initializer_list<std::initializer_list<Ty>> input_list) noexcept;

		CPP20_DYNARRAY_CONSTEXPR void move_array(dynarray &other)noexcept;

		CPP20_DYNARRAY_CONSTEXPR void move_values(dynarray &other) noexcept;

		/**** Non-member functions  ***/

		friend CPP20_DYNARRAY_CONSTEXPR bool operator==(const dynarray &lhs, const dynarray &rhs)
		{
			return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}

#ifdef DYNARRAY_USING_CPP20
		friend CPP20_DYNARRAY_CONSTEXPR auto operator<=>(const dynarray &lhs, const dynarray &rhs)
		{
			return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}
#else
		friend bool operator!=(const dynarray &lhs, const dynarray &rhs)
		{
			return !(lhs == rhs);
		}

		friend bool operator<(const dynarray &lhs, const dynarray &rhs)
		{
			return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}

		friend bool operator>(const dynarray &lhs, const dynarray &rhs)
		{
			return rhs < lhs;
		}

		friend bool operator<=(const dynarray &lhs, const dynarray &rhs)
		{
			return !(rhs < lhs);
		}

		friend bool operator>=(const dynarray &lhs, const dynarray &rhs)
		{
			return !(lhs < rhs);
		}
#endif
	};

	template<typename T, template<typename U> typename _Allocator>
	template<typename Ty>
	inline CPP20_DYNARRAY_CONSTEXPR typename dynarray<T, _Allocator>::size_type
	dynarray<T, _Allocator>::expand_list(std::initializer_list<Ty> init)
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			return init.size();
		}
		else
		{
			size_type count = 0;
			for (Ty sub_list : init)
				count += T::expand_list(sub_list);
			return count;
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ...Args>
	inline CPP20_DYNARRAY_CONSTEXPR typename dynarray<T, _Allocator>::size_type
	dynarray<T, _Allocator>::expand_counts(size_type count, Args && ...args)
	{
		if constexpr (std::is_same_v<T, internal_value_type> || sizeof...(args) == 0)
			return count;
		else
			return T::expand_counts(std::forward<Args>(args)...) * count;
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::initialise()
	{
		current_dimension_array_size = 0;
		current_dimension_array_data = nullptr;
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::verify_size(size_type count)
	{
		if (count > static_cast<size_type>(std::numeric_limits<difference_type>::max()))
			throw std::length_error("array too long");
	}

	template<typename T, template<typename U> typename _Allocator> CPP20_DYNARRAY_CONSTEXPR
	inline void dynarray<T, _Allocator>::allocate_array(size_type count)
	{
		if (count == 0)
		{
			initialise();
			return;
		}

		verify_size(count);
		current_dimension_array_size = count;
		current_dimension_array_data = array_allocator.allocate(count);
		for (size_type i = 0; i < count; ++i)
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ... Args>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::allocate_array(size_type count, Args&& ...args)
	{
		verify_size(count);
		size_type each_block_size = 1;
		if constexpr (!std::is_same_v<T, internal_value_type>)
			each_block_size = T::expand_counts(std::forward<Args>(args)...);

		verify_size(each_block_size);

		size_type entire_array_size = each_block_size * count;
		verify_size(entire_array_size);

		const size_type nested_level = internal_impl::inner_type<T, _Allocator>::nested_level;
		if (nested_level > sizeof...(args) || entire_array_size == 0)
		{
			initialise();
			return;
		}

		current_dimension_array_size = count;
		current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
		for (size_type i = 0; i < current_dimension_array_size; ++i)
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i, std::forward<Args>(args)...);
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename _Alloc_t, typename, typename ... Args>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::allocate_array(size_type count, _Alloc_t &&other_allocator, Args&& ... args)
	{
		verify_size(count);

		size_type each_block_size = 1;
		if constexpr (!std::is_same_v<T, internal_value_type>)
		{
			each_block_size = internal_impl::expand_parameters(std::forward<Args>(args)...);
			verify_size(each_block_size);
		}

		size_type entire_array_size = each_block_size * count;
		verify_size(entire_array_size);

		const size_type nested_level = internal_impl::inner_type<T, _Allocator>::nested_level;
		if (nested_level * 2 > sizeof...(args) || entire_array_size == 0)
		{
			initialise();
			return;
		}

		current_dimension_array_size = count;
		current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
		for (size_type i = 0; i < current_dimension_array_size; ++i)
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i, std::forward<Args>(args)...);
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ... Args>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::allocate_array(std::initializer_list<T> input_list, Args&& ...args)
	{
		size_type count = input_list.size();
		if (count == 0) return;
		verify_size(count);
		current_dimension_array_size = count;
		current_dimension_array_data = array_allocator.allocate(count);
		auto list_iter = input_list.begin();
		for (size_type i = 0; i < count; ++i, ++list_iter)
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i, *list_iter, std::forward<Args>(args)...);
	}


	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::deallocate_array()
	{
		if (current_dimension_array_data)
		{
			for (size_type i = current_dimension_array_size; i != 0; --i)
				std::allocator_traits<allocator_type>::destroy(array_allocator, current_dimension_array_data + i - 1);
			array_allocator.deallocate(current_dimension_array_data, current_dimension_array_size);
			current_dimension_array_data = nullptr;
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::copy_array(const dynarray &other)
	{
		if (other.current_dimension_array_size == 0) return;
		current_dimension_array_size = other.current_dimension_array_size;

		current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
		for (size_type i = 0; i < current_dimension_array_size; ++i)
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i, *(other.current_dimension_array_data + i));
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ...Args>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::copy_array(const dynarray &other, Args&&... args)
	{
		if (other.current_dimension_array_size == 0) return;
		current_dimension_array_size = other.current_dimension_array_size;

		current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
		for (size_type i = 0; i < current_dimension_array_size; ++i)
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i,
															*(other.current_dimension_array_data + i), std::forward<Args>(args)...);
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename InputIterator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::copy_array(InputIterator other_begin, InputIterator other_end)
	{
		static_assert(std::is_same_v<std::decay_t<decltype(*other_begin)>, T> ||
			          std::is_convertible_v<std::decay_t<decltype(*other_begin)>, T>,
		              "invalid iterator, cannot convert to a valid dynarray");
		
		size_type count = static_cast<size_type>(std::abs(other_end - other_begin));
		if (count == 0) return;

		current_dimension_array_size = count;
		current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
		InputIterator other = other_begin;
		for (size_type i = 0; i < current_dimension_array_size; ++i, ++other)
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i, *other);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::loop_copy(const dynarray &other) noexcept
	{
		if (size() == 0 || other.size() == 0) return;

		for (size_type i = 0; i < current_dimension_array_size && i < other.current_dimension_array_size; ++i)
			*(current_dimension_array_data + i) = *(other.current_dimension_array_data + i);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::loop_copy(std::initializer_list<T> input_list) noexcept
	{
		size_type count = input_list.size();
		if (size() == 0 || count == 0) return;

		auto list_iter = input_list.begin();
		for (size_type i = 0; i < count && i < current_dimension_array_size; ++i, ++list_iter)
			*(current_dimension_array_data + i) = *list_iter;
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename Ty>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::loop_copy(std::initializer_list<std::initializer_list<Ty>> input_list) noexcept
	{
		size_type count = input_list.size();
		if (size() == 0 || count == 0) return;

		auto list_iter = input_list.begin();
		for (size_type i = 0; i < count && i < current_dimension_array_size; ++i, ++list_iter)
			(current_dimension_array_data + i)->loop_copy(*list_iter);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::move_array(dynarray &other) noexcept
	{
		if (other.current_dimension_array_size == 0) return;
		current_dimension_array_size = other.current_dimension_array_size;
		array_allocator = other.array_allocator;
		current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
		for (size_type i = 0; i < current_dimension_array_size; ++i)
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i, std::move(*(other.current_dimension_array_data + i)));

		//array_allocator = other.array_allocator;
		//current_dimension_array_size = other.current_dimension_array_size;
		//current_dimension_array_data = other.current_dimension_array_data;
		//other.initialise();
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void
	dynarray<T, _Allocator>::move_values(dynarray &other) noexcept
	{
		if (size() == 0 || other.size() == 0) return;

		for (size_type i = 0; i < current_dimension_array_size && i < other.current_dimension_array_size; ++i)
			current_dimension_array_data[i] = std::move(other.current_dimension_array_data[i]);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR typename dynarray<T, _Allocator>::reference
	dynarray<T, _Allocator>::at(size_type pos)
	{
		if (pos >= size())
			throw std::out_of_range("out of range, incorrect position");
		return (*this)[pos];
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR typename dynarray<T, _Allocator>::const_reference
	dynarray<T, _Allocator>::at(size_type pos) const
	{
		if (pos >= size())
			throw std::out_of_range("out of range, incorrect position");
		return (*this)[pos];
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR typename dynarray<T, _Allocator>::reference
	dynarray<T, _Allocator>::operator[](size_type pos)
	{
		return *(current_dimension_array_data + pos);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR typename dynarray<T, _Allocator>::const_reference
	dynarray<T, _Allocator>::operator[](size_type pos) const
	{
		return *(current_dimension_array_data + pos);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR typename dynarray<T, _Allocator>::reference
	dynarray<T, _Allocator>::back()
	{
		return (*this)[current_dimension_array_size - 1];
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR typename dynarray<T, _Allocator>::const_reference
	dynarray<T, _Allocator>::back() const
	{
		return (*this)[current_dimension_array_size - 1];
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void dynarray<T, _Allocator>::swap(dynarray &other) noexcept
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			difference_type length = std::min<difference_type>(current_dimension_array_size, other.current_dimension_array_size);
			for (difference_type i = 0; i < length; ++i)
				std::swap(*(current_dimension_array_data + i), *(other.current_dimension_array_data + i));
		}
		else
		{
			for (size_type i = 0; i < current_dimension_array_size && i < other.current_dimension_array_size; ++i)
				(current_dimension_array_data + i)->swap(other[i]);
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	inline CPP20_DYNARRAY_CONSTEXPR void dynarray<T, _Allocator>::fill(const internal_value_type & value)
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
			for (size_type i = 0; i < current_dimension_array_size; ++i)
				*(current_dimension_array_data + i) = value;
		else
			for (size_type i = 0; i < current_dimension_array_size; ++i)
				(current_dimension_array_data + i)->fill(value);
	}

}	// namespace vla


#endif //_VLA_HEADER_DYNARRAY_LITE_HPP_