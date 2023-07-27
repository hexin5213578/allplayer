#ifndef UTIL_ONCETOKEN_H_
#define UTIL_ONCETOKEN_H_

#include <functional>
#include <type_traits>

namespace avs_toolkit 
{
    class OnceToken 
    {
    public:
        using task = std::function<void(void)>;

        template<typename FUNC>
        OnceToken(const FUNC& onConstructed, task onDestructed = nullptr) {
            onConstructed();
            m_onDestructed = std::move(onDestructed);
        }

        OnceToken(std::nullptr_t, task onDestructed = nullptr) {
            m_onDestructed = std::move(onDestructed);
        }

        ~OnceToken() {
            if (m_onDestructed) {
                m_onDestructed();
            }
        }

    private:
        OnceToken() = delete;
        OnceToken(const OnceToken&) = delete;
        OnceToken(OnceToken&&) = delete;
        OnceToken& operator=(const OnceToken&) = delete;
        OnceToken& operator=(OnceToken&&) = delete;

    private:
        task m_onDestructed;
    };

} /* namespace avs_toolkit */
#endif /* UTIL_ONCETOKEN_H_ */
