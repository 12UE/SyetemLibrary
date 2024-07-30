
#ifndef ALGORITIM
#define ALGORITIM
namespace SystemLibrary {
	template<typename T>constexpr inline const T& Clamp(const T& m_value, const T& low, const T& high) noexcept { return (m_value < low) ? low : (m_value > high) ? high : m_value; }
	template<class It1, class It2>
	[[nodiscard]] inline It1 SundaySearch(const It1& first, const It1& last, const It2& s_first, const It2& s_last, const char szMask[]) {
		auto m_it = first;
		auto p_it = s_first;
		for (__int64 p_dis = 0; m_it != last && p_it != s_last; p_dis = p_it - s_first) {
			if (*m_it == *p_it || szMask[p_dis] == '?')++m_it, ++p_it;
			else m_it += -p_dis + 1, p_it = s_first;
		}return(p_it == s_last) ? m_it - std::distance(s_first, p_it) : last;
	}
}
namespace std {
    constexpr size_t g_rootseed = 0x92E2194B;
    template<typename... T>index_sequence_for<T...> index;
    template<typename... T>struct TupleHasher {
        static constexpr SAFE_BUFFER inline size_t HashValue(const tuple<T...>& t) noexcept { return HashImpl(t, index<T...>); }
        template<typename Tuple, size_t... I, typename expander = int[]>
        SAFE_BUFFER constexpr static inline size_t HashImpl(Tuple&& t, const index_sequence<I...>&, size_t seed = 0) noexcept {
            (void)expander {
            0, ((seed ^= Hasher(get<I>(forward<Tuple>(t))) + g_rootseed + (seed << 6) + (seed >> 2)), 0)...
        };
            return seed;
        }
        template<typename U>SAFE_BUFFER static inline  size_t Hasher(const U& value) noexcept {
            static hash<U> hasher;
            return hasher(value);
        }
        inline size_t Seed() const noexcept { return g_rootseed; }
    };
    template<typename... T>struct hash<tuple<T...>> {
        SAFE_BUFFER inline constexpr size_t operator()(const tuple<T...>& t) const noexcept { return TupleHasher<T...>::HashValue(t); }
    };
}
#endif
