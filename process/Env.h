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

	file:Env.h

		Utility class for easily working with environment strings.

*************************************************************************************/

#ifndef CPL_PROCESS_ENV_H
#define CPL_PROCESS_ENV_H

#include <vector>
#include <string>

namespace cpl
{
    class EnvStrings
    {
    public:

        EnvStrings() {}
        EnvStrings(const char * arg) : compiledArgs(arg ? arg : "") { vectorArgs.emplace_back(compiledArgs); compiledArgs.push_back('\0'); }
        EnvStrings(const std::string& seq) : compiledArgs(seq) { vectorArgs.emplace_back(compiledArgs); compiledArgs.push_back('\0'); }
        EnvStrings(std::string&& seq) : compiledArgs(seq) { vectorArgs.emplace_back(compiledArgs); compiledArgs.push_back('\0'); }
        EnvStrings(EnvStrings&&) = default;
        EnvStrings& operator = (EnvStrings&&) = default;
        EnvStrings(const EnvStrings&) = default;
        EnvStrings& operator = (const EnvStrings&) = default;

        EnvStrings& string(std::string envString)
        {
            compiledArgs += envString;
            compiledArgs.push_back('\0');

            vectorArgs.emplace_back(std::move(envString));

            return *this;
        }

        EnvStrings& pair(std::string key, std::string val)
        {
            auto envString = std::move(key) + "=" + std::move(val);
            compiledArgs += envString;
            compiledArgs.push_back('\0');

            vectorArgs.emplace_back(std::move(envString));

            return *this;
        }

        friend EnvStrings operator + (EnvStrings left, EnvStrings right)
        {
            left.compiledArgs += right.compiledArgs;
            left.compiledArgs.push_back('\0');
            std::move(right.vectorArgs.begin(), right.vectorArgs.end(), std::back_inserter(left.vectorArgs));
            return std::move(left);
        }

        const std::string& doubleNullList() const
        {
            return compiledArgs;
        }

        std::size_t argc() const
        {
            return vectorArgs.size();
        }

        const std::vector<std::string>& rawStrings() const
        {
            return vectorArgs;
        }

        /// <summary>
        /// It is undefined behaviour to modify the contents of the returned array
        /// </summary>
        char ** environ()
        {
            argPointers.resize(vectorArgs.size() + 1);

            for(std::size_t i = 0; i < vectorArgs.size(); ++i)
            {
                argPointers[i] = const_cast<char *>(vectorArgs[i].c_str());
            }

            argPointers[vectorArgs.size()] = nullptr;

            return &argPointers[0];
        }

    private:

        std::vector<std::string> vectorArgs;
        mutable std::vector<char *> argPointers;
        std::string compiledArgs;
    };
}

#endif
