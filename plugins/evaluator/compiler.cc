    /*

    Copyright (C) 2003 Stefan Westerfeld <stefan@space.twc.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    */

#include "compiler.h"
#include <assert.h>

using std::vector;
using std::string;

string Compiler::tokenize(Symbols& symbols, const vector<char>& code_without_nl, vector<Token>& tokens)
{
    enum {
	stNeutral,
	stNumber,
	stVariable
    } state = stNeutral;

    string currentToken;

    vector<char> code = code_without_nl;
    code.push_back('\n');

    vector<char>::const_iterator ci = code.begin();
    while (ci != code.end())
    {
	if (state == stNeutral)
	{
	    if (isdigit (*ci) || (*ci == '.'))
	    {
		state = stNumber;
		currentToken = *ci;
	    }
	    else if (isalnum (*ci))
	    {
		state = stVariable;
		currentToken = *ci;
	    }
	    else if (isspace (*ci))
	    {
		// skip whitespace chars
	    }
	    else if (*ci == '+')
	    {
		tokens.push_back(Token(Token::PLUS));
	    }
	    else if (*ci == '*')
	    {
		tokens.push_back(Token(Token::MUL));
	    }
	    else if (*ci == '=')
	    {
		tokens.push_back(Token(Token::EQUALS));
	    }
	    else if (*ci == ';')
	    {
		tokens.push_back(Token(Token::SEMICOLON));
	    }
	    else
	    {
		char s[2] = { *ci, 0 };

		return "can't interpret '" + string(s) + "'";
	    }
	    ci++;
	}
	else if (state == stNumber)
	{
	    if (!(isdigit (*ci) || (*ci == '.')))
	    {
		double value =  atof(currentToken.c_str());
		/* printf("NUMBER: %f\n", value); */
		tokens.push_back(Token(Token::NUMBER, value));
		state = stNeutral;
	    }
	    else
	    {
		currentToken += *ci++;
	    }
	}
    	else if (state == stVariable)
	{
	    if (!isalnum (*ci))
	    {
		Token t(Token::VARIABLE);
		t.reg = symbols.alloc(currentToken);
		tokens.push_back(t);
		state = stNeutral;
	    }
	    else
	    {
		currentToken += *ci++;
	    }
	}
    }
    return "";
}

Compiler::Compiler(Symbols& symbols, const vector<Token>& tokens)
    : symbols(symbols), tokens(tokens)
{
    for(int i = 0; i < tokens.size(); i++)
	done.push_back(-1);
}

int Compiler::compile(int begin, int size, vector<Instruction>& instructions)
{
    int reg = -1;
    int end = begin+size;

    printf("compiling [%d:%d] : ", begin, end);
    for(int i=begin;i<end;i++)
    {
	printf("<%s> ",tokens[i].str().c_str());
    }
    printf("\n");

    if (size == 1)
    {
	if (tokens[begin].type == Token::NUMBER)
	{
	    done[begin] = reg = symbols.alloc();
	    instructions.push_back (Instruction::rv(Instruction::SET, reg, tokens[begin].value));
	}
	else if (tokens[begin].type == Token::VARIABLE)
	{
	    // TODO: optimization pass: to remove redundant moves
	    done[begin] = reg = symbols.alloc();
	    instructions.push_back (Instruction::rr(Instruction::MOVE, reg, tokens[begin].reg));
	}
	else
	{
	    assert (false);
	}
    }
    else for(;;)
    {
	/* find highest precedent operator for which no code has been generated yet */
	int best = -1;
	for(int i = begin; i < begin+size; i++)
	{
	    if (done[i] == -1 && tokens[i].is_operator())
	    {
		if (tokens[i].operator_precedence() > tokens[best].operator_precedence())
		    best = i;
	    }
	}

	printf("best is %d\n", best);
	if (best == -1)
	    break;

	if (tokens[best].type == Token::MUL)
	{
	    int reg1 = compile(begin,best-begin,instructions);
	    int reg2 = compile(best+1,end-best-1,instructions);
	    instructions.push_back (Instruction::rr(Instruction::MUL, reg1, reg2));
	    done[best] = reg = reg1;
	}
	else if (tokens[best].type == Token::PLUS)
	{
	    int reg1 = compile(begin,best-begin,instructions);
	    int reg2 = compile(best+1,end-best-1,instructions);
	    instructions.push_back (Instruction::rr(Instruction::ADD, reg1, reg2));
	    done[best] = reg = reg1;
	}
	else if(tokens[best].type == Token::EQUALS)
	{
	    // CHECK that reg1 is an LVALUE
	    assert(best-begin == 1 && tokens[begin].type == Token::VARIABLE);
	    int reg1 = tokens[begin].reg;
	    int reg2 = compile(best+1,end-best-1,instructions);
	    instructions.push_back (Instruction::rr(Instruction::MOVE, reg1, reg2));
	    done[best] = reg = reg1;
	}
	else
	{
	    fprintf(stderr, "can't compile code for token %s\n", tokens[best].str().c_str());
	    assert(false);
	}
    }
    assert(reg != -1 || size == 0);
    return reg;

    /*
    // evaluate expression:
    // 1. treeify
    // 2. evaluate LHS => tmp1
    // 3. evaluate RHS => tmp2
    // 4. result = OP tmp1 tmp2

    if (tokens[0].type == Token::NUMBER
    &&  tokens[1].type == Token::PLUS
    &&  tokens[2].type == Token::NUMBER)
    {
	int reg1 = symbols.alloc();
	int reg2 = symbols.alloc();

	instructions.push_back (Instruction::rv(Instruction::SET, reg1, tokens[0].value));
	instructions.push_back (Instruction::rv(Instruction::SET, reg2, tokens[2].value));
	instructions.push_back (Instruction::rr(Instruction::ADD, reg1, reg2));

	// result in reg1
    }
    */
}

string Compiler::compile(Symbols& symbols, const vector<Token>& tokens, vector<Instruction>& instructions)
{
    Compiler c(symbols, tokens);

    int reg = c.compile(0, tokens.size(), instructions);
    return "";
}
