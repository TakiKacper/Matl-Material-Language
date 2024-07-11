#pragma once

struct parsed_libraries_stack
{
private:
	std::vector<string_ref> libraries_stack;
public:
	inline void push_parsed_lib(const std::string& lib_name, std::string& error);
	inline void pop();
	inline bool is_empty() { return libraries_stack.size() == 0; };
};

void parsed_libraries_stack::push_parsed_lib(const std::string& lib_name, std::string& error)
{
	auto gen_error = [&]()
	{
		error = "Circular libraries inclusion: \n";
		for (auto itr = libraries_stack.begin(); itr != libraries_stack.end(); itr++)
		{
			error += '\t' + std::string(*itr);
			if (itr != libraries_stack.end() - 1) error += '\n';
		}

		return error;
	};

	for (auto& lib : libraries_stack)
	{
		if (lib == lib_name)
			gen_error();
		rethrow_error();
	}

	libraries_stack.push_back({ lib_name });
}

void parsed_libraries_stack::pop()
{
	libraries_stack.pop_back();
}

struct library_parsing_state
{
	size_t iterator = 0;
	int line_counter = 0;
	int this_line_indentation_spaces = 0;
	bool function_body = false;

	std::list<std::string> errors;
	std::shared_ptr<parsed_libraries_stack>	 parsed_libs_stack = nullptr;
	std::list<matl::library_parsing_raport>* parsing_raports;

	function_collection functions;
	libraries_collection libraries;

	string_ref library_name{ "" };
};


using library_keyword_handle = void(*)(
	const std::string& material_source,
	context_public_implementation& context,
	library_parsing_state& state,
	std::string& error
	);

namespace library_keywords_handles
{
	void let(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
	void property(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
	void _using(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
	void func(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
	void _return(const std::string&, context_public_implementation&, library_parsing_state&, std::string&);
}

heterogeneous_map<std::string, library_keyword_handle, hgm_string_solver> library_keywords_handles_map =
{
	{
		{"let",			library_keywords_handles::let},
		{"property",	library_keywords_handles::property},
		{"using",		library_keywords_handles::_using},
		{"func",		library_keywords_handles::func},
		{"return",		library_keywords_handles::_return}
	}
};

void parse_library_implementation(
	const std::string library_name,
	const std::string& library_source,
	context_public_implementation* context,
	std::shared_ptr<parsed_libraries_stack> stack,
	std::list<matl::library_parsing_raport>* parsing_raports
)
{
	{
		std::string error;
		stack->push_parsed_lib(library_name, error);
		if (error != "")
		{
			matl::library_parsing_raport raport;
			raport.library_name = library_name;
			raport.success = false;
			raport.errors = { "[0] " + error };
			parsing_raports->push_back(raport);
			return;
		}
	}

	auto parsed = std::make_shared<parsed_library>();

	library_parsing_state state;
	state.parsed_libs_stack = stack;
	state.library_name = library_name;
	state.parsing_raports = parsing_raports;

	while (!is_at_source_end(library_source, state.iterator))
	{
		state.line_counter++;

		std::string error;

		auto& source = library_source;
		auto& iterator = state.iterator;

		int spaces = get_spaces(source, iterator);
		if (is_at_source_end(library_source, state.iterator)) break;

		if (source.at(iterator) == comment_char) goto _parse_library_next_line;
		if (is_at_line_end(source, iterator)) goto _parse_library_next_line;

		if (state.function_body)
		{
			if (spaces == 0 && state.this_line_indentation_spaces == 0)
			{
				error = "Expected function body";
				state.function_body = false;
				goto _parse_library_handle_error;
			}

			if (spaces == 0 && state.this_line_indentation_spaces != 0)
			{
				error = "Unexpected function end";
				state.function_body = false;
				goto _parse_library_handle_error;
			}

			if (spaces != state.this_line_indentation_spaces && state.this_line_indentation_spaces != 0)
			{
				error = "Indentation level must be consistient";
				goto _parse_library_handle_error;
			}
		}
		else if (spaces != 0)
		{
			error = "Indentation level must be consistient";
			goto _parse_library_handle_error;
		}

		state.this_line_indentation_spaces = spaces;

		{
			string_ref keyword = get_string_ref(source, iterator, error);

			if (error != "") goto _parse_library_handle_error;

			auto kh_itr = library_keywords_handles_map.find(keyword);

			if (kh_itr == library_keywords_handles_map.end())
			{
				error = "Unknown keyword: " + std::string(keyword);
				goto _parse_library_handle_error;
			}

			kh_itr->second(library_source, *context, state, error);
			if (error != "") goto _parse_library_handle_error;
		}

	_parse_library_next_line:
		if (is_at_source_end(library_source, state.iterator)) break;
		get_to_new_line(library_source, state.iterator);

		continue;

	_parse_library_handle_error:
		state.errors.push_back('[' + std::to_string(state.line_counter) + "] " + std::move(error));

		if (is_at_source_end(library_source, state.iterator)) break;
		get_to_new_line(library_source, state.iterator);
	}

	parsed->functions = std::move(state.functions);

	matl::library_parsing_raport raport;
	raport.library_name = library_name;

	if (state.errors.size() == 0)
	{
		raport.success = true;
		context->libraries.insert({ library_name, parsed });
	}
	else
	{
		raport.success = false;
		raport.errors = std::move(state.errors);
	}

	stack->pop();
	parsing_raports->push_back(raport);
}

std::list<matl::library_parsing_raport> matl::parse_library(const std::string library_name, const std::string& library_source, matl::context* context)
{
	if (context == nullptr)
	{
		matl::library_parsing_raport raport;
		raport.success = false;
		raport.errors = { "[0] Cannot parse library without context" };
		raport.library_name = library_name;
		return { raport };
	}

	std::list<matl::library_parsing_raport> raports;
	parse_library_implementation(
		library_name,
		library_source,
		&context->impl->impl,
		std::make_shared<parsed_libraries_stack>(),
		&raports
	);

	return raports;
}

void library_keywords_handles::let
(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	if (!state.function_body)
		throw_error(true, "Cannot declare variables in library");

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto var_name = get_string_ref(source, iterator, error);
	rethrow_error();

	if (var_name.at(0) == symbols_prefix)
		error = "Cannot declare symbols in material";

	get_spaces(source, iterator);
	auto assign_operator = get_char(source, iterator);

	if (assign_operator != '=')
		error = "Expected '='";

	rethrow_error();

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

void library_keywords_handles::property
(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	throw_error(true, "Cannot use property inside library");
}

void library_keywords_handles::_using
(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	throw_error(state.function_body, "Cannot use using in this scope");

	auto& iterator = state.iterator;

	get_spaces(source, iterator);
	auto using_type = get_string_ref(source, iterator, error);
	rethrow_error();

	if (using_type == "domain")
	{
		throw_error(true, "Cannot use domain inside libraries");
	}
	else if (using_type == "library")
	{
		get_spaces(source, iterator);
		auto library_name = get_rest_of_line(source, iterator);

		auto itr = context.libraries.find(library_name);
		throw_error(itr == context.libraries.end(), "No such library: " + std::string(library_name));

		if (itr == context.libraries.end() && context.dlprh != nullptr)
		{
			error = "";

			const std::string* nested_lib_source = context.dlprh(library_name, error);
			rethrow_error();
			throw_error(nested_lib_source == nullptr, "No such library: " + std::string(library_name));

			parse_library_implementation(library_name, *nested_lib_source, &context, state.parsed_libs_stack, state.parsing_raports);

			throw_error(!state.parsing_raports->back().success, error);

			itr = context.libraries.find(library_name);
		}

		rethrow_error();

		state.libraries.insert({ library_name, itr->second });
	}
	else if (using_type == "parameter")
	{
		throw_error(true, "Cannot use parameters inside libraries");
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

void library_keywords_handles::func
(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	get_spaces(source, state.iterator);
	std::string func_name = get_string_ref(source, state.iterator, error);
	throw_error(func_name == "", "Expected function name");
	rethrow_error();

	is_name_unique(
		func_name,
		nullptr,
		nullptr,
		nullptr,
		&state.functions,
		context,
		&state.libraries,
		error
	);
	rethrow_error();

	handles_common::func(func_name, source, context, state, error);
	rethrow_error();
}

void library_keywords_handles::_return
(const std::string& source, context_public_implementation& context, library_parsing_state& state, std::string& error)
{
	handles_common::_return(source, context, state, error);
}
