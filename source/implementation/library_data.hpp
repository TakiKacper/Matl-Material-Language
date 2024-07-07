#pragma once

struct parsed_library
{
	heterogeneous_map<std::string, function_definition, hgm_string_solver> functions;
};
using libraries_collection = heterogeneous_map<std::string, std::shared_ptr<parsed_library>, hgm_string_solver>;