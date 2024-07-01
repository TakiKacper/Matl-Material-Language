#pragma once

//DEFINE MATL_IMPLEMENTATION
//In order to implement parser in this translation unit

#include <string>
#include <list>

#pragma region API declaration

namespace matl
{
	std::string get_language_version();

	struct parsed_material
	{
		bool success = false;
		std::list<std::string> sources;
		std::list<std::string> errors;
	};

	struct domain_parsing_raport
	{
		bool success = false;
		std::list<std::string> errors;
	};

	struct library_parsing_raport
	{
		std::string library_name;
		bool success = false;
		std::list<std::string> errors;
	};

	using custom_using_case_callback = void(std::string args, std::string& error);
	using dynamic_library_parse_request_handle = const std::string*(const std::string& lib_name, std::string& error);

	class context;

	context* create_context(std::string target_language);
	void destroy_context(context*&);

	parsed_material parse_material(const std::string& material_source, matl::context* context);
	std::list<matl::library_parsing_raport> parse_library(const std::string library_name, const std::string& library_source, matl::context* context);
	domain_parsing_raport parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);
}

class matl::context 
{
private:
	struct implementation;
	implementation* impl;
	friend parsed_material matl::parse_material(const std::string& material_source, matl::context* context);
	friend std::list<matl::library_parsing_raport> matl::parse_library(const std::string library_name, const std::string& library_source, matl::context* context);
	friend domain_parsing_raport matl::parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);

public:
	void add_domain_insertion(std::string name, std::string insertion);
	void add_custom_using_case_callback(std::string _case, custom_using_case_callback callback);
	void set_dynamic_library_parse_request_handle(dynamic_library_parse_request_handle handle);

private:
	context();
	~context();

	friend context* matl::create_context(std::string target_language);
	friend void matl::destroy_context(matl::context*& context);
};

#pragma endregion API declaration

#ifdef MATL_IMPLEMENTATION

#define MATL_IMPLEMENTATION_INCLUDED

const std::string language_version = "0.1";

#define rethrow_error() if (error != "") return
#define throw_error(condition, _error) if (condition) { error = _error; return; }

#include <unordered_map>
#include <vector>

#pragma region String Ref

struct string_ref
{
friend bool operator==(const string_ref& ref, const std::string& other);

private:
	const std::string* source;

public:
	size_t begin;
	size_t end;

	string_ref(std::nullptr_t) : source(nullptr), begin(0), end(0) {};

	string_ref(const std::string& _source, size_t _begin, size_t _end)
		: source(&_source), begin(_begin), end(_end) {};

	string_ref(const std::string& _source)
		: source(&_source), begin(0), end(_source.size()) {};

	operator std::string() const
	{
		return source->substr(begin, end - begin);
	}

	void operator= (const string_ref& other)
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

inline bool operator==(const string_ref& ref, const std::string& other)
{
	if (ref.size() != other.size()) return false;

	for (size_t i = 0; i < other.size(); i++)
		if (ref.source->at(i + ref.begin) != other.at(i))
			return false;

	return true;
}

inline bool operator == (const std::string& other, const string_ref& q)
{
	return operator==(q, other);
}

inline bool operator == (const string_ref& q, const string_ref& p)
{
	return q.begin == p.begin && q.end == p.end;
}
#pragma endregion

#pragma region Heterogeneous map

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
		throw std::exception{"Invalid record"};
	}

	template<class _key2>
	inline const _value& at(const _key2& key) const
	{
		for (const_iterator itr = begin(); itr != end(); itr++)
			if (_equality_solver::equal(itr->first, key))
				return itr->second;
		throw std::exception{"Invalid record"};
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

	inline void remove(const _key& key)
	{
		for (const_iterator itr = begin(); itr != end(); itr++)
			if (_equality_solver::equal(itr->first, key))
			{
				records.remove(*itr);
				return;
			}
		throw std::exception{"Invalid record"};
	}
};

struct hgm_string_solver
{
	static inline bool equal(const std::string& parameters_declarations_translator, const std::string& b)
	{
		return parameters_declarations_translator == b;
	}

	static inline bool equal(const std::string& parameters_declarations_translator, const string_ref& b)
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

#pragma endregion

#pragma region Unsorted Set

//set of elements; in case of pushing element that is actually in the set it increase assigne 
template<class _type>
class counting_set
{
	using element_with_counter = std::pair<_type, uint32_t>;
	std::vector<element_with_counter> elements;
	using iterator			= typename std::vector<element_with_counter>::iterator;
	using reverse_iterator	= typename std::vector<element_with_counter>::reverse_iterator;

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

#pragma region String traversion

inline bool is_digit(const char& c)
{
	return (c >= '0' && c <= '9');
}

//does not check if char is a matl operator
//insted checks if char is one of the characters that should end a string_ref
inline bool is_operator(const char& c)
{
	return (c == '+' || c == '-' || c == '*' || c == '/' || c == '^'|| c == '='
		 || c == '>' || c == '<' || c == '!'
		 || c == ')' || c == '(' || c == '.' || c == ',' 
		 || c == '#' || c == ':'
		 || c == '"' || c == '$' || c == '&' || c == '\'' ||  c == '?'
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

inline string_ref get_string_ref(const std::string& source, size_t& iterator, std::string& error)
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

	return string_ref(source, begin, iterator);
}

extern const char comment_char;

inline string_ref get_rest_of_line(const std::string& source, size_t& iterator)
{
	size_t begin = iterator;

	while (!is_at_line_end(source, iterator) && source.at(iterator) != comment_char)
		iterator++;

	if (source.at(iterator) == comment_char)
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

#pragma endregion

#pragma region Types

struct data_type;
struct unary_operator;
struct binary_operator;

struct variable_definition;
struct parameter_definition;
struct function_definition;
struct function_instance;
struct symbol_definition;

const data_type*	   const get_data_type			(const string_ref& name);
const unary_operator*  const get_unary_operator		(const string_ref& symbol);
const binary_operator* const get_binary_operator	(const string_ref& symbol);

struct data_type
{
	std::string name;
	data_type(std::string _name)
		: name(_name) {};
};

struct unary_operator
{
	std::string symbol;
	uint8_t precedence;
	std::string operation_display_name;

	struct valid_types_set;
	std::vector<valid_types_set> allowed_types;

	unary_operator(
		std::string _symbol,
		std::string _operation_display_name,
		uint8_t _precedence,
		std::vector<valid_types_set> _allowed_types)
		: symbol(_symbol), operation_display_name(_operation_display_name),
		precedence(_precedence), allowed_types(_allowed_types) {};
};

struct unary_operator::valid_types_set
{
	const data_type* operand_type;
	const data_type* returned_type;

	valid_types_set(
		std::string _operand_type,
		std::string _returned_type) :
		operand_type(get_data_type(_operand_type)),
		returned_type(get_data_type(_returned_type)) {};
};

struct binary_operator
{
	std::string symbol;
	uint8_t precedence;
	std::string operation_display_name;

	struct valid_types_set;
	std::vector<valid_types_set> allowed_types;

	binary_operator(
		std::string _symbol,
		std::string _operation_display_name,
		uint8_t _precedence,
		std::vector<valid_types_set> _allowed_types)
		: symbol(_symbol), operation_display_name(_operation_display_name),
		precedence(_precedence), allowed_types(_allowed_types) {};
};

struct binary_operator::valid_types_set
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

using named_variable =	std::pair<std::string, variable_definition>;
using named_parameter = std::pair<std::string, parameter_definition>;
using named_function =	std::pair<std::string, function_definition>;

struct expression
{
	struct node;

	struct single_expression
	{
		std::list<node*> nodes;
		single_expression(std::list<node*>& _nodes) : nodes(std::move(_nodes)) {};
		~single_expression();
	};

	struct equation
	{
		single_expression* condition;
		single_expression* value;
		equation(single_expression* _condition, single_expression* _value) :
			condition(_condition), value(_value) {};
		~equation() { delete condition; delete value; };
	};

	std::list<equation*> equations;

	std::vector<named_variable*> used_variables;
	std::vector<function_instance*> used_functions;

	expression(
		std::list<equation*>& _equations, 
		std::vector<named_variable*>& _used_variables,
		std::vector<function_instance*>& _used_functions
	) : 
		equations(std::move(_equations)), 
		used_variables(std::move(_used_variables)), 
		used_functions(std::move(_used_functions))
	{};

	~expression() { for (auto& eq : equations) delete eq; };
};

struct parsed_library;

struct expression::node
{
	enum class node_type
	{
		single_arg_left_parenthesis,
		scalar_literal,
		variable,
		symbol,
		parameter,
		unary_operator,
		binary_operator,
		vector_contructor_operator,
		vector_component_access,
		function,
		library
	} type;

	union node_value
	{
		std::string												scalar_value;
		const named_variable*									variable;
		const symbol_definition*								symbol;
		const named_parameter*									parameter;
		const unary_operator*									unary_operator;
		const binary_operator*									binary_operator;
		std::pair<uint8_t, uint8_t>								vector_info;				//first is how many nodes build the vector, second is it's real size
		std::vector<uint8_t>									included_vector_components;
		std::pair<std::string, function_definition>*			function;
		parsed_library*											library;
		~node_value() {};
	} value;
};

expression::single_expression::~single_expression()
{
	for (auto& node : nodes) 
		delete node;
};

struct variable_definition
{
	const data_type* type;
	expression* definition;
	~variable_definition() { delete definition; }
};
using variables_collection = heterogeneous_map<std::string, variable_definition, hgm_string_solver>;

extern const data_type* bool_data_type;
extern const data_type* texture_data_type;
struct parameter_definition
{
	const data_type* type;

	std::vector<float>	default_value_numeric;
	std::string			default_value_texture;
};
using parameters_collection = heterogeneous_map<std::string, parameter_definition, hgm_string_solver>;

struct function_instance
{
	const function_definition* function;

	bool valid;
	std::vector<const data_type*> arguments_types;
	std::vector<const data_type*> variables_types;
	const data_type* returned_type;

	bool args_matching(const std::vector<const data_type*>& args) const
	{
		for (int i = 0; i < arguments_types.size(); i++)
			if (args.at(i) != arguments_types.at(i))
				return false;
		return true;
	}

	std::string translated;

	function_instance(function_definition* _function) : function(_function) {};
};

struct function_definition
{
	bool valid = true;

	std::string function_name;
	std::string library_name;

	std::vector<std::string> arguments;
	variables_collection variables;
	expression* returned_value;

	std::list<function_instance> instances;

	~function_definition() { delete returned_value; };
};
using function_collection = heterogeneous_map<std::string, function_definition, hgm_string_solver>;

struct property_definition
{
	expression* definition;
	~property_definition() { delete definition;}
};

#pragma endregion

#pragma region Builtins

const uint8_t logical_operators_precedence_begin = 1;
const uint8_t numeric_operators_precedence_begin = 4;

const std::vector<data_type> data_types
{
	{ "bool", },
	{ "scalar", },
	{ "vector2", },
	{ "vector3", },
	{ "vector4", },
	{ "texture", }
};

//unary operators; symbol, display name, precedence, {input type, output type}
const std::vector<unary_operator> unary_operators
{
	{ "-", "negate", numeric_operators_precedence_begin + 3, {
		{"scalar", "scalar"},
		{"vector2", "vector2"},
		{"vector3", "vector3"},
		{"vector4", "vector4"}
	}},

	{ "not", "negate", logical_operators_precedence_begin + 1, {
		{"bool", "bool"},
	}},
};

//binary operators; symbol, display name, precedence, {input type left, input type right, output type}
const std::vector<binary_operator> binary_operators
{
	{ "and", "conjunct", logical_operators_precedence_begin, {
		{"bool", "bool", "bool"}
	} },

	{ "or", "alternate", logical_operators_precedence_begin, {
		{"bool", "bool", "bool"}
	}},

	{ "xor", "exclusively alternate", logical_operators_precedence_begin, {
		{"bool", "bool", "bool"}
	}},

	{ "==", "compare with ==", logical_operators_precedence_begin + 1, {
		{"scalar", "scalar", "bool"},
		{"bool", "bool", "bool"}
	}},

	{ "!=", "compare with !=", logical_operators_precedence_begin + 1, {
		{"scalar", "scalar", "bool"},
		{"bool", "bool", "bool"}
	}},

	{ "<", "compare with <", logical_operators_precedence_begin + 2, {
		{"scalar", "scalar", "bool"}
	}},

	{ ">", "compare with >", logical_operators_precedence_begin + 2, {
		{"scalar", "scalar", "bool"}
	}},

	{ "<=", "compare with <=", logical_operators_precedence_begin + 2, {
		{"scalar", "scalar", "bool"}
	}},

	{ ">=", "compare with >=", logical_operators_precedence_begin + 2, {
		{"scalar", "scalar", "bool"}
	}},

	{ "+", "add", numeric_operators_precedence_begin, {
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

	{ "-", "substract", numeric_operators_precedence_begin, {
		{"scalar", "scalar", "scalar"},

		{"vector2", "scalar", "vector2"},
		{"vector2", "vector2", "vector2"},

		{"vector3", "scalar", "vector3"},
		{"vector3", "vector3", "vector3"},

		{"vector4", "scalar", "vector4"},
		{"vector4", "vector4", "vector4"}
	} },

	{ "*", "multiply", numeric_operators_precedence_begin + 1, {
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

	{ "/", "divide", numeric_operators_precedence_begin + 1, {
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
const data_type* scalar_data_type = get_data_type({ "scalar" });
const data_type* bool_data_type = get_data_type({ "bool" });
const data_type* texture_data_type = get_data_type({ "texture" });

const char comment_char = '#';
const char symbols_prefix = '$';

const uint8_t functions_precedence = numeric_operators_precedence_begin + 2;

inline bool is_vector(const data_type* type)
{
	if (type == get_data_type({ "vector2" })) return true;
	if (type == get_data_type({ "vector3" })) return true;
	if (type == get_data_type({ "vector4" })) return true;

	return false;
}

inline uint8_t get_vector_size(const data_type* type)
{
	if (type == get_data_type({ "scalar" })) return 1;
	if (type == get_data_type({ "vector2" })) return 2;
	if (type == get_data_type({ "vector3" })) return 3;
	if (type == get_data_type({ "vector4" })) return 4;

	return 0;
}

inline const data_type* get_vector_type_of_size(uint8_t size)
{
	switch (size)
	{
	case 1: return get_data_type({ "scalar" });
	case 2: return get_data_type({ "vector2" });
	case 3: return get_data_type({ "vector3" });
	case 4: return get_data_type({ "vector4" });
	}

	return nullptr;
}

inline const data_type* const get_data_type(const string_ref& name)
{
	for (auto& type : data_types)
		if (type.name == name)
			return &type;
	return nullptr;
}

inline const unary_operator* const get_unary_operator(const string_ref& symbol)
{
	for (auto& op : unary_operators)
		if (op.symbol == symbol)
			return &op;
	return nullptr;
}

inline const binary_operator* const get_binary_operator(const string_ref& symbol)
{
	for (auto& op : binary_operators)
		if (op.symbol == symbol)
			return &op;
	return nullptr;
}

#pragma endregion

#pragma region Translators

//variable and it's equation translated
using inlined_variables = std::unordered_map<const named_variable*, std::string>;

struct translator;
struct material_parsing_state;

std::unordered_map<std::string, translator*> translators;

struct translator
{
	using _expression_translator = std::string(*)(
		const expression* const& exp,
		const inlined_variables* inlined
	);
	const _expression_translator expression_translator;

	using _variables_declarations_translator = std::string(*)(
		const string_ref& name,
		const variable_definition* const& var,
		const inlined_variables* inlined
	);
	const _variables_declarations_translator variables_declarations_translator;

	using _parameters_declarations_translator = std::string(*)(
		const string_ref& name,
		const parameter_definition* const& param
	);
	_parameters_declarations_translator parameters_declarations_translator;

	using _function_header_translator = std::string(*)(
		const function_instance* instance
	);
	const _function_header_translator function_header_translator;

	using _function_return_statement_translator = std::string(*)(
		const function_instance* instance,
		const inlined_variables& inlined
	);
	const _function_return_statement_translator function_return_statement_translator;

	translator(
		std::string								_language_name,
		_expression_translator					__expression_translator,
		_variables_declarations_translator		__variable_declaration_translator,
		_parameters_declarations_translator		__parameters_declarations_translator,
		_function_header_translator				__function_header_translator,
		_function_return_statement_translator	__function_return_statement_translator
	) : 
		expression_translator(__expression_translator),
		variables_declarations_translator(__variable_declaration_translator),
		function_header_translator(__function_header_translator),
		function_return_statement_translator(__function_return_statement_translator),
		parameters_declarations_translator(__parameters_declarations_translator)
	{
		translators.insert({ _language_name, this });
	};
};

#pragma endregion

#pragma region Domain types

enum class directive_type
{
	dump_block,
	dump_insertion,
	dump_variables,
	dump_functions,
	dump_property,
	dump_parameters,
	split
};

struct directive
{
	directive_type type;
	std::vector<std::string> payload;
	directive(directive_type _type, std::vector<std::string> _payload)
		: type(std::move(_type)), payload(std::move(_payload)) {};
};

struct symbol_definition
{
	const data_type* type;
	std::string definition;
	symbol_definition(const data_type* _type, std::string _definition)
		: type(_type), definition(_definition) {};
};

struct parsed_domain
{
	std::vector<directive> directives;

	heterogeneous_map<std::string, const data_type*, hgm_string_solver>  properties;
	heterogeneous_map<std::string, symbol_definition, hgm_string_solver> symbols;
};

#pragma endregion

#pragma region Libraries Parsing Types

struct parsed_library
{
	heterogeneous_map<std::string, function_definition, hgm_string_solver> functions;
};
using libraries_collection = heterogeneous_map<std::string, parsed_library*, hgm_string_solver>;

#pragma endregion

#pragma region Context implementation

struct context_public_implementation
{
	heterogeneous_map<std::string, parsed_domain*, hgm_string_solver> domains;
	heterogeneous_map<std::string, std::string, hgm_string_solver> domain_insertions;

	heterogeneous_map<std::string, parsed_library*, hgm_string_solver> libraries;
	heterogeneous_map<std::string, matl::custom_using_case_callback*, hgm_string_solver> custom_using_handles;

	matl::dynamic_library_parse_request_handle* dlprh = nullptr;
	translator* translator = nullptr;

	~context_public_implementation();
};

context_public_implementation::~context_public_implementation()
{
	for (auto& domain : domains)
		delete domain.second;

	for (auto& library : libraries)
		delete library.second;
}

struct matl::context::implementation
{
	context_public_implementation impl;
};

#pragma endregion

#pragma region Domain parsing

struct domain_parsing_state
{
	size_t iterator = 0;
	parsed_domain* domain;

	std::list<std::string> errors;

	bool expose_scope = false;
	bool dump_properties_depedencies_scope = false;
};

using directive_handle =
	void(*)(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);;

namespace domain_directives_handles
{
	void expose(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void end(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void property(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void symbol(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void dump(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void split(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
}

heterogeneous_map<std::string, directive_handle, hgm_string_solver> directives_handles_map =
{
	{
		{"expose",	 domain_directives_handles::expose},
		{"end",		 domain_directives_handles::end},
		{"property", domain_directives_handles::property},
		{"symbol",	 domain_directives_handles::symbol},
		{"dump",	 domain_directives_handles::dump},
		{"split",	 domain_directives_handles::split}
	}
};

matl::domain_parsing_raport matl::parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context)
{
	auto& context_impl = context->impl->impl;

	domain_parsing_state state;
	state.domain = new parsed_domain;

	auto& source = domain_source;
	auto& iterator = state.iterator;

	size_t last_position = 0;
	int line_counter = 0;

	while (true)
	{
		std::string error;
		bool whitespaces_only = true;

		get_to_char_while_counting_lines('<', domain_source, iterator, line_counter, whitespaces_only);

		if (last_position != iterator && !whitespaces_only)
		{
			if (state.dump_properties_depedencies_scope || state.expose_scope) 
				goto _parse_domain_handle_error;

			state.domain->directives.push_back(
				{
					directive_type::dump_block,
					{ std::move(domain_source.substr(last_position, iterator - last_position)) }
				}
			);
		}

		if (is_at_source_end(source, iterator))
			break;

		iterator++;

		{
		auto directive = get_string_ref(source, iterator, error);
		if (error.size() != 0) goto _parse_domain_handle_error;

		auto handle = directives_handles_map.find(directive);
		if (handle == directives_handles_map.end())
		{
			error = "No such directive: " + std::string(directive);
			get_to_char_while_counting_lines('>', domain_source, iterator, line_counter, whitespaces_only);
			goto _parse_domain_handle_error;
		}
		else
		{
			handle->second(source, context->impl->impl, state, error);
			if (error != "") goto _parse_domain_handle_error;

			get_spaces(source, state.iterator);
			if (source.at(state.iterator) != '>')
			{
				error = "Expected directive end";
				goto _parse_domain_handle_error;
			}
		}

		get_to_char_while_counting_lines('>', domain_source, iterator, line_counter, whitespaces_only);
		iterator++;

		last_position = iterator;
		}

		continue;

	_parse_domain_handle_error:
		state.errors.push_back('[' + std::to_string(line_counter) + "] " + std::move(error));
	}

	domain_parsing_raport raport;
	if (state.errors.size() == 0)
	{
		raport.success = true;
		context_impl.domains.insert({ domain_name, state.domain });
	}
	else
	{
		raport.success = false;
		raport.errors = std::move(state.errors);
	}
	return raport;
}

#pragma endregion

#pragma region Libraries Parsing

struct parsed_libraries_stack
{
private:
	std::vector<string_ref> libraries_stack;
public:
	inline void push_parsed_lib(const std::string& lib_name, std::string& error);
	inline void pop();
	inline bool is_empty() { return libraries_stack.size() == 0; };
};

void parsed_libraries_stack::push_parsed_lib(const std::string& lib_name, std::string& error)
{
	auto gen_error = [&]()
	{
		error = "Circular libraries inclusion: \n";
		for (auto itr = libraries_stack.begin(); itr != libraries_stack.end(); itr++)
		{
			error += '\t' + std::string(*itr);
			if (itr != libraries_stack.end() - 1) error += '\n';
		}

		return error;
	};

	for (auto& lib : libraries_stack)
	{
		if (lib == lib_name)
			gen_error();
		rethrow_error();
	}

	libraries_stack.push_back({ lib_name });
}

void parsed_libraries_stack::pop()
{
	libraries_stack.pop_back();
}

struct library_parsing_state
{
	size_t iterator = 0;
	int line_counter = 0;
	int this_line_indentation_spaces = 0;
	bool function_body = false;

	std::list<std::string> errors;
	parsed_libraries_stack* parsed_libs_stack = nullptr;
	std::list<matl::library_parsing_raport>* parsing_raports;

	function_collection functions;
	libraries_collection libraries;

	string_ref library_name{ "" };
};


using library_keyword_handle = void(*)(
	const std::string& material_source,
	context_public_implementation& context,
	library_parsing_state& state,
	std::string& error
);

namespace library_keywords_handles
{
	void let(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
	void property(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
	void _using(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
	void func(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
	void _return(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
}

heterogeneous_map<std::string, library_keyword_handle, hgm_string_solver> library_keywords_handles_map =
{
	{
		{"let",			library_keywords_handles::let},
		{"property",	library_keywords_handles::property},
		{"using",		library_keywords_handles::_using},
		{"func",		library_keywords_handles::func},
		{"return",		library_keywords_handles::_return}
	}
};

void parse_library_implementation(
	const std::string library_name, 
	const std::string& library_source, 
	context_public_implementation* context, 
	parsed_libraries_stack* stack,
	std::list<matl::library_parsing_raport>* parsing_raports
)
{
	bool owns_stack = true;

	if (stack == nullptr)
		stack = new parsed_libraries_stack;
	else
		owns_stack = false;

	{
		std::string error;
		stack->push_parsed_lib(library_name, error);
		if (error != "")
		{
			matl::library_parsing_raport raport;
			raport.library_name = library_name;
			raport.success = false;
			raport.errors = { "[0] " + error};
			parsing_raports->push_back(raport);
			return;
		}
	}

	parsed_library* parsed = new parsed_library;
	library_parsing_state state;

	state.parsed_libs_stack = stack;
	state.library_name = library_name;
	state.parsing_raports = parsing_raports;

	while (!is_at_source_end(library_source, state.iterator))
	{
		state.line_counter++;

		std::string error;

		auto& source = library_source;
		auto& iterator = state.iterator;

		int spaces = get_spaces(source, iterator);
		if (is_at_source_end(library_source, state.iterator)) break;

		if (source.at(iterator) == comment_char) goto _parse_library_next_line;
		if (is_at_line_end(source, iterator)) goto _parse_library_next_line;

		if (state.function_body)
		{
			if (spaces == 0 && state.this_line_indentation_spaces == 0)
			{
				error = "Expected function body";
				state.function_body = false;
				goto _parse_library_handle_error;
			}

			if (spaces == 0 && state.this_line_indentation_spaces != 0)
			{
				error = "Unexpected function end";
				state.function_body = false;
				goto _parse_library_handle_error;
			}

			if (spaces != state.this_line_indentation_spaces && state.this_line_indentation_spaces != 0)
			{
				error = "Indentation level must be consistient";
				goto _parse_library_handle_error;
			}
		}
		else if (spaces != 0)
		{
			error = "Indentation level must be consistient";
			goto _parse_library_handle_error;
		}

		state.this_line_indentation_spaces = spaces;

		{
			string_ref keyword = get_string_ref(source, iterator, error);

			if (error != "") goto _parse_library_handle_error;

			auto kh_itr = library_keywords_handles_map.find(keyword);

			if (kh_itr == library_keywords_handles_map.end())
			{
				error = "Unknown keyword: " + std::string(keyword);
				goto _parse_library_handle_error;
			}

			kh_itr->second(library_source, *context, state, error);
			if (error != "") goto _parse_library_handle_error;
		}

	_parse_library_next_line:
		if (is_at_source_end(library_source, state.iterator)) break;
		get_to_new_line(library_source, state.iterator);

		continue;

	_parse_library_handle_error:
		state.errors.push_back('[' + std::to_string(state.line_counter) + "] " + std::move(error));

		if (is_at_source_end(library_source, state.iterator)) break;
		get_to_new_line(library_source, state.iterator);
	}

	parsed->functions = std::move(state.functions);

	matl::library_parsing_raport raport;
	raport.library_name = library_name;

	if (state.errors.size() == 0)
	{
		raport.success = true;
		context->libraries.insert({ library_name, parsed });
	}
	else
	{
		raport.success = false;
		raport.errors = std::move(state.errors);
	}

	if (owns_stack) delete stack; 
	else stack->pop(); 
	
	parsing_raports->push_back(raport); 
	return;
}

std::list<matl::library_parsing_raport> matl::parse_library(const std::string library_name, const std::string& library_source, matl::context* context)
{
	if (context == nullptr)
	{
		matl::library_parsing_raport raport;
		raport.success = false;
		raport.errors = { "[0] Cannot parse library without context" };
		raport.library_name = library_name;
		return { raport };
	}

	std::list<matl::library_parsing_raport> raports;
	parse_library_implementation(library_name, library_source, &context->impl->impl, nullptr, &raports);

	return raports;
}

#pragma endregion

#pragma region Material parsing

struct material_parsing_state
{
	size_t iterator = 0;
	int line_counter = 0;
	int this_line_indentation_spaces = 0;

	bool function_body = false;

	std::list<std::string> errors;

	variables_collection variables;
	parameters_collection parameters;
	function_collection functions;
	libraries_collection libraries;
	heterogeneous_map<std::string, property_definition, hgm_string_solver> properties;

	const parsed_domain* domain = nullptr;
};

using keyword_handle = void(*)(
	const std::string& material_source, 
	context_public_implementation& context, 
	material_parsing_state& state,
	std::string& error
);

namespace material_keywords_handles
{
	void let(const std::string&, context_public_implementation&, material_parsing_state&, std::string&);
	void property(const std::string&, context_public_implementation&, material_parsing_state&, std::string&);
	void _using(const std::string&, context_public_implementation&, material_parsing_state&, std::string&);
	void func(const std::string&, context_public_implementation&, material_parsing_state&, std::string&);
	void _return(const std::string&, context_public_implementation&, material_parsing_state&, std::string&);
}

heterogeneous_map<std::string, keyword_handle, hgm_string_solver> keywords_handles_map =
{
	{
		{"let",			material_keywords_handles::let},
		{"property",	material_keywords_handles::property},
		{"using",		material_keywords_handles::_using},
		{"func",		material_keywords_handles::func},
		{"return",		material_keywords_handles::_return}
	}
};

//recursively get all variables used by expression, used by dump variables
void get_used_variables_recursive(const expression* exp, counting_set<named_variable*>& to_dump)
{
	for (auto& var : exp->used_variables)
	{
		if (var->second.definition != nullptr && to_dump.insert(var) == 1)
			get_used_variables_recursive(var->second.definition, to_dump);
	}	
}

//same but for functions
void get_used_functions_recursive(expression* exp, counting_set<function_instance*>& to_dump)
{
	for (auto& func : exp->used_functions)
		to_dump.insert(func);

	for (auto& func : exp->used_variables)
		get_used_functions_recursive(func->second.definition, to_dump);
}

matl::parsed_material matl::parse_material(const std::string& material_source, matl::context* context)
{
	if (context == nullptr)
	{
		parsed_material returned_value;
		returned_value.success = false;
		returned_value.errors = { "[0] Cannot parse material without context" };
		return returned_value;
	}

	material_parsing_state state;

	auto& context_impl = context->impl->impl;

	while (!is_at_source_end(material_source, state.iterator))
	{
		state.line_counter++;

		std::string error;

		auto& source = material_source;
		auto& iterator = state.iterator;

		int spaces = get_spaces(source, iterator);
		if (is_at_source_end(material_source, state.iterator)) break;

		if (source.at(iterator) == comment_char) goto _parse_material_next_line;
		if (is_at_line_end(source, iterator)) goto _parse_material_next_line;

		if (state.function_body)
		{
			if (spaces == 0 && state.this_line_indentation_spaces == 0)
			{
				error = "Expected function body";
				state.function_body = false;
				goto _parse_material_handle_error;
			}

			if (spaces == 0 && state.this_line_indentation_spaces != 0)
			{
				error = "Unexpected function end";
				state.function_body = false;
				goto _parse_material_handle_error;
			}
			
			if (spaces != state.this_line_indentation_spaces && state.this_line_indentation_spaces != 0)
			{
				error = "Indentation level must be consistient";
				goto _parse_material_handle_error;
			}
		}
		else if (spaces != 0)
		{
			error = "Indentation level must be consistient";
			goto _parse_material_handle_error;
		}

		state.this_line_indentation_spaces = spaces;

		{
			string_ref keyword = get_string_ref(source, iterator, error);

			if (error != "") goto _parse_material_handle_error;

			auto kh_itr = keywords_handles_map.find(keyword);

			if (kh_itr == keywords_handles_map.end())
			{
				error = "Unknown keyword: " + std::string(keyword);
				goto _parse_material_handle_error;
			}
				
			kh_itr->second(material_source, context_impl, state, error);
			if (error != "") goto _parse_material_handle_error;
		}

	_parse_material_next_line:
		if (is_at_source_end(material_source, state.iterator)) break;
		get_to_new_line(material_source, state.iterator);
		
		continue;

	_parse_material_handle_error:
		state.errors.push_back('[' + std::to_string(state.line_counter) + "] " + std::move(error));

		if (is_at_source_end(material_source, state.iterator)) break;
		get_to_new_line(material_source, state.iterator);
	}

	if (state.domain != nullptr)
		for (auto& prop : state.domain->properties)
			if (state.properties.find(prop.first) == state.properties.end())
				state.errors.push_back("[0] Missing property: " + prop.first);

	if (state.errors.size() != 0)
	{
		parsed_material returned_value;
		returned_value.success = false;
		returned_value.errors = std::move(state.errors);
		return returned_value;
	}

	if (state.domain == nullptr)
	{
		parsed_material returned_value;
		returned_value.success = false;
		returned_value.errors = { "[0] Material does not specify the domain" };
		return returned_value;
	}

	parsed_material returned_value;
	returned_value.sources = { "" };

	constexpr auto preallocated_shader_memory = 5 * 1024;

	returned_value.sources.back().reserve(preallocated_shader_memory);

	auto& translator = context->impl->impl.translator;

	std::string invalid_insertion;

	inlined_variables inlined;

	auto dump_property = [&](const directive& directive)
	{
		returned_value.sources.back() += translator->expression_translator(
			state.properties.at(directive.payload.at(0)).definition,
			&inlined
		);
	};

	auto should_inline_variable = [&](const named_variable* var, const uint32_t& uses_count) -> bool
	{
		return
			var->second.definition->equations.size() == 1 &&							//variables does not contain ifs
			(																			//and
				uses_count <= 1 														//variables is used only once 
				||																		//or
				var->second.definition->equations.front()->value->nodes.size() == 1		//it is made of a single node
			);
	};

	auto translate_variables = [&](counting_set<named_variable*>& variables)
	{
		for (auto itr = variables.rbegin(); itr != variables.rend(); itr++)
		{
			if (should_inline_variable(itr->first, itr->second))
			{
				inlined.insert({
					itr->first,
					"(" + translator->expression_translator(itr->first->second.definition, &inlined) + ")"
				});
			}
			else
			{
				returned_value.sources.back() += translator->variables_declarations_translator(
					itr->first->first,
					&itr->first->second,
					&inlined
				);
			}
		};
	};

	auto dump_variables = [&](const directive& directive)
	{
		counting_set<named_variable*> variables;

		for (auto& prop : directive.payload)
		{
			const auto& prop_exp = state.properties.at(prop).definition;
			get_used_variables_recursive(prop_exp, variables);
		}

		translate_variables(variables);
	};

	auto dump_functions = [&](const directive& directive)
	{
		counting_set<function_instance*> functions;

		for (auto& prop : directive.payload)
		{
			const auto& prop_exp = state.properties.at(prop).definition;
			get_used_functions_recursive(prop_exp, functions);
		}

		for (auto itr = functions.begin(); itr != functions.end(); itr++)
		{
			counting_set<named_variable*> variables;
			get_used_variables_recursive(itr->first->function->returned_value, variables);

			returned_value.sources.back() += translator->function_header_translator(itr->first);
			translate_variables(variables);
			returned_value.sources.back() += translator->function_return_statement_translator(itr->first, inlined);
		}
	};

	auto dump_parameters = [&]()
	{
		for (auto& parameter : state.parameters)
			returned_value.sources.back() += translator->parameters_declarations_translator(parameter.first, &parameter.second);
	};

	for (auto& directive : state.domain->directives)
	{
		using directive_type = directive_type;

		switch (directive.type)
		{
		case directive_type::dump_block:
			returned_value.sources.back() += directive.payload.at(0);
			break;
		case directive_type::dump_insertion:
			returned_value.sources.back() += context_impl.domain_insertions.at(directive.payload.at(0));
			break;
		case directive_type::split:
			returned_value.sources.push_back("");
			returned_value.sources.back().reserve(preallocated_shader_memory);
			break;
		case directive_type::dump_property:
			dump_property(directive);
			break;
		case directive_type::dump_variables:
			dump_variables(directive);
			break;
		case directive_type::dump_functions:
			dump_functions(directive);
			break;
		case directive_type::dump_parameters:
			dump_parameters();
			break;
		}
	}

	return returned_value;
}

#pragma endregion

#pragma region API implementation

std::string matl::get_language_version()
{
	return language_version;
}

matl::context::context()
{
	impl = new implementation;
}

matl::context::~context()
{
	delete impl;
}

matl::context* matl::create_context(std::string target_language)
{
	auto itr = translators.find(target_language);
	if (itr == translators.end())
		return nullptr;

	auto c = new context;
	c->impl->impl.translator = itr->second;
	return c;
}

void matl::destroy_context(context*& context)
{
	delete context;
	context = nullptr;
}

void matl::context::add_domain_insertion(std::string name, std::string insertion)
{
	if (insertion == "")
		impl->impl.domain_insertions.remove(name);
	else
		impl->impl.domain_insertions.insert({ std::move(name), std::move(insertion) });
}

void matl::context::add_custom_using_case_callback(std::string _case, matl::custom_using_case_callback callback)
{
	if (callback == nullptr)
		impl->impl.custom_using_handles.remove(_case);
	else
		impl->impl.custom_using_handles.insert({ std::move(_case), callback });
}

void matl::context::set_dynamic_library_parse_request_handle(dynamic_library_parse_request_handle handle)
{
	impl->impl.dlprh = handle;
}

#pragma endregion

#pragma region Expressions parsing utilities

void instantiate_function(
	function_definition& func_def,
	const std::vector<const data_type*> arguments,
	const function_collection* functions,
	decltype(expression::used_functions)& used_functions,
	std::string& error
);

namespace expressions_parsing_utilities
{
	string_ref get_node_str(const std::string& source, size_t& iterator, std::string& error);
	bool is_function_call(const std::string& source, size_t& iterator);
	bool is_scalar_literal(const string_ref& node_str, std::string& error);
	bool is_unary_operator(const string_ref& node_str);
	bool is_binary_operator(const string_ref& node_str);
	int get_comas_inside_parenthesis(const std::string& source, size_t iterator, std::string& error);
	int get_precedence(const expression::node* node);
	bool is_any_of_left_parentheses(const expression::node* node);
	void insert_operator(
		std::list<expression::node*>& operators_stack,
		std::list<expression::node*>& output,
		expression::node* node
	);
	void shunting_yard(
		const std::string& source,
		size_t& iterator,
		int indentation,
		int& lines_counter,
		variables_collection* variables,
		parameters_collection* parameters,
		function_collection* functions,
		libraries_collection* libraries,
		bool can_use_symbols,
		const parsed_domain* domain,
		expression::node*& new_node,
		std::list<expression::equation*>& equations,
		std::list<expression::node*>& output,
		std::list<expression::node*>& operators,
		std::vector<named_variable*>& used_variables,
		std::vector<function_instance*>& used_functions,
		std::string& error
	);
	void validate_node(
		expression* exp,
		expression::node* n,
		const parsed_domain* domain,
		std::vector<const data_type*>& types,
		const function_collection* functions,
		std::string& error
	);
}

inline string_ref expressions_parsing_utilities::get_node_str(const std::string& source, size_t& iterator, std::string& error)
{
	if (source.size() >= iterator + 1)
	{
		if ((
			source.at(iterator) == '!' || source.at(iterator) == '>' || source.at(iterator) == '<' || source.at(iterator) == '='
			)
			&& source.at(iterator + 1) == '='
			)
		{
			iterator += 2;
			return string_ref{ source, iterator - 2, iterator };
		}
	}

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

		if (!is_digit(c) && c != '.') only_digits = false;

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

	return string_ref{ source, begin, iterator };
}

inline bool expressions_parsing_utilities::is_function_call(const std::string& source, size_t& iterator)
{
	get_spaces(source, iterator);

	if (is_at_line_end(source, iterator))
		return false;

	return (source.at(iterator) == '(');
}

inline bool expressions_parsing_utilities::is_scalar_literal(const string_ref& node_str, std::string& error)
{
	int dot_pos = -1;

	for (int i = 0; i < node_str.size(); i++)
	{
		if (node_str.at(i) == '.' && dot_pos == -1)
			dot_pos = i;
		else if (!is_digit(node_str.at(i)))
			return false;
	}

	if (dot_pos == 0 || dot_pos == node_str.size() - 1)
		error = "Invalid scalar literal: " + std::string(node_str);

	return true;
}

inline bool expressions_parsing_utilities::is_unary_operator(const string_ref& node_str)
{
	return get_unary_operator(node_str) != nullptr;
}

inline bool expressions_parsing_utilities::is_binary_operator(const string_ref& node_str)
{
	return get_binary_operator(node_str) != nullptr;
}

inline int expressions_parsing_utilities::get_comas_inside_parenthesis(const std::string& source, size_t iterator, std::string& error)
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
			if (parenthesis_deepness == 0) goto _get_comas_inside_parenthesis_break;
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
		error = "Mismatched parentheses";

	return comas;
}

int expressions_parsing_utilities::get_precedence(const expression::node* node)
{
	switch (node->type)
	{
	case expression::node::node_type::function:
		return functions_precedence;
	case expression::node::node_type::binary_operator:
		return node->value.binary_operator->precedence;
	case expression::node::node_type::unary_operator:
		return node->value.unary_operator->precedence;
	case expression::node::node_type::vector_component_access:
		return 256;
	}

	return 0;
}

bool expressions_parsing_utilities::is_any_of_left_parentheses(const expression::node* node)
{
	return	node->type == expression::node::node_type::vector_contructor_operator ||
		node->type == expression::node::node_type::single_arg_left_parenthesis ||
		node->type == expression::node::node_type::function;
}

void expressions_parsing_utilities::insert_operator(
	std::list<expression::node*>& operators_stack,
	std::list<expression::node*>& output,
	expression::node* node
)
{
	while (operators_stack.size() != 0)
	{
		auto previous = operators_stack.back();
		if (
			previous->type != expression::node::node_type::single_arg_left_parenthesis &&
			previous->type != expression::node::node_type::vector_contructor_operator &&
			previous->type != expression::node::node_type::function &&
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

void expressions_parsing_utilities::shunting_yard(
	const std::string& source,
	size_t& iterator,
	int indentation,
	int& lines_counter,
	variables_collection* variables,
	parameters_collection* parameters,
	function_collection* functions,
	libraries_collection* libraries,
	bool can_use_symbols,
	const parsed_domain* domain,
	expression::node*& new_node,
	std::list<expression::equation*>& equations,
	std::list<expression::node*>& output,
	std::list<expression::node*>& operators,
	std::vector<named_variable*>& used_variables,
	std::vector<function_instance*>& used_functions,
	std::string& error
)
{
	using node = expression::node;
	using node_type = node::node_type;

	auto push_vector_or_parenthesis = [&]()
	{
		int comas = get_comas_inside_parenthesis(source, iterator - 1, error);

		rethrow_error();

		if (comas == 0)
			new_node->type = node_type::single_arg_left_parenthesis;
		else if (comas <= 3)
		{
			new_node->type = node_type::vector_contructor_operator;
			new_node->value.vector_info.first = comas + 1;
		}
		else
			error = "Constructed vector is too long";

		operators.push_back(new_node);
	};

	auto close_parenthesis = [&]()
	{
		bool found_left_parenthesis = false;
		expression::node* previous = nullptr;

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
			error = "Mismatched parentheses";
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

	auto check_expression = [&]()
	{
		//Push all binary_operators left on the binary_operators stack to the output
		auto itr = operators.rbegin();
		while (itr != operators.rend())
		{
			output.push_back(*itr);
			itr++;
		}

		operators.clear();
		new_node = nullptr;

		size_t operands_check_sum = 0;
		for (auto& n : output)
		{
			switch (n->type)
			{
			case node_type::scalar_literal:
			case node_type::symbol:
			case node_type::variable:
			case node_type::parameter:
				operands_check_sum++;
				break;
			case node_type::function:
				throw_error(operands_check_sum < n->value.function->second.arguments.size(), "Invalid expression");
				operands_check_sum -= n->value.function->second.arguments.size();
				operands_check_sum++;
				break;
			case node_type::vector_contructor_operator:
				throw_error(operands_check_sum < n->value.vector_info.first, "Invalid expression");
				operands_check_sum -= n->value.vector_info.first;
				operands_check_sum++;
				break;
			case node_type::binary_operator:
				throw_error(operands_check_sum < 2, "Invalid expression");
				operands_check_sum -= 2;
				operands_check_sum++;
				break;
			case node_type::single_arg_left_parenthesis:
			case node_type::unary_operator:
			case node_type::vector_component_access:
				throw_error(operands_check_sum < 1, "Invalid expression");
				break;
			}
		}

		throw_error(operands_check_sum != 1, "Invalid expression");
	};

	get_spaces(source, iterator);

	bool accepts_right_unary_operator = true;
	bool expecting_library_function = false;

	string_ref library_name = { nullptr };

	bool if_used = false;
	bool else_used = false;	

	expression* condition_buffer = nullptr;

_shunting_yard_loop:
	while (!is_at_line_end(source, iterator))
	{
		new_node = new node{};
		auto node_str = get_node_str(source, iterator, error);
		rethrow_error();

		if (node_str.at(0) == comment_char)
			goto _shunting_yard_end;		//error for multline

		size_t iterator2 = iterator;
		throw_error(expecting_library_function && !is_function_call(source, iterator2), "Expected function call");

		if (node_str == "if")
		{
			throw_error(else_used, "Cannot add if-s after an else statement");
			throw_error(if_used && output.size() == 0, "Invalid Expression");
			throw_error(!if_used && output.size() != 0, "Invalid Expression");

			if (if_used)
			{
				check_expression();
				equations.back()->value = new expression::single_expression(output);
			}

			if_used = true;
			accepts_right_unary_operator = true;
		}
		else if (node_str == "else")
		{
			throw_error(!if_used, "else can be only used after if statement");
			throw_error(output.size() == 0, "Invalid Expression");
			throw_error(else_used, "Cannot specify two else cases");

			check_expression();
			equations.back()->value = new expression::single_expression(output);

			else_used = true;
			accepts_right_unary_operator = true;
		}
		else if (node_str == ":")
		{
			if (!else_used) check_expression();

			throw_error(!if_used, ": can be only used after if or else statements");
			throw_error(output.size() == 0 && !else_used, "Missing condition");
			throw_error(output.size() != 0 && else_used, "Invalid expression");

			if (!else_used)
			{
				equations.push_back(new expression::equation{
					new expression::single_expression(output),
					nullptr
				});
			}
			else
			{
				equations.push_back(new expression::equation{
					nullptr,
					nullptr
				});
			}

			accepts_right_unary_operator = true;
		}
		else if (is_unary_operator(node_str) && accepts_right_unary_operator)
		{
			new_node->type = node_type::unary_operator;
			new_node->value.unary_operator = get_unary_operator(node_str);

			insert_operator(operators, output, new_node);

			accepts_right_unary_operator = false;
		}
		else if (is_binary_operator(node_str))
		{
			new_node->type = node_type::binary_operator;
			new_node->value.binary_operator = get_binary_operator(node_str);

			insert_operator(operators, output, new_node);

			accepts_right_unary_operator = true;
		}
		else if (node_str.at(0) == '(')
		{
			push_vector_or_parenthesis();
			accepts_right_unary_operator = true;
		}
		else if (node_str.at(0) == ')')
		{
			close_parenthesis();
			accepts_right_unary_operator = false;
		}
		else if (node_str.at(0) == ',')
		{
			handle_comma();
			accepts_right_unary_operator = true;
		}
		else if (node_str.at(0) == '.' && output.size() != 0 && output.back()->type == node_type::variable)
		{
			new_node->type = node_type::vector_component_access;

			get_spaces(source, iterator);
			auto components = get_string_ref(source, iterator, error);

			rethrow_error();
			throw_error(components.size() > 4, "Constructed vector is too long: " + std::string(node_str));

			for (size_t i = 0; i < components.size(); i++)
			{
				auto itr = vector_components_names.find(components.at(i));
				throw_error(itr == vector_components_names.end(), error = "No such vector component: " + components.at(i));
				new_node->value.included_vector_components.push_back(itr->second);
			}

			accepts_right_unary_operator = false;
			operators.push_back(new_node);
		}
		else if (node_str.at(0) == '.' && output.size() != 0 && output.back()->type == node_type::library)
		{
			expecting_library_function = true;
			continue;
		}
		else if (node_str.at(0) == '.')
		{
			throw_error(true, "Invalid expression");
		}
		else if (is_scalar_literal(node_str, error))
		{
			rethrow_error();

			new_node->type = node_type::scalar_literal;
			new_node->value.scalar_value = node_str;
			output.push_back(new_node);

			accepts_right_unary_operator = false;
		}
		else if (node_str.at(0) == symbols_prefix)
		{
			new_node->type = node_type::symbol;
			throw_error(!can_use_symbols, "Cannot use symbols here");

			get_spaces(source, iterator);
			auto symbol_name = get_string_ref(source, iterator, error);
			rethrow_error();

			auto itr = domain->symbols.find(symbol_name);
			throw_error(itr == domain->symbols.end(), "No such symbol: " + std::string(symbol_name));

			new_node->value.symbol = &(itr->second);
			output.push_back(new_node);

			accepts_right_unary_operator = false;
		}
		else if (is_function_call(source, iterator))
		{
			int args_ammount = get_comas_inside_parenthesis(source, iterator - 1, error);
			rethrow_error();

			iterator++; //Jump over the ( char
			throw_error(is_at_source_end(source, iterator), "Unexpected file end");

			if (args_ammount != 0)
				args_ammount++;
			else
			{
				get_spaces(source, iterator);
				if (source.at(iterator) != ')')
					args_ammount = 1;
			}

			function_collection::iterator itr;
			if (expecting_library_function)
			{
				expecting_library_function = false;
				itr = output.back()->value.library->functions.find(node_str);
				throw_error(itr == output.back()->value.library->functions.end(), "No such function: " + std::string(node_str));
				throw_error(!itr->second.valid, "Cannot use invalid function: " + std::string(library_name) + "." + std::string(node_str));
				delete output.back();
				output.pop_back();
			}
			else
			{
				itr = functions->find(node_str);
				throw_error(itr == functions->end(), "No such function: " + std::string(node_str));
				throw_error(!itr->second.valid, "Cannot use invalid function: " + std::string(node_str));
			}

			throw_error(itr->second.arguments.size() != args_ammount,
				"Function " + std::string(node_str)
				+ " takes " + std::to_string(itr->second.arguments.size())
				+ " arguments, not " + std::to_string(args_ammount);
			);

			new_node->type = node_type::function;
			new_node->value.function = &(*itr);

			throw_error(new_node->value.function->second.returned_value == nullptr,
				"Cannot use function: " + std::string(node_str) + " because it is missing return statement"
			);

			insert_operator(operators, output, new_node);

			accepts_right_unary_operator = true;
		}
		else
		{
			accepts_right_unary_operator = false;

			//Check if variable
			auto itr = variables->find(node_str);
			if (itr != variables->end())
			{
				new_node->type = node_type::variable;

				new_node->value.variable = &(*itr);
				output.push_back(new_node);

				used_variables.push_back(&(*itr));

				continue;
			}

			if (parameters != nullptr)
			{
				auto itr2 = parameters->find(node_str);
				if (itr2 != parameters->end())
				{
					new_node->type = node_type::parameter;

					new_node->value.parameter = &(*itr2);
					output.push_back(new_node);

					continue;
				}
			}
	
			//Check if library
			auto itr3 = libraries->find(node_str);
			throw_error(itr3 == libraries->end(), "No such variable: " + std::string(node_str));

			library_name = { node_str };

			new_node->type = node_type::library;
			new_node->value.library = itr3->second;

			output.push_back(new_node);
		}

		get_spaces(source, iterator);
	}

	if (is_at_source_end(source, iterator)) goto _shunting_yard_end;

	{
		size_t iterator_2 = iterator;
		get_to_new_line(source, iterator_2);

		int spaces = get_spaces(source, iterator_2);

		if (is_at_source_end(source, iterator_2)) goto _shunting_yard_end;

		if (spaces > indentation)
		{
			std::string temp_error;

			size_t iterator_3 = iterator_2;
			auto str = get_string_ref(source, iterator_3, temp_error);

			if (temp_error != "" || keywords_handles_map.find(str) != keywords_handles_map.end())
				goto _shunting_yard_end;
			
			lines_counter++;
			iterator = iterator_2;
			goto _shunting_yard_loop;	
		}
	}

_shunting_yard_end:
	check_expression();

	throw_error(if_used && !else_used, "Each if statement must go along with an else statement")

	if (equations.size() != 0)
		equations.back()->value = new expression::single_expression(output);
	else
		equations.push_back(new expression::equation{
			nullptr,
			new expression::single_expression(output)
		});
}

inline void expressions_parsing_utilities::validate_node(
	expression* exp,
	expression::node* n,
	const parsed_domain* domain,
	std::vector<const data_type*>& types,
	const function_collection* functions,
	std::string& error
)
{
	using node = expression::node;

	auto get_type = [&](size_t index_from_end) -> const data_type* const&
	{
		return types.at(types.size() - 1 - index_from_end);
	};

	auto pop_types = [&](size_t ammount)
	{
		types.erase(types.end() - ammount, types.end());
	};

	auto handle_binary_operator = [&](const binary_operator* op)
	{
		auto left = get_type(1);
		auto right = get_type(0);

		for (auto& at : op->allowed_types)
		{
			if (at.left_operand_type != left || at.right_operand_type != right) continue;

			pop_types(2);
			types.push_back(at.returned_type);
			return;
		}
		error = "Cannot " + op->operation_display_name + " types: left: " + left->name + " right: " + right->name;
	};

	auto handle_unary_operator = [&](const unary_operator* op)
	{
		auto operand = get_type(0);

		for (auto& at : op->allowed_types)
		{
			if (at.operand_type != operand) continue;

			pop_types(1);
			types.push_back(at.returned_type);
			return;
		}
		std::string error = "Cannot " + op->operation_display_name + " type: " + operand->name;
	};

	switch (n->type)
	{
	case node::node_type::scalar_literal:
		types.push_back(scalar_data_type); break;
	case node::node_type::variable:
	{
		auto& var = n->value.variable;

		throw_error(var->second.type == nullptr, 
			"Cannot use variable " + std::string(var->first) + " since it's type could not be discern");

		types.push_back(var->second.type);
		break;
	}
	case node::node_type::parameter:
	{
		auto& param = n->value.parameter;

		throw_error(param->second.type == nullptr,
			"Cannot use variable " + std::string(param->first) + " since it's type could not be discern");

		types.push_back(param->second.type);
		break;
	}
	case node::node_type::symbol:
	{
		throw_error(domain == nullptr, "Cannot use symbols since the domain is not loaded yet");
		types.push_back(n->value.symbol->type);
		break;
	}
	case node::node_type::binary_operator:
		handle_binary_operator(n->value.binary_operator);
		rethrow_error();
		break;
	case node::node_type::unary_operator:
		handle_unary_operator(n->value.unary_operator);
		rethrow_error();
		break;
	case node::node_type::vector_component_access:
	{
		auto& type = get_type(0);

		throw_error(!is_vector(type), "Cannot swizzle not-vector type");

		size_t vec_size = get_vector_size(type);

		for (auto& comp : n->value.included_vector_components)
			if (comp > vec_size)
			{
				error = type->name + " does not have " + std::to_string(comp) + " dimensions";
				return;
			}

		size_t new_vec_size = n->value.included_vector_components.size();

		pop_types(1);
		types.push_back(get_vector_type_of_size(static_cast<uint8_t>(new_vec_size)));

		break;
	}
	case node::node_type::vector_contructor_operator:
	{
		const auto& vector_constructor_nodes = n->value.vector_info.first;
		auto& created_vector_size = n->value.vector_info.second = 0;

		for (int i = 0; i < vector_constructor_nodes; i++)
		{
			auto& type = get_type(i);
			throw_error(type != scalar_data_type && !is_vector(type), "Created vector must consist of scalars and vectors only")
			created_vector_size += get_vector_size(type);
		}

		throw_error(created_vector_size > 4, "Created vector is too long - maximal vector length is 4, created vector length is " 
			+ std::to_string(created_vector_size));

		pop_types(vector_constructor_nodes);
		types.push_back(get_vector_type_of_size(created_vector_size));
		break;
	}
	case node::node_type::function:
	{
		auto& func_name = n->value.function->first;
		auto& func_def = n->value.function->second;

		std::vector<const data_type*> arguments_types;

		for (size_t i = func_def.arguments.size() - 1; i != -1; i--)
			arguments_types.push_back(get_type(i));

		auto invalid_arguments_error = [&]()
		{
			auto the_error = "Function " + func_name + " is invalid for arguments: ";
			for (size_t i = 0; i < arguments_types.size(); i++)
			{
				the_error += arguments_types.at(i)->name;
				if (i != arguments_types.size() - 1) the_error += ", ";
			}
			return the_error;
		};

		for (auto& func_instance : func_def.instances)
		{
			if (func_instance.args_matching(arguments_types))
			{
				throw_error(!func_instance.valid, invalid_arguments_error());

				pop_types(func_def.arguments.size());
				types.push_back(func_instance.returned_type);

				exp->used_functions.push_back(&func_instance);

				return;
			}
		}

		instantiate_function(
			func_def,
			arguments_types,
			functions,
			exp->used_functions,
			error
		);

		exp->used_functions.push_back(&func_def.instances.back());

		if (error != "")
			error = invalid_arguments_error() + '\n' + error;
		rethrow_error();

		auto& instance = func_def.instances.back();

		pop_types(func_def.arguments.size());
		types.push_back(instance.returned_type);
	}
	default: static_assert(true, "Unhandled node type");
	}
}

#pragma endregion

#pragma region Expression parsing

expression* get_expression(
	const std::string& source, 
	size_t& iterator, 
	const int& indentation,
	int& line_counter,
	variables_collection* variables,
	parameters_collection* parameters,
	function_collection* functions,
	libraries_collection* libraries,
	const parsed_domain* domain,	//optional, nullptr if symbols are not allowed
	std::string& error
)
{
	expression::node* new_node = nullptr;

	std::list<expression::equation*> equations;
	std::list<expression::node*> output;
	std::list<expression::node*> operators;

	std::vector<named_variable*> used_vars;
	std::vector<function_instance*> used_funcs;

	expressions_parsing_utilities::shunting_yard(
		source, 
		iterator, 
		indentation,
		line_counter,
		variables,
		parameters,
		functions,
		libraries,
		domain != nullptr,
		domain,
		new_node,
		equations,
		output,
		operators,
		used_vars,
		used_funcs,
		error
	);

	if (error != "")
	{
		if (new_node != nullptr) delete new_node;

		for (auto& n : output)
			delete n;

		for (auto& n : operators)
			delete n;

		return nullptr;
	}

	return new expression(equations, used_vars, used_funcs);
}

const data_type* validate_expression(
	expression* exp,
	const parsed_domain* domain,
	const function_collection* functions,
	std::string& error
)
{
	using node = expression::node;
	using data_type = data_type;

	std::vector<const data_type*> types;

	auto validate_literal_expression = [&](const expression::single_expression* le) -> const data_type*
	{
		for (auto& n : le->nodes)
		{
			expressions_parsing_utilities::validate_node(exp, n, domain, types, functions, error);
			if (error != "") break;
		}

		if (error != "") return nullptr;

		const data_type* type = types.back();
		types.clear();

		return type;
	};

	if (exp == nullptr) return nullptr;

	const data_type* type = nullptr;

	int counter = 1;
	for (auto& equation : exp->equations)
	{
		const data_type* value_type = validate_literal_expression(equation->value);
		if (error != "") return nullptr;
		if (type == nullptr) type = value_type;
		else if (value_type != type)
		{
			error = "Variable type must be same in all if cases. First expression type: " + type->name + ", " 
				"Expression number " + std::to_string(counter) + " type: " + value_type->name;
			return nullptr;
		}

		if (equation->condition == nullptr)
		{
			counter++;
			continue;
		}

		const data_type* condition_type = validate_literal_expression(equation->condition);
		if (error != "") return nullptr;
		if (condition_type != bool_data_type)
		{
			error = "Conditions must evaluate to bool. Condition number " + std::to_string(counter) + " evaluate to: " + condition_type->name;
			return nullptr;
		}

		counter++;
	}

	return type;
}

void instantiate_function(
	function_definition& func_def,
	const std::vector<const data_type*> arguments,
	const function_collection* functions,
	decltype(expression::used_functions)& used_functions,
	std::string& error
)
{
	auto itr = func_def.variables.begin();
	for (auto arg = arguments.begin(); arg != arguments.end(); arg++)
	{
		itr->second.type = *arg;
		itr++;
	}

	std::vector<const data_type*> variables_types;

	while (itr != func_def.variables.end())
	{
		auto& var = itr->second;

		std::string error2;

		auto type = validate_expression(var.definition, nullptr, functions, error2);
		var.type = type;

		variables_types.push_back(var.type);

		if (error2 != "" && error != "")
			error += '\n';

		if (error2 != "")
		{
			error += '\t';
			error += error2;
		}

		for (auto& func : var.definition->used_functions)
			used_functions.push_back(func);

		itr++;
	}

	std::string error2;
	auto type = validate_expression(func_def.returned_value, nullptr, functions, error2);

	for (auto& func : func_def.returned_value->used_functions)
		used_functions.push_back(func);

	func_def.instances.push_back({ &func_def });
	auto& instance = func_def.instances.back();

	instance.valid = error == "";
	instance.arguments_types = arguments;

	if (!instance.valid) return;

	instance.variables_types = std::move(variables_types);
	instance.returned_type = type;
}

#pragma endregion

#pragma region Directives handles implementations

void domain_directives_handles::expose(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(state.expose_scope, "Cannot use this directive here");
	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");
	state.expose_scope = true;
}

void domain_directives_handles::end(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(!state.expose_scope && !state.dump_properties_depedencies_scope, "Cannot use this directive here");
	state.expose_scope = false;
	state.dump_properties_depedencies_scope = false;
}

void domain_directives_handles::property(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	if (state.expose_scope)
	{
		get_spaces(source, state.iterator);
		auto type_name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		auto type = get_data_type(type_name);
		if (type == nullptr) error = "No such type: " + std::string(type_name);

		state.domain->properties.insert({ name, type });
	}
	else if (state.dump_properties_depedencies_scope)
	{
		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		auto itr = state.domain->properties.find(name);
		throw_error(itr == state.domain->properties.end(), "No such property: " + std::string(name));

		state.domain->directives.back().payload.push_back(name);
	}
	else
	{
		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		auto itr = state.domain->properties.find(name);
		throw_error(itr == state.domain->properties.end(), "No such property: " + std::string(name));

		state.domain->directives.push_back(
			{ directive_type::dump_property, { std::move(name) } }
		);
	}
}

void domain_directives_handles::symbol(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");

	if (state.expose_scope)
	{
		get_spaces(source, state.iterator);
		auto type_name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		auto type = get_data_type(type_name);
		throw_error(type == nullptr, "No such type: " + std::string(type_name));

		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		get_spaces(source, state.iterator);
		if (source.at(state.iterator) != '=')
		{
			error = "Expected '='";
			return;
		}

		state.iterator++;

		throw_error(source.at(state.iterator) == '>', "Expected symbol definition");

		size_t begin = state.iterator;
		get_to_char('>', source, state.iterator);

		state.domain->symbols.insert({ name, {type, source.substr(begin, state.iterator - begin)} });
	}
	else
		error = "Cannot use this directive here";
}

void domain_directives_handles::dump(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(state.expose_scope, "Cannot use this directive here");
	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");

	get_spaces(source, state.iterator);
	auto dump_type = get_string_ref(source, state.iterator, error);
	rethrow_error();

	if (dump_type == "variables")
	{
		state.domain->directives.push_back({ directive_type::dump_variables, {} });
		state.dump_properties_depedencies_scope = true;
	}
	else if (dump_type == "functions")
	{
		state.domain->directives.push_back({ directive_type::dump_functions, {} });
		state.dump_properties_depedencies_scope = true;
	}
	else if (dump_type == "parameters")
	{
		state.domain->directives.push_back({ directive_type::dump_parameters, {} });
	}
	else if (dump_type == "insertion")
	{
		get_spaces(source, state.iterator);
		auto insertion = get_string_ref(source, state.iterator, error);
		rethrow_error();
		throw_error(context.domain_insertions.find(insertion) == context.domain_insertions.end(), "No such insertion: " + std::string(insertion));
		state.domain->directives.push_back({ directive_type::dump_insertion, { insertion } });
	}
	else
		error = "Invalid dump type: " + std::string(dump_type);

	get_to_char('>', source, state.iterator);
}

void domain_directives_handles::split(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(state.expose_scope, "Cannot use this directive here");
	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");

	state.domain->directives.push_back({ directive_type::split, {} });
}

#pragma endregion

#pragma region Handles commons declarations

namespace handles_common
{
	template<class state_class>
	void func(const std::string& source, context_public_implementation& context, state_class& state, std::string& error);

	template<class state_class>
	void _return(const std::string& source, context_public_implementation& context, state_class& state, std::string& error);
}

#pragma endregion

#pragma region Material keywords handles implementations

void material_keywords_handles::let
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto var_name = get_string_ref(source, iterator, error);
	rethrow_error();

	throw_error(var_name.at(0) == symbols_prefix, "Cannot declare symbols in material");

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);

	if (assign_operator != '=')
		error = "Expected '='";

	rethrow_error();

	throw_error(state.functions.find(var_name) != state.functions.end(),
		"Function named " + std::string(var_name) + " already exists");

	if (!state.function_body)
	{
		throw_error(state.variables.find(var_name) != state.variables.end(),
			"Variable named " + std::string(var_name) + " already exists");

		throw_error(state.parameters.find(var_name) != state.parameters.end(),
			"Parameter named " + std::string(var_name) + " already exists");

		auto& var_def = state.variables.insert({ var_name, {} })->second;
		var_def.definition = get_expression(
			source, 
			iterator, 
			state.this_line_indentation_spaces,
			state.line_counter,
			&state.variables,
			&state.parameters,
			&state.functions,
			&state.libraries,
			state.domain,
			error
		);
		rethrow_error();
		var_def.type = validate_expression(var_def.definition, state.domain, &state.functions, error);
		rethrow_error();
	}
	else
	{
		auto& func_def = state.functions.recent().second;

		throw_error(func_def.variables.find(var_name) != func_def.variables.end(),
			"Variable named " + std::string(var_name) + " already exists");

		auto& var_def = func_def.variables.insert({ var_name, {} })->second;
		var_def.definition = get_expression(
			source,
			iterator,
			state.this_line_indentation_spaces,
			state.line_counter,
			&func_def.variables,
			nullptr,
			&state.functions,
			&state.libraries,
			nullptr,
			error
		);
		if (error != "") func_def.valid = false;
		rethrow_error();
	}
}

void material_keywords_handles::property
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	throw_error(state.domain == nullptr, "Cannot use property since the domain has not yet been specified");
	throw_error(state.function_body, "Cannot use property in this scope");

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto property_name = get_string_ref(source, iterator, error);
	rethrow_error();

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);
	throw_error(assign_operator != '=', "Expected '='");

	auto itr = state.domain->properties.find(property_name);
	throw_error(itr == state.domain->properties.end(), "No such property: " + std::string(property_name));

	throw_error(state.properties.find(property_name) != state.properties.end(), 
		"Equation for property " + std::string(property_name) + " is already specified");

	auto& prop = state.properties.insert({ property_name, {} })->second;
	prop.definition = get_expression(
		source,
		iterator,
		state.this_line_indentation_spaces,
		state.line_counter,
		&state.variables,
		&state.parameters,
		&state.functions,
		&state.libraries,
		state.domain,
		error
	);
	rethrow_error();

	auto type = validate_expression(prop.definition, state.domain, &state.functions, error);
	rethrow_error();

	if (itr->second != type)
		error = "Invalid property type; expected: " + itr->second->name + " got: " + type->name;
}

void material_keywords_handles::_using
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	throw_error(state.function_body, "Cannot use using in this scope");

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto using_type = get_string_ref(source, iterator, error);

	rethrow_error();

	if (using_type == "domain")
	{
		throw_error(state.domain != nullptr, "Domain is already specified");

		get_spaces(source, iterator);
		auto domain_name = get_rest_of_line(source, iterator);
		auto itr = context.domains.find(domain_name);

		throw_error(itr == context.domains.end(), "No such domain: " + std::string(domain_name));

		state.domain = itr->second;
	}
	else if (using_type == "library")
	{
		get_spaces(source, iterator);
		auto library_name = get_rest_of_line(source, iterator);

		auto itr = context.libraries.find(library_name);
		throw_error(itr == context.libraries.end(), "No such library: " + std::string(library_name));

		state.libraries.insert({ library_name, itr->second });
	}
	else if (using_type == "parameter")
	{
		get_spaces(source, iterator);
		auto parameter_name = get_string_ref(source, iterator, error);
		rethrow_error();

		get_spaces(source, iterator);
		auto assign_operator = get_char(source, iterator);
		throw_error(assign_operator != '=', "Expected '='");

		throw_error(state.parameters.find(parameter_name) != state.parameters.end(),
			"Parameter named " + std::string(parameter_name) + " already exists");

		throw_error(state.variables.find(parameter_name) != state.variables.end(),
			"Variable named " + std::string(parameter_name) + " already exists");

		state.parameters.insert({ parameter_name, {} });
		auto& param_def = state.parameters.recent().second;

		//Load default value, determine the type
		param_def.type = scalar_data_type;	//TODO
		param_def.default_value_numeric = { 2 };
	}
	else //Call custom case
	{
		auto itr = context.custom_using_handles.find(using_type);
		throw_error(itr == context.custom_using_handles.end(), "No such using case: " + std::string(using_type));

		get_spaces(source, iterator);
		auto arg = get_rest_of_line(source, iterator);;
		itr->second(arg, error);
	}
}

void material_keywords_handles::func
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	handles_common::func(source, context, state, error);
	rethrow_error();

	auto& name = state.functions.recent().first;
	throw_error(state.variables.find(name) != state.variables.end(),
		"Variable named " + std::string(name) + " already exists");

	throw_error(state.parameters.find(name) != state.parameters.end(),
		"Parameter named " + std::string(name) + " already exists");
}

void material_keywords_handles::_return
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	handles_common::_return(source, context, state, error);
}

#pragma endregion

#pragma region Library keywords handles implementations

void library_keywords_handles::let
	(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	if (!state.function_body)
		throw_error(true, "Cannot declare variables in library");

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto var_name = get_string_ref(source, iterator, error);

	rethrow_error();

	if (var_name.at(0) == symbols_prefix)
		error = "Cannot declare symbols in material";

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);

	if (assign_operator != '=')
		error = "Expected '='";

	rethrow_error();

	auto& func_def = state.functions.recent().second;

	throw_error(func_def.variables.find(var_name) != func_def.variables.end(),
		"Variable named " + std::string(var_name) + " already exists");

	throw_error(state.functions.find(var_name) != state.functions.end(),
		"Function named " + std::string(var_name) + " already exists");

	auto& var_def = func_def.variables.insert({ var_name, {} })->second;
	var_def.definition = get_expression(
		source,
		iterator,
		state.this_line_indentation_spaces,
		state.line_counter,
		&func_def.variables,
		nullptr,
		&state.functions,
		&state.libraries,
		nullptr,
		error
	);
	if (error != "") func_def.valid = false;
	rethrow_error();
}

void library_keywords_handles::property
	(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	throw_error(true, "Cannot use property inside library");
}

void library_keywords_handles::_using
	(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	throw_error(state.function_body, "Cannot use using in this scope");

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto using_type = get_string_ref(source, iterator, error);
	rethrow_error();

	if (using_type == "domain")
	{
		throw_error(true, "Cannot use domain inside libraries");
	}
	else if (using_type == "library")
	{
		get_spaces(source, iterator);
		auto library_name = get_rest_of_line(source, iterator);

		auto itr = context.libraries.find(library_name);
		throw_error(itr == context.libraries.end(), "No such library: " + std::string(library_name));

		if (itr == context.libraries.end() && context.dlprh != nullptr)
		{
			error = "";

			const std::string* nested_lib_source = context.dlprh(library_name, error);
			rethrow_error();
			throw_error(nested_lib_source == nullptr, "No such library: " + std::string(library_name));

			parse_library_implementation(library_name, *nested_lib_source, &context, state.parsed_libs_stack, state.parsing_raports);

			throw_error(!state.parsing_raports->back().success, error);

			itr = context.libraries.find(library_name);
		}

		rethrow_error();

		state.libraries.insert({ library_name, itr->second });
	}
	else if (using_type == "parameter")
	{
		throw_error(true, "Cannot use parameters inside libraries");
	}
	else //Call custom case
	{
		auto itr = context.custom_using_handles.find(using_type);
		throw_error(itr == context.custom_using_handles.end(), "No such using case: " + std::string(using_type));

		get_spaces(source, iterator);
		auto arg = get_rest_of_line(source, iterator);;
		itr->second(arg, error);
	}
}

void library_keywords_handles::func
	(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	handles_common::func(source, context, state, error);
	rethrow_error();

	state.functions.recent().second.library_name = state.library_name;
}

void library_keywords_handles::_return
	(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	handles_common::_return(source, context, state, error);
}

#pragma endregion

#pragma region Common Handles

template<class state_class>
void handles_common::func(const std::string& source, context_public_implementation& context, state_class& state, std::string& error)
{
	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	std::string name = get_string_ref(source, iterator, error);
	throw_error(name == "", "Expected function name");

	throw_error(state.functions.find(name) != state.functions.end(),
		"Function named " + std::string(name) + " already exists");

	get_spaces(source, iterator);

	throw_error(is_at_source_end(source, iterator), "");
	throw_error(source.at(iterator) != '(', "Expected (");

	iterator++;

	std::vector<std::string> arguments;

	while (true)
	{
		get_spaces(source, iterator);

		if (source.at(iterator) == ')') break;

		arguments.push_back(get_string_ref(source, iterator, error));
		if (arguments.back() == "") error = "Expected argument name";
		rethrow_error();

		get_spaces(source, iterator);

		if (source.at(iterator) == ')') break;

		throw_error(source.at(iterator) != ',', "Expected comma");

		iterator++;
	}

	iterator++;
	get_spaces(source, iterator);

	throw_error(!is_at_line_end(source, iterator), "Expected line end");

	auto& func_def = state.functions.insert({name, function_definition{}})->second;
	func_def.arguments = std::move(arguments);
	func_def.function_name = std::move(name);

	for (auto& arg : func_def.arguments)
		func_def.variables.insert({ arg, {} });

	state.function_body = true;
}

template<class state_class>
void handles_common::_return(const std::string& source, context_public_implementation& context, state_class& state, std::string& error)
{
	auto& iterator = state.iterator;
	auto& func_def = state.functions.recent().second;

	throw_error(!state.function_body, "Cannot return value from this scope");

	state.function_body = false;

	func_def.returned_value = get_expression(
		source,
		iterator,
		state.this_line_indentation_spaces,
		state.line_counter,
		&func_def.variables,
		nullptr,
		&state.functions,
		&state.libraries,
		nullptr,
		error
	);
	if (error != "") func_def.valid = false;
	rethrow_error();
}

#pragma endregion

#undef check_error
#undef throw_error

#endif