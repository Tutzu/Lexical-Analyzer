#pragma once

#include <unordered_map>
#include <unordered_set>
#include <iostream>

#include <vector>
#include <string>

#include "automata_config.h"

extern const std::unordered_set<uint8_t> keyword_states;
extern const std::unordered_set<uint8_t> operator_states;
extern const std::unordered_set<uint8_t> separator_states;
extern const std::unordered_set<uint8_t> constant_states;

extern std::vector<std::string> g_strings;

std::vector<std::string>::const_iterator pushString(const std::string&);

enum wordType
{
	KEYWORD,
	OPERATOR,
	SEPARATOR,
	IDENTIFIER,
	CONSTANT,
	ERROR
};

char getSymbol(const char&);

//------------------------

class Token
{
	wordType m_type;
	std::vector<std::string>::const_iterator m_stringPos;

public:
	Token(wordType, std::vector<std::string>::const_iterator);

	wordType getType() const;
	std::vector<std::string>::const_iterator getStringPos() const;

	friend std::ostream& operator<<(std::ostream& out, const Token&) noexcept;
};


class Node
{
	uint8_t m_value;
	bool m_isFinal;
	std::unordered_map<char, uint8_t> m_states;

public:
	Node(const std::vector<uint8_t>&);

	uint8_t getValue() const;
	bool getFinal() const;

	bool operator==(const Node& rhs) const
	{
		return this->m_value == rhs.m_value;
	}

	uint8_t transition(const char&) const;
};


class Automata
{
	std::vector<Node> m_nodes;
	const Node DUMMY_NODE;

public:
	Automata();

	wordType findTokenType(const uint8_t&) const;
	bool finishToken(const Node*&, std::vector<Token>&, std::string&) const;
	std::vector<Token> getTokens(const std::string&, bool&) const;
};