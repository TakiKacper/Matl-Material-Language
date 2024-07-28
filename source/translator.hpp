#pragma once

struct translator;
struct material_parsing_state;

std::unordered_map<std::string, translator*> translators;

struct translator
{
	using _expression_translator = std::string(*)(
		const expression* const& exp,
		const inlined_variables* inlined,
		const std::vector<function_instance*>& functions_instances,
		const std::unordered_map<const symbol_definition*, size_t> current_symbols_definitions
	);
	const _expression_translator expression_translator;

	using _variables_declarations_translator = std::string(*)(
		const string_view& name,
		const variable_definition* const& var,
		const inlined_variables* inlined,
		const std::vector<function_instance*>& functions_instances,
		const std::unordered_map<const symbol_definition*, size_t> current_symbols_definitions
	);
	const _variables_declarations_translator variables_declarations_translator;

	using _parameters_declarations_translator = std::string(*)(
		const string_view& name,
		const parameter_definition* const& param
	);
	_parameters_declarations_translator parameters_declarations_translator;

	using _function_header_translator = std::string(*)(
		const function_instance* instance
	);
	const _function_header_translator function_header_translator;

	using _function_return_statement_translator = std::string(*)(
		const function_instance* instance,
		const inlined_variables& inlined,
		const std::vector<function_instance*>& functions_instances
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