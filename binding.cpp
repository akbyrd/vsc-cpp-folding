#include <string>
#include <sstream>
#include <cstring>
#include <stack>

#include <napi.h>
using namespace Napi;

#include "clang-c/Index.h"

// BUG: if statement folding includes the else block (there's no AST node for
// the else block)

// TODO: Change if, switch, while, for, catch, & range for to work like functions
// TODO: Setting FoldingRangeKind
// TODO: Test all code imaginable
// TODO: Test performance
// TODO: Bonus: Re-parse skipped preprocessor sections to get folding
// TODO: Bonus: Re-parse function/class templates to get folding

#define UNUSED(x) (x);

template<typename T, size_t S>
size_t ArrayLength(const T(&arr)[S]) { UNUSED(arr); return S; }

std::string
clang_getCStr(CXString cxString)
{
	std::string string = clang_getCString(cxString);
	clang_disposeString(cxString);
	return string;
}

unsigned
clang_Cursor_isFunction(CXCursor cursor)
{
	CXCursorKind kind = clang_getCursorKind(cursor);
	switch (kind)
	{
		case CXCursor_FunctionDecl:
		case CXCursor_CXXMethod:
		case CXCursor_Constructor:
		case CXCursor_Destructor:
		case CXCursor_ConversionFunction:
		case CXCursor_FunctionTemplate:
		case CXCursor_LambdaExpr:
			return 1;
	}
	return 0;
}

unsigned
clang_Cursor_isKeywordWithCompound(CXCursor cursor)
{
	CXCursorKind kind = clang_getCursorKind(cursor);
	switch (kind)
	{
		case CXCursor_CaseStmt:
		case CXCursor_DefaultStmt:
		case CXCursor_IfStmt:
		case CXCursor_SwitchStmt:
		case CXCursor_WhileStmt:
		case CXCursor_DoStmt:
		case CXCursor_ForStmt:
		case CXCursor_CXXCatchStmt:
		case CXCursor_CXXTryStmt:
		case CXCursor_CXXForRangeStmt:
		case CXCursor_SEHTryStmt:
		case CXCursor_SEHExceptStmt:
		case CXCursor_SEHFinallyStmt:
			return 1;
	}
	return 0;
}

void
AppendFoldingRange(Env env, Array ranges, unsigned lineStart, unsigned lineEnd)
{
	if (lineEnd != lineStart)
	{
		Object range = Object::New(env);
		range.Set(String::New(env, "start"), lineStart - 1);
		range.Set(String::New(env, "end"),   lineEnd   - 1);
		ranges[ranges.Length()] = range;
	}
}

void
AppendFoldingRange(Env env, Array ranges, CXSourceLocation srcStart, CXSourceLocation srcEnd)
{
	unsigned lineStart, lineEnd;
	clang_getFileLocation(srcStart, nullptr, &lineStart, nullptr, nullptr);
	clang_getFileLocation(srcEnd,   nullptr, &lineEnd,   nullptr, nullptr);
	AppendFoldingRange(env, ranges, lineStart, lineEnd);
}

void
AppendFoldingRange(Env env, Array ranges, CXSourceRange srcRange)
{
	CXSourceLocation srcStart = clang_getRangeStart(srcRange);
	CXSourceLocation srcEnd   = clang_getRangeEnd  (srcRange);
	AppendFoldingRange(env, ranges, srcStart, srcEnd);
}

struct Log
{
	bool   enable;
	Env&   env;
	Array& errors;

	template <class... T>
	void operator()(T&&... args)
	{
		if (!enable) return;

		std::ostringstream s;
		(s << ... << args);
		errors[errors.Length()] = String::New(env, s.str().c_str());
	}
};

Value
ParseFile(const CallbackInfo& info)
{
	Env env = info.Env();
	Array ranges = Array::New(env);
	Array errors = Array::New(env);

	Object result = Object::New(env);
	result.Set("ranges", ranges);
	result.Set("errors", errors);

	Log log = { true, env, errors };

	// HACK: Dear god...
	if (info.Length() != 2)
	{
		log("Wrong number of arguments");
		TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	// HACK: Dear god...
	if (!info[0].IsString() || !info[1].IsString())
	{
		log("Wrong type of arguments");
		TypeError::New(env, "Wrong type of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	std::string fileName = info[0].As<String>().Utf8Value();
	std::string fileContent = info[1].As<String>().Utf8Value();
	log("Filename: ", fileName);

	const char* clangArgs[] = {
		"-fno-delayed-template-parsing",
		"-std=c++17",
		"-x", "c++",
	};

	CXUnsavedFile file = {};
	file.Filename = fileName.c_str();
	file.Contents = fileContent.c_str();
	file.Length = fileContent.size();

	// TODO: Print clang errors (usage errors, not compile errors)
	CXIndex index = clang_createIndex(0, 1);
	CXTranslationUnit unit = clang_createTranslationUnitFromSourceFile(
		index,
		file.Filename,
		ArrayLength(clangArgs), clangArgs,
		1, &file
	);

	CXCursor      unitCursor = clang_getTranslationUnitCursor(unit);
	CXSourceRange unitRange  = clang_getCursorExtent(unitCursor);

	struct
	{
		std::stack<CXToken*> tokenStack;
		CXToken*             prevToken;

		void
		HandleToken(Log log, Env env, Array ranges, CXTranslationUnit unit, CXToken& token, CXTokenKind tokenKind)
		{
			log("token: ", clang_getCStr(clang_getTokenSpelling(unit, token)));
			switch (tokenKind)
			{
				default:
					prevToken = &token;
					break;

				case CXToken_Comment:
					prevToken = nullptr;
					break;

				case CXToken_Punctuation:
				{
					std::string tokenStr = clang_getCStr(clang_getTokenSpelling(unit, token));
					switch (tokenStr[0])
					{
						default:
							prevToken = &token;
							break;

						case '{':
						{
							tokenStack.push(prevToken ? prevToken : &token);
							prevToken = nullptr;

							// DEBUG
							std::string pushTokenStr = clang_getCStr(clang_getTokenSpelling(unit, *tokenStack.top()));
							log("push: ", pushTokenStr);
							break;
						}

						case ';':
							prevToken = nullptr;
							break;

						case '}':
						{
							if (tokenStack.empty()) break;

							CXToken* startToken = tokenStack.top(); tokenStack.pop();
							CXToken* endToken   = &token;
							if (!startToken) break;

							CXSourceRange blockRangeStart = clang_getTokenExtent(unit, *startToken);
							CXSourceRange blockRangeEnd   = clang_getTokenExtent(unit, *endToken);

							CXSourceLocation blockStart = clang_getRangeStart(blockRangeStart);
							CXSourceLocation blockEnd   = clang_getRangeEnd  (blockRangeEnd);
							AppendFoldingRange(env, ranges, blockStart, blockEnd);
							prevToken = nullptr;

							// DEBUG
							std::string tokenStartStr = clang_getCStr(clang_getTokenSpelling(unit, *startToken));
							std::string tokenEndStr   = clang_getCStr(clang_getTokenSpelling(unit, *endToken));
							log("pop & range: ", tokenStartStr, " - ", tokenEndStr);
							break;
						}
					}
					break;
				}
			}
		}
	} ScopeBlock = {};

	struct
	{
		bool     inComment;
		unsigned commentStart;
		unsigned commentEnd;

		void
		HandleToken(Log log, Env env, Array ranges, CXTranslationUnit unit, CXToken token, CXTokenKind tokenKind)
		{
			if (tokenKind == CXToken_Comment)
			{
				CXString      tokenStr  = clang_getTokenSpelling(unit, token);
				const char*   tokenCStr = clang_getCString(tokenStr);
				CXSourceRange srcRange  = clang_getTokenExtent(unit, token);

				bool isBlockComment = strncmp("/*", tokenCStr, 2) == 0;
				if (isBlockComment)
				{
					End(env, ranges);
					AppendFoldingRange(env, ranges, srcRange);
				}
				else
				{
					Begin(env, ranges, srcRange);
				}
			}
			else
			{
				End(env, ranges);
			}
		}

		void
		Begin(Env env, Array ranges, CXSourceRange srcRange)
		{
			CXSourceLocation srcStart = clang_getRangeStart(srcRange);
			CXSourceLocation srcEnd   = clang_getRangeEnd  (srcRange);

			unsigned lineStart, lineEnd;
			clang_getFileLocation(srcStart, nullptr, &lineStart, nullptr, nullptr);
			clang_getFileLocation(srcEnd,   nullptr, &lineEnd,   nullptr, nullptr);

			Begin(env, ranges, lineStart, lineEnd);
		}

		void
		Begin(Env env, Array ranges, unsigned lineStart, unsigned lineEnd)
		{
			if (!inComment)
			{
				inComment    = true;
				commentStart = lineStart;
				commentEnd   = lineEnd;
			}
			else
			{
				if (lineStart > commentEnd + 1)
				{
					End(env, ranges);
					Begin(env, ranges, lineStart, lineEnd);
				}
				else
				{
					commentEnd = lineEnd;
				}
			}
		};

		void
		End(Env env, Array ranges)
		{
			if (inComment)
			{
				inComment = false;
				AppendFoldingRange(env, ranges, commentStart, commentEnd);
			}
		};
	} CommentSequence = {};

	CXToken* tokens;
	unsigned tokenCount;
	clang_tokenize(unit, unitRange, &tokens, &tokenCount);
	for (unsigned i = 0; i < tokenCount; i++)
	{
		CXToken&    token     = tokens[i];
		CXTokenKind tokenKind = clang_getTokenKind(token);

		ScopeBlock     .HandleToken(log, env, ranges, unit, token, tokenKind);
		CommentSequence.HandleToken(log, env, ranges, unit, token, tokenKind);
		//FunctionDecl   .HandleToken(log, env, ranges, unit, token, tokenKind);
	}
	CommentSequence.End(env, ranges);
	clang_disposeTokens(unit, tokens, tokenCount);

	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);

	return result;
}

Object
Init(Env env, Object exports)
{
	exports.Set(String::New(env, "ParseFile"), Function::New(env, ParseFile));
	return exports;
}

NODE_API_MODULE(binding, Init)
