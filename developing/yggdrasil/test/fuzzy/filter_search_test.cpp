//fuzzy_search_test.cpp

#ifndef _MSC_VER
#error "this file test only msvc !!!"
#endif  //  _MSC_VER

#ifndef __MINGW32__
#	define BOOST_HAS_ICU
#endif // __MINGW32__

#ifndef _MSC_VER
#	error "this file test only msvc"
#endif // _MSC_VER

#include <iostream>
#include <yggr/charset/string.hpp>

#include <yggr/charset/utf8_string.hpp>
#include <yggr/fuzzy/filter_string.hpp>

#include <yggr/regular_parse/regex_parse.hpp>
#include <yggr/regular_parse/u32regex_parse.hpp>

#include <set>

#ifdef _MSC_VER
#	include <vld.h>
#endif // _MSC_VER

void string_fuzzy_match(void) // not support chs
{
	//char init_str[] = "abcd efg"; //ok
	//std::list<char> str(init_str, init_str + strlen(init_str)); //ok
	//std::vector<char> str(init_str, init_str + strlen(init_str)); //ok
	//std::set<char> str(init_str, init_str + strlen(init_str)); //ok
	std::string str("abcd efg");
	std::string strf1;

	try
	{
		strf1 = yggr::fuzzy::filter_string::make_string("e f b,");
		std::cout << strf1 << std::endl;
	}
	catch(yggr::fuzzy::filter_string::error_type& e)
	{
		std::cout << e.what() << std::endl;
		return;
	}

	try
	{
		assert(yggr::regular_parse::regex_parse::match(str, strf1) == 1);
		std::cout << yggr::regular_parse::regex_parse::match(str, strf1) << std::endl; // 1
	}
	catch(yggr::regular_parse::regex_parse::error_type& e)
	{
		std::cout << e.what() << std::endl;
	}

	std::string strf2;
	try
	{
		strf2 = yggr::fuzzy::filter_string::make_string(" k po");
		std::cout << strf2 << std::endl;
	}
	catch(yggr::fuzzy::filter_string::error_type& e)
	{
		std::cout << e.what() << std::endl;
		return;
	}

	try
	{
		assert(yggr::regular_parse::regex_parse::match(str, strf2) == 0);
		std::cout << yggr::regular_parse::regex_parse::match(str, strf2) << std::endl; // 0
	}
	catch(yggr::regular_parse::regex_parse::error_type& e)
	{
		std::cout << e.what() << std::endl;
	}
}

#ifndef __MINGW32__
void utf8_string_fuzzy_match(void) // support chs
{
	yggr::charset::utf8_string str("���,����");

	yggr::charset::utf8_string strf1;
	try
	{
		strf1 = yggr::fuzzy::filter_string::make_u32_string(" �� �� ");
		std::cout << strf1 << std::endl;
	}
	catch(yggr::fuzzy::filter_string::error_type& e)
	{
		std::cout << e.what() << std::endl;
	}

	try
	{
		assert(yggr::regular_parse::u32regex_parse::match(str, strf1) == 1);
		std::cout << yggr::regular_parse::u32regex_parse::match(str, strf1) << std::endl;
	}
	catch(const yggr::regular_parse::u32regex_parse::error_type& e)
	{
		std::cout << e.what() << std::endl;
	}

	yggr::charset::utf8_string strf2;

	try
	{
		strf2 = yggr::fuzzy::filter_string::make_u32_string(" ����");
		std::cout << strf2 << std::endl;
	}
	catch(yggr::fuzzy::filter_string::error_type& e)
	{
		std::cout << e.what() << std::endl;
	}

	try
	{
		assert(yggr::regular_parse::u32regex_parse::match(str, strf2) == 0);
		std::cout << yggr::regular_parse::u32regex_parse::match(str, strf2) << std::endl;
	}
	catch(const yggr::regular_parse::u32regex_parse::error_type& e)
	{
		std::cout << e.what() << std::endl;
	}
}
#endif // __MINGW32__


int main(int argc, char* argv[])
{
	string_fuzzy_match();

#ifndef __MINGW32__
	utf8_string_fuzzy_match();
#endif //__MINGW32__
	char cc = 0;
	std::cin >> cc;
	return 0;
}
