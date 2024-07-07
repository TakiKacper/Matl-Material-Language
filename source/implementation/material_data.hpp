#pragma once

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

	std::shared_ptr<const parsed_domain> domain = nullptr;
};