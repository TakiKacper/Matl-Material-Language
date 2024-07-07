#pragma once

struct data_type;
struct unary_operator;
struct binary_operator;

struct variable_definition;
struct parameter_definition;
struct function_definition;
struct function_instance;
struct symbol_definition;

const data_type* const get_data_type(const string_ref& name);
const unary_operator* const get_unary_operator(const string_ref& symbol);
const binary_operator* const get_binary_operator(const string_ref& symbol);

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

using named_variable = std::pair<std::string, variable_definition>;
using named_parameter = std::pair<std::string, parameter_definition>;
using named_function = std::pair<std::string, function_definition>;

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
		const named_variable* variable;
		const symbol_definition* symbol;
		const named_parameter* parameter;
		const unary_operator* unary_operator;
		const binary_operator* binary_operator;
		std::pair<uint8_t, uint8_t>								vector_constructor_info;				//first is how many nodes build the vector, second is it's real size
		std::vector<uint8_t>									included_vector_components;
		std::pair<std::string, function_definition>* function;
		std::shared_ptr<parsed_library>							library;
		~node_value() {};
	} value;

	~node()
	{
		if (type == node_type::scalar_literal)
			value.scalar_value.~basic_string();
		else if (type == node_type::vector_contructor_operator)
			value.vector_constructor_info.~pair();
		else if (type == node_type::vector_component_access)
			value.included_vector_components.~vector();
	}
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
	unsigned int definition_line = 0;

	~variable_definition() { delete definition; }
};
using variables_collection = heterogeneous_map<std::string, variable_definition, hgm_string_solver>;

extern const data_type* bool_data_type;
extern const data_type* texture_data_type;
struct parameter_definition
{
	const data_type* type;

	std::list<float>	default_value_numeric;
	std::string			default_value_texture;
};
using parameters_collection = heterogeneous_map<std::string, parameter_definition, hgm_string_solver>;

struct function_instance
{
	const function_definition* function;

	bool valid;
	const data_type* returned_type;
	std::vector<const data_type*> arguments_types;
	std::vector<const data_type*> variables_types;

	bool args_matching(const std::vector<const data_type*>& args) const
	{
		for (size_t i = 0; i < arguments_types.size(); i++)
			if (args.at(i) != arguments_types.at(i))
				return false;
		return true;
	}

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

	~property_definition() { delete definition; }
};