/*
 * Copyright (C) 2010-2012 Yuriy Miroshnyk, Andrey Antsut
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_CHECK_H
#define _FINI_CHECK_H

#include <cassert>
#include <stdexcept>

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

#define FINI_CHECK(expr)                        \
	{                                           \
	bool finiCheckVar = (expr) ? true : false;  \
	if (!finiCheckVar)                          \
	{                                           \
		assert(expr);                           \
		char msg[2048];                         \
		sprintf_s(msg, sizeof(msg), "FINI_CHECK(%s) failed at %s, line %d (function %s)", #expr, __FILE__, __LINE__, __FUNCTION__); \
		throw std::runtime_error(msg);          \
	}                                           \
	}

#define FINI_ERROR(error)                       \
	{                                           \
	assert(false);                              \
	char msg[2048];                             \
	sprintf_s(msg, sizeof(msg), "FINI_ERROR(%s) at %s, line %d (function %s)", #error, __FILE__, __LINE__, __FUNCTION__); \
	throw std::runtime_error(msg);              \
	}

///////////////////////////////////////////////////////////////////////////////
}

#endif