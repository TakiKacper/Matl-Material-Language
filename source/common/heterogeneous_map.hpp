#pragma once

template<class _key, class _value, class _equality_solver>
class heterogeneous_map
{
public:
	using record = std::pair<_key, _value>;
	using iterator = typename std::list<record>::iterator;
	using const_iterator = typename std::list<record>::const_iterator;

private:
	std::list<record> records;

public:
	heterogeneous_map() {};
	heterogeneous_map(std::list<record> __records)
		: records(std::move(__records)) {};

	inline size_t size() const
	{
		return records.size();
	}

	inline iterator begin()
	{
		return records.begin();
	}

	inline iterator end()
	{
		return records.end();
	}

	inline const_iterator begin() const
	{
		return records.begin();
	}

	inline const_iterator end() const
	{
		return records.end();
	}

	inline record& recent()
	{
		return records.back();
	}

	inline const record& recent() const
	{
		return records.back();
	}

	template<class _key2>
	inline _value& at(const _key2& key)
	{
		for (iterator itr = begin(); itr != end(); itr++)
			if (_equality_solver::equal(itr->first, key))
				return itr->second;
		throw std::exception{};
	}

	template<class _key2>
	inline const _value& at(const _key2& key) const
	{
		for (const_iterator itr = begin(); itr != end(); itr++)
			if (_equality_solver::equal(itr->first, key))
				return itr->second;
		throw std::exception{};
	}

	template<class _key2>
	inline iterator find(const _key2& key)
	{
		for (iterator itr = begin(); itr != end(); itr++)
			if (_equality_solver::equal(itr->first, key))
				return itr;
		return end();
	}

	template<class _key2>
	inline const_iterator find(const _key2& key) const
	{
		for (const_iterator itr = begin(); itr != end(); itr++)
			if (_equality_solver::equal(itr->first, key))
				return itr;
		return end();
	}

	inline iterator insert(record record)
	{
		auto itr = find(record.first);

		if (itr == end())
		{
			records.push_back(std::move(record));
			return --records.end();
		}
		else
		{
			itr->second = std::move(record.second);
			return itr;
		}
	}

	inline void insert(iterator begin, iterator end)
	{
		for (auto itr = begin; itr != end; itr++)
			insert(*itr);
	}

	inline void remove(const _key& key)
	{
		for (const_iterator itr = begin(); itr != end(); itr++)
			if (_equality_solver::equal(itr->first, key))
			{
				records.remove(*itr);
				return;
			}
		throw std::exception{};
	}
};

struct hgm_string_solver
{
	static inline bool equal(const std::string& parameters_declarations_translator, const std::string& b)
	{
		return parameters_declarations_translator == b;
	}

	static inline bool equal(const std::string& parameters_declarations_translator, const string_view& b)
	{
		return b == parameters_declarations_translator;
	}
};

struct hgm_pointer_solver
{
	static inline bool equal(void* parameters_declarations_translator, void* b)
	{
		return parameters_declarations_translator == b;
	}
};