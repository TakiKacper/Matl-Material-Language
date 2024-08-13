#pragma once

void instantiate_function(
	function_definition& func_def,
	const std::vector<const data_type*> arguments,
	decltype(expression::used_functions)& used_functions,
	std::string& error
);

namespace expressions_parsing_utilities
{
	inline string_view get_node_str(const std::string& source, size_t& iterator, std::string& error)
	{
		if (source.size() >= iterator + 1)
		{
			if ((
				source.at(iterator) == '!' || source.at(iterator) == '>' || source.at(iterator) == '<' || source.at(iterator) == '='
				)
				&& source.at(iterator + 1) == '='
				)
			{
				iterator += 2;
				return string_view{ source, iterator - 2, iterator };
			}
		}

		if (is_operator(source.at(iterator)))
		{
			iterator++;
			return string_view{ source, iterator - 1, iterator };
		}

		size_t begin = iterator;

		bool only_digits = true;
		bool dot_used = false;

		while (true)
		{
			const char& c = source.at(iterator);

			if (!is_digit(c) && c != '.') only_digits = false;

			if (is_operator(c))
			{
				if (c != '.') break;
				if (!only_digits) break;
				if (dot_used) break;
				dot_used = true;
			}

			if (is_whitespace(c)) break;

			iterator++; if (iterator == source.size()) break;
		}

		if (iterator == begin)
		{
			error = "Expected token not: ";

			if (source.at(iterator) == ' ')
				error += "space";
			else if (source.at(iterator) == '\n')
				error += "new line";
			else if (source.at(iterator) == '\t')
				error += "tab";
			else
				error += source.at(iterator);
		}

		return string_view{ source, begin, iterator };
	}

	inline bool is_function_call(const std::string& source, size_t& iterator)
	{
		get_spaces(source, iterator);

		if (is_at_line_end(source, iterator))
			return false;

		return (source.at(iterator) == '(');
	}

	inline bool is_scalar_literal(const string_view& node_str, std::string& error)
	{
		size_t dot_pos = -1;

		for (size_t i = 0; i < node_str.size(); i++)
		{
			if (node_str.at(i) == '.' && dot_pos == -1)
				dot_pos = i;
			else if (!is_digit(node_str.at(i)))
				return false;
		}

		if (dot_pos == 0 || dot_pos == node_str.size() - 1)
			error = "Invalid scalar literal: " + std::string(node_str);

		return true;
	}

	inline bool is_unary_operator(const string_view& node_str)
	{
		return get_unary_operator(node_str) != nullptr;
	}

	inline bool is_binary_operator(const string_view& node_str)
	{
		return get_binary_operator(node_str) != nullptr;
	}

	inline int get_comas_inside_parenthesis(const std::string& source, size_t iterator, std::string& error)
	{
		int parenthesis_deepness = 0;
		int comas = 0;

		while (!is_at_line_end(source, iterator))
		{
			switch (source.at(iterator))
			{
			case '(':
				parenthesis_deepness++; break;
			case ')':
				parenthesis_deepness--;
				if (parenthesis_deepness == 0) goto _get_comas_inside_parenthesis_break;
				break;
			case ',':
				if (parenthesis_deepness == 1)
					comas++;
				break;
			}
			iterator++;
		}

	_get_comas_inside_parenthesis_break:
		if (parenthesis_deepness != 0)
			error = "Mismatched parentheses";

		return comas;
	}

	int get_precedence(const expression::node* node)
	{
		switch (node->get_type())
		{
		case expression::node::node_type::function:
			return functions_precedence;
		case expression::node::node_type::binary_operator:
			return node->as_binary_operator()->precedence;
		case expression::node::node_type::unary_operator:
			return node->as_unary_operator()->precedence;
		case expression::node::node_type::vector_component_access_operator:
			return 256;
		}

		return 0;
	}

	bool is_any_of_left_parentheses(const expression::node* node)
	{
		return	node->get_type() == expression::node::node_type::vector_contructor_operator ||
				node->get_type() == expression::node::node_type::left_parenthesis ||
				node->get_type() == expression::node::node_type::function;
	}

	void insert_operator(
		std::list<expression::node*>& operators_stack,
		std::list<expression::node*>& output,
		expression::node* node
	)
	{
		while (operators_stack.size() != 0)
		{
			auto previous = operators_stack.back();
			if (
				previous->get_type() != expression::node::node_type::left_parenthesis &&
				previous->get_type() != expression::node::node_type::vector_contructor_operator &&
				previous->get_type() != expression::node::node_type::function &&
				get_precedence(previous) >= get_precedence(node)
				)
			{
				output.push_back(previous);
				operators_stack.pop_back();
			}
			else break;
		}

		operators_stack.push_back(node);
	};

	void shunting_yard(
		const std::string& source,
		size_t& iterator,
		int indentation,
		int& lines_counter,
		variables_collection* variables,
		parameters_collection* parameters,
		function_collection* functions,
		libraries_collection* libraries,
		const context_public_implementation& context_impl,
		std::shared_ptr<const parsed_domain> domain,
		std::list<expression::exp_case*>& cases,
		std::list<expression::node*>& output,
		std::list<expression::node*>& operators,
		std::vector<named_variable*>& used_variables,
		std::string& error
	)
	{
		using node = expression::node;
		using node_type = node::node_type;

		auto push_vector_or_parenthesis = [&]()
		{
			int comas = get_comas_inside_parenthesis(source, iterator - 1, error);
			rethrow_error();

			node* new_node;

			if (comas == 0)
			{
				new_node = node::new_left_parenthesis();
				operators.push_back(new_node);
			}
			else if (comas <= 3)
			{
				new_node = node::new_vector_contructor_operator(comas + 1, 0);
				operators.push_back(new_node);
			}		
			else
				error = "Constructed vector is too long";

			
		};

		auto close_parenthesis = [&]()
		{
			bool found_left_parenthesis = false;
			expression::node* previous = nullptr;

			while (operators.size() != 0)
			{
				previous = operators.back();
				if (is_any_of_left_parentheses(previous))
				{
					found_left_parenthesis = true;
					break;
				}

				output.push_back(previous);
				operators.pop_back();
			}

			if (!found_left_parenthesis)
				error = "Mismatched parentheses";
			else if (previous->get_type() == node_type::left_parenthesis)
			{
				delete operators.back();
				operators.pop_back();
			}
			else
			{
				output.push_back(previous);
				operators.pop_back();
			}
		};

		auto handle_comma = [&]()
		{
			while (operators.size() != 0)
			{
				auto previous = operators.back();
				if (is_any_of_left_parentheses(previous))
					break;
				output.push_back(previous);
				operators.pop_back();
			}
		};

		auto check_expression = [&]()
		{
			//Push all binary_operators left on the binary_operators stack to the output
			auto itr = operators.rbegin();
			while (itr != operators.rend())
			{
				output.push_back(*itr);
				itr++;
			}

			operators.clear();

			size_t operands_check_sum = 0;
			for (auto& n : output)
			{
				switch (n->get_type())
				{
				case node_type::scalar_literal:
				case node_type::symbol:
				case node_type::variable:
				case node_type::parameter:
					operands_check_sum++;
					break;
				case node_type::function:
					throw_error(operands_check_sum < n->as_function()->second.arguments.size(), "Invalid expression");
					operands_check_sum -= n->as_function()->second.arguments.size();
					operands_check_sum++;
					break;
				case node_type::vector_contructor_operator:
					throw_error(operands_check_sum < n->as_vector_contructor_operator().first, "Invalid expression");
					operands_check_sum -= n->as_vector_contructor_operator().first;
					operands_check_sum++;
					break;
				case node_type::binary_operator:
					throw_error(operands_check_sum < 2, "Invalid expression");
					operands_check_sum -= 2;
					operands_check_sum++;
					break;
				case node_type::left_parenthesis:
				case node_type::unary_operator:
				case node_type::vector_component_access_operator:
					throw_error(operands_check_sum < 1, "Invalid expression");
					break;
				}
			}

			throw_error(operands_check_sum != 1, "Invalid expression");
		};

		get_spaces(source, iterator);

		bool accepts_right_unary_operator = true;
		bool expecting_library_function = false;
		bool expecting_exposed = false;

		string_view library_name = { nullptr };

		bool if_used = false;
		bool else_used = false;

		expression* condition_buffer = nullptr;

	_shunting_yard_loop:
		while (!is_at_line_end(source, iterator))
		{
			auto node_str = get_node_str(source, iterator, error);
			rethrow_error();

			if (node_str.at(0) == comment_char)
				goto _shunting_yard_end;		//error for multline

			{
				size_t iterator2 = iterator;
				throw_error(expecting_library_function && !is_function_call(source, iterator2), "Expected function call");
			}

			if (node_str == "if")
			{
				throw_error(else_used, "Cannot add if-s after an else statement");
				throw_error(if_used && output.size() == 0, "Invalid Expression");
				throw_error(!if_used && output.size() != 0, "Invalid Expression");

				if (if_used)
				{
					check_expression();
					cases.back()->value = new expression::single_expression(output);
				}

				if_used = true;
				accepts_right_unary_operator = true;
			}
			else if (node_str == "else")
			{
				throw_error(!if_used, "else can be only used after if statement");
				throw_error(output.size() == 0, "Invalid Expression");
				throw_error(else_used, "Cannot specify two else cases");

				check_expression();
				cases.back()->value = new expression::single_expression(output);

				else_used = true;
				accepts_right_unary_operator = true;
			}
			else if (node_str == ":")
			{
				if (!else_used) check_expression();

				throw_error(!if_used, ": can be only used after if or else statements");
				throw_error(output.size() == 0 && !else_used, "Missing condition");
				throw_error(output.size() != 0 && else_used, "Invalid expression");

				if (!else_used)
				{
					cases.push_back(new expression::exp_case{
						new expression::single_expression(output),
						nullptr
					});
				}
				else
				{
					cases.push_back(new expression::exp_case{
						nullptr,
						nullptr
					});
				}

				accepts_right_unary_operator = true;
			}
			else if (node_str == domain_exposed_access)
			{
				throw_error(domain == nullptr, "Cannot use domain here");
				expecting_exposed = true;
				accepts_right_unary_operator = false;
			}
			else if (is_unary_operator(node_str) && accepts_right_unary_operator)
			{
				auto new_node = node::new_unary_operator(get_unary_operator(node_str));
				insert_operator(operators, output, new_node);
				accepts_right_unary_operator = false;
			}
			else if (is_binary_operator(node_str))
			{
				auto new_node = node::new_binary_operator(get_binary_operator(node_str));
				insert_operator(operators, output, new_node);
				accepts_right_unary_operator = true;
			}
			else if (node_str.at(0) == '(')
			{
				push_vector_or_parenthesis();
				rethrow_error();
				accepts_right_unary_operator = true;
			}
			else if (node_str.at(0) == ')')
			{
				close_parenthesis();
				accepts_right_unary_operator = false;
			}
			else if (node_str.at(0) == ',')
			{
				handle_comma();
				accepts_right_unary_operator = true;
			}
			else if (node_str.at(0) == '.' && output.size() != 0 && output.back()->get_type() == node_type::library)
			{
				expecting_library_function = true;
				continue;
			}
			else if (node_str.at(0) == '.' && output.size() != 0)
			{
				get_spaces(source, iterator);
				auto components = get_string_ref(source, iterator, error);

				rethrow_error();
				throw_error(components.size() > 4, "Constructed vector is too long: " + std::string(node_str));

				std::vector<uint8_t> included_vector_components;

				for (size_t i = 0; i < components.size(); i++)
				{
					auto itr = vector_components_names.find(components.at(i));
					throw_error(itr == vector_components_names.end(), error = "No such vector component: " + components.at(i));
					included_vector_components.push_back(itr->second);
				}

				auto new_node = node::new_vector_access_operator(included_vector_components);
				accepts_right_unary_operator = false;
				operators.push_back(new_node);
			}
			else if (node_str.at(0) == '.' && expecting_exposed)
			{
				get_spaces(source, iterator);
				auto symbol_name = get_string_ref(source, iterator, error);
				rethrow_error();

				if (is_function_call(source, iterator))
				{
					node_str = symbol_name;
					goto _shunting_yard_function;
				}

				auto itr = domain->symbols.find(symbol_name);
				throw_error(itr == domain->symbols.end(), "No such symbol: " + std::string(symbol_name));

				auto new_node = node::new_symbol(&(itr->second));

				output.push_back(new_node);
				accepts_right_unary_operator = false;
				expecting_exposed = false;
			}
			else if (node_str.at(0) == '.')
			{
				throw_error(true, "Invalid expression");
			}
			else if (is_scalar_literal(node_str, error))
			{
				rethrow_error();
				auto new_node = node::new_scalar_literal(node_str);
				output.push_back(new_node);
				accepts_right_unary_operator = false;
			}
			else if (is_function_call(source, iterator))
			{
			_shunting_yard_function:
				throw_error(functions == nullptr, "Cannot use functions here");

				int args_ammount = get_comas_inside_parenthesis(source, iterator - 1, error);
				rethrow_error();

				iterator++; //Jump over the ( char
				throw_error(is_at_source_end(source, iterator), "Unexpected file end");

				if (args_ammount != 0)
					args_ammount++;
				else
				{
					get_spaces(source, iterator);
					if (source.at(iterator) != ')')
						args_ammount = 1;
				}

				function_collection::iterator itr;
				if (expecting_library_function)
				{
					expecting_library_function = false;
					auto& library = output.back()->as_library();

					itr = library->functions.find(node_str);
					throw_error(itr == library->functions.end(), "No such function: " + std::string(node_str));
					delete output.back();
					output.pop_back();
				}
				else if (expecting_exposed)
				{
					//this cast can be done since domain functions will not be instanciated 
					//(since they are exposed) so their state is not going to change
					expecting_exposed = false;

					itr = const_cast<parsed_domain*>(domain.get())->functions.find(node_str);
					throw_error(itr == domain->functions.end(), "No such function: " + std::string(node_str));
					goto _shunting_yard_process_function;
				}
				else
				{
					itr = functions->find(node_str);
					if (itr != functions->end()) goto _shunting_yard_process_function;

					//this one also is not going to be instanciated
					itr = const_cast<context_public_implementation&>(context_impl).common_functions.find(node_str);
					if (itr != context_impl.common_functions.end()) goto _shunting_yard_process_function;

					throw_error(true, "No such function: " + std::string(node_str));
				}

			_shunting_yard_process_function:
				throw_error(expecting_library_function && !itr->second.valid, "Cannot use invalid function: " + std::string(library_name) + "." + std::string(node_str));
				throw_error(!itr->second.valid, "Cannot use invalid function: " + std::string(node_str));

				throw_error(itr->second.arguments.size() != args_ammount,
					"Function " + std::string(node_str)
					+ " takes " + std::to_string(itr->second.arguments.size())
					+ " arguments, not " + std::to_string(args_ammount);
				);

				throw_error((*itr).second.returned_value == nullptr && !(*itr).second.is_exposed,
					"Cannot use function: " + std::string(node_str) + " because it is missing return statement"
				);

				auto new_node = node::new_function(&(*itr));
				insert_operator(operators, output, new_node);
				accepts_right_unary_operator = true;
			}
			else
			{
				accepts_right_unary_operator = false;

				throw_error(variables == nullptr, "Cannot use variables, parameters and functions in here");

				//Check if variable
				auto itr = variables->find(node_str);

				if (itr != variables->end())
				{
					auto new_node = node::new_variable(&(*itr));
					output.push_back(new_node);
					used_variables.push_back(&(*itr));

					continue;
				}

				auto itr2 = parameters->find(node_str);
				if (parameters != nullptr && itr2 != parameters->end())
				{
					auto new_node = node::new_parameter(&(*itr2));
					output.push_back(new_node);
					continue;
				}

				//Check if library
				auto itr3 = libraries->find(node_str);
				throw_error(itr3 == libraries->end(), "No such variable: " + std::string(node_str));

				library_name = { node_str };
				auto new_node = node::new_library(itr3->second);
				output.push_back(new_node);
			}

			get_spaces(source, iterator);
		}

		if (is_at_source_end(source, iterator)) goto _shunting_yard_end;

		{
			size_t iterator_2 = iterator;
			get_to_new_line(source, iterator_2);

			int spaces = get_spaces(source, iterator_2);

			if (is_at_source_end(source, iterator_2)) goto _shunting_yard_end;

			if (spaces > indentation)
			{
				std::string temp_error;

				size_t iterator_3 = iterator_2;
				auto str = get_string_ref(source, iterator_3, temp_error);

				if (temp_error != "")
					goto _shunting_yard_end;

				lines_counter++;
				iterator = iterator_2;
				goto _shunting_yard_loop;
			}
		}

	_shunting_yard_end:
		check_expression();

		throw_error(expecting_library_function, "Invalid expression");
		throw_error(if_used && !else_used, "Each if statement must go along with an else statement")

			if (cases.size() != 0)
				cases.back()->value = new expression::single_expression(output);
			else
				cases.push_back(new expression::exp_case{
					nullptr,
					new expression::single_expression(output)
					});
	}

	inline void validate_node(
		expression* exp,
		expression::node* n,
		std::vector<const data_type*>& types,
		const std::shared_ptr<const parsed_domain>& domain,
		std::string& error
	)
	{
		using node = expression::node;

		auto get_type = [&](size_t index_from_end) -> const data_type* const&
		{
			return types.at(types.size() - 1 - index_from_end);
		};

		auto pop_types = [&](size_t ammount)
		{
			types.erase(types.end() - ammount, types.end());
		};

		auto handle_binary_operator = [&](const binary_operator_definition* op)
		{
			auto left = get_type(1);
			auto right = get_type(0);

			for (auto& at : op->allowed_types)
			{
				if (at.left_operand_type != left || at.right_operand_type != right) continue;

				pop_types(2);
				types.push_back(at.returned_type);
				return;
			}
			error = "Cannot " + op->operation_display_name + " types: left: " + left->name + " right: " + right->name;
		};

		auto handle_unary_operator = [&](const unary_operator_definition* op)
		{
			auto operand = get_type(0);

			for (auto& at : op->allowed_types)
			{
				if (at.operand_type != operand) continue;

				pop_types(1);
				types.push_back(at.returned_type);
				return;
			}
			std::string error = "Cannot " + op->operation_display_name + " type: " + operand->name;
		};

		auto handle_vector_access = [&]()
		{
			auto& type = get_type(0);

			throw_error(!is_vector(type), "Cannot swizzle not-vector type");

			size_t vec_size = get_vector_size(type);

			for (auto& comp : n->as_vector_access_operator())
				if (comp > vec_size)
				{
					error = type->name + " does not have " + std::to_string(comp) + " dimensions";
					return;
				}

			size_t new_vec_size = n->as_vector_access_operator().size();

			pop_types(1);
			types.push_back(get_vector_type_of_size(static_cast<uint8_t>(new_vec_size)));
		};

		auto handle_vector_constructor = [&]()
		{
			const auto& vector_constructor_nodes = n->as_vector_contructor_operator().first;
			auto& created_vector_size = n->as_vector_contructor_operator().second = 0;

			for (int i = 0; i < vector_constructor_nodes; i++)
			{
				auto& type = get_type(i);
				throw_error(type != scalar_data_type && !is_vector(type), "Created vector must consist of scalars and vectors only")
					created_vector_size += get_vector_size(type);
			}

			throw_error(created_vector_size > 4, "Created vector is too long - maximal vector length is 4, created vector length is "
				+ std::to_string(created_vector_size));

			pop_types(vector_constructor_nodes);
			types.push_back(get_vector_type_of_size(created_vector_size));
		};

		auto handle_function = [&]()
		{
			auto& func_name = n->as_function()->first;
			auto& func_def = n->as_function()->second;

			std::vector<const data_type*> arguments_types;

			for (size_t i = func_def.arguments.size() - 1; i != -1; i--)
				arguments_types.push_back(get_type(i));

			auto invalid_arguments_error = [&]()
			{
				auto the_error = "Function " + func_name + " is invalid for arguments: ";
				for (size_t i = 0; i < arguments_types.size(); i++)
				{
					the_error += arguments_types.at(i)->name;
					if (i != arguments_types.size() - 1) the_error += ", ";
				}
				return the_error;
			};

			for (auto& func_instance : func_def.instances)
			{
				if (func_instance.args_matching(arguments_types))
				{
					throw_error(!func_instance.valid, invalid_arguments_error());

					pop_types(func_def.arguments.size());
					types.push_back(func_instance.returned_type);

					exp->used_functions.back().push_back(&func_instance);

					return;
				}
			}

			if (func_def.is_exposed)
				throw_error(true, invalid_arguments_error());

			instantiate_function(
				func_def,
				arguments_types,
				exp->used_functions,
				error
			);

			exp->used_functions.back().push_back(&func_def.instances.back());

			if (error != "")
				error = invalid_arguments_error() + '\n' + error;
			rethrow_error();

			auto& instance = func_def.instances.back();

			pop_types(func_def.arguments.size());
			types.push_back(instance.returned_type);
		};

		switch (n->get_type())
		{
		case node::node_type::scalar_literal:
			types.push_back(scalar_data_type); break;
		case node::node_type::variable:
		{
			auto var = n->as_variable();

			throw_error(var->second.type == nullptr,
				"Cannot use variable " + std::string(var->first) + " because it's type could not be discern");

			types.push_back(var->second.type);
			break;
		}
		case node::node_type::parameter:
		{
			auto param = n->as_parameter();

			throw_error(param->second.type == nullptr,
				"Cannot use parameter " + std::string(param->first) + " because it's type could not be discern");

			types.push_back(param->second.type);
			break;
		}
		case node::node_type::symbol:
		{
			throw_error(domain == nullptr, "Cannot use symbols since the domain is not specified yet");
			types.push_back(n->as_symbol()->type);
			break;
		}
		case node::node_type::binary_operator:
		{
			handle_binary_operator(n->as_binary_operator());
			rethrow_error();
			break;
		}
		case node::node_type::unary_operator:
		{
			handle_unary_operator(n->as_unary_operator());
			rethrow_error();
			break;
		}
		case node::node_type::vector_component_access_operator:
		{
			handle_vector_access();
			rethrow_error();
			break;
		}
		case node::node_type::vector_contructor_operator:
		{
			handle_vector_constructor();
			rethrow_error();
			break;
		}
		case node::node_type::function:
		{
			handle_function();
			rethrow_error();
			break;
		}
		default: static_assert(true, "Unhandled node type");
		}
	}
}

expression* get_expression(
	const std::string& source,
	size_t& iterator,
	const int& indentation,
	int& line_counter,
	variables_collection* variables,
	parameters_collection* parameters,
	function_collection* functions,
	libraries_collection* libraries,
	const context_public_implementation& context,
	std::shared_ptr<const parsed_domain> domain,	//optional, nullptr if symbols are not allowed
	std::string& error
)
{
	std::list<expression::exp_case*> cases;
	std::list<expression::node*> output;
	std::list<expression::node*> operators;

	std::vector<named_variable*> used_vars;
	std::vector<function_instance*> used_funcs;

	expressions_parsing_utilities::shunting_yard(
		source,
		iterator,
		indentation,
		line_counter,
		variables,
		parameters,
		functions,
		libraries,
		context,
		domain,
		cases,
		output,
		operators,
		used_vars,
		error
	);

	if (error != "")
	{
		for (auto& n : output)
			delete n;

		for (auto& n : operators)
			delete n;

		for (auto& e : cases)
			delete e;

		return nullptr;
	}

	return new expression(cases, used_vars, {});
}

const data_type* validate_expression(
	expression* exp,
	const std::shared_ptr<const parsed_domain>& domain,
	std::string& error
)
{
	using node = expression::node;
	using data_type = data_type;

	std::vector<const data_type*> types;

	exp->used_functions.push_back({});

	auto validate_literal_expression = [&](const expression::single_expression* le) -> const data_type*
	{
		for (auto& n : le->nodes)
		{
			expressions_parsing_utilities::validate_node(exp, n, types, domain, error);
			if (error != "") break;
		}

		if (error != "") return nullptr;

		const data_type* type = types.back();
		types.clear();

		return type;
	};

	if (exp == nullptr) return nullptr;

	const data_type* type = nullptr;

	int counter = 1;
	for (auto& exp_case : exp->cases)
	{
		const data_type* value_type = validate_literal_expression(exp_case->value);
		if (error != "") return nullptr;
		if (type == nullptr) type = value_type;
		else if (value_type != type)
		{
			error = "Variable type must be same in all if cases. First expression type: " + type->name + ", "
				"Expression number " + std::to_string(counter) + " type: " + value_type->name;
			return nullptr;
		}

		if (exp_case->condition == nullptr)
		{
			counter++;
			continue;
		}

		const data_type* condition_type = validate_literal_expression(exp_case->condition);
		if (error != "") return nullptr;
		if (condition_type != bool_data_type)
		{
			error = "Conditions must evaluate to bool. Condition number " + std::to_string(counter) + " evaluate to: " + condition_type->name;
			return nullptr;
		}

		counter++;
	}

	return type;
}

void instantiate_function(
	function_definition& func_def,
	const std::vector<const data_type*> arguments,
	decltype(expression::used_functions)& used_functions,
	std::string& error
)
{
	auto itr = func_def.variables.begin();
	for (auto arg = arguments.begin(); arg != arguments.end(); arg++)
	{
		itr->second.type = *arg;
		itr++;
	}

	std::vector<const data_type*> variables_types;

	while (itr != func_def.variables.end())
	{
		auto& var = itr->second;

		std::string error2;

		auto type = validate_expression(var.value, nullptr, error2);
		var.type = type;

		variables_types.push_back(var.type);

		if (error2 != "" && error != "")
			error += '\n';

		if (error2 != "")
		{
			error += '\t';
			error += error2;
		}

		for (auto& func : var.value->used_functions)
			used_functions.push_back(func);

		itr++;
	}

	std::string error2;
	auto type = validate_expression(func_def.returned_value, nullptr, error2);

	for (auto& func : func_def.returned_value->used_functions)
		used_functions.push_back(func);

	func_def.instances.push_back({ &func_def });
	auto& instance = func_def.instances.back();

	instance.valid = error == "";
	instance.arguments_types = arguments;

	if (!instance.valid) return;

	instance.returned_type = type;
}