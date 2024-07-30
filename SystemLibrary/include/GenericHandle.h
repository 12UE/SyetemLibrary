#ifndef GENRICHANDLE
#define GENRICHANDLE
namespace SystemLibrary {
    namespace Generic {
        namespace CallBacks {
            std::function<DWORD(HANDLE, DWORD)> pWaitForSingleObject = WaitForSingleObject;
            std::function<void(HANDLE)> pCloseHandle = CloseHandle;
            template<typename FuncType>
            static INLINE void SetCallBack(FuncType&& callBack, std::function<FuncType>& pCallBack) NOEXCEPT {
                pCallBack = std::forward<FuncType>(callBack);
            }
            template<typename Func, typename... Args>
            static INLINE AUTOTYPE OnCallBack(const std::function<Func>& pCallBack, Args&&... args) NOEXCEPT {
                return (pCallBack) ? pCallBack(std::forward<Args>(args)...) : decltype(pCallBack(std::forward<Args>(args)...))();
            }
        }
        struct NormalHandle {
            INLINE static void Close(HANDLE& handle)NOEXCEPT {
                CallBacks::OnCallBack(CallBacks::pCloseHandle, handle);
                handle = InvalidHandle();
            }
            INLINE static HANDLE InvalidHandle()NOEXCEPT { return INVALID_HANDLE_VALUE; }
            INLINE static bool IsValid(HANDLE handle)NOEXCEPT { return handle != InvalidHandle() && handle && (uintptr_t)handle > 0; }
            INLINE static DWORD Wait(HANDLE handle, DWORD time)NOEXCEPT { return CallBacks::OnCallBack(CallBacks::pWaitForSingleObject, handle, time); }

        };
        struct FileHandle :public NormalHandle {
            INLINE static void Close(HANDLE& handle)NOEXCEPT {
                FindClose(handle);
                handle = InvalidHandle();
            }
        };
        template<class Ty>
        struct HandleView :public Ty {
            INLINE static void Close(HANDLE& handle)NOEXCEPT {
            }
        };
        template<class T, class Traits>
        class GenericHandle {
            bool m_bOwner = false;
            int refcount = 1;
        public:
            INLINE bool IsValid()NOEXCEPT { return Traits::IsValid(m_handle); }
            T m_handle = Traits::InvalidHandle();
            GenericHandle(const T& handle = Traits::InvalidHandle(), bool bOwner = true) :m_handle(handle), m_bOwner(bOwner) {}
            virtual ~GenericHandle() {
                Release();
            }
            GenericHandle(GenericHandle&) = delete;
            GenericHandle& operator =(const GenericHandle&) = delete;
            INLINE GenericHandle& operator =(GenericHandle&& other)NOEXCEPT {
                if (m_handle != other.m_handle) {
                    m_handle = other.m_handle;
                    m_bOwner = other.m_bOwner;
                    refcount = other.refcount;
                    other.m_handle = Traits::InvalidHandle();
                    other.m_bOwner = false;
                    other.refcount = 0;
                }
                return *this;
            }
            virtual INLINE DWORD Wait(DWORD time = INFINITE)NOEXCEPT {
                return Traits::Wait(m_handle, time);
            }
            virtual INLINE bool operator==(const T& handle)NOEXCEPT {
                return m_handle == handle;
            }
            virtual INLINE bool operator!=(const T& handle)NOEXCEPT {
                return m_handle != handle;
            }
            virtual INLINE operator T() NOEXCEPT {
                return get();
            }
            virtual INLINE operator bool()NOEXCEPT {
                return IsValid();
            }
            virtual INLINE T* operator&()NOEXCEPT {
                return &m_handle;
            }
            virtual INLINE Traits* operator->()NOEXCEPT {
                return (Traits*)this;
            }
            virtual INLINE T get()NOEXCEPT {
                if (IsValid()) {
                    return m_handle;
                }
                else {
                    return Traits::InvalidHandle();
                }
            }
            INLINE void Release() {
                if (refcount > 0) refcount--;
                if (refcount == 0) {
                    if (m_bOwner && IsValid()) {
                        Traits::Close(m_handle);
                        m_bOwner = false;
                    }
                }
            }
            virtual INLINE void reset()NOEXCEPT {
                m_bOwner = false;
                Release();
                m_handle = Traits::InvalidHandle();
            }
            virtual INLINE void attatch()NOEXCEPT {
                m_bOwner = true;
            }
            virtual INLINE void detach()NOEXCEPT {
                m_bOwner = false;
            }
        };
        using THANDLE = GenericHandle<HANDLE, NormalHandle>;
        using FHANDLE = GenericHandle<HANDLE, FileHandle>;
    }
}

#endif // !GENRICHANDLE
