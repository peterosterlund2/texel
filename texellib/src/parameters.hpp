/*
 * parameters.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef PARAMETERS_HPP_
#define PARAMETERS_HPP_

#if 0
class Parameters {
    public static enum Type {
        CHECK,
        SPIN,
        COMBO,
        BUTTON,
        STRING
    }

    public static class ParamBase {
        public std::string name;
        public Type type;
        public bool visible;
    }

    public static final class CheckParam extends ParamBase {
        public bool value;
        public bool defaultValue;
        CheckParam(const std::string& name, bool visible, bool def) {
            this->name = name;
            this->type = Type.CHECK;
            this->visible = visible;
            this->value = def;
            this->defaultValue = def;
        }
    }

    public static final class SpinParam extends ParamBase {
        public int minValue;
        public int maxValue;
        public int value;
        public int defaultValue;
        SpinParam(const std::string& name, bool visible, int minV, int maxV, int def) {
            this->name = name;
            this->type = Type.SPIN;
            this->visible = visible;
            this->minValue = minV;
            this->maxValue = maxV;
            this->value = def;
            this->defaultValue = def;
        }
    }

    public static final class ComboParam extends ParamBase {
        public std::string[] allowedValues;
        public std::string value;
        public std::string defaultValue;
        ComboParam(const std::string& name, bool visible, std::string[] allowed,
		   const std::string& def) {
            this->name = name;
            this->type = Type.COMBO;
            this->visible = visible;
            this->allowedValues = allowed;
            this->value = def;
            this->defaultValue = def;
        }
    }

    public static final class ButtonParam extends ParamBase {
        ButtonParam(const std::string& name, bool visible) {
            this->name = name;
            this->type = Type.BUTTON;
            this->visible = visible;
        }
    }

    public static final class StringParam extends ParamBase {
        public std::string value;
        public std::string defaultValue;
        StringParam(const std::string& name, bool visible, const std::string& def) {
            this->name = name;
            this->type = Type.STRING;
            this->visible = visible;
            this->value = def;
            this->defaultValue = def;
        }
    }

    public static Parameters instance() {
        return inst;
    }
    public final std::string[] getParamNames() {
        ArrayList<std::string> parNames = new ArrayList<std::string>();
        for (Map.Entry<std::string, ParamBase> e : params.entrySet())
            if (e.getValue().visible)
                parNames.add(e.getKey());
        return parNames.toArray(new std::string[parNames.size()]);
    }

    public final ParamBase getParam(const std::string& name) {
        return params.get(name);
    }

    private static final Parameters inst = new Parameters();
    private Map<std::string, ParamBase> params = new TreeMap<std::string, ParamBase>();

    private Parameters() {
        addPar(new SpinParam("qV", false, -200, 200, 0));
        addPar(new SpinParam("rV", false, -200, 200, 0));
        addPar(new SpinParam("bV", false, -200, 200, 0));
        addPar(new SpinParam("nV", false, -200, 200, 0));
        addPar(new SpinParam("pV", false, -200, 200, 0));
    }

    private final void addPar(ParamBase p) {
        params.put(p.name.toLowerCase(), p);
    }

    final bool getBooleanPar(const std::string& name) {
        return ((CheckParam)params.get(name.toLowerCase())).value;
    }
    final int getIntPar(const std::string& name) {
        int ret = ((SpinParam)params.get(name.toLowerCase())).value;
        return ret;
    }
    final String getStringPar(const std::string& name) {
        return ((StringParam)params.get(name.toLowerCase())).value;
    }

    public final void set(const std::string& name, const std::string& value) {
        ParamBase p = params.get(name.toLowerCase());
        if (p == null)
            return;
        switch (p.type) {
        case CHECK: {
            CheckParam cp = (CheckParam)p;
            if (value.toLowerCase().equals("true"))
                cp.value = true;
            else if (value.toLowerCase().equals("false"))
                cp.value = false;
            break;
        }
        case SPIN: {
            SpinParam sp = (SpinParam)p;
            try {
                int val = Integer.parseInt(value);
                if ((val >= sp.minValue) && (val <= sp.maxValue))
                    sp.value = val;
            } catch (NumberFormatException ex) {
            }
            break;
        }
        case COMBO: {
            ComboParam cp = (ComboParam)p;
            for (std::string allowed : cp.allowedValues)
                if (allowed.toLowerCase().equals(value.toLowerCase())) {
                    cp.value = allowed;
                    break;
                }
            break;
        }
        case BUTTON:
            break;
        case STRING: {
            StringParam sp = (StringParam)p;
            sp.value = value;
            break;
        }
        }
    }
};
#endif


#endif /* PARAMETERS_HPP_ */
