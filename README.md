# Markdown Parser

Simple, efficient, single-header C++ (14) markdown to html converison algorithm. Following features are supported:

- Headings (the # syntax)
- Italic text
- Bold Text
- Italic and bold text
- Blockquotes
- Ordered Lists
- Unordered Lists
- Code
- Horizontal Rule
- Link ( \[title](link) form )
- Image
- Fenced Code Block
- Strikethrough
- Html itself

Parser was implemented according to the rules from this website: <https://www.markdownguide.org/cheat-sheet/>

# Pros
- Fast
- Customizable
- Single Header

# Usage

Parsing mardkown is pretty straightforward: 
```cpp
std::string markdown_snippet = R"(
  ## Foo
  ***Bar***
)";

std::cout << markdown_parsing::markdown_to_html(source);
```
You can also provide a custom set of html tags, that should be used instead of the default one.
In example below we create such with overridden html tags for the code blocks so they are also surrounded by a fieldset frame.
See the ``markdown_parsing::html_tags`` definition to see what you can override (ps: almost everything!)
```cpp
markdown_parsing::html_tags tags;
tags.code_block_tags = tags_pair{ "<fieldset><pre><code>", "</code></pre></fieldset>" };

std::cout << markdown_parsing::markdown_to_html(source, tags);
```
Additionaly, you can add a callback function to handle syntax highlighting in code blocks
```cpp
std::string syntax_highlighting(const std::string& language_name, const std::string& source, size_t code_begin, size_t code_end)
{
	std::string result;

	if (language_name == "cpp")
	{
		result += "<span style=\"color:blue\">";
		result += source.substr(code_begin, code_end - code_begin);
		result += "</span>";
	}

	return result;
}

markdown_parsing::html_tags tags;
tags.syntax_highlighting = syntax_highlighting;

std::cout << markdown_parsing::markdown_to_html(source, tags);
```

# Building
This parser is implemented as a typical single header library.  
Once you have your copy of the ``markdown_parser.hpp``, create a file ``markdown_parser.cpp`` inside your project and paste in the following code:
```cpp
#define MARKDOWN_PARSER_IMPLEMENTATION
#include "markdown_parser.hpp"
```
Once you configure your building environement to build with ``markdown_parser.cpp`` you can use the parser everywhere inside your project.
