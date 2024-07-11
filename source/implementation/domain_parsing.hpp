#pragma once

struct domain_parsing_state
{
	size_t iterator = 0;
	std::shared_ptr<parsed_domain> domain;

	std::list<std::string> errors;

	bool expose_scope = false;
	bool dump_properties_depedencies_scope = false;
};

using directive_handle =
void(*)(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);;

namespace domain_directives_handles
{
	void expose(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void end(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void property(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void symbol(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void function(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void dump(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
	void split(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error);
}

heterogeneous_map<std::string, directive_handle, hgm_string_solver> directives_handles_map =
{
	{
		{"expose",	 domain_directives_handles::expose},
		{"end",		 domain_directives_handles::end},
		{"property", domain_directives_handles::property},
		{"symbol",	 domain_directives_handles::symbol},
		{"function", domain_directives_handles::function},
		{"dump",	 domain_directives_handles::dump},
		{"split",	 domain_directives_handles::split}
	}
};

matl::domain_parsing_raport matl::parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context)
{
	auto& context_impl = context->impl->impl;

	domain_parsing_state state;
	state.domain = std::make_shared<parsed_domain>();

	auto& source = domain_source;
	auto& iterator = state.iterator;

	size_t last_position = 0;
	int line_counter = 1;

	while (true)
	{
		std::string error;
		bool whitespaces_only = true;

		get_to_char_while_counting_lines('<', domain_source, iterator, line_counter, whitespaces_only);

		if (last_position != iterator && !whitespaces_only)
		{
			state.domain->directives.push_back(
				{
					directive_type::dump_block,
					{ std::move(domain_source.substr(last_position, iterator - last_position)) }
				}
			);
		}

		if (is_at_source_end(source, iterator))
			break;

		iterator++;

		{
			auto directive = get_string_ref(source, iterator, error);
			if (error.size() != 0) goto _parse_domain_handle_error;

			auto handle = directives_handles_map.find(directive);
			if (handle == directives_handles_map.end())
			{
				error = "No such directive: " + std::string(directive);
				get_to_char_while_counting_lines('>', domain_source, iterator, line_counter, whitespaces_only);
				goto _parse_domain_handle_error;
			}
			else
			{
				handle->second(source, context->impl->impl, state, error);
				if (error != "") goto _parse_domain_handle_error;

				get_spaces(source, state.iterator);
				if (source.at(state.iterator) != '>')
				{
					error = "Expected directive end";
					goto _parse_domain_handle_error;
				}
			}

			get_to_char_while_counting_lines('>', domain_source, iterator, line_counter, whitespaces_only);
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


void domain_directives_handles::expose(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(state.expose_scope, "Cannot use this directive here");
	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");
	state.expose_scope = true;
}

void domain_directives_handles::end(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(!state.expose_scope && !state.dump_properties_depedencies_scope, "Cannot use this directive here");
	state.expose_scope = false;
	state.dump_properties_depedencies_scope = false;
}

void domain_directives_handles::property(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	if (state.expose_scope)
	{
		get_spaces(source, state.iterator);
		auto type_name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		throw_error(state.domain->properties.find(name) != state.domain->properties.end(),
			"Cannot use this name; Property named: " + std::string(name) + " already exists");

		auto type = get_data_type(type_name);
		if (type == nullptr) error = "No such type: " + std::string(type_name);

		state.domain->properties.insert({ name, type });
	}
	else if (state.dump_properties_depedencies_scope)
	{
		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		auto itr = state.domain->properties.find(name);
		throw_error(itr == state.domain->properties.end(), "No such property: " + std::string(name));

		state.domain->directives.back().payload.push_back(name);
	}
	else
	{
		get_spaces(source, state.iterator);
		auto name = get_string_ref(source, state.iterator, error);
		rethrow_error();

		auto itr = state.domain->properties.find(name);
		throw_error(itr == state.domain->properties.end(), "No such property: " + std::string(name));

		state.domain->directives.push_back(
			{ directive_type::dump_property, { std::move(name) } }
		);
	}
}

void domain_directives_handles::symbol(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");
	throw_error(!state.expose_scope, "Cannot use this directive here");

	get_spaces(source, state.iterator);
	auto type_name = get_string_ref(source, state.iterator, error);
	rethrow_error();

	auto type = get_data_type(type_name);
	throw_error(type == nullptr, "No such type: " + std::string(type_name));

	get_spaces(source, state.iterator);
	auto name = get_string_ref(source, state.iterator, error);
	rethrow_error();

	throw_error(state.domain->symbols.find(name) != state.domain->symbols.end(),
		"Cannot use this name; Property named: " + std::string(name) + " already exists");

	get_spaces(source, state.iterator);
	throw_error(source.at(state.iterator) != '=', "Expected '='");
	state.iterator++;

	throw_error(source.at(state.iterator) == '>', "Expected symbol definition");

	size_t begin = state.iterator;
	get_to_char('>', source, state.iterator);

	state.domain->symbols.insert({ name, {type, source.substr(begin, state.iterator - begin)} });
}

void domain_directives_handles::function(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	auto& iterator = state.iterator;

	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");
	throw_error(!state.expose_scope, "Cannot use this directive here");

	get_spaces(source, iterator);
	auto function_name = get_string_ref(source, iterator, error);

	get_spaces(source, iterator);
	throw_error(source.at(iterator) != '=', "Expected '='");
	iterator++;

	get_spaces(source, iterator);
	auto returned_type_str = get_string_ref(source, iterator, error);
	rethrow_error();
	auto returned_type = get_data_type(returned_type_str);
	throw_error(returned_type == nullptr, "No such type: " + std::string(returned_type_str));

	get_spaces(source, iterator);
	auto function_native_name = get_string_ref(source, iterator, error);
	rethrow_error();

	get_spaces(source, iterator);
	throw_error(source.at(iterator) != '(', "Expected '('");
	iterator++;

	std::vector<const data_type*> arguments_types;
	while (true)
	{
		get_spaces(source, iterator);

		auto argument_type_str = get_string_ref(source, iterator, error);
		rethrow_error();
		auto argument_type = get_data_type(argument_type_str);
		throw_error(argument_type == nullptr, "No such type: " + std::string(argument_type_str));

		arguments_types.push_back(argument_type);

		get_spaces(source, iterator);

		if (source.at(iterator) == ')') break;
		else if (source.at(iterator) == ',') { iterator++; continue; }
		else throw_error(true, "Invalid syntax, expected ) or ,");
	}

	auto func_itr = state.domain->functions.find(function_name);
	if (func_itr == state.domain->functions.end())
	{
		func_itr = state.domain->functions.insert({ function_name, {} });
		auto& func_def = func_itr->second;

		func_def.is_exposed = true;

		for (int i = 0; i < arguments_types.size(); i++)
			func_def.arguments.push_back("_d" + std::to_string(i));

		func_def.function_code_name = function_native_name;
		func_def.valid = true;
		func_def.returned_value = nullptr;
		func_def.variables = {};
	}
	auto& func_def = func_itr->second;

	func_def.instances.push_back({ &func_def });
	auto& func_instance = func_def.instances.back();

	func_instance.returned_type = returned_type;
	func_instance.arguments_types = std::move(arguments_types);
	func_instance.valid = true;

	get_to_char('>', source, state.iterator);
}

void domain_directives_handles::dump(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(state.expose_scope, "Cannot use this directive here");
	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");

	get_spaces(source, state.iterator);
	auto dump_type = get_string_ref(source, state.iterator, error);
	rethrow_error();

	if (dump_type == "variables")
	{
		state.domain->directives.push_back({ directive_type::dump_variables, {} });
		state.dump_properties_depedencies_scope = true;
	}
	else if (dump_type == "functions")
	{
		state.domain->directives.push_back({ directive_type::dump_functions, {} });
		state.dump_properties_depedencies_scope = true;
	}
	else if (dump_type == "parameters")
	{
		state.domain->directives.push_back({ directive_type::dump_parameters, {} });
	}
	else if (dump_type == "insertion")
	{
		get_spaces(source, state.iterator);
		auto insertion = get_string_ref(source, state.iterator, error);
		rethrow_error();
		throw_error(context.domain_insertions.find(insertion) == context.domain_insertions.end(), "No such insertion: " + std::string(insertion));
		state.domain->directives.push_back({ directive_type::dump_insertion, { insertion } });
	}
	else
		error = "Invalid dump type: " + std::string(dump_type);

	get_to_char('>', source, state.iterator);
}

void domain_directives_handles::split(const std::string& source, context_public_implementation& context, domain_parsing_state& state, std::string& error)
{
	throw_error(state.expose_scope, "Cannot use this directive here");
	throw_error(state.dump_properties_depedencies_scope, "Cannot use this directive here");

	state.domain->directives.push_back({ directive_type::split, {} });
}