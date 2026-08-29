#ifndef _CACHY_HPP_
#define _CACHY_HPP_

/* Single Header C++ Low Latency LRU Cache */

#include <boost/intrusive/list.hpp>

#include <boost/unordered/unordered_node_map.hpp>

namespace Cachy
{

template<size_t Capacity, class KeyType, class ValueType, template<class> class Allocator = std::allocator>
class LRUCache
{
    using KeyArgType = std::conditional_t<sizeof(KeyType) < 8, KeyType, std::add_const_t<std::add_lvalue_reference_t<KeyType>>>;

	using ValueArgType = std::conditional_t<sizeof(ValueType) < 8, ValueType, std::add_const_t<std::add_lvalue_reference_t<ValueType>>>;

	struct ValueNode
	{
		static constexpr int shift { 8 };

		ValueType value;

		boost::intrusive::list_member_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>> listHook_;

		explicit ValueNode(ValueArgType value): value{ value }, listHook_{} {}

		KeyArgType getKey() const noexcept
		{
		    #if defined(__clang__)

		    return *static_cast<const KeyType*>(static_cast<const void*>(static_cast<const char*>(static_cast<const void*>(this)) - shift));

		    #elif defined(__GNUC__) || defined(__GNUG__)

		    #pragma GCC diagnostic push
		    #pragma GCC diagnostic ignored "-Wpointer-arith"

		    return *static_cast<const KeyType*>(static_cast<const void*>(this) - shift);

		    #pragma GCC diagnostic pop

		    #endif
		}
	};

	using key_type = KeyType;

	using mapped_type = ValueType;

	using value_type = std::pair<const KeyType, ValueNode>;

	using allocator_type = Allocator<std::pair<const KeyType, ValueNode>>;

	using HashMapType = boost::unordered_node_map<KeyType, ValueNode, boost::hash<KeyType>, std::equal_to<KeyType>, Allocator<std::pair<const KeyType, ValueNode>>>;

	using LevelMemberHookOption = boost::intrusive::member_hook<ValueNode, boost::intrusive::list_member_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>, &ValueNode::listHook_>;

	using LevelListType = boost::intrusive::list<ValueNode, LevelMemberHookOption, boost::intrusive::constant_time_size<false>>;

	HashMapType cache{};

	LevelListType items{};

public:

	explicit LRUCache()
	{
		this->cache.max_load_factor(0.5f);

		this->cache.reserve(Capacity);
	}

	constexpr size_t capacity() noexcept { return Capacity; }

	ValueType* get(KeyArgType key) noexcept
	{
        if ( const auto hit { this->cache.find(key) }; hit not_eq this->cache.end() ) [[likely]]
        {   		
    		this->items.splice(this->items.begin(), this->items, this->items.iterator_to(hit->second));

        	return std::addressof(hit->second.value);
        }

        return nullptr;
    }

    void put(KeyArgType key, ValueArgType value) noexcept
    {
        if ( auto hit { this->cache.find(key) }; hit not_eq this->cache.end() ) [[likely]]
        {
            hit->second.value = value;

            this->items.splice(this->items.begin(), this->items, this->items.iterator_to(hit->second));

            return;
        }

        if ( this->cache.size() == Capacity ) [[likely]]
        {
            const KeyType evictKey { this->items.back().getKey() };

            this->items.pop_back();

            this->cache.erase(evictKey);
        }

        this->items.push_front(this->cache.try_emplace(key, value).first->second);
    }

    bool contains(KeyArgType key) const noexcept { return this->cache.contains(key); }
};

} // namespace Cachy

#endif
