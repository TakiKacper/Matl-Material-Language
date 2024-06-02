#pragma once

//DEFINE MATL_IMPLEMENTATION
//In order to implement matl in given translation unit

#include <string>
#include <vector>


#pragma region API declaration

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
	};

	struct domain_parsing_raport
	{
		bool success = false;
		std::vector<std::string> errors;
	};

	using file_request_callback = file_request_response(*)(const std::string& file_name);
	using custom_using_case_callback = void(std::string args, std::string& error);

	class context;

	context* create_context(std::string target_language);
	void destroy_context(context*&);

	parsed_material parse_material(const std::string& material_source, matl::context* context);
	domain_parsing_raport parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);
}

class matl::context 
{
private:
	struct implementation;
	implementation* impl;
	friend parsed_material parse_material(const std::string& material_source, matl::context* context);
	friend domain_parsing_raport parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);

public:
	void set_library_request_callback(file_request_callback callback);
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


#include <list>
#include <unordered_map>
#include <stdexcept>

#pragma region String Ref

struct string_ref
{
friend bool operator==(const string_ref& ref, const std::string& other);

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

	const char& at(size_t id) const
	{
		return source.at(begin + id);
	}
};

inline bool operator==(const string_ref& ref, const std::string& other)
{
	if (ref.size() != other.size()) return false;

	for (size_t i = 0; i < other.size(); i++)
		if (ref.source.at(i + ref.begin) != other.at(i))
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
private:
	using _record = std::pair<_key, _value>;
	using _iterator = typename std::list<_record>::iterator;
	using _const_iterator = typename std::list<_record>::const_iterator;

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

	inline _const_iterator begin() const
	{
		return records.begin();
	}

	inline _const_iterator end() const
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
	inline const _value& at(const _key2& key) const
	{
		for (_const_iterator itr = begin(); itr != end(); itr++)
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

	template<class _key2>
	inline _const_iterator find(const _key2& key) const
	{
		for (_const_iterator itr = begin(); itr != end(); itr++)
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

#pragma endregion

#pragma region String traversion

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

#pragma endregion

#pragma region Matl types

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
		std::pair<std::string, int>								function_name_and_passed_args;
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

#pragma endregion

#pragma region Translators

struct translator;
struct material_parsing_state;

std::unordered_map<std::string, translator*> translators;

struct translator
{
	using expression_translator = std::string(*)(
		const expression* const& exp,
		const material_parsing_state& state
	);
	const expression_translator _expression_translator;

	using variable_declaration_translator = std::string(*)(
		const string_ref& name,
		const variable_definition* const& var,
		const material_parsing_state& state
	);
	const variable_declaration_translator _variable_declaration_translator;

	translator(
		std::string _language_name,
		expression_translator __expression_translator,
		variable_declaration_translator __variable_declaration_translator
	) : _expression_translator(__expression_translator),
		_variable_declaration_translator(__variable_declaration_translator)
	{
		translators.insert({ _language_name, this });
	};
};

#pragma endregion

#pragma region Context implementation

struct context_public_implementation
{
	matl::file_request_callback library_rc = nullptr;

	heterogeneous_map<std::string, parsed_domain*, hgm_solver> domains;
	heterogeneous_map<std::string, matl::custom_using_case_callback*, hgm_solver> custom_using_cases;

	translator* translator;

	~context_public_implementation();
};

context_public_implementation::~context_public_implementation()
{
	for (auto& domain : domains)
		delete domain.second;
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

heterogeneous_map<std::string, directive_handle, hgm_solver> directives_handles_lookup =
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
		auto handle = directives_handles_lookup.find(directive);
		if (handle == directives_handles_lookup.end())
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

#pragma region Material parsing

struct material_parsing_state
{
	size_t iterator = 0;
	int line_counter = 0;
	int this_line_indentation_spaces = 0;

	std::vector<std::string> errors;

	heterogeneous_map<std::string, variable_definition, hgm_solver> variables;
	heterogeneous_map<std::string, property_definition, hgm_solver> properties;

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

heterogeneous_map<std::string, keyword_handle, hgm_solver> keywords_handles =
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
		result.errors = { "Cannot parse material without context" };
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

		state.this_line_indentation_spaces = get_spaces(source, iterator);

		if (is_at_line_end(source, iterator)) goto _parse_material_next_line;
		if (source.at(iterator) == comment_char) goto _parse_material_next_line;

		{
			string_ref keyword = get_string_ref(source, iterator, error);

			if (error != "") goto _parse_material_handle_error;

			auto kh_itr = keywords_handles.find(keyword);

			if (kh_itr == keywords_handles.end())
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
		result.errors = { "Material does not specify the domain" };
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
			result.sources.back() += translator->_expression_translator(
				state.properties.at(directive.payload).definition,
				state
			);
			break;
		case directive_type::dump_variables:
			for (auto& var : state.variables)
				result.sources.back() += translator->_variable_declaration_translator(
					var.first,
					&var.second,
					state
				);
			break;
		case directive_type::dump_functions:
			//TODO
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

void matl::context::set_library_request_callback(file_request_callback callback)
{
	impl->impl.library_rc = callback;
}

void matl::context::add_custom_using_case_callback(std::string _case, matl::custom_using_case_callback callback)
{
	impl->impl.custom_using_cases.insert({ _case, callback });
}

#pragma endregion

#pragma region Expressions parsing utilities

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
		material_parsing_state& state,
		expression::node*& new_node,
		std::list<expression::node*>& output,
		std::list<expression::node*>& operators,
		const std::string& source,
		size_t& iterator,
		std::string& error
	);
	void validate_node(
		expression::node* n, 
		const material_parsing_state& state, 
		std::vector<const data_type*>& types, 
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
	material_parsing_state& state,
	expression::node*& new_node,
	std::list<expression::node*>& output,
	std::list<expression::node*>& operators,
	const std::string& source,
	size_t& iterator,
	std::string& error
)
{
	using node = expression::node;
	using node_type = node::node_type;

	bool accepts_right_unary_operator = true;

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
		auto node_str = get_node_str(source, iterator, error);
		if (error != "") return;

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
		else if (node_str.at(0) == '.')
		{
			new_node->type = node_type::vector_component_access;

			get_spaces(source, iterator);
			auto components = get_string_ref(source, iterator, error);

			if (error != "") return;

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
		else if (is_scalar_literal(node_str, error))
		{
			if (error != "") return;

			new_node->type = node_type::scalar_literal;
			new_node->value.scalar_value = node_str;
			output.push_back(new_node);

			accepts_right_unary_operator = false;
		}
		else if (node_str.at(0) == symbols_prefix)
		{
			new_node->type = node_type::symbol;

			if (state.domain == nullptr)
			{
				error = "Cannot use symbols since the domain is not loaded yet";
				return;
			}

			node_str.begin++;

			auto itr = state.domain->symbols.find(node_str);

			if (itr == state.domain->symbols.end())
			{
				error = "No such symbol: " + std::string(node_str);
				return;
			}

			new_node->value.symbol = &(itr->second);
			output.push_back(new_node);

			accepts_right_unary_operator = false;
		}
		else if (is_function_call(source, iterator))
		{
			int args_ammount = get_comas_inside_parenthesis(source, iterator - 1, error);
			if (error != "") return;

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
		else
		{
			new_node->type = node_type::variable;

			auto itr = state.variables.find(node_str);
			if (itr == state.variables.end())
			{
				error = "No such variable: " + std::string(node_str);
				return;
			}

			new_node->value.variable = &(*itr);

			output.push_back(new_node);

			accepts_right_unary_operator = false;
		}

		get_spaces(source, iterator);
	}

	if (is_at_source_end(source, iterator)) goto _shunting_yard_end;

	{
		size_t iterator_2 = iterator;
		get_to_new_line(source, iterator_2);

		int spaces = get_spaces(source, iterator_2);

		if (is_at_source_end(source, iterator_2)) goto _shunting_yard_end;

		if (spaces > state.this_line_indentation_spaces)
		{
			std::string temp_error;

			size_t iterator_3 = iterator_2;
			auto str = get_string_ref(source, iterator_3, temp_error);

			if (temp_error != "" || keywords_handles.find(str) != keywords_handles.end())
				goto _shunting_yard_end;
			
			state.line_counter++;
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
}

inline void expressions_parsing_utilities::validate_node(
	expression::node* n,
	const material_parsing_state& state, 
	std::vector<const data_type*>& types,
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
		types.push_back(var->second.return_type);
		break;
	}
	case node::node_type::symbol:
	{
		if (state.domain == nullptr)
		{
			error = "Cannot use symbols since the domain is not loaded yet";
			return;
		}

		types.push_back(n->value.symbol->type);
		break;
	}
	case node::node_type::binary_operator:
		handle_binary_operator(n->value.binary_operator); 
		if (error != "") return;
		break;
	case node::node_type::unary_operator:
		handle_unary_operator(n->value.unary_operator); 
		if (error != "") return;
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
		/*const auto& func_name = n->value.function_name_and_passed_args.first;

		function_definition* func_def = nullptr;
		for (auto& func_coll : functions)
		{
			auto itr = func_coll->find(func_name);
			if (itr != func_coll->end())
			{
				func_def = &itr->second;
				break;
			}
		}

		if (func_def == nullptr)
		{
			error_text = "No such function: " + func_name;
			break;
		}

		if (n->value.function_name_and_passed_args.second != func_def->arguments.size())
		{
			error_text = "Function " + func_name
				+ " takes " + std::to_string(func_def->arguments.size())
				+ " arguments, not " + std::to_string(n->value.function_name_and_passed_args.second);
			break;
		}

		std::vector<const data_type*> arguments_types;

		for (size_t i = 0; i < func_def->arguments.size(); i++)
			arguments_types.push_back(get_type(i));

		auto invalid_arguments_error = [&]()
		{
			error_text = "Function " + func_name + " is invalid for arguments: ";
			for (size_t i = arguments_types.size() - 1; i != -1; i--)
			{
				error_text += arguments_types.at(i)->name;
				if (i != 0) error_text += ", ";
			}
		};

		for (auto& func_instance : func_def->instances)
		{
			if (func_instance.match_args(arguments_types))
			{
				if (func_instance.valid)
				{
					pop_types(func_def->arguments.size());
					types.push_back(func_instance.returned_type);
					goto end_of_function_check;
				}

				invalid_arguments_error();
				break;
			}
		}

		{
			std::vector<std::string> instantiating_errors;

			auto& func_instance = instantiate_function(
				func_def,
				arguments_types,
				variables,
				functions,
				instantiating_errors
			);

			if (func_instance.valid)
			{
				pop_types(func_def->arguments.size());
				types.push_back(func_instance.returned_type);
				break;
			}

			invalid_arguments_error();

			for (auto& error : instantiating_errors)
			{
				error_text += "\n\t";
				error_text += error;
			}
		}

	end_of_function_check:
		break;*/
	}
	}

	//if (error_text.size() != 0)
	//	return false;
}

#pragma endregion

#pragma region Expression parsing

expression* get_expression(const std::string& source, size_t& iterator, material_parsing_state& state, std::string& error)
{
	expression::node* new_node = nullptr;

	std::list<expression::node*> output;
	std::list<expression::node*> operators;

	expressions_parsing_utilities::shunting_yard(state, new_node, output, operators, source, iterator, error);

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

const data_type* validate_expression(const expression* exp, const material_parsing_state& state, std::string& error)
{
	using node = expression::node;
	using data_type = data_type;

	std::vector<const data_type*> types;

	if (exp == nullptr) return nullptr;

	for (auto& n : exp->nodes)
		expressions_parsing_utilities::validate_node(n, state, types, error);

	if (error != "") return nullptr;

	return types.back();
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

#pragma region Keywords handles implementations

void material_keywords_handles::let
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto var_name = get_string_ref(source, iterator, error);

	if (error != "") return;

	if (var_name.at(0) == symbols_prefix)
		error = "Cannot declare symbols in material";

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);

	if (assign_operator != '=')
		error = "Expected '='";

	if (error != "") return;

	auto& var_def = state.variables.insert({ var_name, {} })->second;
	var_def.definition = get_expression(source, iterator, state, error);
	var_def.return_type = validate_expression(var_def.definition, state, error);
}

void material_keywords_handles::property
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto property_name = get_string_ref(source, iterator, error);

	if (error != "") return;

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);

	if (assign_operator != '=')
		error = "Expected '='";

	auto& prop = state.properties.insert({ property_name, {} })->second;
	prop.definition = get_expression(source, iterator, state, error);
}

void material_keywords_handles::_using
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto target = get_string_ref(source, iterator, error);

	if (error != "") return;

	if (target == "domain")
	{
		get_spaces(source, iterator);
		auto domain_name = get_rest_of_line(source, iterator);

		if (state.domain != nullptr)
			error = "Domain is already specified";

		auto itr = context.domains.find(domain_name);
		if (itr == context.domains.end())
			error = "No such domain: " + std::string(domain_name);

		if (error != "") return;

		state.domain = itr->second;
	}
	else if (target == "library")
	{
		//Library
	}
	else
	{
		//Call custom
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

}

void material_keywords_handles::_return
	(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{

}

#pragma endregion

#endif