#include "suiteBase.hpp"

class MoveTest : public SuiteBase {
public:
    std::string getName() const { return "moveTest"; }

    cute::suite getSuite() const;
};
