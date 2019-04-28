#include <string>
#include <cstring>

#include <napi.h>
using namespace Napi;

#include "clang-c/Index.h"

// BUG: Folding breaks after parsing errors. CXTranslationUnit_KeepGoing
// doesn't appear to be working correctly.

// BUG: if statement folding includes the else block (there's no AST node for
// the else block)

// BUG: Folding will break for nested function decls (e.g. a lambda as a
// default parameter value)

// TODO: Change if, switch, while, for, catch, & range for to work like functions
// TODO: Setting FoldingRangeKind
// TODO: Test all code imaginable
// TODO: Test with unsaved files
// TODO: Test with making file modifications
// TODO: Test performance
// TODO: Bonus: Re-parse skipped preprocessor sections to get folding
// TODO: Bonus: Re-parse function/class templates to get folding

#define UNUSED(x) (x);

template<typename T, size_t S>
size_t ArrayLength(const T(&arr)[S]) { UNUSED(arr); return S; }

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

Value
ParseFile(const CallbackInfo& info)
{
	Env env = info.Env();

	// HACK: Dear god...
	if (info.Length() != 2)
	{
		TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	// HACK: Dear god...
	if (!info[0].IsString() || !info[1].IsString())
	{
		TypeError::New(env, "Wrong type of arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	std::string fileName = info[0].As<String>().Utf8Value();
	std::string fileContent = info[1].As<String>().Utf8Value();
	Array ranges = Array::New(env);

	const char* clangArgs[] = { "-fno-delayed-template-parsing" };

	CXUnsavedFile file = {};
	file.Filename = fileName.c_str();
	file.Contents = fileContent.c_str();
	file.Length = fileContent.size();

	unsigned parseOptions = clang_defaultEditingTranslationUnitOptions();
	parseOptions |= CXTranslationUnit_SingleFileParse;
	parseOptions |= CXTranslationUnit_KeepGoing;
	parseOptions |= CXTranslationUnit_DetailedPreprocessingRecord;

	CXIndex index = clang_createIndex(0, 0);
	CXTranslationUnit unit = clang_parseTranslationUnit(
		index,
		file.Filename,
		clangArgs, ArrayLength(clangArgs),
		&file, 1,
		parseOptions
	);

	// Use the AST for everything but functions, comments, and #if blocks
	{
		struct Context
		{
			Env&   env;
			Array& ranges;
		} context = { env, ranges };

		CXCursor unitCursor = clang_getTranslationUnitCursor(unit);
		clang_visitChildren(
			unitCursor,
			[](CXCursor cursor, CXCursor parent, CXClientData clientData)
			{
				Context& context = *(Context*) clientData;
				CXCursorKind kind = clang_getCursorKind(cursor);

				bool skip = false;
				skip = skip || clang_Cursor_isFunction(cursor);
				if (kind == CXCursor_CompoundStmt)
				{
					skip = skip || clang_Cursor_isFunction(parent);
					skip = skip || clang_Cursor_isKeywordWithCompound(parent);
				}

				if (!skip)
				{
					CXSourceRange    srcRange = clang_getCursorExtent(cursor);
					CXSourceLocation srcStart = clang_getCursorLocation(cursor);
					CXSourceLocation srcEnd   = clang_getRangeEnd(srcRange);
					AppendFoldingRange(context.env, context.ranges, srcStart, srcEnd);
				}
				return CXChildVisit_Recurse;
			},
			&context
		);
	}

	// Use tokens for functions and comments
	{
		CXCursor      unitCursor = clang_getTranslationUnitCursor(unit);
		CXSourceRange unitRange  = clang_getCursorExtent(unitCursor);

		// NOTE: libclang will tokenize block comments (/* */) as single tokens
		// that span multiple lines but regular comments (//) are always single
		// line tokens. Since we want to fold sequences of regular comments we
		// have to use a small state machine to remember the beginning of a
		// sequence of regular comments.

		struct
		{
			bool     inComment;
			unsigned commentStart;
			unsigned commentEnd;

			void
			HandleToken(Env env, Array ranges, CXTranslationUnit unit, CXToken token, CXTokenKind tokenKind)
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

		struct
		{
			bool inFunction;

			void
			HandleToken(Env env, Array ranges, CXTranslationUnit unit, CXToken token, CXTokenKind kind)
			{
				CXCursor cursor;
				clang_annotateTokens(unit, &token, 1, &cursor);
				if (clang_Cursor_isFunction(cursor))
				{
					inFunction = true;
					if (kind == CXToken_Punctuation)
					{
						CXString    tokenStr  = clang_getTokenSpelling(unit, token);
						const char* tokenCStr = clang_getCString(tokenStr);

						if (strcmp(")", tokenCStr) == 0)
						{
							inFunction = false;
							CXSourceRange    funcRange = clang_getCursorExtent(cursor);
							CXSourceLocation funcEnd   = clang_getRangeEnd(funcRange);
							CXSourceLocation paramEnd  = clang_getTokenLocation(unit, token);
							AppendFoldingRange(env, ranges, paramEnd, funcEnd);
						}
						clang_disposeString(tokenStr);
					}
				}
			}
		} FunctionDecl = {};

		CXToken* tokens;
		unsigned tokenCount;
		clang_tokenize(unit, unitRange, &tokens, &tokenCount);
		for (unsigned i = 0; i < tokenCount; i++)
		{
			CXToken     token     = tokens[i];
			CXTokenKind tokenKind = clang_getTokenKind(token);

			CommentSequence.HandleToken(env, ranges, unit, token, tokenKind);
			FunctionDecl   .HandleToken(env, ranges, unit, token, tokenKind);
		}
		CommentSequence.End(env, ranges);
		clang_disposeTokens(unit, tokens, tokenCount);
	}

	// Use skipped ranges for #if blocks
	{
		CXSourceRangeList* srcRangeList = clang_getAllSkippedRanges(unit);
		for (unsigned i = 0; i < srcRangeList->count; i++)
		{
			// NOTE: Decrement lineEnd by 1 to exclude the closing preprocessor
			// directive (e.g. #else or #endif)

			CXSourceRange    srcRange = srcRangeList->ranges[i];
			CXSourceLocation srcStart = clang_getRangeStart(srcRange);
			CXSourceLocation srcEnd   = clang_getRangeEnd  (srcRange);

			unsigned lineStart, lineEnd;
			clang_getFileLocation(srcStart, nullptr, &lineStart, nullptr, nullptr);
			clang_getFileLocation(srcEnd,   nullptr, &lineEnd,   nullptr, nullptr);

			AppendFoldingRange(env, ranges, lineStart, lineEnd - 1);
		}
		clang_disposeSourceRangeList(srcRangeList);
	}

	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);

	return ranges;
}

Object
Init(Env env, Object exports)
{
	exports.Set(String::New(env, "ParseFile"), Function::New(env, ParseFile));
	return exports;
}

NODE_API_MODULE(binding, Init)
