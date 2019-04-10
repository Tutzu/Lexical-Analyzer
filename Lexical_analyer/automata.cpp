#include "automata.h"
#include <sstream>

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

size_t pushString(const std::string& token)
{
	auto result = std::find(g_strings.begin(), g_strings.end(), token);

	if (result == g_strings.end())
	{
		g_strings.push_back(token);
		result = g_strings.end() - 1;
	}

	return std::distance(g_strings.begin(), result);
}

const std::string getString(const size_t i)
{
	return g_strings[i];
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

Token::Token(wordType type, size_t stringPos) :
	m_type(type), m_stringPos(stringPos)
{
}

wordType Token::getType() const
{
	return m_type;
}

size_t Token::getStringPos() const
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
	out << getString(token.m_stringPos) << std::endl;

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

		if (it->size() % 2 || it->at(0) < previousPos)
		{
			std::stringstream errMessage;
			errMessage << "Automata config invalid on line " << std::distance(automata_config.begin(), it);

			throw std::exception( errMessage.str().c_str() );
		}

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
	if (currentToken.empty())
	{
		return true;
	}

	//Check if final state
	if (!currentNode->getFinal())
	{
		//not ok, push token to result and terminate

		result.emplace_back(ERROR, pushString(currentToken));

		return false;		//Or return?
	}

	result.emplace_back(findTokenType(currentNode->getValue()), pushString(currentToken));

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
		uint8_t transition = currentNode->transition(input[i]);

		if (COMMENT_START_POS_LINE == transition)
		{
			//We're done with this line, rest of it is a comment. Gotta finish the last token
			currentToken.clear();
			
			break;
		}

		if (COMMENT_START_POS_MULTIPLE == transition ||
			COMMENT_START_POS_MULTIPLE + 1 == transition)
		{
			currentNode = &m_nodes[transition];
			isComment = true;

			continue;
		} 
		else if (0 == transition)
		{
			currentNode = &m_nodes[transition]; //Start over
			isComment = false;
			currentToken.clear();

			continue;
		}

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

		if (!isComment)
		{
			currentToken += input[i];
		}

		currentNode = &m_nodes[transition];
	}

	if (!currentToken.empty())
	{
		(void) finishToken(currentNode, result, currentToken);
	}
	
	return result;
}
