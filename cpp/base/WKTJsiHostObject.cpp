#include "WKTJsiHostObject.h"

#include <utility>

// To be able to find objects that aren't cleaned up correctly,
// we can set this value to 1 and debug the constructor/destructor
#define JSI_DEBUG_ALLOCATIONS 0

namespace RNWorklet {

#if JSI_DEBUG_ALLOCATIONS
int objCounter = 0;
std::vector<JsiHostObject *> objects;
#endif

JsiHostObject::JsiHostObject() {
#if JSI_DEBUG_ALLOCATIONS
  objects.push_back(this);
  objCounter++;
#endif
}
JsiHostObject::~JsiHostObject() {
#if JSI_DEBUG_ALLOCATIONS
  for (size_t i = 0; i < objects.size(); ++i) {
    if (objects.at(i) == this) {
      objects.erase(objects.begin() + i);
      break;
    }
  }
  objCounter--;
#endif
}

void JsiHostObject::set(jsi::Runtime &rt, const jsi::PropNameID &name,
                        const jsi::Value &value) {

  auto nameStr = name.utf8(rt);

  /** Check the static setters map */
  const JsiPropertySettersMap &setters = getExportedPropertySettersMap();
  auto setter = setters.find(nameStr);
  if (setter != setters.end()) {
    auto dispatcher = std::bind(setter->second, this, std::placeholders::_1,
                                std::placeholders::_2);
    return dispatcher(rt, value);
  }
}

jsi::Value JsiHostObject::get(jsi::Runtime &runtime,
                              const jsi::PropNameID &name) {
  auto nameStr = name.utf8(runtime);
  auto &cache = _hostFunctionCache.get(runtime);

  // Check function cache
  auto cachedFunc = cache.find(nameStr);
  if (cachedFunc != cache.end()) {
    return cachedFunc->second.asFunction(runtime);
  }

  // Check the static getters map
  const JsiPropertyGettersMap &getters = getExportedPropertyGettersMap();
  auto getter = getters.find(nameStr);
  if (getter != getters.end()) {
    auto dispatcher = std::bind(getter->second, this, std::placeholders::_1);
    return dispatcher(runtime);
  }

  // Check the static function map
  const JsiFunctionMap &funcs = getExportedFunctionMap();
  auto func = funcs.find(nameStr);
  if (func != funcs.end()) {
    auto dispatcher =
        std::bind(func->second, reinterpret_cast<JsiHostObject *>(this),
                  std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3, std::placeholders::_4);

    // Add to cache - it is important to cache the results from the
    // createFromHostFunction function which takes some time.
    return cache
        .emplace(nameStr, jsi::Function::createFromHostFunction(runtime, name,
                                                                0, dispatcher))
        .first->second.asFunction(runtime);
  }

  return jsi::Value::undefined();
}

std::vector<jsi::PropNameID>
JsiHostObject::getPropertyNames(jsi::Runtime &runtime) {
  // statically exported functions
  const auto &funcs = getExportedFunctionMap();

  // Statically exported property getters
  const auto &getters = getExportedPropertyGettersMap();

  // Statically exported property setters
  const auto &setters = getExportedPropertySettersMap();

  std::vector<jsi::PropNameID> propNames;
  propNames.reserve(funcs.size() + getters.size() + setters.size());

  for (auto it = funcs.cbegin(); it != funcs.cend(); ++it) {
    propNames.push_back(jsi::PropNameID::forAscii(runtime, it->first));
  }

  for (auto it = getters.cbegin(); it != getters.cend(); ++it) {
    propNames.push_back(jsi::PropNameID::forUtf8(runtime, it->first));
  }

  for (auto it = getters.cbegin(); it != getters.cend(); ++it) {
    if (getters.count(it->first) == 0) {
      propNames.push_back(jsi::PropNameID::forUtf8(runtime, it->first));
    }
  }
  return propNames;
}

} // namespace RNWorklet
