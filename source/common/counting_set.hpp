template<class _type>
class counting_set
{
	using element_with_counter = std::pair<_type, uint32_t>;
	std::vector<element_with_counter> elements;
	using iterator = typename std::vector<element_with_counter>::iterator;
	using reverse_iterator = typename std::vector<element_with_counter>::reverse_iterator;

public:
	inline uint32_t insert(_type element)
	{
		auto itr = elements.begin();

		while (itr != elements.end())
		{
			if (itr->first == element) break;
			itr++;
		}

		if (itr == elements.end())
		{
			elements.push_back({ element, 1 });
			return 1;
		}

		(*itr).second++;
		return (*itr).second;
	}

	inline iterator begin()
	{
		return elements.begin();
	}

	inline iterator end()
	{
		return elements.end();
	}

	inline reverse_iterator rbegin()
	{
		return elements.rbegin();
	}

	inline reverse_iterator rend()
	{
		return elements.rend();
	}
};
