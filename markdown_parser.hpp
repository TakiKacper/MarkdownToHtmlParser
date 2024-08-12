#pragma once

//Define MARKDOWN_PARSER_IMPLEMENTATION to implement the parser inside the compilation unit

#include <string>
#include <array>

namespace markdown_parsing
{
	struct html_tags
	{
		using tags_pair = std::pair<std::string, std::string>;
		using syntax_highlighting_callback = std::string(*)(const std::string& language_name, const std::string& source, size_t code_begin, size_t code_end);

		std::array<tags_pair, 6> heading_tags = {
			tags_pair{ "<h1>", "</h1>\n" },
			tags_pair{ "<h2>", "</h2>\n" },
			tags_pair{ "<h3>", "</h3>\n" },
			tags_pair{ "<h4>", "</h4>\n" },
			tags_pair{ "<h5>", "</h5>\n" },
			tags_pair{ "<h6>", "</h6>\n" },
		};

		tags_pair italic_tags = tags_pair{ "<em>", "</em>" };
		tags_pair bold_tags = tags_pair{ "<strong>", "</strong>" };
		tags_pair blockquote_tags = tags_pair{ "<blockquote>\n", "</blockquote>\n" };
		tags_pair highlight_tags = tags_pair{ "<mark>", "</mark>" };
		tags_pair strikethrough_tags = tags_pair{ "<del>", "</del>" };

		tags_pair ordered_list_tags = tags_pair{ "<ol>", "</ol>" };
		tags_pair ordered_list_item_tags = tags_pair{ "<li>", "</li>" };

		tags_pair unordered_list_tags = tags_pair{ "<ul>", "</ul>" };
		tags_pair unordered_list_item_tags = tags_pair{ "<li>", "</li>" };

		tags_pair code_tags = tags_pair{ "<code>", "</code>" };
		tags_pair code_block_tags = tags_pair{ "<pre><code>", "</code></pre>" };

		tags_pair link_additional_tags = tags_pair{ "", "" };
		tags_pair image_additional_tags = tags_pair("", "");

		std::string horizontal_rule = "<hr>";
		syntax_highlighting_callback syntax_highlighting = nullptr;
	};

	static std::string markdown_to_html(const std::string& markdown, const html_tags& tags = {});
}

#ifdef MARKDOWN_PARSER_IMPLEMENTATION

#include <vector>
#include <sstream>

namespace markdown_parsing
{
	struct list
	{
		bool ordered;
		size_t items_indentation;
		size_t indentation_diffrence;
	};

	struct args
	{
		std::stringstream& html_out;
		const std::string& markdown;
		const html_tags& tags;

		size_t iterator = 0;
		size_t block_begin = 0;

		size_t line_indentation = 0;

		size_t blockquote_level = 0;
		bool is_paragraph_open = 0;
		bool paragraph_ended_with_space = false;

		std::vector<list> ongoing_lists;

		args(
			decltype(html_out) _html_out,
			decltype(markdown) _markdown,
			decltype(tags) _tags
		) :
			html_out(_html_out),
			markdown(_markdown),
			tags(_tags)
		{};
	};

	/*
		Utility
	*/

	static inline size_t count_char(char c, args& _args);
	static inline size_t get_indentation(args& _args);
	static inline bool is_ordered_list_element(args& _args);

	//Close paragraph if possible
	static inline void try_close_paragraph(args& _args);

	static inline void close_all_lists(args& _args);

	/*
		Dumping text
	*/

	//Copy text between block_begin and iterator into output
	static inline void dump_block(args& _args);

	//Dump block wrapper that adds <p> and </p>
	static inline void dump_paragraph(args& _args, bool close_paragraph);

	/*
		Parsing
		By convention each of the following functions should move iterator to the character next to the last character of given sequence
		eg. parse_heading should move itearator to first \n (end of the heading), parse_asteriks should move iterator to 'm' when parsing *abc*m etc.
	*/

	// a markdown line
	// assume iterator -> first char after \n
	static inline void parse_line(args& _args);

	// #abc
	// ### cba
	// also creates anchor with name = heading name with underscores in place of spaces
	// assumes iterator -> first '#'
	static inline void parse_heading(args& _args);

	// *a*, **a**, ***a***
	// assumes iterator -> first '*'
	static inline void parse_asteriks(args& _args);

	// ~~world is flat~~
	// assumes iterator -> char next to second '~' on the left
	static inline void parse_strikethrough(args& _args);

	// 'print("hello world")` or ``print("hello world")``
	// assumes iterator -> '`'
	static inline void parse_code(args& _args, uint8_t backticks_amount);

	// ```py
	// abc = "hello world"
	// print(abc)
	// ```
	// assumes iterator -> char next to last '`'
	static inline void parse_code_block(args& _args);

	// ---
	// assumes iterator -> last '-'
	static inline void parse_horizontal_rule(args& _args);

	// - a
	// + a
	// * a
	// 123. a
	// assumes iterator -> '-', '*', '+' or a first element of number
	static inline void parse_list(args& _args, bool ordered);

	// <link>
	// assumes iterator -> '<'
	static inline void parse_simple_link(args& _args);

	// [title](link)
	// assumes iterator -> '['
	static inline void parse_named_link(args& _args);

	// ![alt text](file)
	// assumes iterator -> '!'
	static inline void parse_image(args& _args);
};

std::string markdown_parsing::markdown_to_html(const std::string& markdown, const html_tags& tags)
{
	std::stringstream html_out;

	args _args{ html_out, markdown, tags };

	//Parse line
	while (_args.iterator < _args.markdown.size())
		parse_line(_args);				//Requires iter to be set to an char next to '\n' after previous call

	//Finish unfinished things
	try_close_paragraph(_args);
	close_all_lists(_args);

	while (_args.blockquote_level != 0)
	{
		_args.html_out << _args.tags.blockquote_tags.second;
		_args.blockquote_level--;
	}

	return html_out.str();
}

#define iter_good (_args.iterator < _args.markdown.size())

size_t markdown_parsing::count_char(char c, args& _args)
{
	size_t iterator_copy = _args.iterator;
	while (iter_good && _args.markdown.at(_args.iterator) == c) _args.iterator++;
	return _args.iterator - iterator_copy;
}

size_t markdown_parsing::get_indentation(args& _args)
{
	size_t indentation = 0;

	while (iter_good)
	{
		const auto& c = _args.markdown.at(_args.iterator);

		switch (c)
		{
		case ' ':  indentation += 1; _args.iterator++; continue;
		case '\t': indentation += 4; _args.iterator++; continue;
		}

		break;
	}

	return indentation;
}

bool markdown_parsing::is_ordered_list_element(args& _args)
{
	size_t iterator_copy = _args.iterator;
	while (iter_good && isdigit(_args.markdown.at(iterator_copy))) iterator_copy++;
	return _args.markdown.at(iterator_copy) == '.' && iterator_copy != _args.iterator;
}

void markdown_parsing::try_close_paragraph(args& _args)
{
	if (_args.is_paragraph_open)
	{
		_args.html_out << "</p>";
		_args.is_paragraph_open = false;
	}
}

void markdown_parsing::close_all_lists(args& _args)
{
	const auto& ordered_list_tags = _args.tags.ordered_list_tags;
	const auto& ordered_list_item_tags = _args.tags.ordered_list_item_tags;

	const auto& unordered_list_tags = _args.tags.unordered_list_tags;
	const auto& unordered_list_item_tags = _args.tags.unordered_list_item_tags;

	while (_args.ongoing_lists.size() != 0)
	{
		_args.html_out << (_args.ongoing_lists.back().ordered ? ordered_list_item_tags : unordered_list_item_tags).second;	//Close list item
		_args.html_out << (_args.ongoing_lists.back().ordered ? ordered_list_tags : unordered_list_tags).second;				//Close list
		_args.ongoing_lists.pop_back();
	};
}

void markdown_parsing::dump_block(args& _args)
{
	_args.html_out << _args.markdown.substr(_args.block_begin, _args.iterator - _args.block_begin);
	_args.block_begin = _args.iterator;
}

void markdown_parsing::dump_paragraph(args& _args, bool close_paragraph)
{
	//Add space between paragraph fragments if there is no space at back
	if (!_args.paragraph_ended_with_space)
	{
		_args.html_out << ' ';
	}

	//If there is nothing to dump
	if (_args.iterator == _args.block_begin)
	{
		if (close_paragraph) try_close_paragraph(_args);
		else if (!_args.is_paragraph_open)
		{
			_args.is_paragraph_open = true;
			_args.html_out << "<p>";
		}
		return;
	}

	//Do not open paragraph if there is one already and if an list is open
	if (!_args.is_paragraph_open
		&& _args.ongoing_lists.size() == 0
		)
	{
		_args.html_out << "<p>";
		_args.is_paragraph_open = true;
	}

	_args.paragraph_ended_with_space = (_args.markdown.at(_args.iterator - 1) == ' ');
	dump_block(_args);

	if (close_paragraph && _args.is_paragraph_open)
	{
		_args.html_out << "</p>";
		_args.is_paragraph_open = false;
	}
}

void markdown_parsing::parse_line(args& _args)
{
	//Get indentation
	_args.line_indentation = get_indentation(_args);

	//Handle Blockquotes
	size_t dashes = count_char('>', _args);

	while (dashes > _args.blockquote_level)
	{
		_args.html_out << _args.tags.blockquote_tags.first;
		_args.blockquote_level++;
	}

	while (dashes < _args.blockquote_level)
	{
		_args.html_out << _args.tags.blockquote_tags.second;
		_args.blockquote_level--;
	}

	//Skip spaces
	//If there are some blockquotes get indentation again to make lists work
	if (dashes != 0)
		_args.line_indentation = get_indentation(_args);

	//Check if line is blank
	if (_args.markdown.at(_args.iterator) == '\n')
	{
		try_close_paragraph(_args);
		//Move iterator to next line
		_args.iterator++;
		return;
	}

	//If there are some opened list and this line in not a list element, then close all ongoing lists
	if (iter_good &&
		_args.ongoing_lists.size() != 0 &&
		_args.markdown.at(_args.iterator) != '-' &&
		_args.markdown.at(_args.iterator) != '+' &&
		_args.markdown.at(_args.iterator) != '*' &&
		!is_ordered_list_element(_args))
	{
		close_all_lists(_args);
	}

	//Mark text begin
	_args.block_begin = _args.iterator;

	//Iterate
	bool is_first_char = true;
	bool ignore_next = false;
	size_t spaces = 0;

	while (iter_good && _args.markdown.at(_args.iterator) != '\n')
	{
		if (ignore_next)
		{
			ignore_next = false;
			_args.iterator++;
			continue;
		}

		const char& c = _args.markdown.at(_args.iterator);

		if (c == ' ') spaces++;
		else spaces = 0;

		if (c == '\\')
		{
			dump_paragraph(_args, false);
			_args.block_begin = _args.iterator + 1;	//Move begin to the next char so the \ does not get pasted
			_args.iterator++;

			is_first_char = false;
			ignore_next = true;
		}
		else if (c == '#' && is_first_char)
		{
			dump_paragraph(_args, true);
			parse_heading(_args);
		}
		else if ((c == '+' || c == '*') && is_first_char)
		{
			parse_list(_args, false);
		}
		else if (c == '*')
		{
			dump_paragraph(_args, false);
			parse_asteriks(_args);
		}
		else if (c == '`')
		{
			auto amount = static_cast<uint8_t>(count_char('`', _args));

			if (amount == 3)
			{
				_args.iterator -= amount;
				dump_paragraph(_args, true);
				_args.iterator += amount;
				parse_code_block(_args);
			}
			else
			{
				_args.iterator -= amount;
				dump_paragraph(_args, false);
				parse_code(_args, amount);
			}
		}
		else if (c == '~')
		{
			size_t amount = count_char('~', _args);
			if (amount == 2)
			{
				_args.iterator -= amount;
				dump_paragraph(_args, false);
				_args.iterator += amount;
				parse_strikethrough(_args);
			}
		}
		else if (c == '-' && is_first_char)
		{
			size_t amount = count_char('-', _args);
			if (amount >= 3)
			{
				_args.iterator -= amount;
				dump_paragraph(_args, true);
				_args.iterator += amount;
				parse_horizontal_rule(_args);
			}
			else
			{
				parse_list(_args, false);
			}
		}
		else if (is_first_char && is_ordered_list_element(_args))
		{
			//Skip to after '.'
			while (_args.markdown.at(_args.iterator) != '.') _args.iterator++;
			_args.iterator++;
			_args.block_begin = _args.iterator;
			parse_list(_args, true);
		}
		/*else if (c == '<')	Removed to make html injections work
		{
			dump_paragraph(_args, false);
			parse_simple_link(_args);
		}*/
		else if (c == '[')
		{
			dump_paragraph(_args, false);
			parse_named_link(_args);
		}
		else if (c == '!')
		{
			dump_paragraph(_args, false);
			parse_image(_args);
		}
		else
		{
			//If this char is not special, move further
			_args.iterator++;
		}

		is_first_char = false;
	}

	//Dump rest of the line, close paragraph only if there are at least two spaces at line end
	dump_paragraph(_args, spaces >= 2);
	_args.iterator++;
}

void markdown_parsing::parse_heading(args& _args)
{
	size_t level = count_char('#', _args);

	if (_args.markdown.at(_args.iterator) != ' ') return; //No space between # and text = no headline
	_args.iterator++;									  //Skip the space though

	if (level > 6) level = 6;

	auto& tags_pair = _args.tags.heading_tags.at(level - 1);

	_args.block_begin = _args.iterator;
	while (iter_good && _args.markdown.at(_args.iterator) != '\n') _args.iterator++;

	std::string anchor_name = _args.markdown.substr(_args.block_begin, _args.iterator - _args.block_begin);
	for (auto& c : anchor_name)
		if (c == ' ') c = '-';

	_args.html_out << "<a name=\"";
	_args.html_out << anchor_name;
	_args.html_out << "\"></a>";

	_args.html_out << tags_pair.first;

	dump_block(_args);

	_args.html_out << tags_pair.second;
}

void markdown_parsing::parse_asteriks(args& _args)
{
	size_t left_asterisk = count_char('*', _args);

	_args.block_begin = _args.iterator;

	while (
		iter_good &&
		_args.markdown.at(_args.iterator) != '*' &&
		_args.markdown.at(_args.iterator) != '\n'
		)
		_args.iterator++;

	if (!iter_good || _args.markdown.at(_args.iterator) == '\n')
	{
		_args.block_begin -= left_asterisk;
		return;
	}

	size_t right_asteriks = count_char('*', _args);

	while (left_asterisk > right_asteriks)
	{
		_args.html_out << '*';
		left_asterisk--;
	}

	uint8_t level = static_cast<uint8_t>(std::min(left_asterisk, right_asteriks));

	_args.iterator -= right_asteriks;

	switch (level)
	{
	case 1:
	{
		_args.html_out << _args.tags.italic_tags.first;
		dump_block(_args);
		_args.html_out << _args.tags.italic_tags.second;
	} break;
	case 2:
	{
		_args.html_out << _args.tags.bold_tags.first;
		dump_block(_args);
		_args.html_out << _args.tags.bold_tags.second;
	} break;
	case 3:
	{
		_args.html_out << _args.tags.italic_tags.first;
		_args.html_out << _args.tags.bold_tags.first;
		dump_block(_args);
		_args.html_out << _args.tags.bold_tags.second;
		_args.html_out << _args.tags.italic_tags.second;
	}
	}

	_args.iterator += right_asteriks;
	_args.block_begin = _args.iterator;

	while (left_asterisk < right_asteriks)
	{
		_args.html_out << '*';
		right_asteriks--;
	}
}

void markdown_parsing::parse_strikethrough(args& _args)
{
	//Iterate till ~~
	_args.block_begin = _args.iterator;

	while (iter_good)
	{
		if (_args.markdown.at(_args.iterator) == '~')
		{
			size_t amount = count_char('~', _args);
			if (amount == 2) break;
		}
		_args.iterator++;
	}

	_args.html_out << _args.tags.strikethrough_tags.first;
	_args.html_out << _args.markdown.substr(_args.block_begin, _args.iterator - _args.block_begin - 2);
	_args.html_out << _args.tags.strikethrough_tags.second;

	_args.block_begin = _args.iterator - 1;
}

void markdown_parsing::parse_code(args& _args, uint8_t backticks_amount)
{
	//Skip `
	while (iter_good && _args.markdown.at(_args.iterator) == '`') _args.iterator++;
	if (!iter_good) return;

	_args.block_begin = _args.iterator;

	size_t code_end;
	while (true)
	{
		while (iter_good && _args.markdown.at(_args.iterator) != '`') _args.iterator++;
		if (!iter_good) return;
		code_end = _args.iterator;
		if (backticks_amount == count_char('`', _args)) break;
	}

	_args.html_out << _args.tags.code_tags.first;
	_args.html_out << _args.markdown.substr(_args.block_begin, code_end - _args.block_begin);
	_args.html_out << _args.tags.code_tags.second;

	_args.block_begin = _args.iterator;
}

void markdown_parsing::parse_code_block(args& _args)
{
	//Get past whitespaces
	while (iter_good && (_args.markdown.at(_args.iterator) == ' ' || _args.markdown.at(_args.iterator) == '\t'))
		_args.iterator++;

	//Get language name
	_args.block_begin = _args.iterator;

	while (
		_args.markdown.at(_args.iterator) != ' ' &&
		_args.markdown.at(_args.iterator) != '\n' &&
		_args.markdown.at(_args.iterator) != '\t'
		) _args.iterator++;

	std::string language_name = _args.markdown.substr(_args.block_begin, _args.iterator - _args.block_begin);

	//Skip whitespaces and first newline
	get_indentation(_args);
	if (_args.markdown.at(_args.iterator) == '\n') _args.iterator++;

	//Iterate till ```
	_args.block_begin = _args.iterator;

	while (iter_good)
	{
		if (_args.markdown.at(_args.iterator) == '`')
		{
			size_t amount = count_char('`', _args);
			if (amount == 3) break;
		}
		_args.iterator++;
	}

	_args.html_out << _args.tags.code_block_tags.first;

	if (_args.tags.syntax_highlighting == nullptr)
	{
		_args.iterator -= 3;
		dump_block(_args);
		_args.iterator += 3;
	}
	else
	{
		_args.html_out << _args.tags.syntax_highlighting(
			language_name,
			_args.markdown,
			_args.block_begin,
			_args.iterator - 3
		);
	}

	_args.html_out << _args.tags.code_block_tags.second;

	_args.block_begin = _args.iterator;
}

void markdown_parsing::parse_horizontal_rule(args& _args)
{
	_args.html_out << _args.tags.horizontal_rule;
	_args.block_begin = _args.iterator;
}

void markdown_parsing::parse_list(args& _args, bool ordered)
{
	//If there are no opened list or this item is nested, then push a new list and this item
	if (_args.ongoing_lists.size() == 0 || _args.ongoing_lists.back().items_indentation < _args.line_indentation)
	{
		//Calculate diffrence in indentation level between this and previous list
		size_t indentation_diffrence = _args.line_indentation - (_args.ongoing_lists.size() == 0 ? 0 : _args.ongoing_lists.back().items_indentation);

		_args.ongoing_lists.push_back({});

		_args.ongoing_lists.back().ordered = ordered;
		_args.ongoing_lists.back().items_indentation = _args.line_indentation;
		_args.ongoing_lists.back().indentation_diffrence = indentation_diffrence;

		const html_tags::tags_pair* list_tags = ordered ?
			&_args.tags.ordered_list_tags :
			&_args.tags.unordered_list_tags;

		const html_tags::tags_pair* list_item_tags = ordered ?
			&_args.tags.ordered_list_item_tags :
			&_args.tags.unordered_list_item_tags;

		_args.html_out << list_tags->first;		 //Open list
		_args.html_out << list_item_tags->first;  //Open list item
	}
	//There are some list and their indentation is greater, then leave those lists
	else if (_args.ongoing_lists.back().items_indentation > _args.line_indentation)
	{
		size_t current_list_indentation = _args.ongoing_lists.back().items_indentation;

		//Remove all that are nested deeper
		while (_args.ongoing_lists.size() != 1 && current_list_indentation > _args.line_indentation)
		{
			current_list_indentation -= _args.ongoing_lists.back().indentation_diffrence;

			const html_tags::tags_pair* list_tags = _args.ongoing_lists.back().ordered ?
				&_args.tags.ordered_list_tags :
				&_args.tags.unordered_list_tags;

			const html_tags::tags_pair* list_item_tags = _args.ongoing_lists.back().ordered ?
				&_args.tags.ordered_list_item_tags :
				&_args.tags.unordered_list_item_tags;

			_args.html_out << list_item_tags->second;	//Close list item
			_args.html_out << list_tags->second;			//Close list

			_args.ongoing_lists.pop_back();
		}

		//Push this item
		const html_tags::tags_pair* list_item_tags = _args.ongoing_lists.back().ordered ?
			&_args.tags.ordered_list_item_tags :
			&_args.tags.unordered_list_item_tags;

		_args.html_out << list_item_tags->second; //Close previous item
		_args.html_out << list_item_tags->first;	 //Open next item
	}
	//Else (there is a list and this item is in the same scope) simply push this item
	else
	{
		const html_tags::tags_pair* list_item_tags = _args.ongoing_lists.back().ordered ?
			&_args.tags.ordered_list_item_tags :
			&_args.tags.unordered_list_item_tags;

		_args.html_out << list_item_tags->second; //Close previous item
		_args.html_out << list_item_tags->first;	 //Open next item
	}

	_args.block_begin++;
	_args.iterator++;
}

void markdown_parsing::parse_simple_link(args& _args)
{
	//Skip <
	_args.iterator++;

	_args.block_begin = _args.iterator;
	while (iter_good && _args.markdown.at(_args.iterator) != '>') _args.iterator++;
	std::string link = _args.markdown.substr(_args.block_begin, _args.iterator - _args.block_begin);

	_args.html_out << _args.tags.link_additional_tags.first;

	_args.html_out << "<a href=\"";
	_args.html_out << link;
	_args.html_out << "\">";
	_args.html_out << link;
	_args.html_out << "</a>";

	_args.html_out << _args.tags.link_additional_tags.second;

	_args.block_begin = _args.iterator;
}

void markdown_parsing::parse_named_link(args& _args)
{
	_args.block_begin = _args.iterator;

	//Skip [
	_args.iterator++;

	//Get title
	size_t title_begin = _args.iterator;
	while (iter_good && _args.markdown.at(_args.iterator) != ']') _args.iterator++;
	std::string title = _args.markdown.substr(title_begin, _args.iterator - title_begin);

	if (_args.markdown.at(_args.iterator) != ']') return;

	//Skip ]
	_args.iterator++;

	if (!iter_good || _args.markdown.at(_args.iterator) != '(')
		return;

	//Skip (
	_args.iterator++;
	if (!iter_good) return;

	//Get link
	_args.block_begin = _args.iterator;
	while (iter_good && _args.markdown.at(_args.iterator) != ')') _args.iterator++;
	std::string link = _args.markdown.substr(_args.block_begin, _args.iterator - _args.block_begin);

	//Skip )
	_args.iterator++;

	_args.html_out << _args.tags.link_additional_tags.first;

	_args.html_out << "<a href=\"";
	_args.html_out << link;
	_args.html_out << "\">";
	_args.html_out << title;
	_args.html_out << "</a>";

	_args.html_out << _args.tags.link_additional_tags.second;

	_args.block_begin = _args.iterator;
}

void markdown_parsing::parse_image(args& _args)
{
	//Skip !
	_args.iterator++;

	get_indentation(_args);

	if (!iter_good || _args.markdown.at(_args.iterator) != '[')
	{
		//Set block begin to '!'
		_args.block_begin = _args.iterator - 1;
		return;
	}

	//Skip [
	_args.iterator++;

	//Get alt_text
	_args.block_begin = _args.iterator;
	while (iter_good && _args.markdown.at(_args.iterator) != ']') _args.iterator++;
	std::string alt_text = _args.markdown.substr(_args.block_begin, _args.iterator - _args.block_begin);

	if (_args.markdown.at(_args.iterator) != ']') return;

	//Skip ]
	_args.iterator++;

	if (!iter_good || _args.markdown.at(_args.iterator) != '(')
		return;

	//Skip (
	_args.iterator++;
	if (!iter_good) return;

	//Get file
	_args.block_begin = _args.iterator;
	while (iter_good && _args.markdown.at(_args.iterator) != ')') _args.iterator++;
	std::string file = _args.markdown.substr(_args.block_begin, _args.iterator - _args.block_begin);

	//Skip )
	_args.iterator++;

	_args.html_out << _args.tags.image_additional_tags.first;

	_args.html_out << "<img src=\"";
	_args.html_out << file;
	_args.html_out << "\" alt=\"";
	_args.html_out << alt_text;
	_args.html_out << "\">";

	_args.html_out << _args.tags.image_additional_tags.second;

	_args.block_begin = _args.iterator;
}

#undef iter_good

#endif // Markdown_Parser_Implementation