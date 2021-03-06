#include <iostream>
#include <fstream>
#include <exception>
#include <stdexcept>

#include "automata.h"
//Preproccessor directives are resolved before the lexical analysis. Ignore that

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
	in.open(input);

	if(!in.is_open())
	{
		throw std::exception("Could not open input file.\n");
	}

	out.open(output);

	if(!out.is_open())
	{
		in.close();

		throw std::exception( "Could not open output file.\n") ;
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
//			out << *it << std::endl;

			if (it->getType() == wordType::ERROR)
			{
				std::cout << "ERROR on line " << line << ". Word parsed so far: "
					<< getString( it->getStringPos() ) << std::endl;
				return;
			}
		}
	}

	if (m_isComment)	//Multiple line comment at end of file is not ended. Error.
	{
		throw std::exception("Multiple-line comment did not end.");
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
