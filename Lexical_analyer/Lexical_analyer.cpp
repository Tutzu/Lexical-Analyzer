#include <iostream>
#include <fstream>
#include <exception>
#include <stdexcept>

#include "automata.h"
//Preproccessor directives are resolved before the lexical analysis. Ignore that

//Comments need to be removed

class Analyzer
{
	Automata m_automata;
	std::ifstream in;
	std::ofstream out;

	bool m_isComment;
public:
	Analyzer(const std::string, const std::string);
	~Analyzer();

	void run();
};

Analyzer::Analyzer(const std::string input, const std::string output):m_isComment(0)
{
	try {
		in.open(input);
	}
	catch (...)
	{
		std::cout << "Could not open input file.\n";
		
		return;
	}

	try {
		out.open(output);
	}
	catch (...)
	{
		std::cout << "Could not open output file.\n";
		in.close();

		return;
	}
}

Analyzer::~Analyzer()
{
	in.close();
	out.close();
}

void Analyzer::run()
{
	std::string buffer;
	uint32_t line = 0;

	while (in.good())
	{
		buffer.clear();
		std::getline(in, buffer);
		line++;

		//Preprocessor directives (pramgas, includes, conditional compiling) are not handled by the lexical analyzer. Skip those lines.
		if (buffer[0] == '#')
		{
			continue;
		}

		auto res = m_automata.getTokens(buffer, m_isComment);

		for (auto it = res.cbegin(); it != res.cend(); it++)
		{
			std::cout << *it << std::endl;

			if (it->getType() == wordType::ERROR)
			{
				std::cout << "ERROR on line " << line << ". Word parsed so far: "
					<< it->getStringPos() << std::endl;
				return;
			}
		}
	}
}

int main()
{
	try {
		Analyzer analyzer("data.in", "data.out");
		analyzer.run();
	}
	catch (std::exception& e)
	{
		std::cout << "Program failed: " << e.what() << std::endl;
		
		return -1;
	}
	

	std::cout << "File analyzed successfully\n";

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
