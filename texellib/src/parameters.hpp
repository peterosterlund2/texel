/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
*/

/*
 * parameters.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef PARAMETERS_HPP_
#define PARAMETERS_HPP_

#include "util/util.hpp"

#include <memory>
#include <map>
#include <string>
#include <cassert>


class Parameters {
public:
    enum Type {
        CHECK,
        SPIN,
        COMBO,
        BUTTON,
        STRING
    };

    /** Base class for UCI parameters. */
    struct ParamBase {
        std::string name;
        Type type;
        bool visible;

        ParamBase(const std::string& n, Type t, bool v) : name(n), type(t), visible(v) { }

        virtual bool getBoolPar() const { assert(false); return false; }
        virtual int getIntPar() const { assert(false); return 0; }
        virtual std::string getStringPar() const { assert(false); return ""; }
        virtual void set(const std::string& value) { assert(false); }
    private:
        ParamBase(const ParamBase& other) = delete;
        ParamBase& operator=(const ParamBase& other) = delete;
    };

    /** A boolean parameter. */
    struct CheckParam : public ParamBase {
        bool value;
        bool defaultValue;

        CheckParam(const std::string& name, bool def)
            : ParamBase(name, CHECK, true) {
            this->value = def;
            this->defaultValue = def;
        }

        virtual bool getBoolPar() const { return value; }

        virtual void set(const std::string& value) {
            if (toLowerCase(value) == "true")
                this->value = true;
            else if (toLowerCase(value) == "false")
                this->value = false;
        }
    };

    /** An integer parameter. */
    struct SpinParam : public ParamBase {
        int minValue;
        int maxValue;
        int value;
        int defaultValue;

        SpinParam(const std::string& name, int minV, int maxV, int def)
            : ParamBase(name, SPIN, true) {
            this->minValue = minV;
            this->maxValue = maxV;
            this->value = def;
            this->defaultValue = def;
        }

        virtual int getIntPar() const { return value; }

        virtual void set(const std::string& value) {
            int val;
            str2Num(value, val);
            if ((val >= minValue) && (val <= maxValue))
                this->value = val;
        }
    };

    /** A multi-choice parameter. */
    struct ComboParam : public ParamBase {
        std::vector<std::string> allowedValues;
        std::string value;
        std::string defaultValue;

        ComboParam(const std::string& name, const std::vector<std::string>& allowed,
                   const std::string& def)
            : ParamBase(name, COMBO, true) {
            this->allowedValues = allowed;
            this->value = def;
            this->defaultValue = def;
        }

        virtual std::string getStringPar() const { return value; }

        virtual void set(const std::string& value) {
            for (size_t i = 0; i < allowedValues.size(); i++) {
                const std::string& allowed = allowedValues[i];
                if (toLowerCase(allowed) == toLowerCase(value)) {
                    this->value = allowed;
                    break;
                }
            }
        }
    };

    /** An action parameter. */
    struct ButtonParam : public ParamBase {
        ButtonParam(const std::string& name)
            : ParamBase(name, BUTTON, true) { }

        virtual void set(const std::string& value) {
        }
    };

    /** A string parameter. */
    struct StringParam : public ParamBase {
        std::string value;
        std::string defaultValue;

        StringParam(const std::string& name, const std::string& def)
            : ParamBase(name, STRING, true) {
            this->value = def;
            this->defaultValue = def;
        }

        virtual std::string getStringPar() const { return value; }

        virtual void set(const std::string& value) {
            this->value = value;
        }
    };

    /** Get singleton instance. */
    static Parameters& instance();

    void getParamNames(std::vector<std::string>& parNames) {
        parNames.clear();
        for (ParamMap::const_iterator it = params.begin(); it != params.end(); ++it)
            if (it->second->visible)
                parNames.push_back(it->first);
    }

    std::shared_ptr<ParamBase> getParam(const std::string& name) {
        return params[name];
    }

    bool getBoolPar(const std::string& name) const {
        return params.find(toLowerCase(name))->second->getBoolPar();
    }

    int getIntPar(const std::string& name) const {
        return params.find(toLowerCase(name))->second->getIntPar();
    }
    std::string getStringPar(const std::string& name) const {
        return params.find(toLowerCase(name))->second->getStringPar();
    }

    void set(const std::string& name, const std::string& value) {
        ParamMap::iterator it = params.find(toLowerCase(name));
        if (it == params.end())
            return;
        it->second->set(value);
    }

private:
    Parameters();

    void addPar(const std::shared_ptr<ParamBase>& p) {
        params[toLowerCase(p->name)] = p;
    }

    typedef std::map<std::string, std::shared_ptr<ParamBase> > ParamMap;
    ParamMap params;
};


#endif /* PARAMETERS_HPP_ */
