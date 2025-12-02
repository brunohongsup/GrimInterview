#pragma once
#include <memory>
#include <string_view>
#include <string>
#include <mutex>
#include <queue>

class CHttpFile;

constexpr std::string_view g_szEquipmentId = "L01-3503-E";

constexpr std::string_view g_szEquipmentName = "PI Edge-Cutting";

struct Product;

struct IFileIO
{
	virtual bool DoFileIO() = 0;
};

struct HttpIO : IFileIO
{
	CString GetIp();
	
	UINT GetPort();
};

struct HttpPostIO : public HttpIO
{
	HttpPostIO() = delete;

	HttpPostIO(const CString& strUrl);

	bool DoFileIO() override;

	const double GenerateRandom(const double& min, const double& max) const;

	virtual bool PostData(CHttpFile*) = 0;

	CString m_strUrl;
};

struct PostEquipmentIO : public HttpPostIO
{
	PostEquipmentIO(bool bEquipmentStatus);

	bool PostData(CHttpFile* pFile) override;
	
	bool m_bEquipmentStatus;
};

struct PostProductIO : public HttpPostIO
{
	PostProductIO();

	bool PostData(CHttpFile* pFile) override;

	std::shared_ptr<Product> product;
};

struct PostFlatnessIO : public PostProductIO
{
	PostFlatnessIO() = default;

	bool PostData(CHttpFile* pFile) override;
};

struct PostBusbarIO :  PostProductIO
{
	PostBusbarIO() = default;

	bool PostData(CHttpFile* pFile) override;
};

struct FileCleaningIO : public IFileIO
{
	bool DoFileIO() override;
};

struct BackUpModelIO : public IFileIO
{
	bool DoFileIO() override;
};

struct LogIO : public IFileIO
{
public:

	LogIO() = delete;

	LogIO(const CString& str);

	bool DoFileIO() override;
};

class FileIOBackgroundJob
{
public:
	
	 ~FileIOBackgroundJob(); 

	static FileIOBackgroundJob* GetInstance();

	void Add(const std::shared_ptr<IFileIO>& pInfo);

	void Stop();
	
private:

	void fileSave();

	FileIOBackgroundJob();
	
	std::mutex m_mutex;
	
	std::queue<std::shared_ptr<IFileIO>> m_queue;

	static std::unique_ptr<FileIOBackgroundJob> s_pInstance;

	static std::mutex s_mtx;

	bool running;

};
