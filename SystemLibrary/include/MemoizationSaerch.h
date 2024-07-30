#pragma once
#ifndef MEMOIZATIONSEARCH
#define MEMOIZATIONSEARCH 260
#define KB 1024
using CacheValidTimeType = unsigned long long;
constexpr auto INFINITYCACHE = static_cast<CacheValidTimeType>(0xffffffffffffffff);
#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif
#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#define CDECLCALL __cdecl
#ifdef _WIN64
#define STDCALL __stdcall
#define FASTCALL __fastcall
#define THISCALL __thiscall
#endif
#else
#define SAFE_BUFFER
#define CDECLCALL
#define STDCALL
#define FASTCALL
#define THISCALL
#endif
namespace SystemLibrary {
    namespace memoizationsearch {
        template<typename T>class SingleTon {
            SingleTon(const SingleTon&) = delete;
            SingleTon& operator=(const SingleTon&) = delete;
            SingleTon(SingleTon&&) = delete;
            SingleTon& operator=(SingleTon&&) = delete;
        public:
            SingleTon() = default;
            SAFE_BUFFER inline  static T& GetInstance() noexcept {
                static T instance;
                return instance;
            }
            template<typename ...Args>
            SAFE_BUFFER inline  static T& GetInstance(Args&&...args) noexcept {
                static T instance(std::forward<Args>(args)...);
                return instance;
            }
        };
        template<typename T>static inline bool ReadCacheFromFile(std::ifstream& file, T& value) {
            return file && file.read(reinterpret_cast<char*>((void*)&value), sizeof(value)).good();
        }
        template<typename T>static inline bool WriteCacheToFile(std::ofstream& file, const T& value) {
            return file && file.write(reinterpret_cast<char*>((void*)&value), sizeof(value)).good();
        }
        template<typename T, typename U>static inline bool WritePairToFile(std::ofstream& file, std::pair<T, U>& pair) { return !(!WriteCacheToFile(file, pair.first) || !WriteCacheToFile(file, pair.second)); }
        template<typename T, typename U>static inline bool ReadPairFromFile(std::ifstream& file, std::pair<T, U>& pair) { return !(!ReadCacheFromFile(file, pair.first) || !ReadCacheFromFile(file, pair.second)); }
        template<std::size_t I = 0, typename... Tp>static inline typename std::enable_if<I == sizeof...(Tp), bool>::type ReadTupleImpl(std::ifstream&, std::tuple<Tp...>&)noexcept { return true; }
        template<std::size_t I = 0, typename... Tp>static inline typename std::enable_if < I<sizeof...(Tp), bool>::type ReadTupleImpl(std::ifstream& file, std::tuple<Tp...>& t)noexcept {
            if (!ReadCacheFromFile(file, std::get<I>(t)))return false;
            return ReadTupleImpl<I + 1, Tp...>(file, t);
        }
        template<typename... Args>static inline bool ReadTupleFromFile(std::ifstream& file, std::tuple<Args...>& tuple) noexcept { return ReadTupleImpl(file, tuple); }
        template<std::size_t I = 0, typename... Tp>static inline typename std::enable_if<I == sizeof...(Tp), bool>::type WriteTupleImpl(std::ofstream&, const std::tuple<Tp...>&) noexcept { return true; }
        template<std::size_t I = 0, typename... Tp>static inline typename std::enable_if < I< sizeof...(Tp), bool>::type
            WriteTupleImpl(std::ofstream& file, const std::tuple<Tp...>& t) noexcept {
            if (!WriteCacheToFile(file, std::get<I>(t)))return false;
            return WriteTupleImpl<I + 1, Tp...>(file, t);
        }
        template<typename... Args>static inline bool WriteTupleToFile(std::ofstream& file, const std::tuple<Args...>& tuple) noexcept { return WriteTupleImpl(file, tuple); }
        template<>inline bool ReadCacheFromFile<std::string>(std::ifstream& file, std::string& value) {
            if (!file) return false;
            std::size_t size = 0;
            if (!file.read(reinterpret_cast<char*>(&size), sizeof(size)))return false;
            std::string utf8String(size, '\0');
            if (!file.read(&utf8String[0], size))return false;
            value = std::move(utf8String);
            return true;
        }
        template<>inline bool ReadCacheFromFile<std::wstring>(std::ifstream& file, std::wstring& value) {
            if (!file) return  false;
            std::wstring::size_type len = 0;
            if (!file.read(reinterpret_cast<char*>(&len), sizeof(len)).good()) return false;
            std::wstring wstr(len, L'\0');
            if (!file.read(reinterpret_cast<char*>(&wstr[0]), len * sizeof(wchar_t)).good()) return false;
            value = std::move(wstr);
            return true;
        }
        template<>inline bool WriteCacheToFile<std::string>(std::ofstream& file, const  std::string& value) {
            if (!file) return false;
            std::size_t length = value.size();
            if (length == 0) return false;
            if (!file.write(reinterpret_cast<const char*>(&length), sizeof(length)))return false;
            if (!file.write(value.data(), length))return false;
            return true;
        }
        template<>inline bool WriteCacheToFile<std::wstring>(std::ofstream& file, const std::wstring& wstr) {
            if (!file) return  false;
            auto length = wstr.size();
            if (!file.write(reinterpret_cast<const char*>(&length), sizeof(length)).good()) return false;
            if (!file.write(reinterpret_cast<const char*>(wstr.c_str()), wstr.size() * sizeof(wchar_t))) return false;
            return true;
        }
        namespace nonstd {
            static std::recursive_mutex MemoizationGetCurrentTimeMtx;
            static auto program_start = std::chrono::high_resolution_clock::now();
            static CacheValidTimeType count = 0;
            struct scope_lock {
                explicit inline scope_lock(std::recursive_mutex& mtx) : m_mutex(mtx), m_locked(false) { Lock(); }
                inline ~scope_lock() { Unlock(); }
                SAFE_BUFFER inline void Lock() const noexcept { if (!m_locked && m_mutex.try_lock())m_locked = true; }
                SAFE_BUFFER inline void Unlock()const noexcept { if (m_locked) m_mutex.unlock(), m_locked = false; }
                std::recursive_mutex& m_mutex;
                mutable std::atomic<bool> m_locked;
            };
            template<typename T>class ObjectPool :public SingleTon<ObjectPool<T>> {
                std::unordered_map<std::size_t, T*> m_pool;
                unsigned char* objectpool;
                CacheValidTimeType Size = 0;
                CacheValidTimeType offset = 0;
                std::recursive_mutex ObjectPoolMtx;
            public:
                inline ObjectPool() : offset(0) {
                    Size = (4 * KB / sizeof(T)) * sizeof(T);
                    objectpool = new unsigned char[Size];
                    memset(objectpool, 0, Size);
                }
                ~ObjectPool() {
                    scope_lock lock(ObjectPoolMtx);
                    m_pool.clear();
                    delete[] objectpool;
                }
                template<typename U, typename... Args>SAFE_BUFFER inline T& operator()(const U& prefix, Args&&... args)noexcept {
                    auto hash = typeid(T).hash_code() ^ std::TupleHasher<Args...>::Hasher(prefix);
                    auto it = m_pool.find(hash);
                    if (LIKELY(it != m_pool.end()))return *it->second;
                    if (offset + sizeof(T) > Size) {
                        Size *= 2;
                        auto newpool = new unsigned char[Size];
                        memcpy_s(newpool, Size, objectpool, Size / 2);
                        scope_lock lock(ObjectPoolMtx);
                        for (auto& pair : m_pool)pair.second = reinterpret_cast<T*>(newpool + (reinterpret_cast<unsigned char*>(pair.second) - objectpool));
                        delete[] objectpool;
                        objectpool = newpool;
                    }
                    auto newObj = new (objectpool + offset) T(std::forward<Args>(args)...);//placement new
                    offset += sizeof(T);
                    std::ignore = std::async([&]() {scope_lock lock(MemoizationGetCurrentTimeMtx);  m_pool[hash] = newObj; });
                    return *newObj;
                }
            };
            static CacheValidTimeType currenttime = 0;
            SAFE_BUFFER static inline CacheValidTimeType ApproximateGetCurrentTime() noexcept {
                if (LIKELY(count / 4 != 0)) {
                    return currenttime;
                }
                else {
                    std::ignore = std::async([&]() {
                        scope_lock lock(MemoizationGetCurrentTimeMtx);
                        currenttime = std::chrono::duration_cast<std::chrono::microseconds>(program_start.time_since_epoch()).count();
                        if (count > INFINITYCACHE - 1) count = 0;
                        ++count;
                        });
                }
                return currenttime;
            }
            template<typename T>
            SAFE_BUFFER static inline T SafeAdd(const T& a, const T& b) noexcept {
                constexpr auto infnity = std::numeric_limits<T>::infinity();
                return (LIKELY(b > infnity - a)) ? infnity : a + b;
            }
            
            template<std::size_t... Indices>struct index_sequence {};
            template<std::size_t N, std::size_t... Indices>struct make_index_sequence : make_index_sequence<N - 1, N - 1, Indices...> {};
            template<std::size_t... Indices>struct make_index_sequence<0, Indices...> : index_sequence<Indices...> {};
            template<typename F, typename Tuple, std::size_t... Indices>SAFE_BUFFER auto inline ApplyImpl(F&& f, Tuple&& tuple, const index_sequence<Indices...>&)noexcept {
                return std::forward<F>(f)(std::get<Indices>(std::forward<Tuple>(tuple))...);
            }
            template<typename F, typename Tuple, typename Index = nonstd::make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>>SAFE_BUFFER inline auto Apply(F&& f, Tuple&& tuple)noexcept {
                static Index  sequence{};
                return ApplyImpl(std::forward<F>(f), std::forward<Tuple>(tuple), sequence);
            }
            static inline std::string SizeToString(size_t value) noexcept {
                if (value == 0) return "0";
                std::string result{};
                while (value > 0) {
                    char digit = '0' + (value % 10);
                    result = digit + result;
                    value /= 10;
                }
                return result;
            }
            static inline std::string PathTruncate(const std::string& path) noexcept {
                std::string result = path;
                if (result.size() > MEMOIZATIONSEARCH)result.resize(MEMOIZATIONSEARCH);
                result.erase(std::remove_if(result.begin(), result.end(), [](unsigned char c) { return !std::isalnum(c) && c != '/'; }), result.end());
                std::replace(result.begin(), result.end(), '\\', '/');
                return result;
            }
            enum class CallType : unsigned char {
#ifdef _WIN64
                cdeclcall, stdcall, fastcall,
#else
                cdeclcall = 0, stdcall = 0, fastcall = 0,
#endif // _WIN64
            };
            template<typename R, typename... Args>struct CallConv_cdecl { using type = R(CDECLCALL*)(Args...); };
            template<typename R>struct CallConv_cdecl_noargs { using type = R(CDECLCALL*)(); };
#ifdef _WIN64
            template<typename R, typename... Args>struct CallConv_stdcall { using type = R(STDCALL*)(Args...); };
            template<typename R>struct CallConv_stdcall_noargs { using type = R(STDCALL*)(); };
            template<typename R, typename... Args>struct CallConv_fastcall { using type = R(FASTCALL*)(Args...); };
            template<typename R>struct CallConv_fastcall_noargs { using type = R(FASTCALL*)(); };
#endif // _WIN64
            struct CachedFunctionBase {
                mutable void* m_funcAddr;
                mutable CallType m_callType;
                mutable CacheValidTimeType m_cacheTime;
                mutable std::recursive_mutex m_mutex;
                inline  CachedFunctionBase() :m_funcAddr(nullptr), m_callType(CallType::cdeclcall), m_cacheTime(0) {};
                inline  CachedFunctionBase(void* funcAddr, CallType type, CacheValidTimeType cacheTime = MEMOIZATIONSEARCH) : m_funcAddr(funcAddr), m_callType(type), m_cacheTime(cacheTime) {}
                inline void SetCacheTime(CacheValidTimeType cacheTime) noexcept { m_cacheTime = cacheTime; }
                inline CacheValidTimeType GetCacheTime() const { return m_cacheTime; }
                inline operator void* () { return m_funcAddr; }
                inline bool operator==(const CachedFunctionBase& others) noexcept { return m_funcAddr == others.m_funcAddr; }
                template<typename Func>inline bool operator==(Func&& f) noexcept { return m_funcAddr == &f; }
                inline bool operator!=(const CachedFunctionBase& others) noexcept { return m_funcAddr != others.m_funcAddr; }
                template<typename Func>inline bool operator!=(Func&& f) noexcept { return m_funcAddr != &f; }
                inline operator bool()noexcept { return (bool)m_funcAddr; }//check func is valid
            };
            template<typename R>using CacheitemType = std::pair<std::decay_t<R>, CacheValidTimeType>;
            template<typename R, typename... Args>struct CachedFunction : public CachedFunctionBase {
                using FuncType = R(std::decay_t<Args>...);
                mutable std::function<FuncType> m_func;
                using KeyType = std::tuple<std::decay_t<Args>...>;
                using ValueType = CacheitemType<R>;
                using CacheType = std::unordered_map<KeyType, ValueType>;
                mutable std::unique_ptr<CacheType> m_cache;
                mutable CacheType* m_cacheinstance = nullptr;
                using iterator = typename CacheType::iterator;
                mutable iterator m_cacheend;
                inline iterator begin() { return m_cache->begin(); }
                inline iterator end() { return m_cache->end(); }
                inline bool operator<(const CachedFunction& others) { return m_cache->size() < others.m_cache->size(); }
                inline bool operator>(const CachedFunction& others) { return m_cache->size() > others.m_cache->size(); }
                inline bool operator<=(const CachedFunction& others) { return m_cache->size() <= others.m_cache->size(); }
                inline auto Raw() {
#ifdef _WIN64
                    switch (m_callType) {
                    case CallType::cdeclcall: break;
                    case CallType::stdcall:   return reinterpret_cast<typename CallConv_stdcall<R, Args...>::type>(m_funcAddr);
                    case CallType::fastcall:  return reinterpret_cast<typename CallConv_fastcall<R, Args...>::type>(m_funcAddr);
                    }
#endif
                    return reinterpret_cast<typename CallConv_cdecl<R, Args...>::type>(m_funcAddr);
                }
                inline auto operator&() { return this->Raw(); }
                inline bool operator>=(const CachedFunction& others) {
                    return m_cache->size() >= others.m_cache->size();
                }
                inline CachedFunction(const CachedFunction& others) : CachedFunctionBase(others.m_funcAddr, others.m_callType, others.m_cacheTime), m_func(others.m_func), m_cache(std::make_unique<CacheType>()) {
                    if (!others.m_cache->empty())for (const auto& pair : *others.m_cache)m_cache->insert(pair);
                    if (m_cache) {
                        m_cache->reserve(MEMOIZATIONSEARCH);
                        m_cacheend = m_cache->end();
                        m_cacheinstance = m_cache.get();
                    }
                }
                inline CachedFunction(const std::initializer_list<std::pair<KeyType, ValueType>>& list) : CachedFunctionBase(nullptr, CallType::cdeclcall, MEMOIZATIONSEARCH), m_cache(std::make_unique<CacheType>()) {
                    for (auto& item : list)m_cache->insert(item);
                    if (m_cache) {
                        m_cache->reserve(MEMOIZATIONSEARCH);
                        m_cacheend = m_cache->end();
                        m_cacheinstance = m_cache.get();
                    }
                }
                inline CachedFunction(CachedFunction&& other) noexcept : CachedFunctionBase(other.m_funcAddr, other.m_callType, other.m_cacheTime), m_func(std::move(other.m_func)), m_cache(std::move(other.m_cache)) {
                    if (m_cache) {
                        m_cache->reserve(MEMOIZATIONSEARCH);
                        m_cacheend = m_cache->end();
                        m_cacheinstance = m_cache.get();
                    }
                }
                inline CachedFunction& operator=(CachedFunction&& others)noexcept {
                    m_func = std::move(others.m_func);
                    m_cache = std::move(others.m_cache);
                    m_funcAddr = others.m_funcAddr;
                    others.m_funcAddr = 0;
                    if (m_cache) {
                        m_cacheend = m_cache->end();
                        m_cacheinstance = m_cache.get();
                    }
                    return *this;
                }
                inline auto operator*()noexcept { return *m_cache; }
                inline CachedFunction& operator=(const CachedFunction& others)noexcept {
                    scope_lock lock(m_mutex);
                    m_cache = std::make_unique<CacheType>();
                    if (!others.m_cache->empty())for (const auto& pair : *others.m_cache) m_cache->insert(pair);
                    m_func = others.m_func;
                    m_funcAddr = others.m_funcAddr;
                    if (m_cache) {
                        m_cacheinstance = m_cache.get();
                        m_cacheend = m_cache->end();
                    }
                    return *this;
                }
                inline operator std::function<FuncType>()noexcept { return m_func; }
                inline CachedFunction& operator+(const CachedFunction& others) noexcept {
                    scope_lock lock(m_mutex);
                    if (!others.m_cache->empty())for (const auto& pair : *others.m_cache)m_cache->insert(pair);
                    return *this;
                }
                inline CachedFunction& operator+=(const CachedFunction& others) noexcept {
                    scope_lock lock(m_mutex);
                    if (!others.m_cache->empty())for (const auto& pair : *others.m_cache)m_cache->insert(pair);
                    return *this;
                }
                inline CachedFunction& operator-(const CachedFunction& others)noexcept {
                    scope_lock lock(m_mutex);
                    for (const auto& pair : *m_cache) {
                        auto it = others.m_cache->find(pair.first);
                        if (it != others.m_cache->end())m_cache->erase(pair.first);
                    }
                    return *this;
                }
                inline CachedFunction& operator-=(const CachedFunction& others) noexcept {
                    scope_lock lock(m_mutex);
                    for (const auto& pair : *m_cache) {
                        auto it = others.m_cache->find(pair.first);
                        if (it != others.m_cache->end())m_cache->erase(pair.first);
                    }
                    return *this;
                }
                inline CachedFunction(void* funcAddr, CallType type, const std::function<FuncType>& func, CacheValidTimeType cacheTime = MEMOIZATIONSEARCH) : CachedFunctionBase(funcAddr, type, cacheTime), m_func(std::move(func)) {
                    scope_lock lock(m_mutex);
                    m_cache = std::make_unique<CacheType>();
                    if (m_cache) {
                        m_cache->reserve(MEMOIZATIONSEARCH);
                        m_cacheend = m_cache->end();
                        m_cacheinstance = m_cache.get();
                    }
                }
                SAFE_BUFFER inline R operator()(const Args&... args) const noexcept {
                    auto&& argsTuple = std::forward_as_tuple(args...);//构造的开销其实最大要放的最远
                    CacheValidTimeType now = ApproximateGetCurrentTime();
                    auto it = m_cacheinstance->find(argsTuple);
                    if (LIKELY(it != m_cacheend)) {
                        auto&& value = *&(*it).second;
                        if (LIKELY(value.second > now)) return value.first;
                    }
                    R&& ret = nonstd::Apply(m_func, argsTuple);//thread safe execution still exists ,NRVO
                    std::ignore = std::async([argsTuple = std::move(argsTuple), ret, now, this]() {
                        if (LIKELY(m_cacheinstance)) {
                            scope_lock lock(m_mutex);
                            m_cacheinstance->emplace(argsTuple, ValueType{ ret,SafeAdd(now,m_cacheTime) });
                            if (m_cacheinstance->size() > MEMOIZATIONSEARCH) RemoveAllExpire();
                            if (m_cacheinstance->load_factor() > 0.75f)m_cacheinstance->rehash(m_cacheinstance->bucket_count() >> 1);
                            m_cacheend = m_cacheinstance->end();
                        }
                        });
                    return ret;
                }
                inline void CleanCache() const noexcept { std::ignore = std::async([&]() {scope_lock lock(m_mutex); m_cache->clear(); }); }
                inline void CleanCache(const KeyType& parameters) const noexcept {
                    scope_lock lock(m_mutex);
                    auto it = m_cache->find(parameters);
                    if (LIKELY(it != m_cache->end()))  m_cache->erase(it->first);
                }
                inline void SetCacheResetTime(const KeyType& parameters, CacheValidTimeType cacheTime) const noexcept {
                    scope_lock lock(m_mutex);
                    auto it = m_cache->find(parameters);
                    if (LIKELY(it != m_cache->end())) it->second = SafeAdd(ApproximateGetCurrentTime(), cacheTime);
                }
                inline std::string GetUniqueName(const char* szFileName) { return nonstd::PathTruncate(szFileName + nonstd::SizeToString(typeid(decltype(m_func)).hash_code())); }
                inline bool SaveCache(const char* szFileName)noexcept {
                    std::ofstream file(GetUniqueName(szFileName), std::ios::binary | std::ios::trunc);
                    if (!operator>>(file)) return false;
                    file.close();
                    return true;
                }
                inline bool LoadCache(const char* szFileName)noexcept {
                    std::ifstream file(GetUniqueName(szFileName), std::ios::binary);
                    if (!operator<<(file)) return false;
                    file.close();
                    return true;
                }
                inline bool operator>>(std::ofstream& file) {
                    if (!file.is_open()) return false;
                    for (auto& pair : *m_cache)if (!WriteTupleToFile(file, pair.first) || !WritePairToFile(file, pair.second))continue;
                    return true;
                }
                inline bool operator<<(std::ifstream& file) noexcept {
                    if (!file.is_open()) return false;
                    scope_lock lock(m_mutex);
                    while (file) {
                        KeyType key{};
                        ValueType value{};
                        if (!ReadTupleFromFile(file, key) || !ReadPairFromFile(file, value)) continue;
                        if (value.second > ApproximateGetCurrentTime())m_cache->insert(std::make_pair(key, value));
                    }
                    return true;
                }
                inline void operator>>(const CachedFunction& others) noexcept { for (const auto& pair : *m_cache)others.m_cache->insert(pair); }
                inline void operator<<(const CachedFunction& others) noexcept {
                    scope_lock lock(m_mutex);
                    for (const auto& pair : *others.m_cache)m_cache->insert(pair);
                }
                friend inline std::ostream& operator<<(std::ostream& os, const CachedFunction& func)noexcept { return os << func.m_cache->size() << std::endl; }
                constexpr inline std::function<FuncType>* operator->()noexcept { return &m_func; }
                inline bool empty()const noexcept { return m_cache->empty(); }
                inline std::size_t size()const noexcept { return m_cache->size(); }
                SAFE_BUFFER inline iterator operator[](const KeyType& key) noexcept {
                    if (m_cache)return  m_cache->find(key);
                    return m_cache->end();
                }
                inline void RemoveAllExpire() const {
                    if (!m_cache)return;
                    scope_lock lock(m_mutex);
                    for (auto it = m_cache->begin(); it != m_cache->end();) {
                        if (it->second.second <= ApproximateGetCurrentTime()) {
                            it = m_cache->erase(it);
                        }
                        else {
                            ++it;
                        }
                    }
                }
            };
            template<typename R> struct CachedFunction<R> : public CachedFunctionBase {
                mutable std::function<R()> m_func;
                using func_type = R();
                mutable CacheitemType<R> m_cache;
                inline CachedFunction(const CacheitemType<R>& cache) :m_func(nullptr), m_cache(cache) {}
                inline CachedFunction(void* funcAddr, CallType type, const std::function<R()>& func, CacheValidTimeType cacheTime = MEMOIZATIONSEARCH) : CachedFunctionBase(funcAddr, type, cacheTime), m_func(std::move(func)), m_cache(std::make_pair(R{}, 0)) {}
                inline R& SetCacheResetTime(const R& value, const  CacheValidTimeType  cacheTime = ApproximateGetCurrentTime())const noexcept {
                    scope_lock lock(m_mutex);
                    m_cache = std::make_pair(value, cacheTime);
                    return m_cache.first;
                }
                inline operator bool()noexcept { return (bool)m_func; }
                inline operator std::function<R()>()noexcept { return m_func; }
                constexpr inline CachedFunction(CachedFunction&& other) noexcept : CachedFunctionBase(other.m_funcAddr, other.m_callType, other.m_cacheTime), m_func(std::move(other.m_func)), m_cache(std::move(other.m_cache)) {
                    m_funcAddr = std::move(other.m_funcAddr);
                }
                inline CachedFunction& operator=(CachedFunction&& others) noexcept {
                    m_func = std::move(others.m_func);
                    m_cache = std::move(others.m_cache);
                    m_funcAddr = others.m_funcAddr;
                    others.m_funcAddr = nullptr;
                    return *this;
                }
                inline CachedFunction(const CachedFunction& others) :CachedFunctionBase(others.m_funcAddr, others.m_callType, others.m_cacheTime), m_cache(others.m_cache), m_func(others.m_func) {}
                inline CachedFunction& operator=(const CachedFunction& others)noexcept {
                    m_cache = others.m_cache;
                    m_func = others.m_func;
                    m_funcAddr = others.m_funcAddr;
                    return *this;
                }
                inline auto Raw() noexcept {
#ifdef _WIN64
                    switch (m_callType) {
                    case CallType::cdeclcall: break;
                    case CallType::stdcall:return reinterpret_cast<typename CallConv_stdcall_noargs<R>::type>(m_funcAddr);
                    case CallType::fastcall:return reinterpret_cast<typename CallConv_fastcall_noargs<R>::type>(m_funcAddr);
                    }
#endif
                    return reinterpret_cast<typename CallConv_cdecl_noargs<R>::type>(m_funcAddr);
                }
                inline auto operator&() { return this->Raw(); }
                inline bool LoadCache(const char* szFileName)noexcept {
                    std::ifstream file(nonstd::PathTruncate(szFileName + nonstd::SizeToString(typeid(decltype(m_func)).hash_code())), std::ios_base::binary);
                    if (!file.is_open() || !operator<<(file)) return false;
                    file.close();
                    return true;
                }
                inline bool SaveCache(const char* szFileName) noexcept {
                    std::ofstream file(nonstd::PathTruncate(szFileName + nonstd::SizeToString(typeid(decltype(m_func)).hash_code())), std::ios::binary | std::ios::trunc);
                    if (!file.is_open() || !operator>>(file)) return false;
                    file.close();
                    return true;
                }
                inline bool operator >> (const std::ofstream& file) noexcept {
                    if (!file.is_open()) return false;
                    if (!WritePairToFile(file, m_cache))return false;
                    return true;
                }
                inline bool operator << (const std::ifstream& file) noexcept {
                    if (!file.is_open()) return false;
                    scope_lock lock(m_mutex);
                    if (!ReadPairFromFile(file, m_cache))return false;
                    return true;
                }
                inline void operator >> (const CachedFunction& others)noexcept { others.m_cache = m_cache; }
                inline void operator << (const CachedFunction& others) noexcept { m_cache = others.m_cache; }
                friend inline std::ostream& operator<<(std::ostream& os, const CachedFunction&)noexcept { return os << 1; }
                SAFE_BUFFER inline R& operator()() const noexcept {
                    if ((CacheValidTimeType)m_cache.second > (CacheValidTimeType)ApproximateGetCurrentTime()) return m_cache.first;
                    return SetCacheResetTime(m_func(), (CacheValidTimeType)(SafeAdd(ApproximateGetCurrentTime(), m_cacheTime)));
                }
                inline void CleanCache()const noexcept { m_cache.second = ApproximateGetCurrentTime() - 1, m_cache.first = R(); }
                constexpr inline std::function<R()>* operator->()noexcept { return &m_func; }
                inline bool empty()const noexcept { return std::hash(m_cache.first) == std::hash(R{}); }
                inline std::size_t size()const noexcept { return 1; }
                inline R& operator[](int) { return m_cache.first; }
            };
            template <typename F>struct function_traits : function_traits<decltype(&F::operator())> {};
            template <typename R, typename... Args>struct function_traits<R(*)(Args...)> {
                using return_type = R;
                using args_tuple_type = std::tuple<Args...>;
            };
            template <typename R>struct function_traits<R(*)()> {
                using return_type = R;
                using args_tuple_type = std::tuple<>;
            };
            template <typename R, typename... Args>struct function_traits<std::function<R(Args...)>> {
                using return_type = R;
                using args_tuple_type = std::tuple<Args...>;
            };
            template <typename R>struct function_traits<std::function<R()>> {
                using return_type = R;
                using args_tuple_type = std::tuple<>;
            };
            template <typename ClassType, typename R, typename... Args>
            struct function_traits<R(ClassType::*)(Args...) const> {
                using return_type = R;
                using args_tuple_type = std::tuple<Args...>;
            };
            template <typename ClassType, typename R>
            struct function_traits<R(ClassType::*)() const> {
                using return_type = R;
                using args_tuple_type = std::tuple<>;
            };
            template<typename ClassType, typename R, typename... Args>
            struct function_traits<R(ClassType::*)(Args...)> : function_traits<R(Args...)> {};
#ifdef _WIN64
            template <typename F>struct stdcallfunction_traits : stdcallfunction_traits<decltype(&F::operator())> {};
            template <typename R, typename... Args>struct stdcallfunction_traits<R(STDCALL*)(Args...)> :public function_traits<R(STDCALL*)(Args...)> {};
            template <typename R>struct stdcallfunction_traits<R(STDCALL*)()> :public function_traits<R(STDCALL*)()> {};
            template <typename ClassType, typename R, typename... Args>struct stdcallfunction_traits<R(STDCALL ClassType::*)(Args...)> :public function_traits<R(STDCALL ClassType::*)(Args...)> {};
            template <typename ClassType, typename R>struct stdcallfunction_traits<R(STDCALL ClassType::*)()> :public function_traits<R(STDCALL ClassType::*)()> {};
            template <typename ClassType, typename R, typename... Args>struct stdcallfunction_traits<R(STDCALL ClassType::*)(Args...) const> :public function_traits<R(STDCALL ClassType::*)(Args...) const > {};
            template <typename ClassType, typename R>struct stdcallfunction_traits<R(STDCALL ClassType::*)() const> :public function_traits<R(STDCALL ClassType::*)() const > {};
            template <typename F>struct fastcallfunction_traits : fastcallfunction_traits<decltype(&F::operator())> {};
            template <typename R, typename... Args>struct fastcallfunction_traits<R(FASTCALL*)(Args...)> :public function_traits<R(FASTCALL*)(Args...)> {};
            template <typename R>struct fastcallfunction_traits<R(FASTCALL*)()> :public function_traits<R(FASTCALL*)()> {};
            template <typename ClassType, typename R, typename... Args>struct fastcallfunction_traits<R(FASTCALL ClassType::*)(Args...)> :public function_traits<R(FASTCALL ClassType::*)(Args...)> {};
            template <typename ClassType, typename R>struct fastcallfunction_traits<R(FASTCALL ClassType::*)()> :public function_traits<R(FASTCALL ClassType::*)()> {};
            template <typename ClassType, typename R, typename... Args>struct fastcallfunction_traits<R(FASTCALL ClassType::*)(Args...) const> :public function_traits<R(FASTCALL ClassType::*)(Args...) const> {};
            template <typename ClassType, typename R>struct fastcallfunction_traits<R(FASTCALL ClassType::*)() const> :public function_traits <R(FASTCALL ClassType::*)() const> {};
#endif // _WIN64
            template <typename R, typename... Args>constexpr inline static auto& GetCachedFunction(void* funcAddr, CallType type, const std::function<R(Args...)>& func, CacheValidTimeType cacheTime = MEMOIZATIONSEARCH) noexcept {
                return  ObjectPool<CachedFunction<R, Args...>>::GetInstance()(cacheTime, funcAddr, type, func, cacheTime);
            }
            template <typename R>constexpr inline static auto GetCachedFunction(void* funcAddr, CallType type, const std::function<R()>& func, CacheValidTimeType cacheTime = MEMOIZATIONSEARCH) noexcept { return  CachedFunction<R>(funcAddr, type, func, cacheTime); }
            template<typename F, std::size_t... Is, typename DecayF = std::decay_t<F>>constexpr inline static auto MakeCachedImpl(F&& f, CallType type, CacheValidTimeType validtime, const std::index_sequence<Is...>&) noexcept {
                using Functype = typename function_traits<DecayF>::return_type(typename std::tuple_element<Is, typename function_traits<DecayF>::args_tuple_type>::type...);
                return GetCachedFunction((void*)&f, type, std::function<Functype>(std::forward<F>(f)), validtime);
            }
            template<typename T, typename R, typename... Args>SAFE_BUFFER inline auto ThisCall(T& obj, R(T::* func)(Args...))noexcept { return [&obj, func](const Args&... args) -> R {return (obj.*func)(args...); }; }
            template<typename T, typename F>inline auto ConvertMemberFunction(T& obj, F&& func)noexcept { return ThisCall(obj, std::forward<F>(func)); }
#ifdef _MSC_VER
#pragma  warning(  pop  )
#endif
        }
        template<typename T> using ArgsType_v = std::make_index_sequence<std::tuple_size<T>::value>;
        template<typename F>inline  auto makecacheex(F&& f, const  memoizationsearch::nonstd::CallType& calltype, CacheValidTimeType _validtime)noexcept {
            auto validtime = Clamp(_validtime, 1ULL, INFINITYCACHE);
#ifdef _WIN64
            switch (calltype) {
            case  memoizationsearch::nonstd::CallType::cdeclcall:break;
            case  memoizationsearch::nonstd::CallType::stdcall:  return  memoizationsearch::nonstd::MakeCachedImpl(f, calltype, validtime, memoizationsearch::ArgsType_v<typename  memoizationsearch::nonstd::stdcallfunction_traits<std::decay_t<F>>::args_tuple_type>{});
            case  memoizationsearch::nonstd::CallType::fastcall: return  memoizationsearch::nonstd::MakeCachedImpl(f, calltype, validtime, memoizationsearch::ArgsType_v<typename  memoizationsearch::nonstd::fastcallfunction_traits<std::decay_t<F>>::args_tuple_type>{});
            }
#endif // _WIN64
            return  memoizationsearch::nonstd::MakeCachedImpl(f, calltype, validtime, memoizationsearch::ArgsType_v<typename  memoizationsearch::nonstd::function_traits<std::decay_t<F>>::args_tuple_type>{});
        }
        template<typename R, typename...Args> constexpr inline auto& MakeCacheItem(const R& ret, CacheValidTimeType validtime, const Args&...args) noexcept { return std::make_pair(std::make_tuple(args...), std::make_pair(ret, validtime)); }
        template<typename F>constexpr inline auto MakeCache(F&& f, const CacheValidTimeType validtime = MEMOIZATIONSEARCH) noexcept { return makecacheex(f, memoizationsearch::nonstd::CallType::cdeclcall, validtime); }
        template<typename T, class F> inline auto CacheMemberFunction(T&& obj, F&& func) noexcept { return MakeCache(memoizationsearch::nonstd::ConvertMemberFunction(std::forward<T>(obj), std::forward<F>(func))); }
#endif
    }
}

