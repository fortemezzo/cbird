/* Global header
   Copyright (C) 2021 scrubbbbs
   Contact: screubbbebs@gemeaile.com =~ s/e//g
   Project: https://github.com/scrubbbbs/cbird

   This file is part of cbird.

   cbird is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   cbird is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with cbird; if not, see
   <https://www.gnu.org/licenses/>.  */
#define HAVE_PCH

// portability macros
#ifdef __WIN32__
#define malloc_usable_size(x) _msize((x))
#define __STDC_FORMAT_MACROS 1
#endif

#include <QtCore/QtCore>

#if QT_VERSION_MAJOR < 5
#error Qt 5 is required
#endif

#if QT_VERSION_MINOR < 15
#error Qt 5.15 is required
#endif

#include <QtConcurrent/QtConcurrent>
#include <QtSql/QtSql>
#include <QtWidgets/QtWidgets>
#include <QtGui/QtGui>

// make sure asserts are always enabled, for now
#if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#error QT_ASSERT() must be enabled!
#endif

// same for assert(), though I've tried to eradicate it
#ifdef NDEBUG
#error please do not disable assert()
#endif

// most classes are qobject subclasses and should not have
// copy/move constructors or default constructors
// typedefs are useful for overloads and signal/slot connections
#define NO_COPY(className,superClassName) \
    Q_DISABLE_COPY_MOVE(className) \
    typedef superClassName super; \
    typedef className self;

#define NO_COPY_NO_DEFAULT(className,superClassName) \
    NO_COPY(className,superClassName) \
    className() = delete;

// malloc/realloc that uses size of the pointer and count for allocation
#define strict_malloc(ptr, count) \
   reinterpret_cast<decltype(ptr)>(malloc(uint(count)*sizeof(*ptr)))

#define strict_realloc(ptr, count) \
   reinterpret_cast<decltype(ptr)>(realloc(ptr, uint(count)*sizeof(*ptr)))
