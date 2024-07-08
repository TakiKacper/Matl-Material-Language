#pragma once

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
