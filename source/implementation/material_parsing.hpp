#pragma once

using material_keyword_handle = void(*)(
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

heterogeneous_map<std::string, material_keyword_handle, hgm_string_solver> keywords_handles_map =
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
		parsed_material returned_value;
		returned_value.success = false;
		returned_value.errors = { "[0] Cannot parse material without context" };
		return returned_value;
	}

	material_parsing_state state;

	auto& context_impl = context->impl->impl;

	while (!is_at_source_end(material_source, state.iterator))
	{
		state.line_counter++;

		std::string error;

		auto& source = material_source;
		auto& iterator = state.iterator;

		int spaces = get_spaces(source, iterator);
		if (is_at_source_end(material_source, state.iterator)) break;

		if (source.at(iterator) == comment_char) goto _parse_material_next_line;
		if (is_at_line_end(source, iterator)) goto _parse_material_next_line;

		if (state.function_body)
		{
			if (spaces == 0 && state.this_line_indentation_spaces == 0)
			{
				error = "Expected function body";
				state.function_body = false;
				goto _parse_material_handle_error;
			}

			if (spaces == 0 && state.this_line_indentation_spaces != 0)
			{
				error = "Unexpected function end";
				state.function_body = false;
				goto _parse_material_handle_error;
			}

			if (spaces != state.this_line_indentation_spaces && state.this_line_indentation_spaces != 0)
			{
				error = "Indentation level must be consistient";
				goto _parse_material_handle_error;
			}
		}
		else if (spaces != 0)
		{
			error = "Indentation level must be consistient";
			goto _parse_material_handle_error;
		}

		state.this_line_indentation_spaces = spaces;

		{
			string_ref keyword = get_string_ref(source, iterator, error);

			if (error != "") goto _parse_material_handle_error;

			auto kh_itr = keywords_handles_map.find(keyword);

			if (kh_itr == keywords_handles_map.end())
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

	if (state.domain != nullptr)
		for (auto& prop : state.domain->properties)
			if (state.properties.find(prop.first) == state.properties.end())
				state.errors.push_back("[0] Missing property: " + prop.first);

	if (state.errors.size() != 0)
	{
		parsed_material material;
		material.success = false;
		material.errors = std::move(state.errors);
		return material;
	}

	if (state.domain == nullptr)
	{
		parsed_material material;
		material.success = false;
		material.errors = { "[0] Material does not specify the domain" };
		return material;
	}

	parsed_material material;
	material.success = true;
	material.sources = { "" };

	constexpr auto preallocated_shader_memory = 5 * 1024;

	material.sources.back().reserve(preallocated_shader_memory);

	auto& translator = context->impl->impl._translator;

	inlined_variables inlined;

	auto dump_property = [&](const directive& directive)
	{
		auto& property = state.properties.at(directive.payload.at(0));

		material.sources.back() += '(';
		material.sources.back() += translator->expression_translator(
			property.definition,
			&inlined,
			property.definition->used_functions
		);
		material.sources.back() += ')';
	};

	auto should_inline_variable = [&](const named_variable* var, const uint32_t& uses_count) -> bool
	{
		return
			var->second.definition->equations.size() == 1 &&							//variables does not contain ifs
			(																			//and
				uses_count <= 1 														//variables is used only once 
				||																		//or
				var->second.definition->equations.front()->value->nodes.size() == 1		//it is made of a single node
			);
	};

	auto sort_variables = [&](counting_set<named_variable*>& variables)
	{
		std::vector<std::pair<named_variable*, uint32_t>*> order;

		for (auto itr = variables.begin(); itr != variables.end(); itr++)
			order.push_back(&(*itr));

		std::sort(order.begin(), order.end(), [](std::pair<named_variable*, uint32_t>* const& a, std::pair<named_variable*, uint32_t>* const& b)
			{
				return a->first->second.definition_line < b->first->second.definition_line;
			});

		return order;
	};

	auto dump_variables = [&](const directive& directive)
	{
		counting_set<named_variable*> variables;

		for (auto& prop : directive.payload)
		{
			const auto& prop_exp = state.properties.at(prop).definition;
			get_used_variables_recursive(prop_exp, variables);
		}

		auto order = sort_variables(variables);

		for (auto itr = order.begin(); itr != order.end(); itr++)
		{
			if (should_inline_variable((*itr)->first, (*itr)->second))
			{
				inlined.insert({
					(*itr)->first,
					"(" + translator->expression_translator(
						(*itr)->first->second.definition,
						&inlined,
						(*itr)->first->second.definition->used_functions)
					+ ")"
					});
			}
			else
			{
				material.sources.back() += translator->variables_declarations_translator(
					(*itr)->first->first,
					&(*itr)->first->second,
					&inlined,
					(*itr)->first->second.definition->used_functions
				);
			}
		};
	};

	auto dump_functions = [&](const directive& directive)
	{
		counting_set<function_instance*> functions;

		for (auto& prop : directive.payload)
		{
			const auto& prop_exp = state.properties.at(prop).definition;
			get_used_functions_recursive(prop_exp, functions);
		}

		for (auto itr = functions.begin(); itr != functions.end(); itr++)
		{
			inlined_variables function_inlined;

			if ((*itr).first->function->is_exposed) continue;

			counting_set<named_variable*> variables;
			get_used_variables_recursive(itr->first->function->returned_value, variables);

			material.sources.back() += translator->function_header_translator(itr->first);
			
			size_t instance_index = 0;
			auto instances_itr = itr->first->function->instances.begin();
			while (&*instances_itr != itr->first)
			{
				instances_itr++; instance_index++;
			}

			auto order = sort_variables(variables);

			for (auto itr2 = order.begin(); itr2 != order.end(); itr2++)
			{
				auto& used_functions = (*itr2)->first->second.definition->used_functions;
				std::vector<function_instance*> used_functions_subset;

				size_t block_size = used_functions.size() / itr->first->function->instances.size();

				used_functions_subset.insert(
					used_functions_subset.begin(),
					used_functions.begin() + instance_index * block_size,
					used_functions.begin() + instance_index * block_size + block_size
				);

				if (should_inline_variable((*itr2)->first, (*itr2)->second))
				{
					function_inlined.insert({
						(*itr2)->first,
						"(" + translator->expression_translator(
							(*itr2)->first->second.definition,
							&function_inlined,
							used_functions_subset
						) + ")"
					});
				}
				else
				{
					material.sources.back() += translator->variables_declarations_translator(
						(*itr2)->first->first,
						&(*itr2)->first->second,
						&function_inlined,
						used_functions_subset
					);
				}
			};

			auto& used_functions = itr->first->function->returned_value->used_functions;
			std::vector<function_instance*> used_functions_subset;

			size_t block_size = used_functions.size() / itr->first->function->instances.size();

			used_functions_subset.insert(
				used_functions_subset.begin(),
				used_functions.begin() + instance_index * block_size,
				used_functions.begin() + instance_index * block_size + block_size
			);
			material.sources.back() += translator->function_return_statement_translator(itr->first, function_inlined, used_functions_subset);
		}
	};

	auto dump_parameters = [&]()
	{
		for (auto& parameter : state.parameters)
			material.sources.back() += translator->parameters_declarations_translator(parameter.first, &parameter.second);
	};

	for (auto& directive : state.domain->directives)
	{
		using directive_type = directive_type;

		switch (directive.type)
		{
		case directive_type::dump_block:
			material.sources.back() += directive.payload.at(0);
			break;
		case directive_type::dump_insertion:
			material.sources.back() += context_impl.domain_insertions.at(directive.payload.at(0));
			break;
		case directive_type::split:
			material.sources.push_back("");
			material.sources.back().reserve(preallocated_shader_memory);
			break;
		case directive_type::dump_property:
			dump_property(directive);
			break;
		case directive_type::dump_variables:
			dump_variables(directive);
			break;
		case directive_type::dump_functions:
			dump_functions(directive);
			break;
		case directive_type::dump_parameters:
			dump_parameters();
			break;
		}
	}

	for (auto& param : state.parameters)
	{
		material.parameters.push_back({});
		auto& param_info = material.parameters.back();

		param_info.name = std::move(param.first);

		if (param.second.type == scalar_data_type)
			param_info.type = parsed_material::parameter::type::scalar;
		else if (param.second.type == bool_data_type)
			param_info.type = parsed_material::parameter::type::boolean;
		else if (param.second.type == texture_data_type)
			param_info.type = parsed_material::parameter::type::texture;
		else if (param.second.default_value_numeric.size() == 2)
			param_info.type = parsed_material::parameter::type::vector2;
		else if (param.second.default_value_numeric.size() == 3)
			param_info.type = parsed_material::parameter::type::vector3;
		else if (param.second.default_value_numeric.size() == 4)
			param_info.type = parsed_material::parameter::type::vector4;

		param_info.texture_default_value = std::move(param.second.default_value_texture);
		param_info.numeric_default_value = std::move(param.second.default_value_numeric);
	}

	return material;
}

void material_keywords_handles::let
(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto var_name = get_string_ref(source, iterator, error);
	rethrow_error();

	throw_error(var_name.at(0) == symbols_prefix, "Cannot declare symbols in material");

	is_name_unique(
		var_name,
		&state.variables,
		&state.domain->symbols,
		&state.parameters,
		&state.functions,
		context,
		&state.libraries,
		error
	);
	rethrow_error();

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);

	if (assign_operator != '=')
		error = "Expected '='";

	rethrow_error();

	throw_error(state.functions.find(var_name) != state.functions.end(),
		"Function named " + std::string(var_name) + " already exists");

	if (!state.function_body)
	{
		is_name_unique(
			var_name, 
			&state.variables, 
			&state.domain->symbols, 
			&state.parameters, 
			&state.functions, 
			context,
			&state.libraries, 
			error
		);
		rethrow_error();

		auto& var_def = state.variables.insert({ var_name, {} })->second;
		var_def.definition_line = state.line_counter;

		var_def.definition = get_expression(
			source,
			iterator,
			state.this_line_indentation_spaces,
			state.line_counter,
			&state.variables,
			&state.parameters,
			&state.functions,
			&state.libraries,
			context,
			state.domain,
			error
		);
		rethrow_error();

		var_def.type = validate_expression(var_def.definition, state.domain, error);
		rethrow_error();
	}
	else
	{
		auto& func_def = state.functions.recent().second;

		is_name_unique(
			var_name,
			&func_def.variables,
			nullptr,
			nullptr,
			&state.functions,
			context,
			&state.libraries,
			error
		);
		rethrow_error();

		auto& var_def = func_def.variables.insert({ var_name, {} })->second;
		var_def.definition_line = state.line_counter;

		var_def.definition = get_expression(
			source,
			iterator,
			state.this_line_indentation_spaces,
			state.line_counter,
			&func_def.variables,
			nullptr,
			&state.functions,
			&state.libraries,
			context,
			nullptr,
			error
		);
		if (error != "") func_def.valid = false;
		rethrow_error();
	}
}

void material_keywords_handles::property
(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	throw_error(state.domain == nullptr, "Cannot use property since the domain has not yet been specified");
	throw_error(state.function_body, "Cannot use property in this scope");

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto property_name = get_string_ref(source, iterator, error);
	rethrow_error();

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);
	throw_error(assign_operator != '=', "Expected '='");

	auto itr = state.domain->properties.find(property_name);
	throw_error(itr == state.domain->properties.end(), "No such property: " + std::string(property_name));

	throw_error(state.properties.find(property_name) != state.properties.end(),
		"Equation for property " + std::string(property_name) + " is already specified");

	auto& prop = state.properties.insert({ property_name, {} })->second;
	prop.definition = get_expression(
		source,
		iterator,
		state.this_line_indentation_spaces,
		state.line_counter,
		&state.variables,
		&state.parameters,
		&state.functions,
		&state.libraries,
		context,
		state.domain,
		error
	);
	rethrow_error();

	auto type = validate_expression(prop.definition, state.domain, error);
	rethrow_error();

	if (itr->second != type)
		error = "Invalid property type; expected: " + itr->second->name + " got: " + type->name;
}

void material_keywords_handles::_using
(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	throw_error(state.function_body, "Cannot use using in this scope");

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto using_type = get_string_ref(source, iterator, error);

	rethrow_error();

	if (using_type == "domain")
	{
		throw_error(state.domain != nullptr, "Domain is already specified");

		get_spaces(source, iterator);
		auto domain_name = get_rest_of_line(source, iterator);
		auto itr = context.domains.find(domain_name);

		throw_error(itr == context.domains.end(), "No such domain: " + std::string(domain_name));

		state.domain = itr->second;
	}
	else if (using_type == "library")
	{
		get_spaces(source, iterator);
		auto library_name = get_rest_of_line(source, iterator);

		auto itr = context.libraries.find(library_name);
		throw_error(itr == context.libraries.end(), "No such library: " + std::string(library_name));

		state.libraries.insert({ library_name, itr->second });
	}
	else if (using_type == "parameter")
	{
		get_spaces(source, iterator);
		auto parameter_name = get_string_ref(source, iterator, error);
		rethrow_error();

		get_spaces(source, iterator);
		auto assign_operator = get_char(source, iterator);
		throw_error(assign_operator != '=', "Expected '='");

		is_name_unique(
			parameter_name,
			&state.variables,
			&state.domain->symbols,
			&state.parameters,
			&state.functions,
			context,
			&state.libraries,
			error
		);
		rethrow_error();

		state.parameters.insert({ parameter_name, {} });
		auto& param_def = state.parameters.recent().second;

		get_spaces(source, iterator);
		auto value = get_rest_of_line(source, iterator);

		if (expressions_parsing_utilities::is_scalar_literal(value, error))
		{
			param_def.type = scalar_data_type;
			param_def.default_value_numeric = { std::stof(value) };
			rethrow_error();
		}
		else if (source.at(value.begin) == '(')
		{
			iterator = value.begin + 1;
			bool hitted_parentheses_end = false;
			std::string buffer;
			buffer.reserve(16);

			while (!(is_at_line_end(source, iterator)))
			{
				auto& c = source.at(iterator);

				if (c == ',')
				{
					expressions_parsing_utilities::is_scalar_literal(buffer, error);
					rethrow_error();
					param_def.default_value_numeric.push_back({ std::stof(buffer) });
					buffer.clear();
				}
				else if (c == ')')
				{
					hitted_parentheses_end = true;

					expressions_parsing_utilities::is_scalar_literal(buffer, error);
					rethrow_error();
					param_def.default_value_numeric.push_back({ std::stof(buffer) });
					buffer.clear();
				}
				else if (c == ' ' || c == '\t') {}
				else
				{
					throw_error(hitted_parentheses_end, "Unexpected symbol after vector literal end");
					buffer.push_back(c);
				}

				iterator++;
			}

			param_def.type = get_vector_type_of_size(static_cast<uint8_t>(param_def.default_value_numeric.size()));

			throw_error(!hitted_parentheses_end, "Mismatched parentheses");
		}
		else
		{
			param_def.type = texture_data_type;
			param_def.default_value_texture = value;
		}
	}
	else //Call custom case
	{
		auto itr = context.custom_using_handles.find(using_type);
		throw_error(itr == context.custom_using_handles.end(), "No such using case: " + std::string(using_type));

		get_spaces(source, iterator);
		auto arg = get_rest_of_line(source, iterator);;
		itr->second(arg, error);
	}
}

void material_keywords_handles::func
(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	get_spaces(source, state.iterator);
	std::string func_name = get_string_ref(source, state.iterator, error);
	throw_error(func_name == "", "Expected function name");
	rethrow_error();

	is_name_unique(
		func_name,
		&state.variables,
		&state.domain->symbols,
		&state.parameters,
		&state.functions,
		context,
		&state.libraries,
		error
	);
	rethrow_error();

	handles_common::func(func_name, source, context, state, error);
	rethrow_error();
}

void material_keywords_handles::_return
(const std::string& source, context_public_implementation& context, material_parsing_state& state, std::string& error)
{
	handles_common::_return(source, context, state, error);
}