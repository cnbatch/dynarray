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
 * @file   dynarray.hpp
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
  */

#pragma once
#ifndef _VLA_DYNARRAY_
#define _VLA_DYNARRAY_

#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <type_traits>
#include <utility>

namespace vla
{
	namespace internal_impl
	{
		template <typename T, typename _Allocator, typename = void>
		struct inner_type
		{
			using value_type = T;
			enum { nested_level = 0 };
		};

		template <typename T, typename _Allocator>
		struct inner_type<T, _Allocator, std::void_t<typename T::value_type>> : inner_type<typename T::value_type, _Allocator>
		{
			using value_type = typename inner_type<T, _Allocator>::value_type;
			enum { nested_level = inner_type<T, _Allocator>::nested_level + 1 };
		};

		template<typename Skip>
		std::size_t expand_parameters(std::size_t count, const Skip &skip) { return count; }

		template<typename Skip, typename ... Args>
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

		explicit vla_iterator(pointer ptr = nullptr) : dynarray_ptr(ptr) {}
		vla_iterator(const vla_iterator<T> &other_iterator) : dynarray_ptr(other_iterator.dynarray_ptr) {}

		// operators

		reference operator*() const noexcept { return *dynarray_ptr; }

		pointer operator->() const noexcept { return dynarray_ptr; }

		reference operator[](difference_type offset) const noexcept { return dynarray_ptr[offset]; }

		self_reference operator=(const self_value_type & right_iterator) noexcept { dynarray_ptr = right_iterator.dynarray_ptr; return *this; }

		self_reference operator=(pointer ptr) noexcept { dynarray_ptr = ptr; return *this; }

		self_reference operator++() noexcept { ++dynarray_ptr; return *this; }

		self_value_type operator++(int) noexcept { return self_value_type(dynarray_ptr++); }

		self_reference operator--() noexcept { --dynarray_ptr; return *this; }

		self_value_type operator--(int) noexcept { return self_value_type(dynarray_ptr--); }

		self_reference operator+=(difference_type offset) noexcept { dynarray_ptr += offset; return *this; }

		self_reference operator-=(difference_type offset) noexcept { dynarray_ptr -= offset; return *this; }

		self_value_type operator+(difference_type offset) const noexcept { return self_value_type(dynarray_ptr) += offset; }

		self_value_type operator-(difference_type offset) const noexcept { return self_value_type(dynarray_ptr - offset); }

		difference_type operator-(const self_value_type & right_iterator) const noexcept { return dynarray_ptr - right_iterator.dynarray_ptr; }

		bool operator==(const self_value_type &right_iterator) const noexcept { return dynarray_ptr == right_iterator.dynarray_ptr; }

		bool operator!=(const self_value_type &right_iterator) const noexcept { return dynarray_ptr != right_iterator.dynarray_ptr; }

		bool operator<(const self_value_type &right_iterator) const noexcept { return dynarray_ptr < right_iterator.dynarray_ptr; }

		bool operator>(const self_value_type &right_iterator) const noexcept { return dynarray_ptr > right_iterator.dynarray_ptr; }

		bool operator<=(const self_value_type &right_iterator) const noexcept { return dynarray_ptr <= right_iterator.dynarray_ptr; }

		bool operator>=(const self_value_type &right_iterator) const noexcept { return dynarray_ptr >= right_iterator.dynarray_ptr; }

	private:
		pointer dynarray_ptr;
	};

	template<typename T>
	vla_iterator<T> operator+(typename vla_iterator<T>::difference_type offset, const vla_iterator<T> &other) noexcept
	{
		return vla_iterator<T>(other) += offset;
	}

	template<typename T, template<typename U> typename _Allocator = std::allocator>
	class dynarray
	{
		friend class dynarray<dynarray<T, _Allocator>, _Allocator>;
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
	private:
		using internal_value_type = typename internal_impl::inner_type<T, allocator_type>::value_type;
		using internal_pointer_type = internal_value_type *;
	public:
		using contiguous_allocator_type = _Allocator<internal_value_type>;

		// Member functions

		/*!
		 * @brief Default Constructor.
		 * Create a zero-size array.
		 */
		dynarray()
		{
			initialise();
		}

		/*!
		 * @brief Construct by count.
		 * Create a one-dimensional array.
		 *
		 * @param count The size (length) of array
		 */
		dynarray(size_type count)
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
		dynarray(size_type count, allocator_type &&other_allocator)
		{
			initialise(other_allocator);
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
		dynarray(size_type count, Args&& ... args)
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
		dynarray(size_type count, _Alloc_t &&other_allocator, Args&& ... args)
		{
			initialise(other_allocator);
			allocate_array(count, other_allocator, std::forward<Args>(args)...);
		}

		/*!
		 * @brief Duplicate an existing dynarray.
		 * 
		 * @param other Another array to be copied
		 */
		dynarray(const dynarray &other)
		{
			initialise();
			copy_array(other);
		}

		/*!
		 * @brief Initialise with rvalue.
		 *
		 * @param other Another array
		 */
		dynarray(dynarray &&other)
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
		dynarray(const dynarray &other, const allocator_type &other_allocator, Args&& ... args)
		{
			initialise(other_allocator);
			copy_array(other, other_allocator, std::forward<Args>(args)...);
		}

		/*!
		 * @brief Duplicate an existing dynarray by using iterator.
		 * 
		 * @param other_begin begin(), cbegin() of iterator; or rbegin(), crbegin() of reverse iterator
		 * @param other_end end(), cend() of iterator; or rend(), crend() of reverse iterator
		 */

		template<typename InputIterator, typename = decltype(*std::declval<InputIterator&>(), ++std::declval<InputIterator&>(), void())>
		dynarray(InputIterator other_begin, InputIterator other_end)
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
		dynarray(std::initializer_list<T> input_list, const allocator_type &other_allocator = allocator_type())
		{
			initialise(other_allocator);
			allocate_array(input_list);
		}

		/*!
		 * @brief Create a multiple-dimensional array with initializer_list.
		 * 
		 * @param input_listYour initializer_list
		 */
		template<typename Ty>
		dynarray(std::initializer_list<std::initializer_list<Ty>> input_list)
		{
			initialise();
			allocate_array(input_list);
		}

		/*!
		 * @brief Copy an existing dynarray.
		 * 
		 * If using operator= in nested dynarray, the original structure will not change. Replace original values only.
		 * 
		 * @param right_dynarray The right side of '='
		 * @return A copied dynarray
		 */
		dynarray& operator=(const dynarray &right_dynarray) noexcept
		{
			if (entire_array_data || nullptr == this_level_array_head)
			{
				deallocate_array();
				copy_array(right_dynarray);
			}
			else
			{
				loop_copy(right_dynarray);
			}
			return *this;
		}

		/*!
		 * @brief Save an temporary created dynarray.
		 * 
		 * If using operator= in nested dynarray, the original structure will not change. Replace original values only.
		 * 
		 * @param right_dynarray The right side of '='
		 * @return A new dynarray
		 */
		dynarray& operator=(dynarray &&right_dynarray) noexcept
		{
			if (entire_array_data || nullptr == this_level_array_head)
			{
				deallocate_array();
				move_array(right_dynarray);
			}
			else
			{
				loop_copy(right_dynarray);
			}
			return *this;
		}

		/*!
		 * @brief Erase existing data and construct a new dynarray with initializer_list.
		 * 
		 * @param input_list Your initializer_list
		 * @return A new dynarray
		 */
		dynarray& operator=(std::initializer_list<T> input_list) noexcept
		{
			if (entire_array_data || nullptr == this_level_array_head)
			{
				deallocate_array();
				allocate_array(input_list);
			}
			else
			{
				loop_copy(input_list);
			}
			return *this;
		}

		/*!
		 * @brief Erase existing data and construct a new dynarray with initializer_list.
		 *
		 * @param input_list Your initializer_list
		 * @return A new dynarray
		 */
		template<typename Ty>
		dynarray& operator=(std::initializer_list<std::initializer_list<Ty>> input_list) noexcept
		{
			if (entire_array_data || nullptr == this_level_array_head)
			{
				deallocate_array();
				allocate_array(input_list);
			}
			else
			{
				loop_copy(input_list);
			}
			return *this;
		}

		/*!
		 * @brief Deconstruct.
		 * 
		 */
		~dynarray()
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
		reference at(size_type pos);

		/*!
		 * @brief Returns a const reference to the element at specified location pos, with bounds checking.
		 *
		 * If pos is not within the range of the container, an exception of type std::out_of_range is thrown.
		 *
		 * @param pos Position of the element to return
		 * @return Reference to the requested element
		*/
		const_reference at(size_type pos) const;

		/*!
		 * Returns a reference to the element at specified location pos. No bounds checking is performed.
		 * 
		 * @param pos Position of the element to return
		 * @return Reference to the requested element
		*/
		reference operator[](size_type pos);

		/*!
		 * Returns a const reference to the element at specified location pos. No bounds checking is performed.
		 *
		 * @param pos Position of the element to return
		 * @return Reference to the requested element
		*/
		constexpr const_reference operator[](size_type pos) const;

		/*!
		 * @brief Returns a reference to the first element in the container.
		 * 
		 * Calling front() on an empty container should be undefined. But for everyone's convenience, nullptr is returned here.
		 * 
		 * @return Reference to the first element
		*/
		reference front() { return (*this)[0]; }

		/*!
		 * @brief Returns a const reference to the first element in the container.
		 * 
		 * Calling front() on an empty container should be undefined. But for everyone's convenience, nullptr is returned here.
		 *
		 * @return Const reference to the first element
		*/
		const_reference front() const { return (*this)[0]; }

		/*!
		 * @brief Returns a reference to the last element in the container.
		 * 
		 * Calling back() on an empty container should be undefined. But for everyone's convenience, nullptr is returned here.
		 *
		 * @return Reference to the last element
		*/
		reference back();

		/*!
		 * @brief Returns a const reference to the first element in the container.
		 * 
		 * Calling back() on an empty container should be undefined. But for everyone's convenience, nullptr is returned here.
		 *
		 * @return Const reference to the first element
		*/
		const_reference back() const;

		/*!
		 * @brief Returns pointer to the innermost underlying array serving as element storage.
		 * 
		 * For everyone's convenience, calling data() on an empty container will return nullptr.
		 *
		 * @return Pointer to the innermost underlying element storage.
		 * For non-empty containers, the returned pointer compares equal to the address of the first element.
		 * 
		*/
		internal_pointer_type data() noexcept { return this_level_array_head; }

		/*!
		 * @brief Returns const pointer to the innermost underlying array serving as element storage.
		 * 
		 * For everyone's convenience, calling data() on an empty container will return nullptr.
		 *
		 * @return Const pointer to the innermost underlying element storage.
		 * For non-empty containers, the returned pointer compares equal to the address of the first element.
		*/
		const internal_pointer_type data() const noexcept { return this_level_array_head; }

		/*!
		 * @brief Returns pointer to the underlying array serving as element storage.
		 * 
		 * For everyone's convenience, calling data() on an empty container will return nullptr.
		 *
		 * @return Pointer to the underlying element storage.
		 * For non-empty containers, the returned pointer compares equal to the address of the first element.
		*/
		pointer get() noexcept { return current_dimension_array_data; }

		/*!
		 * @brief Returns const pointer to the underlying array serving as element storage.
		 * 
		 * For everyone's convenience, calling data() on an empty container will return nullptr.
		 *
		 * @return Const pointer to the underlying element storage.
		 * For non-empty containers, the returned pointer compares equal to the address of the first element.
		*/
		const pointer get() const noexcept { return current_dimension_array_data; }

		/*!
		 * @brief Checks if the container has no elements.
		 * @return true if the container is empty, false otherwise
		*/
		bool empty() const noexcept { return static_cast<bool>(current_dimension_array_size); }

		/*!
		 * @brief Returns the number of elements in the container.
		 * @return The number of elements in the container.
		*/
		size_type size() const noexcept { return current_dimension_array_size; }

		/*!
		 * @brief Returns the maximum number of elements the container is able to hold due to system or library implementation limitations.
		 * 
		 * This value typically reflects the theoretical limit on the size of the container, at most std::numeric_limits<difference_type>::max().
		 * At runtime, the size of the container may be limited to a value smaller than max_size() by the amount of RAM available.
		 * 
		 * @return Maximum number of elements.
		*/
		size_type max_size() const noexcept { return std::numeric_limits<difference_type>::max(); }

		/*!
		 * @brief Exchanges the contents of the container with those of other.
		 * 
		 * Does not invoke any move, copy, or swap operations on individual elements.
		 * 
		 * All iterators and references remain valid. The past-the-end iterator is invalidated.
		 * 
		 * @param other dynarray to exchange the contents with
		*/
		void swap(dynarray &other) noexcept;

		/*!
		 * @brief Assigns the given value value to all elements in the container.
		 * @param value The value to assign to the elements
		*/
		void fill(const internal_value_type& value);


		// Iterators

		/*!
		 * @brief Returns an iterator to the first element of the vector.
		 * 
		 * If the vector is empty, the returned iterator will be equal to end().
		 * 
		 * @return Iterator to the first element.
		*/
		iterator begin() noexcept { return iterator(current_dimension_array_data); }

		/*!
		 * @brief Returns an iterator to the first element of the vector.
		 *
		 * If the vector is empty, the returned iterator will be equal to end().
		 *
		 * @return Iterator to the first element.
		*/
		const_iterator begin() const noexcept { return const_iterator(current_dimension_array_data); }

		/*!
		 * @brief Returns an iterator to the first element of the vector.
		 *
		 * If the vector is empty, the returned iterator will be equal to end().
		 *
		 * @return Iterator to the first element.
		*/
		const_iterator cbegin() const noexcept { return begin(); }

		/*!
		 * @brief Returns an iterator to the element following the last element of the vector.
		 *
		 * This element acts as a placeholder; attempting to access it results in undefined behavior.
		 *
		 * @return Iterator to the element following the last element.
		*/
		iterator end() noexcept { return iterator(current_dimension_array_data + current_dimension_array_size); }

		/*!
		 * @brief Returns an iterator to the element following the last element of the vector.
		 *
		 * This element acts as a placeholder; attempting to access it results in undefined behavior.
		 *
		 * @return Iterator to the element following the last element.
		*/
		const_iterator end() const noexcept { return const_iterator(current_dimension_array_data + current_dimension_array_size); }

		/*!
		 * @brief Returns an iterator to the element following the last element of the vector.
		 *
		 * This element acts as a placeholder; attempting to access it results in undefined behavior.
		 *
		 * @return Iterator to the element following the last element.
		*/
		const_iterator cend() const noexcept { return end(); }

		/*!
		 * @brief Returns a reverse iterator to the first element of the reversed vector.
		 *
		 * It corresponds to the last element of the non-reversed vector. If the vector is empty, the returned iterator is equal to rend().
		 *
		 * @return Reverse iterator to the first element.
		*/
		reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

		/*!
		 * @brief Returns a reverse iterator to the first element of the reversed vector.
		 *
		 * It corresponds to the last element of the non-reversed vector. If the vector is empty, the returned iterator is equal to rend().
		 *
		 * @return Reverse iterator to the first element.
		*/
		const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

		/*!
		 * @brief Returns a reverse iterator to the first element of the reversed vector.
		 *
		 * It corresponds to the last element of the non-reversed vector. If the vector is empty, the returned iterator is equal to rend().
		 *
		 * @return Reverse iterator to the first element.
		*/
		const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

		/*!
		 * @brief Returns a reverse iterator to the element following the last element of the reversed vector.
		 *
		 * It corresponds to the element preceding the first element of the non-reversed vector.
		 * This element acts as a placeholder, attempting to access it results in undefined behavior.
		 *
		 * @return Reverse iterator to the element following the last element.
		*/
		reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

		/*!
		 * @brief Returns a reverse iterator to the element following the last element of the reversed vector.
		 *
		 * It corresponds to the element preceding the first element of the non-reversed vector.
		 * This element acts as a placeholder, attempting to access it results in undefined behavior.
		 *
		 * @return Reverse iterator to the element following the last element.
		*/
		const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

		/*!
		 * @brief Returns a reverse iterator to the element following the last element of the reversed vector.
		 *
		 * It corresponds to the element preceding the first element of the non-reversed vector.
		 * This element acts as a placeholder, attempting to access it results in undefined behavior.
		 *
		 * @return Reverse iterator to the element following the last element.
		*/
		const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

	private:

		allocator_type array_allocator;
		contiguous_allocator_type contiguous_allocator;


		internal_pointer_type entire_array_data;	// always nullptr in nested-dynarray

		size_type current_dimension_array_size;
		pointer current_dimension_array_data;	// as node managers if not innermost layer

		internal_pointer_type this_level_array_head;
		internal_pointer_type this_level_array_tail;

		void initialise(const allocator_type &other_allocator = allocator_type());

		size_type get_block_size() const { return static_cast<size_type>(this_level_array_tail - this_level_array_head + 1); }

		template<typename Ty>
		static size_type expand_list(std::initializer_list<Ty> init);

		template<typename ... Args>
		static size_type expand_counts(size_type count, Args&& ... args);

		template<typename ... Args>
		static contiguous_allocator_type expand_allocator(const allocator_type &_allocator, Args&& ... args);

		template<typename Skip, typename ... Args>
		static contiguous_allocator_type expand_allocators(const Skip &ignore, const allocator_type &_allocator, Args&& ... args);

		void verify_size(size_type count);

		void allocate_array(size_type count);

		void allocate_array(internal_pointer_type starting_address, size_type count);

		template<typename ...Args>
		void allocate_array(size_type count, Args&& ... args);

		template<typename ...Args>
		void allocate_array(internal_pointer_type starting_address, size_type count, Args&& ... args);

		template<typename _Alloc_t, typename = std::enable_if_t<std::is_same_v<std::decay_t<_Alloc_t>, allocator_type>>, typename ...Args>
		void allocate_array(size_type count, _Alloc_t &&other_allocator, Args&& ... args);

		template<typename _Alloc_t, typename = std::enable_if_t<std::is_same_v<std::decay_t<_Alloc_t>, allocator_type>>, typename ...Args>
		void allocate_array(internal_pointer_type starting_address, size_type count, _Alloc_t &&other_allocator, Args&& ... args);

		void allocate_array(std::initializer_list<T> input_list);

		template<typename Ty>
		void allocate_array(std::initializer_list<std::initializer_list<Ty>> input_list);

		template<typename Ty>
		void allocate_array(internal_pointer_type starting_address, std::initializer_list<Ty> input_list);

		void deallocate_array();

		void copy_array(const dynarray &other);

		template<typename ...Args>
		void copy_array(const dynarray &other, const allocator_type &other_allocator, Args&& ... args);

		void copy_array(internal_pointer_type starting_address, const dynarray &other);

		template<typename ...Args>
		void copy_array(internal_pointer_type starting_address, const dynarray &other, const allocator_type &other_allocator, Args&& ... args);

		template<typename InputIterator>
		void copy_array(InputIterator other_begin, InputIterator other_end);

		void loop_copy(const dynarray &right_dynarray);

		void loop_copy(std::initializer_list<T> input_list);

		template<typename Ty>
		void loop_copy(std::initializer_list<std::initializer_list<Ty>> input_list);

		void move_array(dynarray &other);
	};

	template<typename T, template<typename U> typename _Allocator>
	template<typename Ty>
	inline typename dynarray<T, _Allocator>::size_type dynarray<T, _Allocator>::expand_list(std::initializer_list<Ty> init)
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
	inline typename dynarray<T, _Allocator>::size_type dynarray<T, _Allocator>::expand_counts(size_type count, Args && ...args)
	{
		if constexpr (std::is_same_v<T, internal_value_type> || sizeof...(args) == 0)
			return count;
		else
			return T::expand_counts(std::forward<Args>(args)...) * count;
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ...Args>
	inline typename dynarray<T, _Allocator>::contiguous_allocator_type dynarray<T, _Allocator>::expand_allocator(const allocator_type &_allocator, Args && ...args)
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
			return _allocator;
		else if constexpr (sizeof...(args) == 0)
			return contiguous_allocator_type();
		else
			return T::expand_allocator(std::forward<Args>(args)...);
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename Skip, typename ...Args>
	inline typename dynarray<T, _Allocator>::contiguous_allocator_type dynarray<T, _Allocator>::expand_allocators(const Skip &skip, const allocator_type &_allocator, Args && ...args)
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
			return _allocator;
		else if constexpr (sizeof...(args) == 0)
			return contiguous_allocator_type();
		else
			return T::expand_allocators(std::forward<Args>(args)...);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::initialise(const allocator_type &other_allocator)
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
			contiguous_allocator = other_allocator;
		else
			contiguous_allocator = contiguous_allocator_type();

		array_allocator = other_allocator;
		entire_array_data = nullptr;
		current_dimension_array_size = 0;
		current_dimension_array_data = nullptr;
		this_level_array_head = nullptr;
		this_level_array_tail = nullptr;
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::verify_size(size_type count)
	{
		if (count > static_cast<size_type>(std::numeric_limits<difference_type>::max()))
			throw std::length_error("array too long");
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::allocate_array(size_type count)
	{
		if (count == 0)
		{
			initialise(this->array_allocator);
			return;
		}

		verify_size(count);
		current_dimension_array_size = count;
		entire_array_data = current_dimension_array_data = array_allocator.allocate(count);
		for (size_type i = 0; i < count; ++i)
			std::allocator_traits<allocator_type>::construct(array_allocator, entire_array_data + i);

		this_level_array_head = entire_array_data;
		this_level_array_tail = this_level_array_head + count - 1;
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::allocate_array(internal_pointer_type starting_address, size_type count)
	{
		// reached the innermost layer
		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_size = count;
			current_dimension_array_data = nullptr;	// the data (T*) belong to outermost layer, not this layer

			entire_array_data = nullptr;	// always nullptr in nested-dynarray

			this_level_array_head = starting_address;
			this_level_array_tail = this_level_array_head + count - 1;
		}
		else initialise();
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ... Args>
	inline void dynarray<T, _Allocator>::allocate_array(size_type count, Args&& ...args)
	{
		verify_size(count);
		size_type each_block_size = 1;
		if constexpr (!std::is_same_v<T, internal_value_type>)
			each_block_size = T::expand_counts(std::forward<Args>(args)...);

		verify_size(each_block_size);

		size_type entire_array_size = each_block_size * count;
		verify_size(entire_array_size);

		const size_type nested_level = internal_impl::inner_type<T, allocator_type>::nested_level;
		if (nested_level > sizeof...(args) || entire_array_size == 0)
		{
			initialise(this->array_allocator);
			return;
		}

		entire_array_data = contiguous_allocator.allocate(entire_array_size);

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_data = entire_array_data;
			current_dimension_array_size = entire_array_size;
			for (size_type i = 0; i < entire_array_size; ++i)
				std::allocator_traits<contiguous_allocator_type>::construct(contiguous_allocator, entire_array_data + i, std::forward<Args>(args)...);
		}
		else
		{
			current_dimension_array_size = count;
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
			for (size_type i = 0; i < current_dimension_array_size; ++i)
			{
				internal_pointer_type starting_address = entire_array_data + i * each_block_size;
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->contiguous_allocator = contiguous_allocator;
				(current_dimension_array_data + i)->allocate_array(starting_address, std::forward<Args>(args)...);
			}
		}

		this_level_array_head = entire_array_data;
		this_level_array_tail = this_level_array_head + entire_array_size - 1;
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ... Args>
	inline void dynarray<T, _Allocator>::allocate_array(internal_pointer_type starting_address, size_type count, Args&& ... args)
	{
		entire_array_data = nullptr;	// always nullptr in nested-dynarray

		size_type each_block_size = 1;
		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_size = count;
			current_dimension_array_data = nullptr;	// the data (T*) belong to outermost layer, not this layer

			entire_array_data = nullptr;	// always nullptr in nested-dynarray

			for (size_type i = 0; i < count; ++i)
				std::allocator_traits<allocator_type>::construct(contiguous_allocator, starting_address + i, std::forward<Args>(args)...);
		}
		else
		{
			each_block_size = T::expand_counts(std::forward<Args>(args)...);
			current_dimension_array_size = count;
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);

			for (size_type i = 0; i < current_dimension_array_size; ++i)
			{
				internal_pointer_type next_starting_address = starting_address + i * each_block_size;
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->contiguous_allocator = contiguous_allocator;
				(current_dimension_array_data + i)->allocate_array(next_starting_address, std::forward<Args>(args)...);
			}
		}

		this_level_array_head = starting_address;
		this_level_array_tail = this_level_array_head + each_block_size * count - 1;
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename _Alloc_t, typename, typename ... Args>
	inline void dynarray<T, _Allocator>::allocate_array(size_type count, _Alloc_t &&other_allocator, Args&& ... args)
	{
		verify_size(count);

		size_type each_block_size = 1;
		if constexpr (!std::is_same_v<T, internal_value_type>)
		{
			each_block_size = internal_impl::expand_parameters(std::forward<Args>(args)...);
			contiguous_allocator = expand_allocators(std::forward<Args>(args)...);
			verify_size(each_block_size);
		}

		size_type entire_array_size = each_block_size * count;
		verify_size(entire_array_size);

		const size_type nested_level = internal_impl::inner_type<T, allocator_type>::nested_level;
		if (nested_level * 2 > sizeof...(args) || entire_array_size == 0)
		{
			initialise(other_allocator);
			return;
		}

		entire_array_data = contiguous_allocator.allocate(entire_array_size);

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_data = entire_array_data;
			current_dimension_array_size = entire_array_size;
			for (size_type i = 0; i < entire_array_size; ++i)
				std::allocator_traits<contiguous_allocator_type>::construct(contiguous_allocator, entire_array_data + i, std::forward<Args>(args)...);
		}
		else
		{
			current_dimension_array_size = count;
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
			for (size_type i = 0; i < current_dimension_array_size; ++i)
			{
				internal_pointer_type starting_address = entire_array_data + i * each_block_size;
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->contiguous_allocator = contiguous_allocator;
				(current_dimension_array_data + i)->allocate_array(starting_address, std::forward<Args>(args)...);
			}
		}

		this_level_array_head = entire_array_data;
		this_level_array_tail = this_level_array_head + entire_array_size - 1;
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename _Alloc_t, typename, typename ...Args>
	inline void dynarray<T, _Allocator>::allocate_array(internal_pointer_type starting_address, size_type count, _Alloc_t &&other_allocator, Args&& ... args)
	{
		entire_array_data = nullptr;	// always nullptr in nested-dynarray
		array_allocator = other_allocator;

		size_type each_block_size = 1;
		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_size = count;
			current_dimension_array_data = nullptr;	// the data (T*) belong to outermost layer, not this layer

			entire_array_data = nullptr;	// always nullptr in nested-dynarray

			for (size_type i = 0; i < count; ++i)
				std::allocator_traits<allocator_type>::construct(contiguous_allocator, starting_address + i, std::forward<Args>(args)...);
		}
		else
		{
			each_block_size = internal_impl::expand_parameters(std::forward<Args>(args)...);
			contiguous_allocator = expand_allocators(std::forward<Args>(args)...);
			std::initializer_list<size_type> all_args = { static_cast<size_type>(args)... };
			each_block_size = std::accumulate(all_args.begin(), all_args.end(), each_block_size, std::multiplies<size_type>());

			current_dimension_array_size = count;
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);

			for (size_type i = 0; i < current_dimension_array_size; ++i)
			{
				internal_pointer_type next_starting_address = starting_address + i * each_block_size;
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->contiguous_allocator = contiguous_allocator;
				(current_dimension_array_data + i)->allocate_array(next_starting_address, other_allocator, std::forward<Args>(args)...);
			}
		}

		this_level_array_head = current_dimension_array_data;
		this_level_array_tail = this_level_array_head + each_block_size * count - 1;
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::allocate_array(std::initializer_list<T> input_list)
	{
		size_type count = input_list.size();
		if (count == 0) return;
		verify_size(count);
		current_dimension_array_size = count;
		entire_array_data = current_dimension_array_data = array_allocator.allocate(count);
		auto list_iter = input_list.begin();
		for (size_type i = 0; i < count; ++i, ++list_iter)
			std::allocator_traits<allocator_type>::construct(array_allocator, entire_array_data + i, *list_iter);

		this_level_array_head = entire_array_data;
		this_level_array_tail = this_level_array_head + count - 1;
	}


	template<typename T, template<typename U> typename _Allocator>
	template<typename Ty>
	inline void dynarray<T, _Allocator>::allocate_array(std::initializer_list<std::initializer_list<Ty>> input_list)
	{
		size_type count = input_list.size();
		if (count == 0) return;
		verify_size(count);

		size_type entire_array_size = expand_list(input_list);
		if (entire_array_size == 0) return;
		verify_size(entire_array_size);

		entire_array_data = contiguous_allocator.allocate(entire_array_size);

		current_dimension_array_size = count;
		current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
		auto list_iter = input_list.begin();
		for (size_type i = 0; i < current_dimension_array_size; ++i, ++list_iter)
		{
			internal_pointer_type starting_address = entire_array_data + i * T::expand_list(*list_iter);
			std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
			(current_dimension_array_data + i)->contiguous_allocator = contiguous_allocator;
			(current_dimension_array_data + i)->allocate_array(starting_address, *list_iter);
		}
		this_level_array_head = entire_array_data;
		this_level_array_tail = this_level_array_head + entire_array_size - 1;
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename Ty>
	inline void dynarray<T, _Allocator>::allocate_array(internal_pointer_type starting_address, std::initializer_list<Ty> input_list)
	{
		size_type count = input_list.size();
		verify_size(count);
		current_dimension_array_size = count;
		entire_array_data = nullptr;

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_data = nullptr;
			auto list_iter = input_list.begin();
			for (size_type i = 0; i < count; ++i, ++list_iter)
				std::allocator_traits<allocator_type>::construct(array_allocator, starting_address + i, *list_iter);

			this_level_array_head = starting_address;
			this_level_array_tail = this_level_array_head + count - 1;
		}
		else
		{
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
			auto list_iter = input_list.begin();
			for (size_type i = 0; i < current_dimension_array_size; ++i, ++list_iter)
			{
				internal_pointer_type next_starting_address = starting_address + i * T::expand_list(*list_iter);
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->contiguous_allocator = contiguous_allocator;
				(current_dimension_array_data + i)->allocate_array(next_starting_address, *list_iter);
			}

			this_level_array_head = starting_address;
			this_level_array_tail = this_level_array_head + expand_list(input_list) - 1;
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::deallocate_array()
	{
		size_type entire_array_size = static_cast<size_type>(this_level_array_tail - this_level_array_head + 1);
		// one layer only
		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			if (entire_array_data && current_dimension_array_data)
			{
				for (size_type i = entire_array_size; i != 0; --i)
					std::allocator_traits<allocator_type>::destroy(array_allocator, entire_array_data + i - 1);
				array_allocator.deallocate(entire_array_data, entire_array_size);
				entire_array_data = current_dimension_array_data = nullptr;
			}
		}

		if (current_dimension_array_data)
		{
			for (size_type i = current_dimension_array_size; i != 0; --i)
				std::allocator_traits<allocator_type>::destroy(array_allocator, current_dimension_array_data + i - 1);
			array_allocator.deallocate(current_dimension_array_data, current_dimension_array_size);
			current_dimension_array_data = nullptr;
		}

		if (entire_array_data)
		{
			for (size_type i = entire_array_size; i != 0; --i)
				std::allocator_traits<contiguous_allocator_type>::destroy(contiguous_allocator, entire_array_data + i - 1);
			contiguous_allocator.deallocate(entire_array_data, entire_array_size);
			entire_array_data = nullptr;
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::copy_array(const dynarray &other)
	{
		size_type entire_array_size = static_cast<size_type>(other.this_level_array_tail - other.this_level_array_head + 1);
		current_dimension_array_size = other.current_dimension_array_size;
		if (entire_array_size == 0 || current_dimension_array_size == 0) return;

		entire_array_data = contiguous_allocator.allocate(entire_array_size);
		for (size_type i = 0; i < entire_array_size; ++i)
			std::allocator_traits<contiguous_allocator_type>::construct(contiguous_allocator, entire_array_data + i, *(other.entire_array_data + i));

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_data = entire_array_data;
		}
		else
		{
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
			for (size_type i = 0; i < current_dimension_array_size; ++i)
			{
				internal_pointer_type starting_address = entire_array_data + i * other[i].get_block_size();
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->copy_array(starting_address, *(other.current_dimension_array_data + i));
			}
		}
		this_level_array_head = entire_array_data;
		this_level_array_tail = this_level_array_head + entire_array_size - 1;
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ...Args>
	inline void dynarray<T, _Allocator>::copy_array(const dynarray &other, const allocator_type &other_allocator, Args&&... args)
	{
		size_type entire_array_size = static_cast<size_type>(other.this_level_array_tail - other.this_level_array_head + 1);
		current_dimension_array_size = other.current_dimension_array_size;
		if (entire_array_size == 0 || current_dimension_array_size == 0) return;
		
		if constexpr (!std::is_same_v<T, internal_value_type>)
			contiguous_allocator = expand_allocator(std::forward<Args>(args)...);

		entire_array_data = contiguous_allocator.allocate(entire_array_size);
		for (size_type i = 0; i < entire_array_size; ++i)
			std::allocator_traits<contiguous_allocator_type>::construct(contiguous_allocator, entire_array_data + i, *(other.entire_array_data + i));

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_data = entire_array_data;
		}
		else
		{
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
			for (size_type i = 0; i < current_dimension_array_size; ++i)
			{
				internal_pointer_type starting_address = entire_array_data + i * other[i].get_block_size();
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->contiguous_allocator = contiguous_allocator;
				(current_dimension_array_data + i)->copy_array(starting_address, *(other.current_dimension_array_data + i), std::forward<Args>(args)...);
			}
		}
		this_level_array_head = entire_array_data;
		this_level_array_tail = this_level_array_head + entire_array_size - 1;
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::copy_array(internal_pointer_type starting_address, const dynarray & other)
	{
		size_type entire_array_size = static_cast<size_type>(other.this_level_array_tail - other.this_level_array_head + 1);
		current_dimension_array_size = other.current_dimension_array_size;
		entire_array_data = nullptr;

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_data = nullptr;
			this_level_array_head = starting_address;
			this_level_array_tail = this_level_array_head + current_dimension_array_size - 1;
		}
		else
		{
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
			for (size_type i = 0; i < current_dimension_array_size; ++i)
			{
				internal_pointer_type next_starting_address = starting_address + i * other[i].get_block_size();
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->copy_array(next_starting_address, *(other.current_dimension_array_data + i));
			}
			this_level_array_head = starting_address;
			this_level_array_tail = this_level_array_head + other.get_block_size() - 1;
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename ...Args>
	inline void dynarray<T, _Allocator>::copy_array(internal_pointer_type starting_address, const dynarray & other, const allocator_type &other_allocator, Args&& ... args)
	{
		array_allocator = other_allocator;
		size_type entire_array_size = static_cast<size_type>(other.this_level_array_tail - other.this_level_array_head + 1);
		current_dimension_array_size = other.current_dimension_array_size;
		entire_array_data = nullptr;

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_data = nullptr;
			this_level_array_head = starting_address;
			this_level_array_tail = this_level_array_head + current_dimension_array_size - 1;
		}
		else
		{
			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
			for (size_type i = 0; i < current_dimension_array_size; ++i)
			{
				internal_pointer_type next_starting_address = starting_address + i * other[i].get_block_size();
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->contiguous_allocator = contiguous_allocator;
				(current_dimension_array_data + i)->copy_array(next_starting_address, *(other.current_dimension_array_data + i), std::forward<Args>(args)...);
			}
			this_level_array_head = starting_address;
			this_level_array_tail = this_level_array_head + other.get_block_size() - 1;
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename InputIterator>
	inline void dynarray<T, _Allocator>::copy_array(InputIterator other_begin, InputIterator other_end)
	{
		size_type count = static_cast<size_type>(std::abs(other_end - other_begin));
		if (count == 0) return;

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			current_dimension_array_size = count;
			entire_array_data = current_dimension_array_data = array_allocator.allocate(count);

			InputIterator other_iterator = other_begin;
			for (size_type i = 0; i < count; ++i, ++other_iterator)
				std::allocator_traits<allocator_type>::construct(array_allocator, entire_array_data + i, *other_iterator);

			this_level_array_head = entire_array_data;
			this_level_array_tail = this_level_array_head + count - 1;
		}
		else if constexpr (std::is_same_v<InputIterator, iterator> || std::is_same_v<InputIterator, const_iterator>)
		{
			internal_pointer_type start_address = other_begin->this_level_array_head;
			internal_pointer_type end_address = (other_end - 1)->this_level_array_tail;
			size_type entire_array_size = static_cast<size_type>(end_address - start_address + 1);
			entire_array_data = contiguous_allocator.allocate(entire_array_size);
			current_dimension_array_size = count;

			for (size_type i = 0; i < entire_array_size; ++i)
				std::allocator_traits<contiguous_allocator_type>::construct(contiguous_allocator, entire_array_data + i, *(start_address + i));

			current_dimension_array_data = array_allocator.allocate(current_dimension_array_size);
			InputIterator other = other_begin;
			for (size_type i = 0; i < current_dimension_array_size; ++i, ++other)
			{
				internal_pointer_type starting_address = entire_array_data + i * other->get_block_size();
				std::allocator_traits<allocator_type>::construct(array_allocator, current_dimension_array_data + i);
				(current_dimension_array_data + i)->copy_array(starting_address, *other);
			}

			this_level_array_head = entire_array_data;
			this_level_array_tail = this_level_array_head + entire_array_size - 1;
		}
		else
		{
			static_assert(std::is_same_v<T, internal_value_type> ||
				std::is_same_v<InputIterator, iterator> || std::is_same_v<InputIterator, const_iterator>,
				"cannot convert to a valid dynarray");
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::loop_copy(const dynarray &right_dynarray)
	{
		if (right_dynarray.size() == 0) return;

		if constexpr (std::is_same_v<T, internal_value_type>)
		{
			difference_type length = std::min<difference_type>(this_level_array_tail - this_level_array_head + 1,
				right_dynarray.this_level_array_tail - right_dynarray.this_level_array_head + 1);
			for (difference_type i = 0; i < length; ++i)
				*(this_level_array_head + i) = *(right_dynarray.this_level_array_head + i);
		}
		else
		{
			for (size_type i = 0; i < current_dimension_array_size && i < right_dynarray.current_dimension_array_size; ++i)
				*(current_dimension_array_data + i) = *(right_dynarray.current_dimension_array_data + i);
		}
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::loop_copy(std::initializer_list<T> input_list)
	{
		size_type count = input_list.size();
		if (count == 0) return;
		verify_size(count);
		auto list_iter = input_list.begin();
		for (size_type i = 0; i < count && i < current_dimension_array_size; ++i, ++list_iter)
			*(this_level_array_head + i) = *list_iter;
	}

	template<typename T, template<typename U> typename _Allocator>
	template<typename Ty>
	inline void dynarray<T, _Allocator>::loop_copy(std::initializer_list<std::initializer_list<Ty>> input_list)
	{
		size_type count = input_list.size();
		if (count == 0) return;
		verify_size(count);
		auto list_iter = input_list.begin();
		for (size_type i = 0; i < count && i < current_dimension_array_size; ++i, ++list_iter)
			(current_dimension_array_data + i)->loop_copy(*list_iter);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::move_array(dynarray & other)
	{
		array_allocator = other.array_allocator;
		contiguous_allocator = other.contiguous_allocator;
		entire_array_data = other.entire_array_data;
		current_dimension_array_size = other.current_dimension_array_size;
		current_dimension_array_data = other.current_dimension_array_data;
		this_level_array_head = other.this_level_array_head;
		this_level_array_tail = other.this_level_array_tail;

		other.initialise();
	}

	template<typename T, template<typename U> typename _Allocator>
	inline typename dynarray<T, _Allocator>::reference dynarray<T, _Allocator>::at(size_type pos)
	{
		if (pos >= size())
			throw std::out_of_range("out of range, incorrect position");
		return (*this)[pos];
	}

	template<typename T, template<typename U> typename _Allocator>
	inline typename dynarray<T, _Allocator>::const_reference dynarray<T, _Allocator>::at(size_type pos) const
	{
		if (pos >= size())
			throw std::out_of_range("out of range, incorrect position");
		return (*this)[pos];
	}

	template<typename T, template<typename U> typename _Allocator>
	inline typename dynarray<T, _Allocator>::reference dynarray<T, _Allocator>::operator[](size_type pos)
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
			return *(this_level_array_head + pos);
		else
			return *(current_dimension_array_data + pos);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline constexpr typename dynarray<T, _Allocator>::const_reference dynarray<T, _Allocator>::operator[](size_type pos) const
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
			return *(this_level_array_head + pos);
		else
			return *(current_dimension_array_data + pos);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline typename dynarray<T, _Allocator>::reference dynarray<T, _Allocator>::back()
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
			return *(this_level_array_tail);
		else
			return (*this)[current_dimension_array_size - 1];
	}

	template<typename T, template<typename U> typename _Allocator>
	inline typename dynarray<T, _Allocator>::const_reference dynarray<T, _Allocator>::back() const
	{
		if constexpr (std::is_same_v<T, internal_value_type>)
			return *(this_level_array_tail);
		else
			return (*this)[current_dimension_array_size - 1];
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::swap(dynarray &other) noexcept
	{
		std::swap(array_allocator, other.array_allocator);
		std::swap(contiguous_allocator, other.contiguous_allocator);
		std::swap(entire_array_data, other.entire_array_data);
		std::swap(current_dimension_array_size, other.current_dimension_array_size);
		std::swap(current_dimension_array_data, other.current_dimension_array_data);
		std::swap(this_level_array_head, other.this_level_array_head);
		std::swap(this_level_array_tail, other.this_level_array_tail);
	}

	template<typename T, template<typename U> typename _Allocator>
	inline void dynarray<T, _Allocator>::fill(const internal_value_type & value)
	{
		for (auto current_address = this_level_array_head;
			current_address != this_level_array_tail + 1;
			++current_address)
			*current_address = value;
	}

}	// namespace vla


#endif //_VLA_DYNARRAY_