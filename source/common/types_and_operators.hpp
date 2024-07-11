#pragma once

struct data_type;
struct unary_operator_definition;
struct binary_operator_definition;

struct variable_definition;
struct parameter_definition;
struct function_definition;
struct function_instance;
struct symbol_definition;

const data_type* const get_data_type(const string_ref& name);
const unary_operator_definition* const get_unary_operator(const string_ref& symbol);
const binary_operator_definition* const get_binary_operator(const string_ref& symbol);

struct data_type
{
	std::string name;
	data_type(std::string _name)
		: name(_name) {};
};

struct unary_operator_definition
{
	std::string symbol;
	uint8_t precedence;
	std::string operation_display_name;

	struct valid_types_set;
	std::vector<valid_types_set> allowed_types;

	unary_operator_definition(
		std::string _symbol,
		std::string _operation_display_name,
		uint8_t _precedence,
		std::vector<valid_types_set> _allowed_types)
		: symbol(_symbol), operation_display_name(_operation_display_name),
		precedence(_precedence), allowed_types(_allowed_types) {};
};

struct unary_operator_definition::valid_types_set
{
	const data_type* operand_type;
	const data_type* returned_type;

	valid_types_set(
		std::string _operand_type,
		std::string _returned_type) :
		operand_type(get_data_type(_operand_type)),
		returned_type(get_data_type(_returned_type)) {};
};

struct binary_operator_definition
{
	std::string symbol;
	uint8_t precedence;
	std::string operation_display_name;

	struct valid_types_set;
	std::vector<valid_types_set> allowed_types;

	binary_operator_definition(
		std::string _symbol,
		std::string _operation_display_name,
		uint8_t _precedence,
		std::vector<valid_types_set> _allowed_types)
		: symbol(_symbol), operation_display_name(_operation_display_name),
		precedence(_precedence), allowed_types(_allowed_types) {};
};

struct binary_operator_definition::valid_types_set
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
public:
	enum class node_type
	{
		left_parenthesis,
		scalar_literal,
		variable,
		symbol,
		parameter,
		unary_operator,
		binary_operator,
		vector_contructor_operator,
		vector_component_access_operator,
		function,
		library
	};

private:
	node_type type;
	void* value;

public:
	node(node_type&& _type, void* ptr) : type(std::move(_type)), value(std::move(ptr)) {};

	inline node_type get_type() const noexcept
	{
		return type;
	};

	inline ~node()
	{
		auto type = get_type();

		if (value == nullptr) return;

		switch (type)
		{
		case expression::node::node_type::scalar_literal:
			delete reinterpret_cast<std::string*>(value); break;		
		case expression::node::node_type::vector_contructor_operator:
			delete reinterpret_cast<std::pair<uint8_t, uint8_t>*>(value); break;
		case expression::node::node_type::vector_component_access_operator:
			delete reinterpret_cast<std::vector<uint8_t>*>(value); break;
		case expression::node::node_type::library:
			delete reinterpret_cast<std::shared_ptr<parsed_library>*>(value); break;
		}
	};


	static inline node* new_left_parenthesis()
	{return new node{ node_type::left_parenthesis, nullptr };}

	static inline node* new_scalar_literal(const string_ref& literal)
	{return new node{ node_type::scalar_literal, new std::string(literal) };}

	static inline node* new_variable(const named_variable* variable)
	{return new node{ node_type::variable, const_cast<named_variable*>(variable) };}

	static inline node* new_symbol(const symbol_definition* symbol)
	{return new node{ node_type::symbol, const_cast<symbol_definition*>(symbol) };}

	static inline node* new_parameter(const named_parameter* parameter)
	{return new node{ node_type::parameter, const_cast<named_parameter*>(parameter) };}

	static inline node* new_unary_operator(const unary_operator_definition* operato)
	{return new node{ node_type::unary_operator, const_cast<unary_operator_definition*>(operato) };}

	static inline node* new_binary_operator(const binary_operator_definition* operato)
	{return new node{ node_type::binary_operator, const_cast<binary_operator_definition*>(operato) };}

	static inline node* new_vector_contructor_operator(uint8_t child_nodes, uint8_t vector_size)
	{return new node{ node_type::vector_contructor_operator, new std::pair<uint8_t, uint8_t>{child_nodes, vector_size} };}

	static inline node* new_vector_access_operator(std::vector<uint8_t>& components)
	{return new node{ node_type::vector_component_access_operator, new std::vector<uint8_t>(std::move(components))}; }

	static inline node* new_function(const named_function* function)
	{return new node{ node_type::function, const_cast<named_function*>(function) };}

	static inline node* new_library(const std::shared_ptr<parsed_library> library)
	{return new node{ node_type::library,  new std::shared_ptr<parsed_library>{library} };}


	inline std::string& as_scalar_literal() const
	{return *reinterpret_cast<std::string*>(value);}

	inline named_variable* as_variable() const
	{return reinterpret_cast<named_variable*>(value);}

	inline symbol_definition* as_symbol() const
	{return reinterpret_cast<symbol_definition*>(value);}

	inline named_parameter* as_parameter() const
	{return reinterpret_cast<named_parameter*>(value);}

	inline unary_operator_definition* as_unary_operator() const
	{return reinterpret_cast<unary_operator_definition*>(value);}

	inline binary_operator_definition* as_binary_operator() const
	{return reinterpret_cast<binary_operator_definition*>(value);}

	inline std::pair<uint8_t, uint8_t>& as_vector_contructor_operator() const
	{return *reinterpret_cast<std::pair<uint8_t, uint8_t>*>(value);}

	inline std::vector<uint8_t>& as_vector_access_operator() const
	{return *reinterpret_cast<std::vector<uint8_t>*>(value);}

	inline named_function* as_function() const
	{return reinterpret_cast<named_function*>(value);}

	inline std::shared_ptr<parsed_library>& as_library() const
	{return *reinterpret_cast<std::shared_ptr<parsed_library>*>(value);}
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

struct parsed_library;
struct function_definition
{
	bool valid = true;
	bool is_exposed = false;

	std::string function_code_name;
	std::pair<std::string, std::shared_ptr<parsed_library>>* library = nullptr;

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