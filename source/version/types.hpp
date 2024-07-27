#pragma once

const std::vector<data_type> data_types
{
	{ "bool", },
	{ "scalar", },
	{ "vector2", },
	{ "vector3", },
	{ "vector4", },
	{ "texture", }
};

inline bool is_vector(const data_type* type)
{
	if (type == get_data_type({ "vector2" })) return true;
	if (type == get_data_type({ "vector3" })) return true;
	if (type == get_data_type({ "vector4" })) return true;

	return false;
}

inline uint8_t get_vector_size(const data_type* type)
{
	if (type == get_data_type({ "scalar" })) return 1;
	if (type == get_data_type({ "vector2" })) return 2;
	if (type == get_data_type({ "vector3" })) return 3;
	if (type == get_data_type({ "vector4" })) return 4;

	return 0;
}

inline const data_type* get_vector_type_of_size(uint8_t size)
{
	switch (size)
	{
	case 1: return get_data_type({ "scalar" });
	case 2: return get_data_type({ "vector2" });
	case 3: return get_data_type({ "vector3" });
	case 4: return get_data_type({ "vector4" });
	}

	return nullptr;
}

inline const data_type* const get_data_type(const string_view& name)
{
	for (auto& type : data_types)
		if (type.name == name)
			return &type;
	return nullptr;
}