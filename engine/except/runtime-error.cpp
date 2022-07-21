#include "runtime-error.hpp"

/**
 * Defines a runtime exception.
 *
 * @param message The error message.
 * @param kind Error kind from {@link ErrorKind} enum.
 * @param code The error code, defaults to 0.
 */
vtrs::RuntimeError::RuntimeError(std::string& message, int kind, int code) : std::runtime_error(message) {
    this->m_code = code;
    this->m_kind = kind;
}

/**
 * Returns the error code.
 *
 * @return The error code.
*/
int vtrs::RuntimeError::getCode() {
    return this->m_code;
}

/**
 * Returns the error kind.
 *
 * @return The error kind.
*/
int vtrs::RuntimeError::getKind() {
    return this->m_kind;
}
