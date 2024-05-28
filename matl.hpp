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
	enum class exception_type
	{
		unexpected_end_of_line,
		unknown_keyword,

		symbol_declaration,
		invalid_syntax,

		invalid_scalar_literal,
		mismatched_parentheses
	};

	void matl_throw(exception_type type, std::vector<std::string>&& sub_errors);

	class matl_exception : public std::exception
	{
	friend void matl_throw(exception_type type, std::vector<std::string>&& sub_errors);

	private:
		std::string message;
		matl_exception() : std::exception{""} {};
	public:
		const char* what() const override
		{
			return message.c_str();
		}
	};

	void matl_throw(exception_type type, std::vector<std::string>&& sub_errors)
	{
		std::string message;

		switch (type)
		{
		case matl_internal::exception_type::unexpected_end_of_line:
			message = "Unexpected end of line"; break;
		case matl_internal::exception_type::unknown_keyword:
			message = "Unknown keyword"; break;
		case matl_internal::exception_type::symbol_declaration:
			message = "Symbols cannot be declared in material"; break;
		case matl_internal::exception_type::invalid_syntax:
			message = "Invalid syntax"; break;
		case matl_internal::exception_type::invalid_scalar_literal:
			message = "Invalid scalar literal"; break;
		}

		for (auto& error : sub_errors)
			message += "\n\t" + error;

		auto exc = matl_exception{};
		exc.message = std::move(message);
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

		const char& at(size_t id)
		{
			return source.at(begin + id);
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

	bool operator == (const std::string& other, const string_ref& q)
	{
		return operator==(q, other);
	}

	bool operator == (const string_ref& q, const string_ref& p)
	{
		return q.begin == p.begin && q.end == p.end;
	}
};

/*
=======================================
	HETEROGENEOUS MAP
=======================================
*/

namespace matl_internal
{
	template<class _key, class _value, class _equality_solver>
	class heterogeneous_map
	{
	private:
		using _record = std::pair<_key, _value>;
		using _iterator = typename std::vector<_record>::iterator;

		std::vector<_record> records;

	public:
		heterogeneous_map() {};
		heterogeneous_map(std::vector<_record> __records)
			: records(std::move(__records)) {};

		inline _iterator begin()
		{
			return records.begin();
		}

		inline _iterator end()
		{
			return records.end();
		}

		template<class _key2>
		inline _value& at(const _key2& key)
		{
			for (_iterator itr = begin(); itr != end(); itr++)
				if (_equality_solver::equal(itr->first, key))
					return itr->second;
			throw std::runtime_error{"Invalid record"};
		}

		template<class _key2>
		inline _iterator find(const _key2& key)
		{
			for (_iterator itr = begin(); itr != end(); itr++)
				if (_equality_solver::equal(itr->first, key))
					return itr;
			return end();
		}

		inline _iterator insert(_record record)
		{
			auto itr = find(record.first);

			if (itr == end())
			{
				records.push_back(std::move(record));
				return records.end() - 1;
			}
			else
			{
				itr->second = std::move(record.second);
				return itr;
			}
		}
	};

	struct hgm_solver
	{
		static inline bool equal(const std::string& a, const std::string& b)
		{
			return a == b;
		}

		static inline bool equal(const std::string& a, const string_ref& b)
		{
			return b == a;
		}
	};

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

	inline const char& get_char(const std::string& source, size_t& iterator)
	{
		return source.at(iterator++);
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
			matl_throw(exception_type::unexpected_end_of_line, {});

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

		struct expression
		{
			struct node
			{
				enum class node_type
				{
					single_arg_left_parenthesis,
					scalar_literal,
					variable,
					symbol,
					unary_operator,
					binary_operator,
					vector_contructor_operator,
					vector_component_access,
					function,

				} type;

				union node_value
				{
					std::string						scalar_value;
					std::string						variable_name;
					std::string						symbol_name;
					const unary_operator*			unary_operator;
					const binary_operator*			binary_operator;
					uint8_t							vector_size;
					std::vector<uint8_t>			included_vector_components;
					std::pair<std::string, int>		function_name_and_passed_args;
					~node_value() {};
				} value;
			};

			std::list<node*> nodes;

			~expression()
			{
				for (auto& n : nodes)
					delete n;
			}
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
		const char symbols_prefix = '$';

		const uint8_t functions_precedence = 3;
	}
}

/*
=======================================
	MATL TYPE DEFINITIONS
=======================================
*/

namespace matl_internal
{
	namespace typedefs
	{
		struct variable_definition
		{
			const math::data_type* return_type;
			math::expression definition;
		};

		struct property_definition
		{
			math::expression definition;
		};
	}
}

/*
=======================================
	EXPRESSIONS PARSING
=======================================
*/

namespace matl_internal
{
	namespace expressions_parsing_internal
	{
		string_ref get_node_str(const std::string& source, size_t& iterator)
		{
			if (is_operator(source.at(iterator)))
			{
				iterator++;
				return string_ref{ source, iterator - 1, iterator };
			}

			size_t begin = iterator;

			bool only_digits = true;
			bool dot_used = false;

			while (true)
			{
				const char& c = source.at(iterator);

				if (!isdigit(c) && c != '.') only_digits = false;

				if (is_operator(c))
				{
					if (c != '.') break;
					if (!only_digits) break;
					if (dot_used) break;
					dot_used = true;
				}

				if (is_whitespace(c)) break;

				iterator++; if (iterator == source.size()) break;
			}

			if (iterator == begin)
				matl_throw(exception_type::unexpected_end_of_line, {});

			return string_ref{ source, begin, iterator };
		}

		bool is_function_call(const std::string& source, size_t& iterator)
		{
			get_spaces(source, iterator);
			return (source.at(iterator) == '(');
		}

		bool is_scalar_literal(const std::string& node_str)
		{
			int dot_pos = -1;

			for (int i = 0; i < node_str.size(); i++)
				if (node_str.at(i) == '.' && dot_pos == -1)
					dot_pos = i;
				else if (!isdigit(node_str.at(i)))
					return false;

			if (dot_pos == 0 || dot_pos == node_str.size() - 1)
				matl_throw(exception_type::invalid_scalar_literal, { node_str });

			return true;
		}

		bool is_unary_operator(const std::string& node_str)
		{
			if (node_str.size() != 1) return false;
			return get_unary_operator(node_str.at(0)) != nullptr;
		}

		bool is_binary_operator(const std::string& node_str)
		{
			if (node_str.size() != 1) return false;
			return get_binary_operator(node_str.at(0)) != nullptr;
		}

		int get_comas_inside_parenthesis(const std::string& source, size_t iterator)
		{
			int parenthesis_deepness = 0;
			int comas = 0;

			while (!is_at_line_end(source, iterator))
			{
				switch (source.at(iterator))
				{
				case '(':
					parenthesis_deepness++; break;
				case ')':
					parenthesis_deepness--;
					if (parenthesis_deepness == 0)
						goto _get_comas_inside_parenthesis_break;
					break;
				case ',':
					if (parenthesis_deepness == 1)
						comas++;
					break;
				}
				iterator++;
			}

		_get_comas_inside_parenthesis_break:
			if (parenthesis_deepness != 0)
				matl_throw(exception_type::mismatched_parentheses, {});

			return comas;
		}

		int get_precedence(const math::expression::node* node)
		{
			switch (node->type)
			{
			case math::expression::node::node_type::function:
				return builtins::functions_precedence;
			case math::expression::node::node_type::binary_operator:
				return node->value.binary_operator->precedence;
			case math::expression::node::node_type::unary_operator:
				return node->value.unary_operator->precedence;
			case math::expression::node::node_type::vector_component_access:
				return 256;
			}

			return 0;
		}

		bool is_any_of_left_parentheses(const math::expression::node* node)
		{
			return	node->type == math::expression::node::node_type::vector_contructor_operator ||
					node->type == math::expression::node::node_type::single_arg_left_parenthesis ||
					node->type == math::expression::node::node_type::function;
		}

		void insert_operator(
			std::list<math::expression::node*>& operators_stack, 
			std::list<math::expression::node*>& output,
			math::expression::node* node
		)
		{
			while (operators_stack.size() != 0)
			{
				auto previous = operators_stack.back();
				if (
					previous->type != math::expression::node::node_type::single_arg_left_parenthesis &&
					previous->type != math::expression::node::node_type::vector_contructor_operator &&
					get_precedence(previous) >= get_precedence(node)
					)
				{
					output.push_back(previous);
					operators_stack.pop_back();
				}
				else break;
			}

			operators_stack.push_back(node);
		}

		void shunting_yard(
			math::expression::node*& new_node,
			std::list<math::expression::node*>& output,
			std::list<math::expression::node*>& operators,
			const std::string& source,
			size_t& iterator
		);
	}

	math::expression get_expression(const std::string& source, size_t& iterator)
	{
		math::expression::node* new_node = nullptr;

		std::list<math::expression::node*> output;
		std::list<math::expression::node*> operators;

		try
		{
			expressions_parsing_internal::shunting_yard(new_node, output, operators, source, iterator);
		}
		catch (const matl_exception& exc)
		{
			if (new_node != nullptr) delete new_node;

			for (auto& n : output)
				delete n;

			for (auto& n : operators)
				delete n;

			throw exc;
		}

		math::expression exp;
		exp.nodes = std::move(output);
		return exp;
	}

	const math::data_type* validate_expression(const math::expression& exp)
	{
		return nullptr;
	}
}

/*
=======================================
	PARSING
=======================================
*/

namespace matl_internal
{
	struct parsing_state
	{
		size_t iterator = 0;
		int line_conter = 0;

		//std::vector<int> errors;

		heterogeneous_map<std::string, typedefs::variable_definition, hgm_solver> variables;
		heterogeneous_map<std::string, typedefs::property_definition, hgm_solver> properties;
	};

	void parse_line(const std::string& material_source, matl::context* context, parsing_state& state);

	using keyword_handle = void(*)(const std::string& material_source, matl::context* context, parsing_state& state);

	namespace matl_keywords
	{
		void let(const std::string& source, matl::context* context, parsing_state& state)
		{
			auto& iterator = state.iterator;

			get_spaces(source, iterator);
			auto var_name = get_string_ref(source, iterator);

			if (var_name.at(0) == builtins::symbols_prefix)
				matl_throw(exception_type::symbol_declaration, {});

			get_spaces(source, iterator);
			auto assign_operator = get_char(source, iterator);

			if (assign_operator != '=')
				matl_throw(exception_type::invalid_syntax, {});

			auto exp = get_expression(source, iterator);

			typedefs::variable_definition var_def;
			var_def.return_type = validate_expression(var_def.definition);
			var_def.definition = std::move(exp);

			state.variables.insert({ var_name, std::move(var_def) });
		}

		void property(const std::string& source, matl::context* context, parsing_state& state)
		{

		}

		void _using(const std::string& source, matl::context* context, parsing_state& state)
		{

		}

		void func(const std::string& source, matl::context* context, parsing_state& state)
		{

		}

		void _return(const std::string& source, matl::context* context, parsing_state& state)
		{

		}
	}

	heterogeneous_map<std::string, keyword_handle, hgm_solver> keywords_handles =
	{
		{
			{"let", matl_keywords::let},
			{"property", matl_keywords::property},
			{"using", matl_keywords::_using},
			{"func", matl_keywords::func},
			{"return", matl_keywords::_return}
		}	
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

inline void matl_internal::parse_line(const std::string& material_source, matl::context* context, parsing_state& state)
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
