#pragma once

struct string_view
{
	friend bool operator==(const string_view& ref, const std::string& other);

private:
	const std::string* source;

public:
	size_t begin;
	size_t end;

	string_view(std::nullptr_t) : source(nullptr), begin(0), end(0) {};

	string_view(const std::string& _source, size_t _begin, size_t _end)
		: source(&_source), begin(_begin), end(_end) {};

	string_view(const std::string& _source)
		: source(&_source), begin(0), end(_source.size()) {};

	operator std::string() const
	{
		return source->substr(begin, end - begin);
	}

	void operator= (const string_view& other)
	{
		source = other.source;
		begin = other.begin;
		end = other.end;
	}

	size_t size() const
	{
		return end - begin;
	}

	const char& at(size_t id) const
	{
		return source->at(begin + id);
	}
};

inline bool operator==(const string_view& ref, const std::string& other)
{
	if (ref.size() != other.size()) return false;

	for (size_t i = 0; i < other.size(); i++)
		if (ref.source->at(i + ref.begin) != other.at(i))
			return false;

	return true;
}

inline bool operator == (const std::string& other, const string_view& q)
{
	return operator==(q, other);
}

inline bool operator == (const string_view& q, const string_view& p)
{
	return q.begin == p.begin && q.end == p.end;
}

inline bool is_digit(const char& c)
{
	return (c >= '0' && c <= '9');
}

//does not check if char is a matl operator
//insted checks if char is one of the characters that should end a string_ref
inline bool is_operator(const char& c)
{
	return (c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '='
		|| c == '>' || c == '<' || c == '!'
		|| c == ')' || c == '(' || c == '.' || c == ','
		|| c == '#' || c == ':'
		|| c == '"' || c == '$' || c == '&' || c == '\'' || c == '?'
		|| c == '>' || c == '<' || c == '@' || c == ']' || c == '[' || c == ']' || c == '`' || c == '~');
}

inline bool is_whitespace(const char& c)
{
	return (c == ' ' || c == '\t' || c == '\0' || c == '\n');
}

inline bool is_at_source_end(const std::string& source, const size_t& iterator)
{
	return (iterator >= source.size() || source.at(iterator) == '\0');
};

inline bool is_at_line_end(const std::string& source, const size_t& iterator)
{
	return (is_at_source_end(source, iterator) || source.at(iterator) == '\n');
};

inline const char& get_char(const std::string& source, size_t& iterator)
{
	return source.at(iterator++);
}

inline void get_to_char(char target, const std::string& source, size_t& iterator)
{
	while (source.at(iterator) != target)
	{
		iterator++;
		if (is_at_source_end(source, iterator))
			return;
	}
}

inline void get_to_char_while_counting_lines(char target, const std::string& source, size_t& iterator, int& line_counter, bool& whitespaces_only)
{
	whitespaces_only = true;

	while (source.at(iterator) != target)
	{
		if (source.at(iterator) == '\n') line_counter++;
		else if (!is_whitespace(source.at(iterator))) whitespaces_only = false;

		iterator++;
		if (is_at_source_end(source, iterator))
			return;
	}
}

inline int get_spaces(const std::string& source, size_t& iterator)
{
	int spaces = 0;

	while (true)
	{
		const char& c = source.at(iterator);

		if (c == ' ')
			spaces++;
		else if (c == '\t')
			spaces += 4;
		else
			break;

		iterator++; if (iterator == source.size()) break;
	}

	return spaces;
}

inline string_view get_string_ref(const std::string& source, size_t& iterator, std::string& error)
{
	size_t begin = iterator;

	while (true)
	{
		const char& c = source.at(iterator);

		if (is_operator(c) || is_whitespace(c)) break;

		iterator++; if (iterator == source.size()) break;
	}

	if (iterator == begin)
	{
		error = "Expected token not: ";

		if (source.at(iterator) == ' ')
			error += "space";
		else if (source.at(iterator) == '\n')
			error += "new line";
		else if (source.at(iterator) == '\t')
			error += "tab";
		else
			error += source.at(iterator);
	}

	return string_view(source, begin, iterator);
}

extern const char comment_char;

inline string_view get_rest_of_line(const std::string& source, size_t& iterator)
{
	size_t begin = iterator;

	while (!is_at_line_end(source, iterator) && source.at(iterator) != comment_char)
		iterator++;

	if (!is_at_line_end(source, iterator) && source.at(iterator) == comment_char)
		iterator -= 1;

	while (is_whitespace(source.at(iterator - 1)))
		iterator--;

	return { source, begin, iterator };
}

inline void get_to_new_line(const std::string& source, size_t& iterator)
{
	while (!is_at_line_end(source, iterator))
		iterator++;
	iterator++;
}