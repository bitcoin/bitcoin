#!/usr/bin/env python
'''
This tool eliminates the need for "using namespace std" from c++ files by
prepending "std::" in front of all standard library types
from files passed on command line.

Usage:  remove-using-namespace-std.py [filepath ...]

Limitations:
- makes no backups of modified files
- modifies in place
- does not care what files you pass to it
- assumes it can keep the entire file in memory

Always use only on files that are under version control!

Copyright (c) 2017 The Bitcoin Unlimited developers
Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''

import sys
import re

# REVISIT: Add keywords as they are found to be necessary
# Currently not listing the entire list of keywords in the standard library
# NOTE: The basic regular expression available with Python doesn't allow variable
#       length lookbacks, so the below cases don't accurately cover all possiblities
#       as there could be an arbitrary amount of whitespace, including newlines
#       between a variable/method name and the precursor symbol (i.e. ., ->, [, &)
# The below lookbacks only cover the most common options
keywords = r"(?<!::)".join((      # Ignore keywords already prefixed with ::
			r"(?<!\.)",           # Ignore keywords prefixed with . (member access)
			r"(?<!->)",           # Ignore keywords prefixed with -> (dereference)
			r"(?<!&)",            # Ignore keywords prefixed with "&"
			r"(?<!&\s)",          # Ignore keywords prefixed with "& "
			r"(?<!\[)",           # Ignore keywords prefixed with "["
			r"(?<!\[\s)",         # Ignore keywords prefixed with "[ "
			r"\b(",               # Start with word break
			r"abs|",
			r"advance|",
			r"atomic|",
			r"back_inserter|",
			r"cin|",
			r"ceil|",
			r"cerr|",
			r"cout|",
			r"deque|",
			r"equal|",
			r"exception|",
			r"exp|",
			r"find|",
			r"find_if|",
			r"fixed|",
			r"floor|",
			r"getline|",
			r"ifstream|",
			r"istream|",
			r"istringstream|",
			r"list|",
			r"locale|",
			r"log|",
			r"log10|",
			r"make_heap|",
			r"make_pair|",
			r"map|",
			r"max|",
			r"min|",
			r"multimap|",
			r"numeric_limits|",
			r"ofstream|",
			r"ostream|",
			r"ostringstream|",
			r"out_of_range|",
			r"pair|",
			r"pop_heap|",
			r"pow|",
			r"priority_queue|",
			r"push_heap|",
			r"rename|",
			r"reverse|",
			r"runtime_error|",
			r"set|",
			r"setfill|",
			r"setw|",
			r"setprecision|",
			r"sort|",
			r"streamsize|",
			r"string|",
			r"stringstream|",
			r"swap|",
			r"transform|",
			r"unique_ptr|",
			r"vector|",
			r"wstring",
			r")\b"                # End with word break
))
usingNamespace = r"(^\s*)?using(\s+)namespace(\s+)std(\s*;)?"

# Use capture group 1 to replace keyword with std::<keyword>
replacement = r"std::\1"

# Temp strings so we can remove includes, strings, and comments before performing
# keyword replacement, as we don't want to do keyword replacement in these areas
includeTemp = "#includes#"
stringTemp = "#strings#"
commentTemp = "#comments#"

# Temp lists of all includes, strings, and comments which are removed so we can
# replace them back into the file after keyword replacement is complete
includes = []
strings = []
comments = []

# Removes all includes, comments, and strings, replacing them with temp values
# and storing the original values in temp lists so we can replace them in original
# form once keyword replacement is complete
def remove_includes_comments_and_strings(string):
	pattern = r"(^.*?#include[^\r\n]*$)|(\".*?\"|\'.*?\')|(/\*.*?\*/|//[^\r\n]*$)"
	# first group captures whole lines where #include is defined
	# second group captures quoted strings (double or single)
	# third group captures comments (//single-line or /* multi-line */)
	regex = re.compile(pattern, re.MULTILINE|re.DOTALL)
	def _replacer(match):
		# if the 3rd group (capturing comments) is not None,
		# it means we have captured a non-quoted (real) comment string.
		if match.group(3) is not None:
			comments.append(match.group(3))
			return commentTemp # return the temp comment string
			
		# if the 2nd group (capturing quoted string) is not None,
		# it means we have captured a quoted string.
		elif match.group(2) is not None:
			strings.append(match.group(2))
			return stringTemp # return the temp string string
			
		else: # otherwise, we will return the 1st group
			includes.append(match.group(1))
			return includeTemp # return the temp include string
			
	return regex.sub(_replacer, string)

# Replaces comments one-at-a-time in the order we stored them during initial replacement
def callback_comments(match):
	return next(callback_comments.v)
callback_comments.v=iter(comments)

# Replaces strings one-at-a-time in the order we stored them during initial replacement
def callback_strings(match):
	return next(callback_strings.v)
callback_strings.v=iter(strings)

# Replaces includes one-at-a-time in the order we stored them during initial replacement
def callback_includes(match):
	return next(callback_includes.v)
callback_includes.v=iter(includes)


if __name__ == "__main__":
	for filename in sys.argv[1:]:
		# Reset the temp lists and iterators (necessary for multi-file processing)
		includes[:] = []
		callback_includes.v=iter(includes)
		strings[:] = []
		callback_strings.v=iter(strings)
		comments[:] = []
		callback_comments.v=iter(comments)
		
		# Read in file content as one string
		file = open(filename, mode='r').read()
		
		# Remove comments, strings, and includes as we don't want to
		# replace std:: types within these areas
		noComment = remove_includes_comments_and_strings(file)
		
		# Before we continue with replacement, and while all the comments and
		# strings are removed, check to make sure the `using namespace std` line
		# is actually in this code file.  If it is not then changing the
		# keywords to std:: is changing the definition of working non-std
		# references, which isn't what we want.
		if re.search(usingNamespace, noComment) is None:
			print 'SKIPPED:  %s' % filename
			continue
		
		# Now perform std:: replacement
		replaced = re.sub(keywords, replacement, noComment)
		
		# Also remove the `using namespace std;` line
		replacedNamespace = re.sub(usingNamespace, "", replaced)
		
		# Now we need to restore the comments and strings
		reComment = re.sub(commentTemp, callback_comments, replacedNamespace)
		reString =  re.sub(stringTemp, callback_strings, reComment)
		reInclude =  re.sub(includeTemp, callback_includes, reString)

		# overwrite with std:: modified content
		with open(filename, mode='w') as f:
			f.seek(0)
			f.write(reInclude)
		print 'COMPLETE: %s' % filename
