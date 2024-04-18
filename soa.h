// Copyright 2022 Alexandr Marchenko. All Rights Reserved.

#pragma once


#ifndef __SOA_H_INCLUDED__
#define __SOA_H_INCLUDED__


#include <type_traits>
#include <memory>
#include <utility>
#include <tuple>
#include <algorithm>
#include <vector>
#include <cassert>
#include <assert.h>


//#define DEBUG 1;
#ifdef DEBUG
#define SOA_ASSERT(EXPR) assert(EXPR)
//if (!(EXPR)){ __debugbreak();}
#else
#define SOA_ASSERT(EXPR) ((void)0)
#endif


namespace soa_helper
{
	namespace soa_helper_private
	{
		/********/
		// Reverseа IntegerSequence
		template<typename T, typename IntSequence>
		struct integer_sequence_reverse_helper;
		template<typename T, T... N>
		struct integer_sequence_reverse_helper<T, std::integer_sequence<T, N...>>
		{
			using integer_sequence_reversed = std::integer_sequence<T, (sizeof...(N)) - 1 - N...>;
		};

		/********/
		// Tuple size in bytes
		template<typename TupleType>
		struct tuple_tipesize_sum_helper;
		template<>
		struct tuple_tipesize_sum_helper<std::tuple<>>
		{
			static constexpr size_t size = 0;
		};
		template<typename First, typename... Rest>
		struct tuple_tipesize_sum_helper<std::tuple<First, Rest...>>
		{
			static constexpr size_t size = sizeof(First) + tuple_tipesize_sum_helper<std::tuple<Rest...>>::size;
		};


		/********/
		// type by index
		template<typename T, typename TupleType>
		struct tuple_element_index_helper;
		template<typename T>
		struct tuple_element_index_helper<T, std::tuple<>>
		{
			static constexpr uint32_t value = 0;
		};
		template<typename T, typename... Rest>
		struct tuple_element_index_helper<T, std::tuple<T, Rest...>>
		{
			static constexpr uint32_t value = 0;
			using RestTuple = std::tuple<Rest...>;

			static_assert(tuple_element_index_helper<T, RestTuple>::value == std::tuple_size<RestTuple>::value, "type appears more than once in tuple");
		};
		template<typename T, typename First, typename... Rest>
		struct tuple_element_index_helper<T, std::tuple<First, Rest...>>
		{
			using RestTuple = std::tuple<Rest...>;
			static constexpr uint32_t value = 1 + tuple_element_index_helper<T, RestTuple>::value;
		};


	}; // namespace soa_helper_private


	/********/
	// Chooses between two types by bool predicate
	template<bool predicate, typename TrueType, typename FalseType>
	struct choose_type;
	template<typename TrueType, typename FalseType>
	struct choose_type<true, TrueType, FalseType>
	{
		using type = TrueType;
	};
	template<typename TrueType, typename FalseType>
	struct choose_type<false, TrueType, FalseType>
	{
		using type = FalseType;
	};

	using namespace soa_helper_private;

	/********/
	// Reversed integer sequence
	template<typename T, T N>
	using make_integer_sequence_reversed = typename integer_sequence_reverse_helper<T, std::make_integer_sequence<T, N>>::integer_sequence_reversed;


	/********/
	// Tuple size in bytes
	template<typename TupleType>
	struct tuple_types_size
	{
		static constexpr size_t value = tuple_tipesize_sum_helper<TupleType>::size;
	};


	/********/
	// type by index
	template<typename T, typename TupleType, bool bAssert = true>
	struct tuple_index
	{
		static constexpr uint32_t value = tuple_element_index_helper<T, TupleType>::value;
		static_assert(!bAssert || value < std::tuple_size<TupleType>::value, "type does not appear in tuple");
	};


	/********/
	// Is one element type? in template param
	template<typename... types>
	struct is_one_param
	{
		static constexpr uint32_t size = sizeof...(types);
		static constexpr bool value = (size == 1);
	};


	/********/
	// tuple contains type T
	template<typename TupleCheck, typename T>
	struct check_tuple_type
	{
		check_tuple_type<TupleCheck, T>() //
		{
			static_assert(tuple_index<T, TupleCheck, false>::value < std::tuple_size<TupleCheck>::value, "type does not appear in tuple");
		}
	};
	template<typename TupleCheck, typename... ElemType>
	struct check_tuple_type<TupleCheck, std::tuple<ElemType...>>
	{
		check_tuple_type()
		{
			int tmp[] = { 0, (check_tuple_type<TupleCheck, ElemType>(), 0)... };
			(void)tmp;
		}
	};


	/********/
	template<typename T>
	struct remove_const_ref
	{
		//remove_cvref<T>::type c++20
		using type =
			typename std::remove_cv
			<
			typename std::remove_const
			<
			typename std::remove_reference<T>::type
			>::type
			>::type;
	};


	/********/
	// type = const T or T
	template<typename T, bool bForceConst = false>
	struct maybe_const
	{
		using type_rem_cvref = typename remove_const_ref<T>::type;
		using type = typename choose_type<std::is_const<T>::value || bForceConst, const type_rem_cvref, type_rem_cvref>::type;
	};


	/********/
	// FunctionTraits
	namespace functionTraits_private
	{
		template<typename T>
		struct function_traits_impl;

		template<typename ClassType, typename ReturnType, typename... Types>
		struct function_traits_impl<ReturnType(ClassType::*)(Types...)>
		{
			using result_type = ReturnType;
			using arg_tuple = std::tuple<Types...>;
			using arg_tuple_decay = std::tuple<typename maybe_const<Types>::type_rem_cvref...>;
			using arg_tuple_maybeconst = std::tuple<typename maybe_const<Types>::type...>;
		};

		template<typename ClassType, typename ReturnType, typename... Types>
		struct function_traits_impl<ReturnType(ClassType::*)(Types...) const> : function_traits_impl<ReturnType(ClassType::*)(Types...)>
		{};

		template<typename ReturnType, typename... Types>
		struct function_traits_impl<ReturnType(Types...)>
		{
			using result_type = ReturnType;
			using arg_tuple = std::tuple<Types...>;
			using arg_tuple_decay = std::tuple<typename maybe_const<Types>::type_rem_cvref...>;
			using arg_tuple_maybeconst = std::tuple<typename maybe_const<Types>::type...>;
		};

		template<typename ReturnType, typename... Types>
		struct function_traits_impl<ReturnType(*)(Types...)> : function_traits_impl<ReturnType(Types...)>
		{};

		//#include <functional>
		//template<typename ReturnType, typename... Types>
		//struct function_traits_impl<std::function<ReturnType(Types...)>> : function_traits_impl<ReturnType(Types...)>
		//{};

	} // namespace functionTraits_private

	template<typename Function, typename V = void>
	struct function_traits //
		: functionTraits_private::function_traits_impl<Function>
	{};

	//void (__cdecl FSoAModule::StartupModule::<lambda_1>::* )(float &,int &) const
	template<typename Function>
	struct function_traits<Function, decltype((void)&std::remove_reference<Function>::type::operator())> //
		: functionTraits_private::function_traits_impl<decltype(&std::remove_reference<Function>::type::operator())>
	{};


	/********/
	// ForElements lambda
	template<bool bConst, typename RetType, typename TupleType>
	struct make_function_type;
	template<bool bConst, typename RetType, typename... Types>
	struct make_function_type<bConst, RetType, std::tuple<Types...>>
	{
		using type = RetType(typename maybe_const<Types, bConst>::type&...);
	};


	template<bool bConst, typename TupleType>
	struct lambda_function;
	template<bool bConst, typename... Types>
	struct lambda_function<bConst, std::tuple<Types...>>
	{
		using function_type = typename make_function_type<bConst, void, std::tuple<typename maybe_const<Types, bConst>::type...>>::type;
	};
	template<bool bConst, typename Lambda>
	struct lambda_function
	{
		using function_type = typename make_function_type<bConst, void, typename function_traits<Lambda>::arg_tuple_maybeconst>::type;
	};


	/********/
	template<typename TupleType, typename IntegerSequence>
	struct tuple_firstN_elem_helper;
	template<typename TupleType, uint32_t... N>
	struct tuple_firstN_elem_helper<TupleType, std::integer_sequence<uint32_t, N...>>
	{
		using type = std::tuple<typename std::tuple_element<N, TupleType>::type...>;
	};
	template<typename TupleType, uint32_t Num>
	struct tuple_firstN_elem
	{
		using type = typename tuple_firstN_elem_helper<TupleType, std::make_integer_sequence<uint32_t, Num>>::type;
	};
	template<typename TupleType, uint32_t Num>
	struct tuple_firstN_elem_size
	{
		static constexpr uint32_t value = tuple_types_size<typename tuple_firstN_elem<TupleType, Num>::type>::value;
	};


} // namespace soa_helper


using namespace soa_helper;


/** SoA */
template<typename Alloc, typename... Ts>
class soa_base
{
	static_assert(std::is_same<std::tuple<Ts...>, std::tuple<typename remove_const_ref<Ts>::type...>>::value, "error: need remove const ref from types ");

	template<typename Other_allocator_type, typename... OtherTs>
	friend class soa_base;

public:
	using Ts_tuple = std::tuple<Ts...>;
	using soa_allocator = Alloc;
	using self = soa_base<soa_allocator, Ts...>;

	

	template<typename... ElemType>
	struct soa_type_check
	{
		soa_type_check()
		{
			constexpr uint32_t _TupTypeNum = std::tuple_size<std::tuple<ElemType...>>::value;
			static_assert(_TupTypeNum > 0, "no type in template argument");
			check_tuple_type<Ts_tuple, std::tuple<remove_const_ref<ElemType>::type...>>();
		}
	};

private:
	/********/

	uint8_t* m_data;
	int32_t m_size;
	int32_t m_max_size;
	soa_allocator m_allocator_inst;

private:
	/********/

	static constexpr size_t bytes_elements = tuple_types_size<Ts_tuple>::value;

	template<typename T>
	static constexpr uint32_t type_offset(const uint32_t max_size)
	{
		constexpr uint32_t type_index = tuple_index<T, Ts_tuple>::value;
		constexpr uint32_t offset_size = tuple_firstN_elem_size<Ts_tuple, type_index>::value;
		return offset_size * max_size;
	}
	template<typename T>
	uint32_t type_offset() const
	{
		return self::template type_offset<T>(this->m_max_size);
	}


	uint8_t* get_data() { return m_data; }
	const uint8_t* get_data() const { return m_data; }

	template<typename T>
	T* get_data()
	{
		return reinterpret_cast<T*>(this->get_data() + this->type_offset<T>());
	}
	template<typename T>
	const T* get_data() const
	{
		return reinterpret_cast<const T*>(this->get_data() + this->type_offset<T>());
	}

	template<typename T>
	T* get_data(const int32_t index)
	{
		return this->get_data<T>() + index;
	}
	template<typename T>
	const T* get_data(const int32_t index) const
	{
		return this->get_data<T>() + index;
	}


#pragma region Public
public:
	/********/

	//constructor
	soa_base() : m_data(nullptr), m_size(0), m_max_size(0) {}
	~soa_base()
	{
		destroy_elements(0, m_size);
		using alloc_traits = std::allocator_traits<soa_allocator>;
		alloc_traits::deallocate(m_allocator_inst, m_data, m_max_size * bytes_elements);
		m_size = 0;
	}

	int32_t size() const { return this->m_size; }
	int32_t max_size() const { return this->m_max_size; }

	bool is_valid_index(const int32_t index) const { return index >= 0 && index < this->m_size; }


	int32_t push_back(Ts&&... in_elements) { return this->push_back_impl(std::make_tuple(std::forward<Ts>(in_elements)...)); }
	int32_t push_back(const Ts&... in_elements) { return this->push_back_impl(std::make_tuple(in_elements...)); }

	int32_t insert(const int32_t index, Ts&&... in_elements) { return this->insert_impl(index, std::make_tuple(std::forward<Ts>(in_elements)...)); }
	int32_t insert(const int32_t index, const Ts&... in_elements) { return this->insert_impl(index, std::make_tuple(in_elements...)); }


	void remove_at(const int32_t index, const int32_t count = 1, const bool bshrink = false)
	{
		if (count)
		{
			SOA_ASSERT((count >= 0) && (index >= 0) && (index + count <= m_size));

			destroy_elements(index, count);

			const int32_t NumToMove = m_size - index - count;
			if (NumToMove)
			{
				this->relocate_elements<false>((index - count), index, count);
			}
			m_size -= count;

			if (bshrink)
			{
				resize_data(m_size);
			}
		}
	}
	void remove_at_swap(const int32_t index, const int32_t count = 1, const bool bshrink = true)
	{
		if (count)
		{
			SOA_ASSERT((count >= 0) && (index >= 0) && (index + count <= m_size));

			destroy_elements(index, count);

			const int32_t NumElementsInHole = count;
			const int32_t NumElementsAfterHole = m_size - (index + count);
			const int32_t NumElementsToMoveIntoHole = std::min(NumElementsInHole, NumElementsAfterHole);
			if (NumElementsToMoveIntoHole)
			{
				this->relocate_elements<false>(index, m_size - NumElementsToMoveIntoHole, NumElementsToMoveIntoHole);
			}
			m_size -= count;

			if (bshrink)
			{
				shrink();
			}
		}
	}

	void empty(const int32_t new_size = 0)
	{
		destroy_elements(0, m_size);
		m_size = 0;
		if (m_max_size != new_size)
		{
			resize_data(new_size);
		}
	}
	void reset(const int32_t new_size = 0)
	{
		if (new_size <= m_max_size)
		{
			int tmp[] = { 0, (this->destroy_element<Ts>(this->get_data<Ts>(), m_size), 0)... };
			(void)tmp;
			m_size = 0;
		}
		else
		{
			empty(new_size);
		}
	}
	void clear() { empty(size()); }

	void shrink()
	{
		if (m_max_size != m_size)
		{
			resize_data(m_size);
		}
	}

	void reserve(const int32_t reserve_size)
	{
		SOA_ASSERT(reserve_size >= 0);
		if (reserve_size > m_max_size)
		{
			resize_data(reserve_size);
		}
	}

	void resize(const int32_t new_size, const bool shrink = true)
	{
		if (new_size > size())
		{
			const int32_t index = add_data(new_size - m_size);
			for (int32_t index = 0; index < new_size; ++index)
			{
				this->construct_elements(index, Ts()...);
			}
		}
		else if (new_size < size())
		{
			remove_at(new_size, size() - new_size, shrink);
		}
	}
	void resize(const Ts&... in_elements, const int32_t new_size, const bool shrink = true)
	{
		if (new_size > size())
		{
			const int32_t index = add_data(new_size - m_size);
			for (int32_t index = 0; index < new_size; ++index)
			{
				this->construct_elements(index, in_elements...);
			}
		}
		else if (new_size < size())
		{
			remove_at(new_size, size() - new_size, shrink);
		}
	}

	void init(const Ts&... in_elements, const int32_t count)
	{
		empty(count);
		for (int32_t index = 0; index < count; ++index)
		{
			this->construct_elements(index, in_elements...);
		}
	}

	void swap(const int32_t first_index, const int32_t second_index)
	{
		SOA_ASSERT(this->is_valid_index(first_index));
		SOA_ASSERT(this->is_valid_index(second_index));
		if (first_index != second_index)
		{
			auto a = this->get_tuple(first_index);
			auto b = this->get_tuple(second_index);
			a.swap(b);
		}
	}


	/********/

	template<typename T>
	typename maybe_const<T>::type& get_single(const int32_t index)
	{
		range_check(index);
		return this->get_data<typename std::remove_const<T>::type>()[index];
	}
	template<typename T>
	const T& get_single(const int32_t index) const
	{
		range_check(index);
		return this->get_data<typename std::remove_const<T>::type>()[index];
	}

	std::tuple<Ts&...> get_tuple(const int32_t index) { return std::tie(this->get_single<Ts>(index)...); }
	std::tuple<const Ts&...> get_tuple(const int32_t index) const { return std::tie(this->get_single<Ts>(index)...); }

	template<typename... ElemType>
	std::tuple<ElemType&...> get_tuple(const int32_t index)
	{
		return std::tie(this->get_single<ElemType>(index)...);
	}
	template<typename... ElemType>
	std::tuple<const ElemType&...> get_tuple(const int32_t index) const
	{
		return std::tie(this->get_single<ElemType>(index)...);
	}

	template<typename... ElemType>
	typename std::enable_if<is_one_param<ElemType...>::value, typename std::tuple_element<0, std::tuple<ElemType...>>::type&>::type get(const int32_t index)
	{
		using T = typename std::tuple_element<0, std::tuple<ElemType...>>::type;
		return this->get_single<T>(index);
	}
	template<typename... ElemType>
	typename std::enable_if<is_one_param<ElemType...>::value, const typename std::tuple_element<0, std::tuple<ElemType...>>::type&>::type get(const int32_t index) const
	{
		using T = typename std::tuple_element<0, std::tuple<ElemType...>>::type;
		return this->get_single<T>(index);
	}

	template<typename... ElemType>
	typename std::enable_if<!is_one_param<ElemType...>::value, std::tuple<ElemType&...>>::type get(const int32_t index)
	{
		return this->get_tuple<ElemType...>(index);
	}
	template<typename... ElemType>
	typename std::enable_if<!is_one_param<ElemType...>::value, std::tuple<const ElemType&...>>::type get(const int32_t index) const
	{
		return this->get_tuple<ElemType...>(index);
	}

	// todo: get_type_index not need?
	//template<uint32_t... N>
	//auto& get_type_index(const int32_t index)
	//{
	//	return std::tie(this->get<std::tuple_element<N, Ts_tuple>::type>(index)...);
	//}
	//template<uint32_t... N>
	//const auto& get_type_index(const int32_t index) const
	//{
	//	return std::tie(this->get<std::tuple_element<N, Ts_tuple>::Type>(index)...);
	//}

	std::tuple<Ts&...> operator[](const int32_t index) { return this->get_tuple<Ts...>(index); }
	std::tuple<const Ts&...> operator[](const int32_t index) const { return this->get_tuple<Ts...>(index); }

	/********/

	template<typename... ElemType>
	typename std::enable_if<is_one_param<ElemType...>::value, std::vector<typename std::tuple_element<0, std::tuple<ElemType...>>::type*>>::type get_ptr()
	{
		soa_type_check<ElemType...>();
		using OutT = typename std::tuple_element<0, std::tuple<ElemType...>>::type;
		return this->get_data<OutT>();
	}
	template<typename... ElemType>
	typename std::enable_if<!is_one_param<ElemType...>::value, std::tuple<std::vector<ElemType*>...>>::type get_ptr()
	{
		soa_type_check<ElemType...>();
		return std::make_tuple(this->get_ptr<ElemType>()...);
	}


	//std::vector
	template<typename... ElemType>
	typename std::enable_if<is_one_param<ElemType...>::value, std::vector<typename std::tuple_element<0, std::tuple<ElemType...>>::type>>::type
		get_vector_copy() const
	{
		soa_type_check<ElemType...>();

		using OutT = typename std::tuple_element<0, std::tuple<ElemType...>>::type;
		const OutT* begin_ptr = this->get_data<OutT>();
		const OutT* end_ptr = this->get_data<OutT>() + (this->size());
		return std::vector<OutT>(begin_ptr, end_ptr);
	}
	template<typename... ElemType>
	typename std::enable_if<!is_one_param<ElemType...>::value, std::tuple<std::vector<ElemType>...>>::type get_vector_copy() const
	{
		soa_type_check<ElemType...>();
		return std::make_tuple(get_vector_copy<ElemType>()...);
	}
	//todo: std::span c++20 //#include <span>

	template<typename... ElemType>
	std::vector<std::tuple<ElemType...>> get_vector_aos_copy() const
	{
		soa_type_check<ElemType...>();

		std::vector<std::tuple<ElemType...>> out;
		out.reserve(this->size());
		for (auto It = this->iterator_const<float, int>(); It; ++It)
		{
			std::tuple<const ElemType&...> Tup = *It;
			out.push_back(std::make_tuple(std::get<tuple_index<ElemType, Ts_tuple>::value>(Tup)...));
		}
		return out;
	}

	/********/

#pragma endregion Public

#pragma region Private

private:
	template<typename T>
	typename std::enable_if<std::is_move_constructible<T>::value>::type relocate_type(T* dest, const T* source, uint32_t count)
	{
		std::memmove(dest, source, sizeof(T) * count);
	}
	template<typename T>
	typename std::enable_if<!std::is_move_constructible<T>::value>::type relocate_type(T* dest, const T* source, uint32_t count)
	{
		using alloc_traits = std::allocator_traits<soa_allocator>;
		while (count)
		{
			alloc_traits::construct(this->m_allocator_inst, dest++, T(*source));
			alloc_traits::destroy(this->m_allocator_inst, source++);
			--count;
		}
	}


	template<uint32_t... N>
	__forceinline void relocate_data_seq(uint8_t* dest, const uint8_t* source, int32_t old_max, int32_t new_max, int32_t count, std::integer_sequence<uint32_t, N...>)
	{
		int tmp[] = {
			0,
			(this->relocate_type<std::tuple_element<N, Ts_tuple>::type>(
				 reinterpret_cast<std::tuple_element<N, Ts_tuple>::type*>( //
					 dest + this->type_offset<std::tuple_element<N, Ts_tuple>::type>(new_max)),
				 reinterpret_cast<const std::tuple_element<N, Ts_tuple>::type*>( //
					 source + this->type_offset<std::tuple_element<N, Ts_tuple>::type>(old_max)),
				 count),
			 0)... };
		(void)tmp;
	}

	template<uint32_t... N>
	__forceinline void relocate_elements_seq(int32_t dest_index, int32_t source_index, int32_t count, std::integer_sequence<uint32_t, N...>)
	{
		int tmp[] = {
			0,
			(this->relocate_type<std::tuple_element<N, Ts_tuple>::type>(
				 reinterpret_cast<std::tuple_element<N, Ts_tuple>::type*>(this->get_data<std::tuple_element<N, Ts_tuple>::type>() + dest_index),
				 reinterpret_cast<const std::tuple_element<N, Ts_tuple>::type*>(this->get_data<std::tuple_element<N, Ts_tuple>::type>() + source_index),
				 count),
			 0)... };
		(void)tmp;
	}

	template<bool bFromFirst = true>
	void relocate_elements(int32_t dest_index, int32_t source_index, int32_t count)
	{
		if (count > 0 && dest_index != source_index)
		{
			using IntegerSeqFromFirst = std::make_integer_sequence<uint32_t, std::tuple_size<Ts_tuple>::value>;
			using IntegerSeqfromLast = make_integer_sequence_reversed<uint32_t, std::tuple_size<Ts_tuple>::value>;
			using IntegerSeq = typename choose_type<bFromFirst, IntegerSeqFromFirst, IntegerSeqfromLast>::type;

			this->relocate_elements_seq(dest_index, source_index, count, IntegerSeq());
		}
	}


	__declspec(noinline) void resize_data(int32_t new_max)
	{
		using alloc_traits = std::allocator_traits<soa_allocator>;

		if (new_max != m_max_size)
		{
			const int32_t old_max = m_max_size;
			m_max_size = new_max; // todo: calculate max

			if (m_data)
			{
				uint8_t* old_data = m_data;
				m_data = alloc_traits::allocate(m_allocator_inst, new_max * bytes_elements);

				//relocate_data
				if (m_size > 0)
				{
					using IntegerSeq = std::make_integer_sequence<uint32_t, std::tuple_size<Ts_tuple>::value>;
					this->relocate_data_seq(m_data, old_data, old_max, m_max_size, m_size, IntegerSeq());
				}

				alloc_traits::deallocate(m_allocator_inst, old_data, old_max * bytes_elements);
			}
			else
			{
				m_data = alloc_traits::allocate(m_allocator_inst, new_max * bytes_elements);
			}
		}
	}

private:
	template<typename T, typename... Types>
	void construct_element(T* const source, Types&&... args)
	{
		using alloc_traits = std::allocator_traits<soa_allocator>;
		alloc_traits::construct(this->m_allocator_inst, source, std::forward<Types>(args)...);
	}
	template<typename T>
	void destroy_element(T* const source, const int32_t count)
	{
		using alloc_traits = std::allocator_traits<soa_allocator>;
		alloc_traits::destroy(this->m_allocator_inst, source);
	}

	template<typename... Types>
	void construct_elements(int32_t index, Types&&... args)
	{
		SOA_ASSERT(this->is_valid_index(index));

		int tmp[] = { 0, (this->construct_element(this->get_data<Ts>() + index, std::forward<Types>(args)), 0)... };
		(void)tmp;
	}
	void destroy_elements(int32_t index, const int32_t count)
	{
		SOA_ASSERT(index >= 0 && index < size() && count > 0);
		using alloc_traits = std::allocator_traits<soa_allocator>;
		soa_allocator m_allocator_inst;
		for (int32_t i = 0; i < count; i++)
		{
			int tmp[] = { 0, (alloc_traits::destroy(m_allocator_inst, get_data<Ts>() + index + i), 0)... };
			(void)tmp;
		}
	}

	int32_t add_data(const int32_t count = 1)
	{
		SOA_ASSERT(count >= 0);

		const int32_t old_size = m_size;
		if (m_size + count > m_max_size)
		{
			resize_data(m_size + count);
		}
		m_size += count;
		return old_size;
	}
	void insert_data(int32_t index, int32_t count)
	{
		SOA_ASSERT((count >= 0) && this->is_valid_index(index));

		const int32_t old_size = m_size;
		if (m_size + count > m_max_size)
		{
			resize_data(m_size + count);
		}

		this->relocate_elements<false>(index + count, index, count);
		m_size += count;
	}

	template<typename T>
	T* emplace(const int32_t index, T&& args)
	{
		T* const Ptr = this->get_data<T>() + index;
		this->construct_element<T, T>(Ptr, std::forward<T>(args));
		return Ptr;
	}

	template<typename... Types>
	int32_t push_back_impl(std::tuple<Types...>&& args)
	{
		const int32_t index = this->add_data(1);
		int tmp[] = { 0, (this->emplace<Ts>(index, std::forward<Ts>(std::get<tuple_index<Ts, Ts_tuple>::value>(args))), 0)... };
		(void)tmp;
		return index;
	}

	template<typename... Types>
	int32_t insert_impl(const int32_t index, std::tuple<Types...>&& args)
	{
		insert_data(index, 1);
		int tmp[] = { 0, (this->emplace<Ts>(index, std::forward<Ts>(std::get<tuple_index<Ts, Ts_tuple>::value>(args))), 0)... };
		(void)tmp;
		return index;
	}

	__forceinline void range_check(const int32_t index) const
	{
		SOA_ASSERT((m_size >= 0) && (m_max_size >= m_size));
		SOA_ASSERT(index >= 0 && index < this->m_size);
	}

#pragma endregion Private

#pragma region Iterator
public:
	template<bool bConst, typename... ElemType>
	using it_tup_element_type = std::tuple<typename maybe_const<ElemType, bConst>::type&...>;

	template<bool bConst, typename... ElemType>
	using it_one_element_type = typename std::tuple_element<0, it_tup_element_type<bConst, ElemType...>>::type;

	template<bool bConst, typename... ElemType>
	using it_element_type =											//
		typename choose_type<										//
		std::tuple_size<std::tuple<ElemType...>>::value == 1, //
		it_one_element_type<bConst, ElemType...>,				//
		it_tup_element_type<bConst, ElemType...>				//
		>::type;

	// tuple wrapper for  Argument-dependent lookup swap(tuple<Ts&...>& a, tuple<Ts&...>& b) {...}
	struct tuple_wrapper : std::tuple<Ts&...>
	{
		tuple_wrapper(const std::tuple<Ts&...> In) : std::tuple<Ts&...>(In) {}
		//tuple_wrapper(std::tuple<Ts&...>&& In) : std::tuple<Ts&...>(std::forward<std::tuple<Ts&...>>(In)) {}

		tuple_wrapper operator=(std::tuple<Ts...>&& Other)
		{
			int tmp[] = {
				0,
				(std::get<tuple_index<Ts, std::tuple<Ts...>>::value>(*this) = std::move(std::get<tuple_index<Ts, std::tuple<Ts...>>::value>(Other)), 0)... };
			(void)tmp;
			return *this;
		}
		tuple_wrapper operator=(const std::tuple<Ts...>& Other)
		{
			int Temp1[] = { 0, (std::get<tuple_index<Ts, Ts_tuple>::value>(*this) = std::get<tuple_index<Ts, std::tuple<Ts...>>::value>(Other), 0)... };
			(void)Temp1;
			return *this;
		}

		tuple_wrapper operator=(std::tuple<Ts&...> Other)
		{
			int tmp[] = { 0, (std::get<tuple_index<Ts, Ts_tuple>::value>(*this) = std::get<tuple_index<Ts, Ts_tuple>::value>(Other), 0)... };
			(void)tmp;
			return *this;
		}
		tuple_wrapper operator=(const std::tuple<Ts&...>& Other)
		{
			int Temp1[] = { 0, (std::get<tuple_index<Ts, Ts_tuple>::value>(*this) = std::get<tuple_index<Ts, Ts_tuple>::value>(Other), 0)... };
			(void)Temp1;
			return *this;
		}

		operator std::tuple<Ts...>() const { return std::make_tuple(std::get<tuple_index<Ts, Ts_tuple>::value>(*this)...); }
		operator std::tuple<Ts&...>() { return *this; }

		// Argument-dependent lookup
		friend void swap(tuple_wrapper a, tuple_wrapper b) { a.swap(b); }
	};

	// Iterator
	template<bool bConst, bool bWrapType = false, typename... ElemType>
	class soa_iterator_base
	{
		friend class soa_sort_iterator;

	public:
		using container_type = typename choose_type<bConst, const self, self>::type;

		//iterator_traits
		using difference_type = int;
		using iterator_category = typename std::random_access_iterator_tag;
		using reference = it_element_type<bConst, ElemType...>;
		using value_type = typename choose_type<											//
			std::tuple_size<std::tuple<ElemType...>>::value == 1,							//
			typename std::tuple_element<0, it_tup_element_type<bConst, ElemType...>>::type, //
			std::tuple<ElemType...>>::type;

		using pointer = std::tuple<ElemType*...>; //todo


		soa_iterator_base<bConst, bWrapType, ElemType...>() : m_container_ptr(nullptr), m_index(-1) {}
		soa_iterator_base<bConst, bWrapType, ElemType...>(container_type& in_soa, const int32_t start_index = 0) //
			: m_container_ptr(&in_soa)
			, m_index(start_index)
		{
			//soa_type_check<ElemType...>();
		}
		soa_iterator_base<bConst, bWrapType, ElemType...>(const soa_iterator_base<bConst, bWrapType, ElemType...>& Other)
			: soa_iterator_base<bConst, bWrapType, ElemType...>(*Other.m_container_ptr, Other.m_index)
		{}

		int32_t get_index() const { return m_index; }
		bool is_valid_index() const { return m_container_ptr->is_valid_index(get_index()); }
		void reset() { m_index = 0; }
		void set_to_end() { m_index = m_container_ptr->size(); }

		template<bool enabled = !bConst>
		typename std::enable_if<enabled, void>::type remove_current() { m_container_ptr->remove_at(m_index--); }
		//typename std::enable_if<bConst, void>::type remove_current() { }

		operator bool() const { return is_valid_index(); }

		soa_iterator_base& operator++() // Advances iterator to the next element in the container.
		{
			++m_index;
			return *this;
		}
		soa_iterator_base operator++(int32_t)
		{
			soa_iterator_base Tmp(*this);
			++m_index;
			return Tmp;
		}
		soa_iterator_base operator+(int32_t Offset) const
		{
			soa_iterator_base Tmp(*this);
			return Tmp += Offset;
		}
		soa_iterator_base& operator+=(const int32_t Offset)
		{
			m_index += Offset;
			return *this;
		}

		soa_iterator_base& operator--() // Moves iterator to the previous element in the container.
		{
			--m_index;
			return *this;
		}
		soa_iterator_base operator--(int32_t)
		{
			soa_iterator_base Tmp(*this);
			--m_index;
			return Tmp;
		}
		soa_iterator_base operator-(int32_t Offset) const
		{
			soa_iterator_base Tmp(*this);
			return Tmp -= Offset;
		}
		soa_iterator_base& operator-=(const int32_t Offset) { return *this += -Offset; }


		soa_iterator_base operator=(const soa_iterator_base& Other)
		{
			this->m_container_ptr = Other.m_container_ptr;
			this->m_index = Other.m_index;
			return *this;
		}
		int32_t operator-(const soa_iterator_base& Other) const { return get_index() - Other.get_index(); }
		bool operator<(const soa_iterator_base& Other) const { return get_index() < Other.get_index(); }
		bool operator>(const soa_iterator_base& Other) const { return get_index() < Other.get_index(); }
		bool operator==(const soa_iterator_base& Other) const { return m_container_ptr == Other.m_container_ptr && m_index == Other.m_index; }
		bool operator!=(const soa_iterator_base& Other) const { return m_container_ptr != Other.m_container_ptr || m_index != Other.m_index; }

		explicit operator it_element_type<bConst, ElemType...>()
		{
			return m_container_ptr->template get<typename std::remove_const<ElemType>::type...>(m_index);
		}
		explicit operator it_element_type<bConst, ElemType...>() const
		{
			return m_container_ptr->template get<typename std::remove_const<ElemType>::type...>(m_index);
		}

		typename choose_type<bWrapType, tuple_wrapper, it_element_type<bConst, ElemType...>>::type operator*() const
		{
			return m_container_ptr->template get<typename std::remove_const<ElemType>::type...>(m_index);
		}

	protected:
		container_type* m_container_ptr;
		int32_t m_index;
	};
	template<bool bConst, typename... ElemType>
	using soa_iterator = soa_iterator_base<bConst, false, ElemType...>;

	template<bool bConst, typename... ElemType>
	struct soa_range_iterator
	{
		using container_type = typename choose_type<bConst, const self, self>::type;

		explicit soa_range_iterator(container_type& In) : Container(In) { soa_type_check<ElemType...>(); }
		container_type& Container;

		soa_iterator<bConst, ElemType...> begin() { return soa_iterator_base<false, false, ElemType...>(Container); }
		soa_iterator<bConst, ElemType...> end() { return soa_iterator_base<false, false, ElemType...>(Container, Container.size()); }

		soa_iterator<bConst, ElemType...> begin() const { return soa_iterator_base<true, false, ElemType...>(Container); }
		soa_iterator<bConst, ElemType...> end() const { return soa_iterator_base<true, false, ElemType...>(Container, Container.size()); }
	};


	soa_iterator<false, Ts...> iterator() { return soa_iterator_base<false, false, Ts...>(*this); }
	soa_iterator<false, Ts...> iterator() const { return soa_iterator_base<true, false, Ts...>(*this); }
	soa_iterator<true, Ts...> iterator_const() const { return this->iterator(); }

	template<typename... ElemType>
	soa_iterator<false, ElemType...> iterator()
	{
		return soa_iterator_base<false, false, ElemType...>(*this);
	}
	template<typename... ElemType>
	soa_iterator<false, ElemType...> iterator() const
	{
		return soa_iterator_base<true, false, ElemType...>(*this);
	}
	template<typename... ElemType>
	soa_iterator<true, ElemType...> iterator_const() const
	{
		return soa_iterator_base<true, false, ElemType...>(*this);
	}


	soa_range_iterator<false, Ts...> range_iterator() { return soa_range_iterator<false, Ts...>(*this); }
	soa_range_iterator<false, Ts...> range_iterator() const { return soa_range_iterator<true, Ts...>(*this); }
	soa_range_iterator<true, Ts...> range_iterator_const() const { return soa_range_iterator<true, Ts...>(*this); }

	template<typename... ElemType>
	auto range_iterator()
	{
		return soa_range_iterator<false, ElemType...>(*this);
	}
	template<typename... ElemType>
	auto range_iterator() const
	{
		return soa_range_iterator<true, ElemType...>(*this);
	}
	template<typename... ElemType>
	auto range_iterator_const() const
	{
		return soa_range_iterator<true, ElemType...>(*this);
	}


	soa_iterator<false, Ts...> begin() { return this->iterator(); }
	soa_iterator<true, Ts...> begin() const { return this->iterator_const(); }
	soa_iterator<false, Ts...> end() { return soa_iterator_base<false, false, Ts...>(*this, size()); }
	soa_iterator<true, Ts...> end() const { return soa_iterator_base<true, false, Ts...>(*this, size()); }

#pragma endregion Iterator

#pragma region ForEach
private:
	template<typename FuncType, typename... ElemType>
	static auto invoke_func(FuncType&& Func, std::tuple<ElemType...> Elem)
	{
		return std::invoke<FuncType, ElemType...>( //
			std::forward<FuncType>(Func),
			std::get<tuple_index<ElemType, std::tuple<ElemType...>>::value>(Elem)...);
	}

	template<typename FuncType, typename T, typename... U>
	void for_each_elem_internal(FuncType&& Body)
	{
		for (std::tuple<T&, U&...> It : this->range_iterator<T, U...>())
		{
			self::template invoke_func<FuncType, T&, U&...>(std::forward<FuncType>(Body), It);
		}
	}
	template<typename FuncType, typename T, typename... U>
	void for_each_elem_internal(FuncType&& Body) const
	{
		for (std::tuple<const T&, const U&...> It : this->range_iterator<T, U...>())
		{
			self::template invoke_func<FuncType, const T&, const U&...>(std::forward<FuncType>(Body), It);
		}
	}

	template<typename FuncType, typename TupleType, uint32_t... N>
	void for_each_elem_seq(FuncType&& Body, std::integer_sequence<uint32_t, N...>&&)
	{
		for_each_elem_internal<FuncType, typename std::tuple_element<N, TupleType>::type...>(std::forward<FuncType>(Body));
	}
	template<typename FuncType, typename TupleType, uint32_t... N>
	void for_each_elem_seq(FuncType&& Body, std::integer_sequence<uint32_t, N...>&&) const
	{
		for_each_elem_internal<FuncType, typename std::tuple_element<N, TupleType>::type...>(std::forward<FuncType>(Body));
	}

public:
	//todo: support different const/ref arguments: - auto lambda = [](const A& a, B& b , const C c, D d...) {};
	template<typename FuncType>
	void for_each(FuncType Body)
	{
		using ArgTup = typename function_traits<FuncType>::arg_tuple_maybeconst;
		using IntegerSequence = std::make_integer_sequence<uint32_t, std::tuple_size<ArgTup>::value>;

		this->for_each_elem_seq<FuncType, ArgTup>(std::forward<FuncType>(Body), IntegerSequence());
	}
	template<typename FuncType>
	void for_each(FuncType Body) const
	{
		using ArgTup = typename function_traits<FuncType>::arg_tuple_maybeconst;
		using IntegerSequence = std::make_integer_sequence<uint32_t, std::tuple_size<ArgTup>::value>;

		this->for_each_elem_seq<FuncType, ArgTup>(std::forward<FuncType>(Body), IntegerSequence());
	}

	//todo: support different const/ref arguments: - auto lambda = [](const A& a, B& b , const C c, D d...) {};
	template<typename... ElemType>
	void for_each(typename lambda_function<false, ElemType...>::function_type Body)
	{
		using FuncType = void(ElemType & ...);
		this->for_each_elem_internal<FuncType, ElemType...>(std::forward<FuncType>(Body));
	}
	template<typename... ElemType>
	void for_each(typename lambda_function<true, ElemType...>::function_type Body) const
	{
		using FuncType = void(const ElemType&...);
		this->for_each_elem_internal<FuncType, ElemType...>(std::forward<FuncType>(Body));
	}


#pragma endregion ForEach

#pragma region Sort
private:
	template<typename PredicateType>
	struct predicate_projection
	{
		using ArgumentsTuple = typename function_traits<PredicateType>::arg_tuple_decay;
		using ArgType1 = typename std::tuple_element<0, ArgumentsTuple>::type;
		using ArgType2 = typename std::tuple_element<1, ArgumentsTuple>::type;
		static constexpr uint32_t ArgIdx1 = tuple_index<ArgType1, std::tuple<Ts...>>::value;
		static constexpr uint32_t ArgIdx2 = tuple_index<ArgType2, std::tuple<Ts...>>::value;

		static_assert(std::is_same<ArgType1, ArgType2>::value, "Predicate types does not appear in tuple");

		predicate_projection(PredicateType In) : ReversePredicate(In) {}
		const PredicateType& ReversePredicate;

		bool operator()(const std::tuple<const Ts&...>& A, const std::tuple<const Ts&...>& B) const
		{ //
			return ReversePredicate(std::get<ArgIdx1>(A), std::get<ArgIdx2>(B));
		}
		bool operator()(std::tuple<Ts&...>& A, std::tuple<Ts&...>& B) const
		{ //
			return ReversePredicate(std::get<ArgIdx1>(A), std::get<ArgIdx2>(B));
		}
	};

public:
	template<typename PredicateType>
	void sort(PredicateType Predicate)
	{
		auto Start = soa_iterator_base<false, true, Ts...>(*this, 0);
		auto End = soa_iterator_base<false, true, Ts...>(*this, this->size());
		std::sort(Start, End, predicate_projection<PredicateType>(Predicate));
	}
#pragma endregion Sort
};

template<typename... Ts>
using soa = soa_base<std::allocator<uint8_t>, Ts...>;


#endif
