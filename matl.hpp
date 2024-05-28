#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <exception>

//DEFINE MATL_IMPLEMENTATION
//In order to implement matl in given translation unit


/*
=======================================
	API DECLARATION
=======================================
*/

namespace matl
{
	std::string get_language_version();

	struct file_request_response
	{
		bool success;
		std::string source;
	};

	using file_request_callback = file_request_response(*)(const std::string& file_name);

	class context;

	context* create_context();
	void destroy_context(context*&);

	class context
	{
	private:
		struct implementation;
		implementation* impl;

	public:
		void set_domain_request_callback(file_request_callback callback);
		void set_library_request_callback(file_request_callback callback);

		void set_cache_domains(bool do_cache);
		void set_cache_libraries(bool do_cache);

	private:
		context();
		~context();

		friend context* matl::create_context();
		friend void matl::destroy_context(matl::context*& context);
	};

	struct parsed_material
	{
		bool success = false;
		std::string source;
		std::vector<std::string> errors;
		//Add Reflection
	};

	parsed_material parse_material(const std::string& material_source, matl::context* context);
}

/*
=======================================
	MATL IMPLEMENTATION
=======================================
*/

#ifdef MATL_IMPLEMENTATION

/*
=======================================
	MATL EXCEPTION
=======================================
*/

namespace matl_internal
{
	void matl_throw(std::string error);

	class matl_exception : public std::exception
	{
	friend void matl_throw(std::string error);

	private:
		std::string message;
		matl_exception() : std::exception{""} {};
	public:
		const char* what() const override
		{
			return message.c_str();
		}
	};

	void matl_throw(std::string error)
	{
		auto exc = matl_exception{};
		exc.message = std::move(error);
		throw exc;
	}
}

/*
=======================================
	STRING REF
=======================================
*/

namespace matl_internal
{
	struct string_ref
	{
	private:
		const std::string& source;
	public:
		size_t begin;
		size_t end;

		string_ref(const std::string& _source, size_t _begin, size_t _end)
			: source(_source), begin(_begin), end(_end) {};

		string_ref(const std::string& _source)
			: source(_source), begin(0), end(_source.size()) {};

		operator std::string() const
		{
			return source.substr(begin, end - begin);
		}

		size_t size() const
		{
			return end - begin;
		}

		friend bool operator == (const string_ref& q, const std::string& other);
	};

	bool operator == (const string_ref& q, const std::string& other)
	{
		if (q.size() != other.size()) return false;

		for (size_t i = 0; i < other.size(); i++)
			if (q.source.at(i + q.begin) != other.at(i))
				return false;

		return true;
	}

	bool operator == (const string_ref& q, const string_ref& p)
	{
		return q.begin == p.begin && q.end == p.end;
	}
}

/*
=======================================
	STRING TRAVERSION
=======================================
*/

namespace matl_internal
{
	inline bool is_operator(const char& c)
	{
		return (c == '+' || c == '-' || c == '*' || c == '/' || c == '='
			|| c == '>' || c == '<'
			|| c == ')' || c == '(' || c == '.' || c == ',');
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

	inline string_ref get_string_ref(const std::string& source, size_t& iterator)
	{
		size_t begin = iterator;

		while (true)
		{
			const char& c = source.at(iterator);

			if (is_operator(c) || is_whitespace(c)) break;

			iterator++; if (iterator == source.size()) break;
		}

		if (iterator == begin)
			matl_throw("Unexpected end of line; Expected token");

		return string_ref(source, begin, iterator);
	}

	inline void get_to_new_line(const std::string& source, size_t& iterator)
	{
		while (!is_at_line_end(source, iterator))
			iterator++;
		iterator++;
	}
}

/*
=======================================
	INTERNAL TYPES
=======================================
*/

namespace matl_internal
{
	namespace math
	{
		struct data_type;
		struct unary_operator;
		struct binary_operator;
	}

	const math::data_type* const get_data_type(string_ref name);
	const math::unary_operator* const get_unary_operator(const char& symbol);
	const math::binary_operator* const get_binary_operator(const char& symbol);

	namespace math
	{
		struct data_type
		{
			std::string name;

			data_type(std::string _name)
				: name(_name) {};
		};

		struct unary_operator
		{
			char symbol;
			uint8_t precedence;
			std::string operation_display_name;

			struct valid_types_set
			{
				const data_type* operand_type;
				const data_type* returned_type;
				valid_types_set(
					std::string _operand_type,
					std::string _returned_type) :
					operand_type(get_data_type(_operand_type)),
					returned_type(get_data_type(_returned_type)) {};
			};

			std::vector<valid_types_set> allowed_types;

			unary_operator(
				char _symbol,
				std::string _operation_display_name,
				uint8_t _precedence,
				std::vector<valid_types_set> _allowed_types)
				: symbol(_symbol), operation_display_name(_operation_display_name),
				precedence(_precedence), allowed_types(_allowed_types) {};
		};

		struct binary_operator
		{
			char symbol;
			uint8_t precedence;
			std::string operation_display_name;

			struct valid_types_set
			{
				const data_type* left_operand_type;
				const data_type* right_operand_type;
				const data_type* returned_type;

				valid_types_set(
					std::string _left_operand_type,
					std::string _right_operand_type,
					std::string _returned_type) :
					left_operand_type(get_data_type(_left_operand_type)),
					right_operand_type(get_data_type(_right_operand_type)),
					returned_type(get_data_type(_returned_type)) {};
			};

			std::vector<valid_types_set> allowed_types;

			binary_operator(
				char _symbol,
				std::string _operation_display_name,
				uint8_t _precedence,
				std::vector<valid_types_set> _allowed_types)
				: symbol(_symbol), operation_display_name(_operation_display_name),
				precedence(_precedence), allowed_types(_allowed_types) {};
		};
	}
}

/*
=======================================
	BUILTINS
=======================================
*/

namespace matl_internal
{
	namespace builtins
	{
		//language data types
		const std::vector<matl_internal::math::data_type> data_types
		{
			{ "scalar", },
			{ "vec2", },
			{ "vec3", },
			{ "vec4", },
			{ "texture", }
		};

		//unary operators; symbol, display name, precedence, {input type, output type}
		const std::vector<matl_internal::math::unary_operator> unary_operators
		{
			{ '-', "negate", 4, {
				{"scalar", "scalar"},
				{"vec2", "vec2"},
				{"vec3", "vec3"},
				{"vec4", "vec4"}
			}},
		};

		//binary operators; symbol, display name, precedence, {input type left, input type right, output type}
		const std::vector<matl_internal::math::binary_operator> binary_operators
		{
			{ '+', "add", 1, {
				{"scalar", "scalar", "scalar"},

				{"scalar", "vec2", "vec2"},
				{"vec2", "scalar", "vec2"},
				{"vec2", "vec2", "vec2"},

				{"scalar", "vec3", "vec3"},
				{"vec3", "scalar", "vec3"},
				{"vec3", "vec3", "vec3"},

				{"scalar", "vec4", "vec4"},
				{"vec4", "scalar", "vec4"},
				{"vec4", "vec3", "vec4"}
			}},

			{ '-', "substract",1, {
				{"scalar", "scalar", "scalar"},

				{"vec2", "scalar", "vec2"},
				{"vec2", "vec2", "vec2"},

				{"vec3", "scalar", "vec3"},
				{"vec3", "vec3", "vec3"},

				{"vec4", "scalar", "vec4"},
				{"vec4", "vec3", "vec4"}
			} },

			{ '*', "multiply",2, {
				{"scalar", "scalar", "scalar"},

				{"scalar", "vec2", "vec2"},
				{"vec2", "scalar", "vec2"},
				{"vec2", "vec2", "vec2"},

				{"scalar", "vec3", "vec3"},
				{"vec3", "scalar", "vec3"},
				{"vec3", "vec3", "vec3"},

				{"scalar", "vec4", "vec4"},
				{"vec4", "scalar", "vec4"},
				{"vec4", "vec4", "vec4"}
			} },

			{ '/', "divide",2, {
				{"scalar", "scalar", "scalar"},

				{"vec2", "scalar", "vec2"},
				{"vec2", "vec2", "vec2"},

				{"vec3", "scalar", "vec3"},
				{"vec3", "vec3", "vec3"},

				{"vec4", "scalar", "vec4"},
				{"vec4", "vec3", "vec4"}
			} },
		};

		//names of vector components; used to access vector components eg. normal_vector.x; base_color.rgb
		const std::unordered_map<char, uint8_t> vector_components_names = {
			{ 'x', 1 },
			{ 'y', 2 },
			{ 'z', 3 },
			{ 'w', 4 },

			{ 'r', 1 },
			{ 'g', 2 },
			{ 'b', 3 },
			{ 'a', 4 }
		};

		//data type applied to all integer and float literals eg. 1; 2; 3.14
		const matl_internal::math::data_type* scalar_data_type = get_data_type({ "scalar" });

		const char comment_char = '#';
	}
}


/*
=======================================
	PARSE IMPLEMENTATION
=======================================
*/

namespace matl_internal
{
	struct parsing_state
	{
		size_t iterator = 0;
		int line_conter = 0;
	};

	void parse_line(const std::string& material_source, matl::context* context, parsing_state& state);

	using keyword_handle = void(*)(const std::string& material_source, matl::context* context, parsing_state& state);

	std::unordered_map<std::string, keyword_handle> keywords_handles = 
	{
		//{"let", nullptr}
	};
}

matl::parsed_material matl::parse_material(const std::string& material_source, matl::context* context)
{
	matl_internal::parsing_state state;

	while (!matl_internal::is_at_source_end(material_source, state.iterator))
	{
		matl_internal::parse_line(material_source, context, state);

		if (matl_internal::is_at_source_end(material_source, state.iterator)) break;
		matl_internal::get_to_new_line(material_source, state.iterator);
	}

	return {};
}

void matl_internal::parse_line(const std::string& material_source, matl::context* context, parsing_state& state)
{
	auto& source = material_source;
	auto& iterator = state.iterator;

	int spaces = get_spaces(source, iterator);

	if (is_at_line_end(source, iterator)) return;
	if (source.at(iterator) == builtins::comment_char) return;
	
	string_ref keyword = get_string_ref(source, iterator);

	auto kh_itr = keywords_handles.find(keyword);

	if (kh_itr == keywords_handles.end())
	{
		//Error
		return;
	}
	
	kh_itr->second(material_source, context, state);
}

/*
=======================================
	BUILTINS RELATED FUNCTIONS IMPLEMENTATION
=======================================
*/

namespace matl_internal
{
	const math::data_type* const get_data_type(string_ref name)
	{
		for (auto& type : builtins::data_types)
			if (type.name == name)
				return &type;
		return nullptr;
	}

	const math::unary_operator* const get_unary_operator(const char& symbol)
	{
		for (auto& op : builtins::unary_operators)
			if (op.symbol == symbol)
				return &op;
		return nullptr;
	}

	const math::binary_operator* const get_binary_operator(const char& symbol)
	{
		for (auto& op : builtins::binary_operators)
			if (op.symbol == symbol)
				return &op;
		return nullptr;
	}
}

/*
=======================================
	API IMPLEMENTATION
=======================================
*/

const std::string language_version = "0.1";
std::string matl::get_language_version()
{
	return language_version;
}

/*
=======================================
	CONTEXT IMPLEMENTATION
=======================================
*/

namespace matl
{
	struct context::implementation
	{
		file_request_callback domain_rc = nullptr;
		file_request_callback library_rc = nullptr;

		bool cache_domains = false;
		bool cache_libraries = false;
	};

	context::context()
	{
		impl = new implementation;
	}

	context::~context()
	{
		delete impl;
	}

	context* create_context()
	{
		return new context;
	}

	void destroy_context(context*& context)
	{
		delete context;
	}

	void context::set_domain_request_callback(file_request_callback callback)
	{
		impl->domain_rc = callback;
	}

	void context::set_library_request_callback(file_request_callback callback)
	{
		impl->library_rc = callback;
	}

	void context::set_cache_domains(bool do_cache)
	{
		impl->cache_domains = do_cache;
	}

	void context::set_cache_libraries(bool do_cache)
	{
		impl->cache_libraries = do_cache;
	}
}

#endif // MATL_IMPLEMENTATION
