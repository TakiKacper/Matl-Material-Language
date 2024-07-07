#pragma once

const uint8_t logical_operators_precedence_begin = 1;
const uint8_t numeric_operators_precedence_begin = 4;

const uint8_t functions_precedence = numeric_operators_precedence_begin + 2;

//unary operators; symbol, display name, precedence, {input type, output type}
const std::vector<unary_operator> unary_operators
{
	{ "-", "negate", numeric_operators_precedence_begin + 3, {
		{"scalar", "scalar"},
		{"vector2", "vector2"},
		{"vector3", "vector3"},
		{"vector4", "vector4"}
	}},

	{ "not", "negate", logical_operators_precedence_begin + 1, {
		{"bool", "bool"},
	} },
};

//binary operators; symbol, display name, precedence, {input type left, input type right, output type}
const std::vector<binary_operator> binary_operators
{
	{ "and", "conjunct", logical_operators_precedence_begin, {
		{"bool", "bool", "bool"}
	} },

	{ "or", "alternate", logical_operators_precedence_begin, {
		{"bool", "bool", "bool"}
	} },

	{ "xor", "exclusively alternate", logical_operators_precedence_begin, {
		{"bool", "bool", "bool"}
	} },

	{ "==", "compare with ==", logical_operators_precedence_begin + 1, {
		{"scalar", "scalar", "bool"},
		{"bool", "bool", "bool"}
	} },

	{ "!=", "compare with !=", logical_operators_precedence_begin + 1, {
		{"scalar", "scalar", "bool"},
		{"bool", "bool", "bool"}
	} },

	{ "<", "compare with <", logical_operators_precedence_begin + 2, {
		{"scalar", "scalar", "bool"}
	} },

	{ ">", "compare with >", logical_operators_precedence_begin + 2, {
		{"scalar", "scalar", "bool"}
	} },

	{ "<=", "compare with <=", logical_operators_precedence_begin + 2, {
		{"scalar", "scalar", "bool"}
	} },

	{ ">=", "compare with >=", logical_operators_precedence_begin + 2, {
		{"scalar", "scalar", "bool"}
	} },

	{ "+", "add", numeric_operators_precedence_begin, {
		{"scalar", "scalar", "scalar"},

		{"scalar", "vector2", "vector2"},
		{"vector2", "scalar", "vector2"},
		{"vector2", "vector2", "vector2"},

		{"scalar", "vector3",  "vector3"},
		{"vector3", "scalar",  "vector3"},
		{"vector3", "vector3", "vector3"},

		{"scalar", "vector4",  "vector4"},
		{"vector4", "scalar",  "vector4"},
		{"vector4", "vector4", "vector4"}
	} },

	{ "-", "substract", numeric_operators_precedence_begin, {
		{"scalar", "scalar", "scalar"},

		{"vector2", "scalar", "vector2"},
		{"vector2", "vector2", "vector2"},

		{"vector3", "scalar", "vector3"},
		{"vector3", "vector3", "vector3"},

		{"vector4", "scalar", "vector4"},
		{"vector4", "vector4", "vector4"}
	} },

	{ "*", "multiply", numeric_operators_precedence_begin + 1, {
		{"scalar", "scalar", "scalar"},

		{"scalar", "vector2", "vector2"},
		{"vector2", "scalar", "vector2"},
		{"vector2", "vector2", "vector2"},

		{"scalar", "vector3", "vector3"},
		{"vector3", "scalar", "vector3"},
		{"vector3", "vector3", "vector3"},

		{"scalar", "vector4", "vector4"},
		{"vector4", "scalar", "vector4"},
		{"vector4", "vector4", "vector4"}
	} },

	{ "/", "divide", numeric_operators_precedence_begin + 1, {
		{"scalar", "scalar", "scalar"},

		{"vector2", "scalar", "vector2"},
		{"vector2", "vector2", "vector2"},

		{"vector3", "scalar", "vector3"},
		{"vector3", "vector3", "vector3"},

		{"vector4", "scalar", "vector4"},
		{"vector4", "vector4", "vector4"}
	} },
};

//names of vector components; used to access vector components eg. normal_vector.x; base_color.rgb
const std::unordered_map<char, uint8_t> vector_components_names = {
	{ 'x', 1 },
	{ 'y', 2 },
	{ 'z', 3 },
	{ 'w', 4 },

	{ 'r', 1 },
	{ 'g', 2 },
	{ 'b', 3 },
	{ 'a', 4 }
};

inline const unary_operator* const get_unary_operator(const string_ref& symbol)
{
	for (auto& op : unary_operators)
		if (op.symbol == symbol)
			return &op;
	return nullptr;
}

inline const binary_operator* const get_binary_operator(const string_ref& symbol)
{
	for (auto& op : binary_operators)
		if (op.symbol == symbol)
			return &op;
	return nullptr;
}