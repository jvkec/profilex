#include "model/types.hpp"

namespace profilex::model {

std::string to_string(const Verdict verdict) {
    switch (verdict) {
        case Verdict::likely_improvement:
            return "likely improvement";
        case Verdict::likely_regression:
            return "likely regression";
        case Verdict::inconclusive:
            return "inconclusive";
    }

    return "inconclusive";
}

}  // namespace profilex::model
