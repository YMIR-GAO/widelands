/*
 * Copyright (C) 2002-2018 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef WL_LOGIC_MAP_OBJECTS_TRIBES_PROGRAM_RESULT_H
#define WL_LOGIC_MAP_OBJECTS_TRIBES_PROGRAM_RESULT_H

namespace Widelands {
enum ProgramResult { None = 0, Failed = 1, Completed = 2, Skipped = 3 };
enum ProgramResultHandlingMethod { Fail, Complete, Skip, Continue, Repeat };
}

#endif  // end of include guard: WL_LOGIC_MAP_OBJECTS_TRIBES_PROGRAM_RESULT_H
