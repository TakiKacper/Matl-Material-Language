#pragma once

//DEFINE MATL_IMPLEMENTATION
//In order to implement parser in this translation unit

#include <string>
#include <vector>


#pragma region API declaration

namespace matl
{
	std::string get_language_version();

	struct parsed_material
	{
		bool success = false;
		std::vector<std::string> sources;
		std::vector<std::string> errors;
	};

	struct domain_parsing_raport
	{
		bool success = false;
		std::vector<std::string> errors;
	};

	struct library_parsing_raport
	{
		bool success = false;
		std::vector<std::string> errors;
	};

	using custom_using_case_callback = void(std::string args, std::string& error);

	class context;

	context* create_context(std::string target_language);
	void destroy_context(context*&);

	parsed_material parse_material(const std::string& material_source, matl::context* context);
	library_parsing_raport parse_library(const std::string library_name, const std::string& library_source, matl::context* context);
	domain_parsing_raport parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);
}

class matl::context 
{
private:
	struct implementation;
	implementation* impl;
	friend parsed_material parse_material(const std::string& material_source, matl::context* context);
	friend library_parsing_raport parse_library(const std::string library_name, const std::string& library_source, matl::context* context);
	friend domain_parsing_raport parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);

public:
	void add_custom_using_case_callback(std::string _case, custom_using_case_callback callback);

private:
	context();
	~context();

	friend context* matl::create_context(std::string target_language);
	friend void matl::destroy_context(matl::context*& context);
};

#pragma endregion API declaration


#ifdef MATL_IMPLEMENTATION

const std::string language_version = "0.1";


#define rethrow_error() if (error != "") return
#define throw_error(condition, _error) if (condition) { error = _error; return; } 1

#include <list>
#include <unordered_map>
#include <stdexcept>

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
		throw std::runtime_error{"Invalid record"};
	}

	template<class _key2>
	inline const _value& at(const _key2& key) const
	{
		for (const_iterator itr = begin(); itr != end(); itr++)
			if (_equality_solver::equal(itr->first, key))
				return itr->second;
		throw std::runtime_error{"Invalid record"};
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

#pragma endregion

#pragma region String traversion

inline bool is_operator(const char& c)
{
	return (c == '+' || c == '-' || c == '*' || c == '/' || c == '='
		|| c == '>' || c == '<'
		|| c == ')' || c == '(' || c == '.' || c == ',' 
		|| c == '#');
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
		std::string error = "Expected token not: ";

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
struct function_definition;
struct symbol_definition;

const data_type*	   const get_data_type			(string_ref name);
const unary_operator*  const get_unary_operator		(const char& symbol);
const binary_operator* const get_binary_operator	(const char& symbol);

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

	struct valid_types_set;
	std::vector<valid_types_set> allowed_types;

	unary_operator(
		char _symbol,
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
	char symbol;
	uint8_t precedence;
	std::string operation_display_name;

	struct valid_types_set;
	std::vector<valid_types_set> allowed_types;

	binary_operator(
		char _symbol,
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

struct expression
{
	struct node;

	std::list<node*> nodes;

	~expression();
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
		const std::pair<std::string, variable_definition>*		variable;
		const symbol_definition*								symbol;
		const unary_operator*									unary_operator;
		const binary_operator*									binary_operator;
		uint8_t													vector_size;
		std::vector<uint8_t>									included_vector_components;
		std::pair<std::string, function_definition>*			function;
		parsed_library*											library;
		~node_value() {};
	} value;
};

expression::~expression()
{
	for (auto& n : nodes)
		delete n;
}

struct variable_definition
{
	const data_type* return_type;
	expression* definition;
	~variable_definition() { delete definition; }
};
using variables_collection = heterogeneous_map<std::string, variable_definition, hgm_solver>;

struct function_instance
{
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
};

struct function_definition
{
	bool valid = true;
	std::string library_name;

	std::vector<std::string> arguments;
	variables_collection variables;
	expression* result;

	std::list<function_instance> instances;

	~function_definition() { delete result; };
};
using function_collection = heterogeneous_map<std::string, function_definition, hgm_solver>;

struct property_definition
{
	expression* definition;
	~property_definition() { delete definition;}
};

#pragma endregion

#pragma region Builtins

const std::vector<data_type> data_types
{
	{ "scalar", },
	{ "vector2", },
	{ "vector3", },
	{ "vector4", },
	{ "texture", }
};

//unary operators; symbol, display name, precedence, {input type, output type}
const std::vector<unary_operator> unary_operators
{
	{ '-', "negate", 4, {
		{"scalar", "scalar"},
		{"vector2", "vector2"},
		{"vector3", "vector3"},
		{"vector4", "vector4"}
	}},
};

//binary operators; symbol, display name, precedence, {input type left, input type right, output type}
const std::vector<binary_operator> binary_operators
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

	{ '-', "substract", 1, {
		{"scalar", "scalar", "scalar"},

		{"vector2", "scalar", "vector2"},
		{"vector2", "vector2", "vector2"},

		{"vector3", "scalar", "vector3"},
		{"vector3", "vector3", "vector3"},

		{"vector4", "scalar", "vector4"},
		{"vector4", "vector4", "vector4"}
	} },

	{ '*', "multiply", 2, {
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

	{ '/', "divide", 2, {
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

const char comment_char = '#';
const char symbols_prefix = '$';

const uint8_t functions_precedence = 3;

bool is_vector(const data_type* type)
{
	if (type == get_data_type({ "vector2" })) return true;
	if (type == get_data_type({ "vector3" })) return true;
	if (type == get_data_type({ "vector4" })) return true;

	return false;
}

uint8_t get_vector_size(const data_type* type)
{
	if (type == get_data_type({ "vector2" })) return 2;
	if (type == get_data_type({ "vector3" })) return 3;
	if (type == get_data_type({ "vector4" })) return 4;

	return 0;
}

const data_type* get_vector_type_of_size(uint8_t type)
{
	switch (type)
	{
	case 1: return get_data_type({ "scalar" });
	case 2: return get_data_type({ "vector2" });
	case 3: return get_data_type({ "vector3" });
	case 4: return get_data_type({ "vector4" });
	}

	return nullptr;
}

inline const data_type* const get_data_type(string_ref name)
{
	for (auto& type : data_types)
		if (type.name == name)
			return &type;
	return nullptr;
}

inline const unary_operator* const get_unary_operator(const char& symbol)
{
	for (auto& op : unary_operators)
		if (op.symbol == symbol)
			return &op;
	return nullptr;
}

inline const binary_operator* const get_binary_operator(const char& symbol)
{
	for (auto& op : binary_operators)
		if (op.symbol == symbol)
			return &op;
	return nullptr;
}

#pragma endregion

#pragma region Domain types

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

	heterogeneous_map<std::string, const data_type*, hgm_solver>  properties;
	heterogeneous_map<std::string, symbol_definition, hgm_solver> symbols;
};

struct parsed_library
{
	heterogeneous_map<std::string, function_definition, hgm_solver> functions;
};
using libraries_collection = heterogeneous_map<std::string, parsed_library*, hgm_solver>;

#pragma endregion

#pragma region Translators

struct translator;
struct material_parsing_state;

std::unordered_map<std::string, translator*> translators;

struct used_function_instance_info;

struct translator
{
	using expressions_translator = std::string(*)(
		const expression* const& exp,
		const material_parsing_state& state
	);
	const expressions_translator _expression_translator;

	using variables_declarations_translator = std::string(*)(
		const string_ref& name,
		const variable_definition* const& var,
		const material_parsing_state& state
	);
	const variables_declarations_translator _variable_declaration_translator;

	using functions_translator = std::string(*)(
		const function_instance* instance,
		const used_function_instance_info* info,
		const material_parsing_state& state
	);
	const functions_translator _functions_translator;

	translator(
		std::string _language_name,
		expressions_translator __expression_translator,
		variables_declarations_translator __variable_declaration_translator,
		functions_translator __functions_translator
	) : _expression_translator(__expression_translator),
		_variable_declaration_translator(__variable_declaration_translator),
		_functions_translator(__functions_translator)
	{
		translators.insert({ _language_name, this });
	};
};

#pragma endregion

#pragma region Context implementation

struct context_public_implementation
{
	heterogeneous_map<std::string, parsed_domain*, hgm_solver> domains;
	heterogeneous_map<std::string, parsed_library*, hgm_solver> libraries;
	heterogeneous_map<std::string, matl::custom_using_case_callback*, hgm_solver> custom_using_cases;

	translator* translator;

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

	std::vector<std::string> errors;

	bool expose_closure = false;
};

using directive_handle =
	void(*)(const std::string& material_source, matl::context* context, domain_parsing_state& state, std::string& error);

namespace domain_directives_handles
{
	void expose(const std::string&, matl::context*, domain_parsing_state&, std::string&);
	void end(const std::string&, matl::context*, domain_parsing_state&, std::string&);
	void property(const std::string&, matl::context*, domain_parsing_state&, std::string&);
	void symbol(const std::string&, matl::context*, domain_parsing_state&, std::string&);
	void dump(const std::string&, matl::context*, domain_parsing_state&, std::string&);
	void split(const std::string&, matl::context*, domain_parsing_state&, std::string&);
}

heterogeneous_map<std::string, directive_handle, hgm_solver> directives_handles_map =
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

		line_counter++;

		get_to_char('<', domain_source, iterator);

		if (last_position != iterator)
		{
			state.domain->directives.push_back(
				{
					directive_type::dump_block,
					std::move(domain_source.substr(last_position, iterator - last_position))
				}
			);
		}

		if (is_at_source_end(source, iterator))
			break;

		iterator++;
		auto directive = get_string_ref(source, iterator, error);

		if (error.size() != 0) goto _parse_domain_handle_error;

		{
		auto handle = directives_handles_map.find(directive);
		if (handle == directives_handles_map.end())
		{
			error = "No such directive: " + std::string(directive);
			get_to_char('>', domain_source, iterator);
			goto _parse_domain_handle_error;
		}
		else
		{
			handle->second(source, context, state, error);
			if (error != "") goto _parse_domain_handle_error;

			get_spaces(source, state.iterator);
			if (source.at(state.iterator) != '>')
			{
				error = "Expected directive end";
				goto _parse_domain_handle_error;
			}
		}

		get_to_char('>', domain_source, iterator);
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

struct library_parsing_state
{
	size_t iterator = 0;
	int line_counter = 0;
	int this_line_indentation_spaces = 0;
	bool function_body = false;

	std::vector<std::string> errors;

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

heterogeneous_map<std::string, library_keyword_handle, hgm_solver> library_keywords_handles_map =
{
	{
		{"let",			library_keywords_handles::let},
		{"property",	library_keywords_handles::property},
		{"using",		library_keywords_handles::_using},
		{"func",		library_keywords_handles::func},
		{"return",		library_keywords_handles::_return}
	}
};

matl::library_parsing_raport matl::parse_library(const std::string library_name, const std::string& library_source, matl::context* context)
{
	if (context == nullptr)
	{
		library_parsing_raport result;
		result.success = false;
		result.errors = { "[0] Cannot parse library without context" };
		return result;
	}

	parsed_library* parsed = new parsed_library;
	library_parsing_state state;

	state.library_name = library_name;

	auto& context_impl = context->impl->impl;

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

			kh_itr->second(library_source, context_impl, state, error);
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

	library_parsing_raport raport;

	if (state.errors.size() == 0)
	{
		raport.success = true;
		context->impl->impl.libraries.insert({ library_name, parsed });
	}
	else
	{
		raport.success = false;
		raport.errors = std::move(state.errors);
	}

	return raport;
}

#pragma endregion

#pragma region Material parsing

struct used_function_instance_info
{
	string_ref function_name;
	function_definition* func_def;

	used_function_instance_info(string_ref _function_name, function_definition* _func_def) :
		function_name(_function_name), func_def(_func_def) {};
};
using used_functions_instances = std::unordered_map<function_instance*, used_function_instance_info>;

struct material_parsing_state
{
	size_t iterator = 0;
	int line_counter = 0;
	int this_line_indentation_spaces = 0;

	bool function_body = false;

	std::vector<std::string> errors;

	variables_collection variables;
	function_collection functions;
	libraries_collection libraries;
	heterogeneous_map<std::string, property_definition, hgm_solver> properties;

	used_functions_instances ever_used_functions;

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

heterogeneous_map<std::string, keyword_handle, hgm_solver> keywords_handles_map =
{
	{
		{"let",			material_keywords_handles::let},
		{"property",	material_keywords_handles::property},
		{"using",		material_keywords_handles::_using},
		{"func",		material_keywords_handles::func},
		{"return",		material_keywords_handles::_return}
	}
};

matl::parsed_material matl::parse_material(const std::string& material_source, matl::context* context)
{
	if (context == nullptr)
	{
		parsed_material result;
		result.success = false;
		result.errors = { "[0] Cannot parse material without context" };
		return result;
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
		parsed_material result;
		result.success = false;
		result.errors = std::move(state.errors);
		return result;
	}

	if (state.domain == nullptr)
	{
		parsed_material result;
		result.success = false;
		result.errors = { "[0] Material does not specify the domain" };
		return result;
	}

	parsed_material result;
	result.sources = { "" };

	auto& translator = context->impl->impl.translator;

	for (auto& directive : state.domain->directives)
	{
		using directive_type = directive_type;

		switch (directive.type)
		{
		case directive_type::dump_block:
			result.sources.back() += directive.payload;
			break;
		case directive_type::split:
			result.sources.push_back("");
			break;
		case directive_type::dump_property:
		{
			result.sources.back() += translator->_expression_translator(
				state.properties.at(directive.payload).definition,
				state
			);
			break;
		}
		case directive_type::dump_variables:
			for (auto& var : state.variables)
				result.sources.back() += translator->_variable_declaration_translator(
					var.first,
					&var.second,
					state
				);
			break;
		case directive_type::dump_functions:
			for (auto& func : state.ever_used_functions)
				result.sources.back() += translator->_functions_translator(
					func.first,
					&func.second,
					state
				);
			break;
		}
	}

	return result;
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

void matl::context::add_custom_using_case_callback(std::string _case, matl::custom_using_case_callback callback)
{
	impl->impl.custom_using_cases.insert({ _case, callback });
}

#pragma endregion

#pragma region Expressions parsing utilities

void instantiate_function(
	function_definition& func_def,
	const std::vector<const data_type*> arguments,
	const function_collection* functions,
	used_functions_instances* used_func_instances,
	std::string& error
);

namespace expressions_parsing_utilities
{
	string_ref get_node_str(const std::string& source, size_t& iterator, std::string& error);
	bool is_function_call(const std::string& source, size_t& iterator);
	bool is_scalar_literal(const string_ref& node_str, std::string& error);
	bool is_unary_operator(const std::string& node_str);
	bool is_binary_operator(const std::string& node_str);
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
		function_collection* functions,
		libraries_collection* libraries,
		bool can_use_symbols,
		const parsed_domain* domain,
		expression::node*& new_node,
		std::list<expression::node*>& output,
		std::list<expression::node*>& operators,
		std::string& error
	);
	void validate_node(
		expression::node* n,
		const parsed_domain* domain,
		std::vector<const data_type*>& types,
		const function_collection* functions,
		used_functions_instances* used_func_instances,
		std::string& error
	);
}

string_ref expressions_parsing_utilities::get_node_str(const std::string& source, size_t& iterator, std::string& error)
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

bool expressions_parsing_utilities::is_function_call(const std::string& source, size_t& iterator)
{
	get_spaces(source, iterator);

	if (is_at_line_end(source, iterator))
		return false;

	return (source.at(iterator) == '(');
}

bool expressions_parsing_utilities::is_scalar_literal(const string_ref& node_str, std::string& error)
{
	int dot_pos = -1;

	for (int i = 0; i < node_str.size(); i++)
	{
		if (node_str.at(i) == '.' && dot_pos == -1)
			dot_pos = i;
		else if (!isdigit(node_str.at(i)))
			return false;
	}

	if (dot_pos == 0 || dot_pos == node_str.size() - 1)
		error = "Invalid scalar literal: " + std::string(node_str);

	return true;
}

bool expressions_parsing_utilities::is_unary_operator(const std::string& node_str)
{
	if (node_str.size() != 1) return false;
	return get_unary_operator(node_str.at(0)) != nullptr;
}

bool expressions_parsing_utilities::is_binary_operator(const std::string& node_str)
{
	if (node_str.size() != 1) return false;
	return get_binary_operator(node_str.at(0)) != nullptr;
}

int expressions_parsing_utilities::get_comas_inside_parenthesis(const std::string& source, size_t iterator, std::string& error)
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
	function_collection* functions,
	libraries_collection* libraries,
	bool can_use_symbols,
	const parsed_domain* domain,
	expression::node*& new_node,
	std::list<expression::node*>& output,
	std::list<expression::node*>& operators,
	std::string& error
)
{
	using node = expression::node;
	using node_type = node::node_type;

	bool accepts_right_unary_operator = true;
	bool expecting_library_function = false;
	string_ref library_name = {nullptr};
	
	auto push_vector_or_parenthesis = [&]()
	{
		int comas = get_comas_inside_parenthesis(source, iterator - 1, error);

		if (error != "")
			return;

		if (comas == 0)
			new_node->type = node_type::single_arg_left_parenthesis;
		else if (comas <= 3)
		{
			new_node->type = node_type::vector_contructor_operator;
			new_node->value.vector_size = comas + 1;
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

	get_spaces(source, iterator);

_shunting_yard_loop:
	while (!is_at_line_end(source, iterator))
	{
		new_node = new node{};
		auto node_str = get_node_str(source, iterator, error);
		rethrow_error();

		if (node_str.at(0) == comment_char)
			goto _shunting_yard_end;

		size_t iterator2 = iterator;
		throw_error(expecting_library_function && !is_function_call(source, iterator2), "Expected function call");

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

			if (components.size() > 4)
			{
				error = "Constructed vector is too long: " + std::string(node_str);
				return;
			}			

			for (size_t i = 0; i < components.size(); i++)
			{
				auto itr = vector_components_names.find(components.at(i));
				if (itr == vector_components_names.end())
				{
					error = "No such vector component: " + components.at(i);
					return;
				}
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

			node_str.begin++;

			auto itr = domain->symbols.find(node_str);

			throw_error(itr == domain->symbols.end(), "No such symbol: " + std::string(node_str));

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
				if (get_char(source, iterator) != ')')
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

			throw_error(new_node->value.function->second.result == nullptr,
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

				continue;
			}
	
			//Check if library
			auto itr2 = libraries->find(node_str);
			throw_error(itr2 == libraries->end(), "No such variable: " + std::string(node_str));

			library_name = { node_str };

			new_node->type = node_type::library;
			new_node->value.library = itr2->second;

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
			operands_check_sum++; 
			break;
		case node_type::function:
			throw_error(operands_check_sum < n->value.function->second.arguments.size(), "Invalid expression");
			operands_check_sum -= n->value.function->second.arguments.size();
			operands_check_sum++;
			break;
		case node_type::vector_contructor_operator:
			throw_error(operands_check_sum < n->value.vector_size, "Invalid expression");
			operands_check_sum -= n->value.vector_size;
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
}

inline void expressions_parsing_utilities::validate_node(
	expression::node* n,
	const parsed_domain* domain,
	std::vector<const data_type*>& types,
	const function_collection* functions,
	used_functions_instances* used_func_instances,
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

		throw_error(var->second.return_type == nullptr, 
			"Cannot use variable: " + std::string(var->first) + " since it's type could not be discern");

		types.push_back(var->second.return_type);
		break;
	}
	case node::node_type::symbol:
	{
		if (domain == nullptr)
		{
			error = "Cannot use symbols since the domain is not loaded yet";
			return;
		}

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

		if (!is_vector(type))
		{
			error = "Cannot swizzle not-vector type";
			return;
		}
		else
		{
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
		}

		break;
	}
	case node::node_type::vector_contructor_operator:
	{
		for (int i = 0; i < n->value.vector_size; i++)
		{
			auto& type = get_type(i);
			if (type != scalar_data_type)
			{
				error = "Created vector must consist of scalars only";
				return;
			}
		}

		pop_types(n->value.vector_size);
		types.push_back(get_vector_type_of_size(n->value.vector_size));
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
				if (i != 0) the_error += ", ";
			}
			return the_error;
		};

		for (auto& func_instance : func_def.instances)
		{
			if (func_instance.args_matching(arguments_types))
			{
				if (func_instance.valid)
				{
					pop_types(func_def.arguments.size());
					types.push_back(func_instance.returned_type);

					used_func_instances->insert({ &func_instance, { func_name, &func_def } });
					return;
				}

				throw_error(true, invalid_arguments_error());
			}
		}

		instantiate_function(
			func_def,
			arguments_types,
			functions,
			used_func_instances,
			error
		);

		used_func_instances->insert({ &func_def.instances.back(), { func_name, &func_def } });

		if (error != "")
			error = invalid_arguments_error() + '\n' + error;
		rethrow_error();

		auto& instance = func_def.instances.back();

		pop_types(func_def.arguments.size());
		types.push_back(instance.returned_type);
	}
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
	function_collection* functions,
	libraries_collection* libraries,
	const parsed_domain* domain,	//optional, nullptr if symbols are not allowed
	std::string& error
)
{
	expression::node* new_node = nullptr;

	std::list<expression::node*> output;
	std::list<expression::node*> operators;

	expressions_parsing_utilities::shunting_yard(
		source, 
		iterator, 
		indentation,
		line_counter,
		variables,
		functions,
		libraries,
		domain != nullptr,
		domain,
		new_node,
		output,
		operators,
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

	auto* exp = new expression;
	exp->nodes = std::move(output);
	return exp;
}

const data_type* validate_expression(
	const expression* exp,
	const parsed_domain* domain,
	const function_collection* functions,
	used_functions_instances* used_func_instances,
	std::string& error
)
{
	using node = expression::node;
	using data_type = data_type;

	std::vector<const data_type*> types;

	if (exp == nullptr) return nullptr;

	for (auto& n : exp->nodes)
	{
		expressions_parsing_utilities::validate_node(n, domain, types, functions, used_func_instances, error);
		if (error != "") break;
	}

	if (error != "") return nullptr;

	return types.back();
}

void instantiate_function(
	function_definition& func_def,
	const std::vector<const data_type*> arguments,
	const function_collection* functions,
	used_functions_instances* used_func_instances,
	std::string& error
)
{
	auto itr = func_def.variables.begin();
	for (auto arg = arguments.rbegin(); arg != arguments.rend(); arg++)
	{
		itr->second.return_type = *arg;
		itr++;
	}

	std::vector<const data_type*> variables_types;

	while (itr != func_def.variables.end())
	{
		auto& var = itr->second;

		std::string error2;

		auto type = validate_expression(var.definition, nullptr, functions, used_func_instances, error2);
		var.return_type = type;

		variables_types.push_back(var.return_type);

		if (error2 != "")
		{
			error += '\t';
			error += error2;
			error += '\n';
		}

		itr++;
	}

	std::string error2;
	auto return_type = validate_expression(func_def.result, nullptr, functions, used_func_instances, error2);

	func_def.instances.push_back({});
	auto& instance = func_def.instances.back();

	instance.valid = error == "";

	if (!instance.valid) return;

	instance.arguments_types = arguments;
	instance.variables_types = std::move(variables_types);
	instance.returned_type = return_type;
}

#pragma endregion

#pragma region Directives handles implementations

void domain_directives_handles::expose(const std::string& source, matl::context* context, domain_parsing_state& state, std::string& error)
{
	if (state.expose_closure == true)
		error = "Cannot use this directive here";

	state.expose_closure = true;
}

void domain_directives_handles::end(const std::string& source, matl::context* context, domain_parsing_state& state, std::string& error)
{
	if (state.expose_closure == false)
		error = "Cannot use this directive here";

	state.expose_closure = false;
}

void domain_directives_handles::property(const std::string& source, matl::context* context, domain_parsing_state& state, std::string& error)
{
	if (state.expose_closure)
	{
		get_spaces(source, state.iterator);
		auto type_name = get_string_ref(source, state.iterator, error);

		if (error != "")
			return;

		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);

		if (error != "")
			return;

		auto type = get_data_type(type_name);
		if (type == nullptr) error = "No such type: " + std::string(type_name);

		state.domain->properties.insert({ name, type });
	}
	else
	{
		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);

		if (error != "")
			return;

		auto itr = state.domain->properties.find(name);
		if (itr == state.domain->properties.end())
		{
			error = "No such property: " + std::string(name);
			return;
		}

		state.domain->directives.push_back(
			{ directive_type::dump_property, std::move(name) }
		);
	}
}

void domain_directives_handles::symbol(const std::string& source, matl::context* context, domain_parsing_state& state, std::string& error)
{
	if (state.expose_closure)
	{
		get_spaces(source, state.iterator);
		auto type_name = get_string_ref(source, state.iterator, error);
		if (error != "") return;

		auto type = get_data_type(type_name);
		if (type == nullptr)
		{
			error = "No such type: " + std::string(type_name);
			return;
		}

		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);

		if (error != "") return;

		get_spaces(source, state.iterator);
		if (source.at(state.iterator) != '=')
		{
			error = "Expected '='";
			return;
		}

		state.iterator++;

		if (source.at(state.iterator) == '>')
		{
			error = ("Expected symbol definition");
			return;
		}

		size_t begin = state.iterator;
		get_to_char('>', source, state.iterator);

		state.domain->symbols.insert({ name, {type, source.substr(begin, state.iterator - begin)} });
	}
	else
		error = "Cannot use this directive here";
}

void domain_directives_handles::dump(const std::string& source, matl::context* context, domain_parsing_state& state, std::string& error)
{
	std::string temperr;
	get_spaces(source, state.iterator);
	auto dump_what = get_string_ref(source, state.iterator, temperr);

	if (dump_what == "variables")
		state.domain->directives.push_back(
			{ directive_type::dump_variables, {} }
	);
	else if (dump_what == "functions")
		state.domain->directives.push_back(
			{ directive_type::dump_functions, {} }
	);
	else
		error = "Invalid dump target: " + std::string(dump_what);

	get_to_char('>', source, state.iterator);
}

void domain_directives_handles::split(const std::string& source, matl::context* context, domain_parsing_state& state, std::string& error)
{
	state.domain->directives.push_back(
		{ directive_type::split, {} }
	);
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

	if (var_name.at(0) == symbols_prefix)
		error = "Cannot declare symbols in material";

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);

	if (assign_operator != '=')
		error = "Expected '='";

	rethrow_error();

	auto check_if_already_exists = [&](const variables_collection& collection)
	{
		throw_error(collection.find(var_name) != collection.end(), 
			"Variable " + std::string(var_name) + " already exists");
	};

	if (!state.function_body)
	{
		check_if_already_exists(state.variables);

		auto& var_def = state.variables.insert({ var_name, {} })->second;
		var_def.definition = get_expression(
			source, 
			iterator, 
			state.this_line_indentation_spaces,
			state.line_counter,
			&state.variables,
			&state.functions,
			&state.libraries,
			state.domain,
			error
		);
		rethrow_error();
		var_def.return_type = validate_expression(var_def.definition, state.domain, &state.functions, &state.ever_used_functions, error);
		rethrow_error();
	}
	else
	{
		auto& func_def = state.functions.recent().second;

		check_if_already_exists(func_def.variables);

		auto& var_def = func_def.variables.insert({ var_name, {} })->second;
		var_def.definition = get_expression(
			source,
			iterator,
			state.this_line_indentation_spaces,
			state.line_counter,
			&func_def.variables,
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

	if (state.function_body)
	{
		error = "Cannot use property in this scope";
		return;
	}

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto property_name = get_string_ref(source, iterator, error);

	rethrow_error();

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);

	if (assign_operator != '=')
		error = "Expected '='";

	auto itr = state.domain->properties.find(property_name);
	if (itr == state.domain->properties.end())
	{
		error = "No such property: " + std::string(property_name);
		return;
	}

	auto& prop = state.properties.insert({ property_name, {} })->second;
	prop.definition = get_expression(
		source,
		iterator,
		state.this_line_indentation_spaces,
		state.line_counter,
		&state.variables,
		&state.functions,
		&state.libraries,
		state.domain,
		error
	);
	rethrow_error();

	auto type = validate_expression(prop.definition, state.domain, &state.functions, &state.ever_used_functions, error);
	rethrow_error();

	if (itr->second != type)
		error = "Invalid property type; expected: " + itr->second->name + " recived: " + type->name;
}

void material_keywords_handles::_using
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	if (state.function_body)
	{
		error = "Cannot use using in this scope";
		return;
	}

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto target = get_string_ref(source, iterator, error);

	rethrow_error();

	if (target == "domain")
	{
		get_spaces(source, iterator);
		auto domain_name = get_rest_of_line(source, iterator);

		if (state.domain != nullptr)
			error = "Domain is already specified";

		auto itr = context.domains.find(domain_name);
		if (itr == context.domains.end())
			error = "No such domain: " + std::string(domain_name);

		rethrow_error();

		state.domain = itr->second;
	}
	else if (target == "library")
	{
		get_spaces(source, iterator);
		auto library_name = get_rest_of_line(source, iterator);

		auto itr = context.libraries.find(library_name);
		if (itr == context.libraries.end())
			error = "No such library: " + std::string(library_name);

		rethrow_error();

		state.libraries.insert({ library_name, itr->second });
	}
	else //Call custom case
	{
		auto itr = context.custom_using_cases.find(target);
		if (itr == context.custom_using_cases.end())
		{
			error = "No such using case: " + std::string(target);
			return;
		}
		else
		{
			get_spaces(source, iterator);
			auto arg = get_rest_of_line(source, iterator);;
			itr->second(arg, error);
		}
	}
}

void material_keywords_handles::func
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	handles_common::func(source, context, state, error);
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

	auto check_if_already_exists = [&](const variables_collection& collection)
	{
		throw_error(collection.find(var_name) != collection.end(),
			"Variable " + std::string(var_name) + " already exists");
	};

	auto& func_def = state.functions.recent().second;

	check_if_already_exists(func_def.variables);

	auto& var_def = func_def.variables.insert({ var_name, {} })->second;
	var_def.definition = get_expression(
		source,
		iterator,
		state.this_line_indentation_spaces,
		state.line_counter,
		&func_def.variables,
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
	if (state.function_body)
	{
		error = "Cannot use using in this scope";
		return;
	}

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto target = get_string_ref(source, iterator, error);

	rethrow_error();

	if (target == "domain")
	{
		throw_error(true, "Cannot use domain inside library");
	}
	else if (target == "library")
	{
		throw_error(true, "Cannot use other library inside library yet");
		//Avoid self include and circular include
	}
	else //Call custom case
	{
		auto itr = context.custom_using_cases.find(target);
		if (itr == context.custom_using_cases.end())
		{
			error = "No such using case: " + std::string(target);
			return;
		}
		else
		{
			get_spaces(source, iterator);
			auto arg = get_rest_of_line(source, iterator);;
			itr->second(arg, error);
		}
	}
}

void library_keywords_handles::func
	(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	handles_common::func(source, context, state, error);
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
	if (name == "") error = "Expected function name";
	rethrow_error();

	throw_error(state.functions.find(name) != state.functions.end(),
		"Function " + std::string(name) + " already exists");

	{
		get_spaces(source, iterator);
		if (is_at_source_end(source, iterator))
		{
			error = "";
			return;
		}

		if (source.at(iterator) != '(')
		{
			error = "Expected (";
			return;
		}

		iterator++;
	}

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

	if (!is_at_line_end(source, iterator))
	{
		error = "Expected line end";
		return;
	}

	auto& func_def = state.functions.insert({ name, {} })->second;
	func_def.arguments = std::move(arguments);

	for (auto& arg : func_def.arguments)
		func_def.variables.insert({ arg, {} });

	state.function_body = true;
}

template<class state_class>
void handles_common::_return(const std::string& source, context_public_implementation& context, state_class& state, std::string& error)
{
	auto& iterator = state.iterator;
	auto& func_def = state.functions.recent().second;

	if (!state.function_body)
	{
		error = "Cannot return value from this scope";
		return;
	}

	state.function_body = false;

	func_def.result = get_expression(
		source,
		iterator,
		state.this_line_indentation_spaces,
		state.line_counter,
		&func_def.variables,
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