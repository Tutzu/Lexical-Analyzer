#include "automata.h"
#include <iostream>
//------------------------Definitions



const static std::unordered_set<uint8_t> keyword_states = 
{
	29, 56
};


const static std::unordered_set<uint8_t> operator_states = 
{
	15, 16, 17, 18, 19, 20, 21, 22, 23
};

const static std::unordered_set<uint8_t> separator_states =
{
	125
};

const static std::unordered_set<uint8_t> constant_states = 
{
	4, 8, 9, 10, 12, 14
};


static std::vector<std::string> g_strings;

//------------------------Methods

std::vector<std::string>::const_iterator pushString(const std::string& token)
{
	std::vector<std::string>::const_iterator result = std::find(g_strings.begin(), g_strings.end(), token);

	if (result == g_strings.cend())
	{
		g_strings.push_back(token);
		result = g_strings.cend() - 1;
	}

	std::cout << *result << std::endl;

	return result;
}

char getSymbol(const char& x)
{
	if (isalpha(x) || x == '_')
		return 'W';	//letter or _

	if (isalnum(x))
		return 'N';	//number

	return '$';	//anything
}

//------------------------Token

Token::Token(wordType type, std::vector<std::string>::const_iterator stringPos) :
	m_type(type), m_stringPos(stringPos)
{

}

wordType Token::getType() const
{
	return m_type;
}

std::vector<std::string>::const_iterator Token::getStringPos() const
{
	return m_stringPos;
}

std::ostream & operator<<(std::ostream & out, const Token& token) noexcept
{
	out <<	(token.m_type == ERROR ? "Error identifying token type for " :
			(token.m_type == OPERATOR ? "{Operator} " :
			(token.m_type == KEYWORD ? "{Keyord} " :
			(token.m_type == SEPARATOR ? "{Separator} " :
			(token.m_type == IDENTIFIER ? "{Identifier} " : "{Constant} ")))));
	out << *(token.m_stringPos) << std::endl;

	return out;
}


//------------------------Node

Node::Node(const std::vector<uint8_t>& config)
{
	m_value = config[VALUE_POS];
	m_isFinal = (bool)config[IS_FINAL_POS];

	for (size_t i = TRANSITION_POS; i < config.size(); i += 2)
	{
		m_states.insert({ config[i], config[i + 1] });
	}

	/*for (auto it = m_states.begin(); it != m_states.end(); it++)
	{
		std::cout << it->first << " -> " << (int)it->second << std::endl;
	}*/

}

uint8_t Node::getValue() const
{
	return m_value;
}

bool Node::getFinal() const
{
	return m_isFinal;
}

uint8_t Node::transition(const char& key) const
{
	auto result = m_states.find(key);

	if (result == m_states.end())
	{
		char symbol = getSymbol(key);

		result = m_states.find(symbol);
		
		if (result == m_states.end())
		{
			result = m_states.find('$');

			if (result == m_states.end())
			{
				return TRANSITION_ERROR;
			}
		}
	}

	return result->second;
}

//------------------------Automata

Automata::Automata(): DUMMY_NODE(std::vector<uint8_t>{DUMMY_NODE_VALUE, 0})
{
	uint8_t previousPos = 0;
	for (auto it = automata_config.begin(); it != automata_config.end(); ++it)
	{

		if (it->size() % 2)
		{
			throw new std::exception("Automata config invalid on line " + it->size());
		}
		
		auto smth = it -> at(0);

		for (uint8_t i = previousPos + 1; i < it->at(0); i++)
		{
			m_nodes.push_back(DUMMY_NODE);	//gapfiller
		}

		previousPos = it->at(0);

		m_nodes.emplace_back(*it);
	}
}

wordType Automata::findTokenType(const uint8_t& state) const
{
	if (keyword_states.find(state) != keyword_states.end())
	{
		return KEYWORD;
	}

	if (operator_states.find(state) != operator_states.end())
	{
		return OPERATOR;
	}

	if (separator_states.find(state) != separator_states.end())
	{
		return SEPARATOR;
	}

	if (constant_states.find(state) != constant_states.end())
	{
		return CONSTANT;
	}

	return IDENTIFIER;
}

bool Automata::finishToken(const Node*& currentNode, std::vector<Token>& result, std::string& currentToken) const
{	
	//Check if final state
	if (!currentNode->getFinal())
	{
		//not ok, push token to result and terminate
		//currentToken.pop_back();

		result.emplace_back(ERROR, pushString(currentToken));

		return false;		//Or return?
	}

	result.emplace_back(findTokenType(currentNode->getValue()), pushString(currentToken));

	std::cout << *(result.cend() - 1);

	currentToken.clear();
	currentNode = &m_nodes[0];

	return true;
}

std::vector<Token> Automata::getTokens(const std::string& input, bool& isComment) const
{
	std::vector<Token> result;
	std::string currentToken;

	
	size_t length = input.length();
	
	//If isComment is true, start directly from 252 (COMMENT_START_POS_MULTIPLE) (lambda transitions form 0 to 252)
	const Node *currentNode = (isComment ? &m_nodes[COMMENT_START_POS_MULTIPLE] : &m_nodes[0]);

	

	//Discard starting spaces and tabs
	size_t i = 0;
	for (i = 0; i < length && (' ' == input[i] || '\t' == input[i]); i++)
	{ }

	for(; i < length; i++)
	{
		
		//If there is a '/' and we're not in state 0, finish the token and start over from 0 (possible comment handling)
		if ('/' == input[i] && 0 != currentNode->getValue())
		{
			if (false == finishToken(currentNode, result, currentToken))
			{
				break;
			}

			--i;

			continue;
		}

		uint8_t transition = currentNode->transition(input[i]);

		if (TRANSITION_ERROR == transition)	//No valid transition
		{
			////Check if final state
			if (false == finishToken(currentNode, result, currentToken))
			{//!!
				break;
			}

			if (input[i] != ' ' && input[i] != '\t')
			{
				--i;	//So we don't skip this character
			}

			continue;
		}

		if (DUMMY_NODE == m_nodes[transition])
		{
			//If transition to DUMMY_NODE node is made, config file is wrong. throw error.
			throw new std::exception("Config file is invalid. DUMMY_NODE reached.");
		}

		currentToken += input[i];
		currentNode = &m_nodes[transition];

		if (COMMENT_START_POS_MULTIPLE == transition)
		{
			isComment = true;
		}
		else if (0 == transition)
		{
			//If transition to 0 is made, multiple line comment is over
			isComment = false;
		}
	}

	if (!currentToken.empty())
	{
		(void)finishToken(currentNode, result, currentToken);
	}

	//TODO: FIXME
	for (auto it = result.cbegin(); it != result.cend(); it++)
	{
		std::cout << *it << std::endl;
	}
	
	return result;
}
