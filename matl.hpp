#pragma once

#include <string>
#include <vector>


//DEFINE MATL_IMPLEMENTATION
//In order to implement matl in given translation unit


//API DECLARATION
namespace matl
{
	std::string get_language_version();

	struct file_request_response
	{
		bool success;
		std::string source;
	};

	struct parsed_material
	{
		bool success = false;
		std::vector<std::string> sources;
		std::vector<std::string> errors;
		//Add Reflection
	};

	using file_request_callback = file_request_response(*)(const std::string& file_name);

	class context;

	context* create_context();
	void destroy_context(context*&);

	parsed_material parse_material(const std::string& material_source, matl::context* context);
	void parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);

	class context
	{
	private:
		struct implementation;
		implementation* impl;
		friend parsed_material parse_material(const std::string& material_source, matl::context* context);
		friend void parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);

	public:
		void set_library_request_callback(file_request_callback callback);

	private:
		context();
		~context();

		friend context* matl::create_context();
		friend void matl::destroy_context(matl::context*& context);
	};
}

//MATL IMPLEMENTATION
#ifdef MATL_IMPLEMENTATION

#include <unordered_map>
#include <exception>

//MATL EXCEPTION
namespace matl_internal
{
	enum class exception_type
	{
		unexpected_end_of_line,
		unknown_keyword,

		domain_already_specified,
		no_such_domain,

		symbol_declaration,
		invalid_syntax,

		invalid_scalar_literal,
		mismatched_parentheses,

		invalid_directive,
		cannot_use_directive_here,
		end_of_directive_expected,
		invalid_property_type,
		invalid_dump_type,

		vector_too_long,
		invalid_vector_component,
		unknown_token
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
		case matl_internal::exception_type::mismatched_parentheses:
			message = "Mismatched parentheses"; break;
		case matl_internal::exception_type::invalid_directive:
			message = "No such directive"; break;
		case matl_internal::exception_type::cannot_use_directive_here:
			message = "Cannot use this directive here"; break;
		case matl_internal::exception_type::end_of_directive_expected:
			message = "Expected end of directive"; break;
		case matl_internal::exception_type::invalid_property_type:
			message = "Invalid property type"; break;
		case matl_internal::exception_type::invalid_dump_type:
			message = "Invalid dump type"; break;
		case matl_internal::exception_type::vector_too_long:
			message = "Vector can have up to 4 dimensions"; break;
		case matl_internal::exception_type::invalid_vector_component:
			message = "Invalid vector component"; break;
		case matl_internal::exception_type::unknown_token:
			message = "Unknown token"; break;
		case matl_internal::exception_type::no_such_domain:
			message = "No such domain"; break;
		case matl_internal::exception_type::domain_already_specified:
			message = "Domain has already been selected"; break;
		}

		for (auto& error : sub_errors)
			message += "\n\t" + error;

		auto exc = matl_exception{};
		exc.message = std::move(message);
		throw exc;
	}
}

//STRING REF
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

//HETEROGENEOUS MAP
namespace matl_internal
{
	template<class _key, class _value, class _equality_solver>
	class heterogeneous_map
	{
	private:
		using _record = std::pair<_key, _value>;
		using _iterator = typename std::list<_record>::iterator;

		std::list<_record> records;

	public:
		heterogeneous_map() {};
		heterogeneous_map(std::list<_record> __records)
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
				return --records.end();
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

//STRING TRAVERSION
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

	inline void get_to_char(char target, const std::string& source, size_t& iterator)
	{
		while (source.at(iterator) != target)
		{
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

	inline string_ref get_rest_of_line(const std::string& source, size_t& iterator)
	{
		size_t begin = iterator;

		while (!is_at_line_end(source, iterator))
			iterator++;
		
		return { source, begin, iterator };
	}

	inline void get_to_new_line(const std::string& source, size_t& iterator)
	{
		while (!is_at_line_end(source, iterator))
			iterator++;
		iterator++;
	}
}

//INTERNAL TYPES
namespace matl_internal
{
	namespace typedefs
	{
		struct variable_definition;
	}

	namespace domain
	{
		struct symbol;
	}

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
					typedefs::variable_definition*	variable;
					domain::symbol*					symbol;
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

//BUILTINS
namespace matl_internal
{
	namespace builtins
	{
		//language data types
		const std::vector<matl_internal::math::data_type> data_types
		{
			{ "scalar", },
			{ "vector2", },
			{ "vector3", },
			{ "vector4", },
			{ "texture", }
		};

		//unary operators; symbol, display name, precedence, {input type, output type}
		const std::vector<matl_internal::math::unary_operator> unary_operators
		{
			{ '-', "negate", 4, {
				{"scalar", "scalar"},
				{"vector2", "vector2"},
				{"vector3", "vector3"},
				{"vector4", "vector4"}
			}},
		};

		//binary operators; symbol, display name, precedence, {input type left, input type right, output type}
		const std::vector<matl_internal::math::binary_operator> binary_operators
		{
			{ '+', "add", 1, {
				{"scalar", "scalar", "scalar"},

				{"scalar", "vector2", "vector2"},
				{"vector2", "scalar", "vector2"},
				{"vector2", "vector2", "vector2"},

				{"scalar", "vector3",  "vector3"},
				{"vector3", "scalar",  "vector3"},
				{"vector3", "vector3", "vector3"},

				{"scalar", "vector4",  "vector4"},
				{"vector4", "scalar",  "vector4"},
				{"vector4", "vector4", "vector4"}
			}},

			{ '-', "substract",1, {
				{"scalar", "scalar", "scalar"},

				{"vector2", "scalar", "vector2"},
				{"vector2", "vector2", "vector2"},

				{"vector3", "scalar", "vector3"},
				{"vector3", "vector3", "vector3"},

				{"vector4", "scalar", "vector4"},
				{"vector4", "vector4", "vector4"}
			} },

			{ '*', "multiply",2, {
				{"scalar", "scalar", "scalar"},

				{"scalar", "vector2", "vector2"},
				{"vector2", "scalar", "vector2"},
				{"vector2", "vector2", "vector2"},

				{"scalar", "vector3", "vector3"},
				{"vector3", "scalar", "vector3"},
				{"vector3", "vector3", "vector3"},

				{"scalar", "vector4", "vector4"},
				{"vector4", "scalar", "vector4"},
				{"vector4", "vector4", "vector4"}
			} },

			{ '/', "divide",2, {
				{"scalar", "scalar", "scalar"},

				{"vector2", "scalar", "vector2"},
				{"vector2", "vector2", "vector2"},

				{"vector3", "scalar", "vector3"},
				{"vector3", "vector3", "vector3"},

				{"vector4", "scalar", "vector4"},
				{"vector4", "vector4", "vector4"}
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

//MATL TYPE DEFINITIONS
namespace matl_internal
{
	namespace typedefs
	{
		struct variable_definition
		{
			const math::data_type* return_type;
			math::expression* definition;

			~variable_definition()
			{
				delete definition;
			}
		};

		struct property_definition
		{
			math::expression* definition;

			~property_definition()
			{
				delete definition;
			}
		};
	}
}

//DOMAIN PARSING TYPES
namespace matl_internal
{
	namespace domain
	{
		enum class directive_type
		{
			dump_block,
			dump_variables,
			dump_functions,
			dump_property,
			split
		};

		struct directive
		{
			directive_type type;
			std::string payload;
			directive(directive_type _type, std::string _payload)
				: type(std::move(_type)), payload(std::move(_payload)) {};
		};

		struct symbol
		{
			const math::data_type* type;
			std::string definition;
			symbol(const math::data_type* _type, std::string _definition)
				: type(_type), definition(_definition) {};
		};

		struct parsed_domain
		{
			std::vector<directive> directives;

			heterogeneous_map<std::string, const math::data_type*, hgm_solver> properties;
			heterogeneous_map<std::string, symbol, hgm_solver> symbols;
		};

		struct parsing_state
		{
			size_t iterator = 0;
			parsed_domain* domain;

			bool expose_closure = false;
		};

		using directive_handle =
			void(*)(const std::string& material_source, matl::context* context, parsing_state& state);

		namespace directive_handles
		{
			void expose(const std::string& source, matl::context* context, parsing_state& state)
			{
				if (state.expose_closure == true)
					matl_throw(exception_type::cannot_use_directive_here, { "expose" });

				state.expose_closure = true;
			}

			void end(const std::string& source, matl::context* context, parsing_state& state)
			{
				if (state.expose_closure == false)
					matl_throw(exception_type::cannot_use_directive_here, { "end" });

				state.expose_closure = false;
			}

			void property(const std::string& source, matl::context* context, parsing_state& state)
			{
				if (state.expose_closure)
				{
					get_spaces(source, state.iterator);
					auto type_name = get_string_ref(source, state.iterator);

					get_spaces(source, state.iterator);
					auto name = get_string_ref(source, state.iterator);

					auto type = get_data_type(type_name);
					if (type == nullptr) matl_throw(exception_type::invalid_property_type, { type_name });

					state.domain->properties.insert({ name, type });
				}
				else
				{
					get_spaces(source, state.iterator);
					auto name = get_string_ref(source, state.iterator);

					state.domain->directives.push_back(
						{ directive_type::dump_property, std::move(name) }
					);
				}
			}

			void symbol(const std::string& source, matl::context* context, parsing_state& state)
			{
				if (state.expose_closure)
				{
					get_spaces(source, state.iterator);
					auto type_name = get_string_ref(source, state.iterator);

					auto type = get_data_type(type_name);
					if (type == nullptr) matl_throw(exception_type::invalid_property_type, { type_name });

					get_spaces(source, state.iterator);
					auto name = get_string_ref(source, state.iterator);

					get_spaces(source, state.iterator);
					if (source.at(state.iterator) != '=')
						matl_throw(exception_type::invalid_syntax, { "expected '='" });

					state.iterator++;

					if (source.at(state.iterator) == '>')
						matl_throw(exception_type::invalid_syntax, { "expected symbol definition" });

					size_t begin = state.iterator;
					get_to_char('>', source, state.iterator);

					state.domain->symbols.insert({ name, {type, source.substr(begin, state.iterator - begin)} });
				}
				else
					matl_throw(exception_type::cannot_use_directive_here, { "symbol" });
			}

			void dump(const std::string& source, matl::context* context, parsing_state& state)
			{
				get_spaces(source, state.iterator);
				auto dump_what = get_string_ref(source, state.iterator);

				if (dump_what == "variables")
					state.domain->directives.push_back(
						{ directive_type::dump_variables, {} }
				);
				else if (dump_what == "functions")
					state.domain->directives.push_back(
						{ directive_type::dump_functions, {} }
				);
				else
					matl_throw(exception_type::invalid_dump_type, { dump_what });

				get_to_char('>', source, state.iterator);
			}

			void split(const std::string& source, matl::context* context, parsing_state& state)
			{
				state.domain->directives.push_back(
					{ directive_type::split, {} }
				);
			}
		}

		heterogeneous_map<std::string, directive_handle, hgm_solver> directives_handles =
		{
			{
				{"expose",	 directive_handles::expose},
				{"end",		 directive_handles::end},
				{"property", directive_handles::property},
				{"symbol",	 directive_handles::symbol},
				{"dump",	 directive_handles::dump},
				{"split",	 directive_handles::split}
			}
		};
	}
}

//MATERIAL PARSING TYPES
namespace matl_internal
{
	struct parsing_state
	{
		size_t iterator = 0;
		int line_counter = 0;

		//std::vector<int> errors;

		heterogeneous_map<std::string, typedefs::variable_definition, hgm_solver> variables;
		heterogeneous_map<std::string, typedefs::property_definition, hgm_solver> properties;

		const domain::parsed_domain* domain = nullptr;
	};
}

//EXPRESSIONS PARSING
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
		};

		void shunting_yard(
			parsing_state& state,
			math::expression::node*& new_node,
			std::list<math::expression::node*>& output,
			std::list<math::expression::node*>& operators,
			const std::string& source,
			size_t& iterator
		)
		{
			using node = math::expression::node;
			using node_type = node::node_type;

			std::string node_str;
			bool accepts_right_unary_operator = true;

			auto push_vector_or_parenthesis = [&]()
			{
				int comas = get_comas_inside_parenthesis(source, iterator - 1);

				if (comas == 0)
					new_node->type = node_type::single_arg_left_parenthesis;
				else if (comas <= 3)
				{
					new_node->type = node_type::vector_contructor_operator;
					new_node->value.vector_size = comas + 1;
				}
				else
					matl_throw(exception_type::vector_too_long, {});

				operators.push_back(new_node);
			};

			auto close_parenthesis = [&]()
			{
				bool found_left_parenthesis = false;
				math::expression::node* previous = nullptr;

				while (operators.size() != 0)
				{
					previous = operators.back();
					if (is_any_of_left_parentheses(previous))
					{
						found_left_parenthesis = true;
						break;
					}

					output.push_back(previous);
					operators.pop_back();
				}

				if (!found_left_parenthesis)
					matl_throw(exception_type::mismatched_parentheses, {});
				else if (previous->type == node_type::single_arg_left_parenthesis)
					operators.pop_back();
				else
				{
					output.push_back(previous);
					operators.pop_back();
				}

				delete new_node;
			};

			auto handle_comma = [&]()
			{
				while (operators.size() != 0)
				{
					auto previous = operators.back();
					if (is_any_of_left_parentheses(previous))
						break;
					output.push_back(previous);
					operators.pop_back();
				}
			};

			get_spaces(source, iterator);
			while (!is_at_line_end(source, iterator))
			{
				node_str = get_node_str(source, iterator);
				new_node = new node{};

				if (is_unary_operator(node_str) && accepts_right_unary_operator)
				{
					new_node->type = node_type::unary_operator;
					new_node->value.unary_operator = get_unary_operator(node_str.at(0));

					insert_operator(operators, output, new_node);

					accepts_right_unary_operator = false;
				}
				else if (is_binary_operator(node_str))
				{
					new_node->type = node_type::binary_operator;
					new_node->value.binary_operator = get_binary_operator(node_str.at(0));

					insert_operator(operators, output, new_node);

					accepts_right_unary_operator = true;
				}
				else if (node_str[0] == '(')
				{
					push_vector_or_parenthesis();
					accepts_right_unary_operator = true;
				}
				else if (node_str[0] == ')')
				{
					close_parenthesis();
					accepts_right_unary_operator = false;
				}
				else if (node_str[0] == ',')
				{
					handle_comma();
					accepts_right_unary_operator = true;
				}
				else if (node_str[0] == '.')
				{
					new_node->type = node_type::vector_component_access;

					get_spaces(source, iterator);
					auto components = get_string_ref(source, iterator);

					if (components.size() > 4)
						matl_throw(exception_type::vector_too_long, {});

					for (size_t i = 0; i < components.size(); i++)
					{
						auto itr = builtins::vector_components_names.find(components.at(i));
						if (itr == builtins::vector_components_names.end())
							matl_throw(exception_type::invalid_vector_component, { components, std::string{components.at(i)}});
						new_node->value.included_vector_components.push_back(itr->second);
					}

					accepts_right_unary_operator = false;
					operators.push_back(new_node);
				}
				else if (is_function_call(source, iterator))
				{
					int args_ammount = get_comas_inside_parenthesis(source, iterator - 1);

					if (args_ammount != 0)
						args_ammount++;

					new_node->type = node_type::function;
					new_node->value.function_name_and_passed_args = {
						std::move(node_str),
						args_ammount
					};

					insert_operator(operators, output, new_node);
					iterator++;	//Jump over the ( char

					accepts_right_unary_operator = true;
				}
				else if (is_scalar_literal(node_str))
				{
					new_node->type = node_type::scalar_literal;
					new_node->value.scalar_value = node_str;
					output.push_back(new_node);

					accepts_right_unary_operator = false;
				}
				else if (node_str.at(0) == builtins::symbols_prefix)
				{
					/*new_node->type = node_type::symbol;
					//new_node->value.symbol_name = std::move(node_str.substr(1));
					output.push_back(new_node);

					accepts_right_unary_operator = false;*/
					matl_throw(exception_type::unknown_token, { node_str });
				}
				else
				{
					new_node->type = node_type::variable;

					auto itr = state.variables.find(node_str);
					if (itr == state.variables.end())
						matl_throw(exception_type::unknown_token, { node_str });
					
					new_node->value.variable = &itr->second;

					output.push_back(new_node);

					accepts_right_unary_operator = false;
				}

				get_spaces(source, iterator);
			}

			//Push all binary_operators left on the binary_operators stack to the output
			auto itr = operators.rbegin();
			while (itr != operators.rend())
			{
				output.push_back(*itr);
				itr++;
			}
		}
	}

	math::expression* get_expression(const std::string& source, size_t& iterator, parsing_state& state)
	{
		math::expression::node* new_node = nullptr;

		std::list<math::expression::node*> output;
		std::list<math::expression::node*> operators;

		try
		{
			expressions_parsing_internal::shunting_yard(state, new_node, output, operators, source, iterator);
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

		auto* exp = new math::expression;
		exp->nodes = std::move(output);
		return exp;
	}

	const math::data_type* validate_expression(const math::expression* exp)
	{
		return nullptr;
	}
}

//CONTEXT MEMBERS IMPLEMENTATION
namespace matl_internal
{
	namespace domain
	{
		struct parsed_domain;
	}

	struct context_implementation
	{
		matl::file_request_callback library_rc = nullptr;

		matl_internal::heterogeneous_map<
			std::string,
			matl_internal::domain::parsed_domain*,
			matl_internal::hgm_solver
		> domains;

		~context_implementation();
	};

}
namespace matl
{
	struct context::implementation
	{
		matl_internal::context_implementation impl;
	};
}

//DOMAIN PARSING
void matl::parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context)
{
	matl_internal::domain::parsing_state state;
	state.domain = new matl_internal::domain::parsed_domain;

	auto& source = domain_source;
	auto& iterator = state.iterator;

	size_t last_position = 0;

	while (true)
	{
		matl_internal::get_to_char('<', domain_source, iterator);

		if (last_position != iterator)
		{
			state.domain->directives.push_back(
				{
					matl_internal::domain::directive_type::dump_block,
					std::move(domain_source.substr(last_position, iterator - last_position))
				}
			);
		}

		if (matl_internal::is_at_source_end(source, iterator))
			break;

		iterator++;
		auto directive = matl_internal::get_string_ref(source, iterator);

		auto handle = matl_internal::domain::directives_handles.find(directive);
		if (handle == matl_internal::domain::directives_handles.end())
		{
			matl_internal::get_to_char('>', domain_source, iterator);
		}
		else
		{
			handle->second(source, context, state);

			matl_internal::get_spaces(source, state.iterator);
			if (source.at(state.iterator) != '>')
				matl_internal::matl_throw(
					matl_internal::exception_type::end_of_directive_expected, 
					{}
			);
		}

		matl_internal::get_to_char('>', domain_source, iterator);
		iterator++;

		last_position = iterator;
	}
	
	context->impl->impl.domains.insert({ domain_name, state.domain });
}

//MATERIAL PARSING
namespace matl_internal
{
	void parse_line(const std::string& material_source, context_implementation& context, parsing_state& state);

	using keyword_handle = void(*)(const std::string& material_source, context_implementation& context, parsing_state& state);

	namespace matl_keywords
	{
		void let(const std::string& source, context_implementation& context, parsing_state& state)
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

			auto& var_def = state.variables.insert({ var_name, {} })->second;
			var_def.definition = get_expression(source, iterator, state);
			var_def.return_type = validate_expression(var_def.definition);
		}

		void property(const std::string& source, context_implementation& context, parsing_state& state)
		{
			auto& iterator = state.iterator;

			get_spaces(source, iterator);
			auto property_name = get_string_ref(source, iterator);

			get_spaces(source, iterator);
			auto assign_operator = get_char(source, iterator);

			if (assign_operator != '=')
				matl_throw(exception_type::invalid_syntax, {});

			auto& prop = state.properties.insert({ property_name, {} })->second;
			prop.definition = get_expression(source, iterator, state);;
		}

		void _using(const std::string& source, context_implementation& context, parsing_state& state)
		{
			auto& iterator = state.iterator;

			get_spaces(source, iterator);
			auto target = get_string_ref(source, iterator);

			if (target == "domain")
			{
				get_spaces(source, iterator);
				auto domain_name = get_rest_of_line(source, iterator);

				if (state.domain != nullptr)
					matl_throw(exception_type::domain_already_specified, {});

				auto itr = context.domains.find(domain_name);
				if (itr == context.domains.end())
					matl_throw(exception_type::no_such_domain, { target });

				state.domain = itr->second;
			}
			else
			{
				//Call custom
			}
		}

		void func(const std::string& source, context_implementation& context, parsing_state& state)
		{

		}

		void _return(const std::string& source, context_implementation& context, parsing_state& state)
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
	if (context == nullptr)
	{
		return {};	//Error
	}

	matl_internal::parsing_state state;

	while (!matl_internal::is_at_source_end(material_source, state.iterator))
	{
		matl_internal::parse_line(material_source, context->impl->impl, state);

		if (matl_internal::is_at_source_end(material_source, state.iterator)) break;
		matl_internal::get_to_new_line(material_source, state.iterator);
	}

	if (state.domain == nullptr)//|| state.errors.size() != 0)
	{
		return {}; //Error
	}

	parsed_material result;
	result.sources = { "" };

	for (auto& directive : state.domain->directives)
	{
		using directive_type = matl_internal::domain::directive_type;

		switch (directive.type)
		{
		case directive_type::dump_block:
			result.sources.back() += directive.payload;
			break;
		case directive_type::split:
			result.sources.push_back("");
		}
	}

	return std::move(result);
}
inline void matl_internal::parse_line(const std::string& material_source, context_implementation& context, parsing_state& state)
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

//BUILTINS RELATED FUNCTIONS IMPLEMENTATION
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

//API IMPLEMENTATION
const std::string language_version = "0.1";
std::string matl::get_language_version()
{
	return language_version;
}

//CONTEXT IMPLEMENTATION
namespace matl_internal
{
	context_implementation::~context_implementation()
	{
		for (auto& x : domains)
			delete x.second;
	}
}

namespace matl
{
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
		context = nullptr;
	}

	void context::set_library_request_callback(file_request_callback callback)
	{
		impl->impl.library_rc = callback;
	}
}

#endif // MATL_IMPLEMENTATION
