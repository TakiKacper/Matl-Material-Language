#pragma once
//matl parser api, exposed to client

namespace matl
{
	std::string get_language_version();

	struct parsed_material
	{
		bool success = false;

		std::list<std::string> sources;
		std::list<std::string> errors;

		struct parameter
		{
			std::string name;

			enum class type : uint8_t
			{
				boolean,
				scalar,
				vector2,
				vector3,
				vector4,
				texture
			} type;

			std::list<float> numeric_default_value;
			std::string		 texture_default_value;
		};

		std::list<parameter> parameters;
	};

	struct domain_parsing_raport
	{
		bool success = false;

		std::list<std::string> errors;
	};

	struct library_parsing_raport
	{
		bool success = false;

		std::string library_name;
		std::list<std::string> errors;
	};

	using custom_using_case_callback = void(std::string args, std::string& error);
	using dynamic_library_parse_request_handle = const std::string* (const std::string& lib_name, std::string& error);

	class context;

	context* create_context(std::string target_language);
	void destroy_context(context*);

	parsed_material parse_material(const std::string& material_source, matl::context* context);
	std::list<matl::library_parsing_raport> parse_library(const std::string library_name, const std::string& library_source, matl::context* context);
	domain_parsing_raport parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);
}

class matl::context
{
private:
	struct implementation;
	implementation* impl;
	friend parsed_material matl::parse_material(const std::string& material_source, matl::context* context);
	friend std::list<matl::library_parsing_raport> matl::parse_library(const std::string library_name, const std::string& library_source, matl::context* context);
	friend domain_parsing_raport matl::parse_domain(const std::string domain_name, const std::string& domain_source, matl::context* context);

public:
	void add_domain_insertion(std::string name, std::string insertion);
	void add_custom_using_case_callback(std::string _case, custom_using_case_callback callback);
	void set_dynamic_library_parse_request_handle(dynamic_library_parse_request_handle handle);

private:
	context();
	~context();

	friend context* matl::create_context(std::string target_language);
	friend void matl::destroy_context(matl::context* context);
};