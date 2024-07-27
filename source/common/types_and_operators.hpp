#pragma once

struct data_type;
struct unary_operator_definition;
struct binary_operator_definition;

struct variable_definition;
struct parameter_definition;
struct function_definition;
struct function_instance;
struct symbol_definition;

const data_type* const get_data_type(const string_view& name);
const unary_operator_definition* const get_unary_operator(const string_view& symbol);
const binary_operator_definition* const get_binary_operator(const string_view& symbol);

/*
	Matl data type like scalar or vector
	- name : type name
	Concrete data types are definied in source/version/types.hpp
*/
struct data_type
{
	std::string name;
	data_type(std::string _name)
		: name(_name) {};
};

/*
	Matl operator that takes only one operand, like negation (-a)
	symbol : the operator itself (eg. "-" for negation)
	precedence : operator's precedence value
	operation_display_name : name of operation used in error messages
	allowed_types : set types for which the operator is valid, eg. negation is valid for scalar but not texture. Also contains information about the type returned for given operands types
	Concrete operators are definied in source/version/operators.hpp
*/
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

/*
	unary operator valid_types_set
	operand_type : the operand type
	reurned_type : type returned
*/
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

/*
	Matl operator that takes only two operands, like addition (a + b)
	- symbol : the operator itself (eg. "+" for addition)
	- precedence : operator's precedence value
	- operation_display_name : name of operation used in error messages
	- allowed_types : set types for which the operator is valid, eg. addition is valid for scalar + scalar but not for vector2 + vector3. Also contains information about the type returned for given operands types
	Concrete operators are definied in source/version/operators.hpp
*/
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

/*
	unary operator valid_types_set
	- left_operand_type : the left side operand type
	- right_operand_type : the right side operand type
	- reurned_type : type returned
*/
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

//Pair : variable name + variable_definition
using named_variable = std::pair<std::string, variable_definition>;
//Pair : parameter name + parameter_definition
using named_parameter = std::pair<std::string, parameter_definition>;
//Pair : function name + function_definition
using named_function = std::pair<std::string, function_definition>;

/*
	Matl expression is a class for storing mathematical expressions like a + b
	Expressions are stored in reverse polish notation as nodes, splited into exp_cases and single_expression
	node corresponds to a single operator or operand in expresion eg. "a", "b", "+", "function(" etc.
	single_expression is a class that contains nodes for single sequence of operands connected with operators (eg. "c(a + b, d)")
	exp_case is a struct of two single_expression pointers (exp_case own's those objects) that corresponds to an case in conditional expressions
	Simple expressions like a + b are single exp_case with case's condition == nullptr
	Conditional expression are made of many exp_cases where each case is one if/else.
	For else case the condtion is nullptr
	- cases : list of expressions exp_cases
	- used_variables : list of variables used by expression, important when deciding whether to put the variable into the result shader
	- used_functions : list of functions instances used by expressions, important when deciding whether to put the function into the result shader
	Expressions are generated by get_expression(...) definied in source/common/expression_parsing.hpp
	Their type correctness is validated by the validate_expression(...) definied in the same file as get_expression(...)
*/
struct expression
{
	//node corresponds to a single operator or operand in expresion eg. "a", "b", "+", "function(" etc.
	struct node;

	//single_expression is a class that contains nodes for single sequence of operands connected with operators (eg. "c(a + b, d)")
	struct single_expression
	{
		std::list<node*> nodes;
		single_expression(std::list<node*>& _nodes) : nodes(std::move(_nodes)) {};
		~single_expression();
	};

	//exp_case is a struct of two single_expression pointers (exp_case own's those objects) that corresponds to an case in conditional expressions
	struct exp_case
	{
		single_expression* condition;
		single_expression* value;
		exp_case(single_expression* _condition, single_expression* _value) :
			condition(_condition), value(_value) {};
		~exp_case() { delete condition; delete value; };
	};

	//list of expressions exp_cases
	//Simple expressions like a + b are single exp_case with case's condition == nullptr
	//Conditional expression are made of many exp_cases where each case is one if/else.
	std::list<exp_case*> cases;

	//list of variables used by expression, important when deciding whether to put the variable into the result shader
	std::vector<named_variable*> used_variables;

	//list of functions instances used by expressions, important when deciding whether to put the function into the result shader
	std::vector<std::vector<function_instance*>> used_functions;

	expression(
		std::list<exp_case*>& _cases,
		std::vector<named_variable*>& _used_variables,
		std::vector<std::vector<function_instance*>> _used_functions
	) :
		cases(std::move(_cases)),
		used_variables(std::move(_used_variables)),
		used_functions(std::move(_used_functions))
	{};

	~expression() { for (auto& eq : cases) delete eq; };
};

struct parsed_library;

/*
	node corresponds to a single operator or operand in expresion eg. "a", "b", "+", "function(" etc.
	each node is of one of the types from node::node_type
	depending on the type expression may contain diffrent values
	- type : node type
	- value : pointer to the node data / node data itself. Depends on type
*/ 
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

	static inline node* new_scalar_literal(const string_view& literal)
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


	//Returned string is a number like "-3.14" or "2"
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

	//Returned value is a pair : amount of children nodes + amount of constructed vector's components
	inline std::pair<uint8_t, uint8_t>& as_vector_contructor_operator() const
	{return *reinterpret_cast<std::pair<uint8_t, uint8_t>*>(value);}

	//Returned value is an array of used vector constructor (eg. 1, 0, 2 for .yxz or .grb)
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

/*
	variable_definition
	- type : variable type
	- definition_line : line at which variable was definied
	created every time variable is created, stored in material or function
*/
struct variable_definition
{
	const data_type* type;

	expression* value;
	unsigned int definition_line = 0;

	~variable_definition() { delete value; }
};
//map : variable name to variable definition
using variables_collection = heterogeneous_map<std::string, variable_definition, hgm_string_solver>;

extern const data_type* bool_data_type;
extern const data_type* texture_data_type;

/*
	parameter_definition
	- type : parameter type
	- default_value_numeric : if parameter type is scalar/vector contains list of default values (one value for scalar, two for vector2 ...)
	- default_value_texture : the texture name
	created every time parameter is created
*/
struct parameter_definition
{
	const data_type* type;

	std::list<float>	default_value_numeric;
	std::string			default_value_texture;
};
//map : parameter name to parameter definition
using parameters_collection = heterogeneous_map<std::string, parameter_definition, hgm_string_solver>;

/*
	if You think of matl functions as of c++'s templated functions, function_instance is a function template specialisation
	it stores information whether the function is valid for given set of arguments, and what is the returned type of the function for such arguments
	- function : the instantiated function
	- valid : whether the function is valid for given set of arguments (no inner errors)
	- function_native_name : explained below
	- returned_type : function returned value type
	- arguments_types : function's arguments types indeed
	- translated : cached function translation
	functions instances are generated by instatiate_function(...) definied in source/common/expressions_parsing.hpp
*/ 
struct function_instance
{
	const function_definition* function;

	//If function is exposed it's name may be aliased 
	//and this field contains it's real, native name to be 
	//used when translating into target language
	//When function is a normal matl function then this 
	//field have no real meaning
	std::string function_native_name;

	//whether the function is valid for given set of arguments (no inner errors)
	bool valid = false;

	//function returned value type
	const data_type* returned_type = nullptr;

	//function's arguments types indeed
	std::vector<const data_type*> arguments_types;

	//cache translated function for future parse_material calls
	std::string translated;

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

/*
	if You think of matl functions as of c++'s templated functions, function_definition is a function template, specialised with function_instance(s)
	- valid : is function is valid at all (contains return, no inner errors)
	- is_exposed : whether the function is exposed by domain or by context
	- function_name_ptr : pointer to the function name
	- library : pair : library name + library pointer; library that owns the function (nullptr if declared in material)
	- arguments : function's arguments names
	- variables : all variables definied inside function
	- returned_value : the expression after return keyword
*/
struct function_definition
{
	//is function is valid at all (contains return, no inner errors)
	bool valid = true;

	//whether the function is exposed by domain or by context
	bool is_exposed = false;

	//pointer to the function name
	std::string* function_name_ptr;

	//pair : library name + library pointer; library that owns the function (nullptr if declared in material)
	std::pair<std::string, std::shared_ptr<parsed_library>>* library = nullptr;

	//function's arguments names
	std::vector<std::string> arguments;

	//all variables definied inside function
	variables_collection variables;

	//the expression after return keyword
	expression* returned_value;

	std::list<function_instance> instances;

	~function_definition() { delete returned_value; };
};
using function_collection = heterogeneous_map<std::string, function_definition, hgm_string_solver>;

//property value
//value assigned to a property
struct property_value
{
	expression* value;
	~property_value() { delete value; }
};