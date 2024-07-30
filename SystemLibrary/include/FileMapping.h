#ifndef FILEMAPPING
#define FILEMAPPING
namespace SystemLibrary {
	namespace FileMapping {
		using UDWORD = unsigned long long;
		typedef class FileMapView {//�ļ�ӳ����ͼ �������ڴ����ļ�
		private:
			Generic::THANDLE m_hFile = NULL;
			Generic::THANDLE m_hMap = NULL;
			void* m_pView = nullptr;//�ļ�ָ��
			DWORD m_dwSize = 0;
			std::string m_FileName;
			bool m_Owner = false;//�Ƿ�ӵ���ļ� �����Ƿ�ر��ļ�
			bool m_IsRealFile = false;//�Ƿ�����ʵ�ļ�
			bool m_IsReadOnly = true;
			DWORD m_FileMapAccess = 0;
			DWORD m_CreateMappingAccess = 0;
			DWORD m_FileOpenAccess = 0;
			DWORD m_FileShareAccess = 0;
			bool FileExists(const std::string& FileName) {
				if (FileName.length() == 0) return false;
				return GetFileAttributesA(FileName.data()) != INVALID_FILE_ATTRIBUTES;
			}
		public:
			~FileMapView() { Close(); }
			FileMapView(const std::string& FileName = "", bool ReadOnly = true, DWORD dwSize = 4096) {
				m_IsReadOnly = ReadOnly;
				if (!m_IsReadOnly) {
					m_FileMapAccess = FILE_MAP_ALL_ACCESS;
					m_CreateMappingAccess = PAGE_READWRITE;
					m_FileOpenAccess = GENERIC_ALL;
					m_FileShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
				}
				else {
					m_FileMapAccess = FILE_MAP_READ;
					m_CreateMappingAccess = PAGE_READONLY;
					m_FileOpenAccess = GENERIC_READ;
					m_FileShareAccess = FILE_SHARE_READ;
				}
				if (!TryOpen(FileName, dwSize)) return;
			}
			BOOL TryOpen(const std::string& FileName, DWORD dwSize = 0) {
				if (FileExists(FileName)) {//�ļ�����������ļ�
					m_hFile = CreateFileA(FileName.data(), m_FileOpenAccess, m_FileShareAccess, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					m_IsRealFile = true;
					if (dwSize == 0) {
						dwSize = GetFileSize(m_hFile, NULL);
						m_dwSize = dwSize;
					}
				}
				m_hMap = OpenFileMappingA(m_FileMapAccess, FALSE, FileName.data());//���Դ����е��ļ�ӳ��
				if (m_hMap) {
					if (!m_IsRealFile) {
						m_hFile = INVALID_HANDLE_VALUE;
						m_hMap = CreateFileMappingA(m_hFile, NULL, m_CreateMappingAccess, 0, dwSize, FileName.data());
					}
					else {
						m_hMap = CreateFileMappingA(m_hFile, NULL, m_CreateMappingAccess, 0, 0, NULL);
					}
					m_Owner = true;
					
				}
				m_pView = MapViewOfFile(m_hMap, m_FileMapAccess, 0, 0, 0);
				if (!m_pView) {
					return FALSE;
				}
				m_FileName = FileName;
				m_dwSize = dwSize;
				return TRUE;
			}
			void Close() {
				if (!m_Owner) return;//����ӵ���� ���ر��ļ�
				if (m_pView != nullptr) {
					UnmapViewOfFile(m_pView);
					m_pView = nullptr;
				}
				m_dwSize = 0;
				m_Owner = false;//�ļ��رպ���ӵ��
			}
			BOOL FlushFile() {
				if (!m_pView || !m_dwSize) return FALSE;
				return FlushViewOfFile(m_pView, m_dwSize);
			}
			DWORD GetSize() { return m_dwSize; }
			const std::string& GetFileName() { return m_FileName; }
			template<typename T = void> T* ref() {
				if (!m_pView) std::runtime_error("FileMapView::get() m_pView is nullptr");
				return reinterpret_cast<T*>(m_pView);
			}
			template<typename T>
			T get() {
				if (!m_pView) std::runtime_error("FileMapView::get() m_pView is nullptr");
				return *reinterpret_cast<T*>(m_pView);
			}
			template<typename T>
			FileMapView& operator<<(const T& data) {
				WriteImage((PVOID)&data, sizeof(T));
				return *this;
			}
			template<typename T>
			FileMapView& operator>>(T& data) {
				ReadImage((PVOID)&data, sizeof(T));
				return *this;
			}
			bool IsValid() { return m_pView != nullptr; }
		private:
			DWORD WriteImage(PVOID pInputBufer, SIZE_T nSize, DWORD dwOffset = 0) {
				if (dwOffset > m_dwSize) {
					return 0;
				}
				if (m_pView && pInputBufer && nSize) {
					auto WriteSize = Clamp<SIZE_T>(nSize, 0, m_dwSize);
					auto pWriteAddr = (UDWORD)m_pView + dwOffset;
					memcpy((PVOID)pWriteAddr, pInputBufer, WriteSize);
					if (m_IsRealFile) FlushViewOfFile((PVOID)pWriteAddr, WriteSize);//ˢ���ļ�
					return (DWORD)WriteSize;
				};
				return 0;
			}
			DWORD ReadImage(PVOID pOutputBufer, SIZE_T nSize, DWORD dwOffset = 0) {
				if (dwOffset > m_dwSize) {
					return 0;
				}
				if (m_pView && pOutputBufer && nSize) {
					auto ReadSize = Clamp<SIZE_T>(nSize, 0, m_dwSize);
					auto pReadAddr = (UDWORD)m_pView + dwOffset;
					memcpy(pOutputBufer, (PVOID)pReadAddr, ReadSize);
					return (DWORD)ReadSize;
				}
				return 0;
			}
		}*PFileMapView;
	}
}
#endif // !FILEMAPPING

