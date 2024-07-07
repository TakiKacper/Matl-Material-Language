#pragma once

//DEFINE MATL_IMPLEMENTATION
//In order to implement parser in this translation unit

#include <string>
#include <list>

#include "source/api.hpp"

#ifdef MATL_IMPLEMENTATION

const std::string language_version = "0.5";

#define MATL_IMPLEMENTATION_INCLUDED

#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "source/common/common.hpp"
#include "source/common/string_traversion.hpp"
#include "source/common/heterogeneous_map.hpp"
#include "source/common/counting_set.hpp"
#include "source/common/types_and_operators.hpp"

#include "source/version/types.hpp"
#include "source/version/operators.hpp"
#include "source/version/special_operators.hpp"

//Quick lookup
const data_type* scalar_data_type = get_data_type({ "scalar" });
const data_type* bool_data_type = get_data_type({ "bool" });
const data_type* texture_data_type = get_data_type({ "texture" });

//variable and it's equation translated
using inlined_variables = std::unordered_map<const named_variable*, std::string>;

#include "source/translator.hpp"

#include "source/implementation/domain_data.hpp"
#include "source/implementation/library_data.hpp"
#include "source/implementation/material_data.hpp"

struct context_public_implementation
{
	heterogeneous_map<std::string, std::shared_ptr<parsed_domain>, hgm_string_solver> domains;
	heterogeneous_map<std::string, std::string, hgm_string_solver> domain_insertions;

	heterogeneous_map<std::string, std::shared_ptr<parsed_library>, hgm_string_solver> libraries;
	heterogeneous_map<std::string, matl::custom_using_case_callback*, hgm_string_solver> custom_using_handles;

	matl::dynamic_library_parse_request_handle* dlprh = nullptr;
	translator* translator = nullptr;
};

struct matl::context::implementation
{
	context_public_implementation impl;
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

#include "source/common/expressions_parsing.hpp"

namespace handles_common
{
	template<class state_class>
	void func(const std::string& source, context_public_implementation& context, state_class& state, std::string& error);

	template<class state_class>
	void _return(const std::string& source, context_public_implementation& context, state_class& state, std::string& error);
}

#include "source/implementation/domain_parsing.hpp"
#include "source/implementation/library_parsing.hpp"
#include "source/implementation/material_parsing.hpp"

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

template<class state_class>
void handles_common::func(const std::string& source, context_public_implementation& context, state_class& state, std::string& error)
{
	throw_error(state.function_body, "Cannot declare function inside another function");

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