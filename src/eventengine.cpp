#include <jinx/eventengine.hpp>

namespace jinx {

JINX_ERROR_IMPLEMENT(event_engine, {
    switch(code.as<ErrorEventEngine>()) {
        case ErrorEventEngine::NoError:
            return "No error";
        case ErrorEventEngine::FailedToRegisterTimerEvent:
            return "Failed to register timer event";
        case ErrorEventEngine::FailedToRegisterIOEvent:
            return "Failed to register IO event";
        case ErrorEventEngine::FailedToRegisterSignalEvent:
            return "Failed to register signal event";
        case ErrorEventEngine::FailedToRemoveTimerEvent:
            return "Failed to remove timer event";
        case ErrorEventEngine::FailedToRemoveIOEvent:
            return "Failed to remove IO event";
        case ErrorEventEngine::FailedToRemoveSignalEvent:
            return "Failed to remove signal event";
    }
});

} // namespace jinx