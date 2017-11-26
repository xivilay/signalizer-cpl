/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2017 Janus Lynggaard Thorborg (www.jthorborg.com)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	See \licenses\ for additional details on licenses associated with this program.

**************************************************************************************

	file:Args.h

		Utility class for easily working with command-line arguments.

*************************************************************************************/

#ifndef CPL_PROCESS_ARGS_H
#define CPL_PROCESS_ARGS_H

#include <vector>
#include <string>

namespace cpl
{
	class Args
	{
	public:

		Args() {}
		Args(std::string seq) : compiledArgs(std::move(seq)) { vectorArgs.emplace_back(compiledArgs); compiledArgs.push_back(' '); }
		Args(Args&&) = default;
		Args& operator = (Args&&) = default;
		Args(const Args&) = default;
		Args& operator = (const Args&) = default;

		enum Flags
		{
			None = 0,
			/// <summary>
			/// Args and vals will be pre- and postfixed with "
			/// </summary>
			Escaped,
			/// <summary>
			/// Removes default spacing between key-val arg pairs
			/// </summary>
			NoSpace
		};

		Args& cwd(const std::string& workingDirectory, int flags = None)
		{
			wd = workingDirectory;
			wdIsEscaped = false;
			if (wd.size())
			{
				auto endC = wd[wd.size() - 1];
				if (endC != '\\' || endC != '/')
					wd += '/';

				wdIsEscaped = !!(flags & Escaped);
			}
			return *this;
		}

		Args& arg(const std::string& arg, int flags = None)
		{
			if (wdIsEscaped || (flags & Escaped))
				compiledArgs += "\"" + wd + arg + "\"";
			else
				compiledArgs += wd + arg;

			vectorArgs.emplace_back(wd + arg);

			compiledArgs.push_back(' ');

			return *this;
		}

		Args& argPair(const std::string& key, const std::string& val, int flags = None)
		{
			compiledArgs += key + ((flags & NoSpace) ? "" : " ");

			if (flags & NoSpace)
			{
				vectorArgs.emplace_back(key + wd + val);
			}
			else
			{
				vectorArgs.emplace_back(key);
				vectorArgs.emplace_back(wd + val);
			}

			if (wdIsEscaped || (flags & Escaped))
				compiledArgs += "\"" + wd + val + "\"";
			else
				compiledArgs += wd + val;

			compiledArgs.push_back(' ');

			return *this;
		}

		friend Args operator + (Args left, const Args& right)
		{
			left.compiledArgs += " " + right.compiledArgs;
			std::move(right.vectorArgs.begin(), right.vectorArgs.end(), std::back_inserter(left.vectorArgs));
			return std::move(left);
		}

		Args& operator += (Args right)
		{
			compiledArgs += " " + right.compiledArgs;
			std::move(right.vectorArgs.begin(), right.vectorArgs.end(), std::back_inserter(vectorArgs));
			return *this;
		}

		const std::string& commandLine() const
		{
			return compiledArgs;
		}

		std::size_t argc() const
		{
			return vectorArgs.size();
		}

		const std::vector<std::string>& rawArgs() const
		{
			return vectorArgs;
		}

		/// <summary>
		/// It is undefined behaviour to modify the contents of the returned array
		/// </summary>
		char * const * argv()
		{
			argPointers.resize(vectorArgs.size() + 1);

			for (std::size_t i = 0; i < vectorArgs.size(); ++i)
			{
				argPointers[i] = const_cast<char *>(vectorArgs[i].c_str());
			}

			argPointers[vectorArgs.size()] = nullptr;

			return &argPointers[0];
		}

	private:

		bool dirty = false;
		bool wdIsEscaped = false;

		std::vector<std::string> vectorArgs;
		mutable std::vector<char *> argPointers;
		std::string compiledArgs, wd;
	};
}

#endif
