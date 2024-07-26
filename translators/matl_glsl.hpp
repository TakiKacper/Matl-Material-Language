#pragma once

#ifndef MATL_IMPLEMENTATION
	#error Include the translator in the same compilation unit as the matl implementation
#endif

#ifndef MATL_IMPLEMENTATION_INCLUDED
	#error Include matl.h first
#else

namespace matl_glsl
{
	inline std::string variable_name_formater(const string_ref& name)
	{
		return "_matl_v_" + std::string(name);
	}

	inline std::string parameter_name_formater(const string_ref& name)
	{
		return "_matl_p_" + std::string(name);
	}

	inline std::string function_name_formater(const std::string& name)
	{
		return "_matl_f_" + name;
	}

	inline std::string function_from_lib_name_formater(const string_ref& library_name, const string_ref& name)
	{
		return "_matl_lf_" + std::string(library_name) +  "_" + std::string(name);
	}

	inline const char* translate_type_name(const data_type* type)
	{
		if (type->name == "bool")
			return "bool";
		if (type->name == "scalar")
			return "float";
		if (type->name == "vector2")
			return "vec2";
		if (type->name == "vector3")
			return "vec3";
		if (type->name == "vector4")
			return "vec4";
		if (type->name == "texture")
			return "sampler2D";
		return "invalid";
	}

	inline const char* translate_unary_operator(const unary_operator_definition* operato)
	{
		if (operato->symbol == "not")
			return "!";
		return operato->symbol.c_str();
	}

	inline const char* translate_binary_operator(const binary_operator_definition* operato)
	{
		if (operato->symbol == "and")
			return " && ";
		if (operato->symbol == "or")
			return " || ";
		if (operato->symbol == "xor")
			return " ^^ ";
		return operato->symbol.c_str();
	}

	std::string translate_single_expression(
		const expression::single_expression* const& le,
		const inlined_variables* inlined,
		const std::vector<function_instance*>& functions_instances,
		const std::unordered_map<const symbol_definition*, size_t> current_symbols_definitions
	)
	{
		size_t functions_instances_iterator = 0;

		std::vector<std::pair<uint8_t, std::string>> values;

		auto get_value = [&](size_t index_from_end) -> std::pair<uint8_t, std::string>&
		{
			return *(values.end() - index_from_end - 1);
		};

		auto remove_value = [&](size_t index_from_end)
		{
			values.erase(values.end() - index_from_end - 1, values.end() - index_from_end);
		};

		auto push_parenthesis = [&](size_t index_from_end)
		{
			get_value(index_from_end).second.insert(0, "(");
			get_value(index_from_end).second.push_back(')');
		};

		auto push_parenthesis_if_needed = [&](size_t index_from_end, const uint8_t& next_operator_precedence)
		{
			auto& target = get_value(index_from_end);

			if (target.first != 0 && target.first < next_operator_precedence)
				push_parenthesis(index_from_end);
		};

		auto push_binary_operator = [&](const binary_operator_definition* _operator)
		{
			auto& target = get_value(1);

			push_parenthesis_if_needed(1, _operator->precedence);
			push_parenthesis_if_needed(0, _operator->precedence);

			target.second += translate_binary_operator(_operator);
			target.second += std::move(get_value(0).second);

			target.first = _operator->precedence;

			remove_value(0);
		};

		auto push_unary_operator = [&](const unary_operator_definition* _operator)
		{
			if (get_value(0).first != 0) push_parenthesis(0);
			get_value(0).second.insert(0, translate_unary_operator(_operator));
		};

		auto push_vector = [&](std::pair<uint8_t, uint8_t> vec_info)
		{
			std::string base = "vec";
			base.push_back(+char('0' + vec_info.second));
			base.push_back('(');

			for (int i = vec_info.first - 1; i != -1; i--)
			{
				base += std::move(get_value(i).second);
				remove_value(i);
				if (i != 0) base.push_back(',');
			}

			base.push_back(')');

			values.push_back({ 0, base });
		};

		auto access_vector_members = [&](const std::vector<uint8_t>& members)
		{
			auto& target = get_value(0).second;
			target.push_back('.');

			for (auto& member : members)
			{
				switch (member)
				{
				case 1: target.push_back('x'); break;
				case 2: target.push_back('y'); break;
				case 3: target.push_back('z'); break;
				case 4: target.push_back('w'); break;
				}
			}
		};

		auto push_function = [&](const named_function* func)
		{
			std::string base;
			
			if (func->second.is_exposed)
				base += functions_instances.at(functions_instances_iterator)->function_native_name;
			else if (func->second.library == nullptr)
				base += function_name_formater(func->first);
			else
				base += function_from_lib_name_formater(
					func->second.library->first,
					func->first
				);

			base.push_back('(');

			for (size_t i = func->second.arguments.size() - 1; i != -1; i--)
			{
				base += std::move(get_value(i).second);
				remove_value(i);
				if (i != 0) base.push_back(',');
			}

			base.push_back(')');

			values.push_back({ 0, base });

			functions_instances_iterator++;
		};

		auto push_scalar = [&](const std::string& scalar_value)
		{
			if (scalar_value.find('.') != scalar_value.npos)
				values.push_back({ 0, scalar_value });
			else
				values.push_back({ 0, scalar_value + ".0" });
		};

		auto push_variable = [&](const named_variable* var)
		{
			auto itr = inlined->find(var);
			if (itr == inlined->end())
				values.push_back({ 0, variable_name_formater(var->first) });
			else
				values.push_back({ 0, itr->second});
		};

		using node = ::expression::node;

		for (auto& node : le->nodes)
		{
			switch (node->get_type())
			{
			case node::node_type::variable:
				push_variable(node->as_variable()); break;
			case node::node_type::symbol:
				values.push_back({ 0, node->as_symbol()->definitions.at(current_symbols_definitions.at(node->as_symbol()))}); break;
			case node::node_type::parameter:
				values.push_back({ 0, parameter_name_formater(node->as_parameter()->first)}); break;
			case node::node_type::scalar_literal:
				push_scalar(node->as_scalar_literal()); break;
			case node::node_type::binary_operator:
				push_binary_operator(node->as_binary_operator()); break;
			case node::node_type::unary_operator:
				push_unary_operator(node->as_unary_operator()); break;
			case node::node_type::vector_contructor_operator:
				push_vector(node->as_vector_contructor_operator()); break;
			case node::node_type::vector_component_access_operator:
				access_vector_members(node->as_vector_access_operator()); break;
			case node::node_type::function:
				push_function(node->as_function()); break;
			default: static_assert(true, "Unhandled node type");
			}
		}

		return get_value(0).second;
	}

	std::string translate_expression(
		const expression* const& exp,
		const inlined_variables* inlined,
		const std::vector<function_instance*>& functions_instances,
		const std::unordered_map<const symbol_definition*, size_t> current_symbols_definitions
	)
	{
		std::string result;

		if (exp->equations.size() == 1)
		{
			result += translate_single_expression(exp->equations.front()->value, inlined, functions_instances, current_symbols_definitions);
			return result;
		}

		int counter = 0;
		for (auto& equation : exp->equations)
		{
			if (equation->condition == nullptr)
			{
				result += translate_single_expression(equation->value, inlined, functions_instances, current_symbols_definitions);
				result.reserve(result.size() + counter);
				while (counter != 0) { result += ')'; counter--; }
				break;
			}

			result += translate_single_expression(equation->condition, inlined, functions_instances, current_symbols_definitions);
			result += "?(";
			result += translate_single_expression(equation->value, inlined, functions_instances, current_symbols_definitions);
			result += "):(";

			counter++;
		}

		return result;
	}

	std::string translate_variable(
		const string_ref& name,
		const variable_definition* const& var,
		const inlined_variables* inlined,
		const std::vector<function_instance*>& functions_instances,
		const std::unordered_map<const symbol_definition*, size_t> current_symbols_definitions
	)
	{
		std::string result;
		result.reserve(64);

		result += translate_type_name(var->type);
		result += " ";
		result += variable_name_formater(name);
		result += " = ";
		result += translate_expression(var->definition, inlined, functions_instances, current_symbols_definitions);
		result += ";\n";

		return result;
	}

	std::string translate_parameter_opengl(
		const string_ref& name,
		const parameter_definition* const& param
	)
	{
		std::string result;
		result.reserve(64);
		
		result += "uniform ";
		result += translate_type_name(param->type);
		result += " ";
		result += parameter_name_formater(name);
		result += ";\n";

		return result;
	}

	std::string translate_function_header(const function_instance* instance)
	{
		std::string result;
		result.reserve(128);

		result += translate_type_name(instance->returned_type);
		result += " ";

		if (instance->function->library == nullptr)
			result += function_name_formater(*instance->function->function_name_ptr);
		else
			result += function_from_lib_name_formater(instance->function->library->first, *instance->function->function_name_ptr);

		result += '(';

		for (size_t i = 0; i < instance->function->arguments.size(); i++)
		{
			result += translate_type_name(instance->arguments_types.at(i));
			result += ' ';
			result += variable_name_formater(instance->function->arguments.at(i));

			if (i != instance->function->arguments.size() - 1) result += ", ";
		}

		result += ")\n{\n";

		return result;
	}

	std::string translate_function_return(
		const function_instance* instance, 
		const inlined_variables& inlined,
		const std::vector<function_instance*>& used_instances
	)
	{
		std::string result;
		result.reserve(128);

		result += "\treturn ";
		result += translate_expression(instance->function->returned_value, &inlined, used_instances, {});
		result += ";\n";

		result += "};\n";

		return result;
	}

	::translator translator{"opengl_glsl", translate_expression, translate_variable, translate_parameter_opengl, translate_function_header, translate_function_return};
};
#endif