#include "pch.h"
#include "FileIOBackgroundJob.h"
#include <filesystem>
#include <afxinet.h>
#include <random>

#include "GrimInterview.h"
#include "json.hpp"
#include "Threadpool.h"
#include "Product.h"
#include "ProductManager.h"

using json = nlohmann::json;

std::mutex FileIOBackgroundJob::s_mtx;

std::unique_ptr<FileIOBackgroundJob> FileIOBackgroundJob::s_pInstance = nullptr;

FileIOBackgroundJob::~FileIOBackgroundJob()
{
	running = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

FileIOBackgroundJob* FileIOBackgroundJob::GetInstance()
{
	if (s_pInstance == nullptr)
	{
		std::lock_guard<std::mutex> lock(s_mtx);
		if (s_pInstance == nullptr)
			s_pInstance = std::unique_ptr<FileIOBackgroundJob>(new FileIOBackgroundJob());
	}

	return s_pInstance.get();
}

void FileIOBackgroundJob::Add(const std::shared_ptr<IFileIO>& pInfo)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_queue.push(pInfo);
}

void FileIOBackgroundJob::Stop()
{
	running = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		while (!m_queue.empty())
		{
			auto front = m_queue.front();
			m_queue.pop();
			front->DoFileIO();
		}
	}
}

void FileIOBackgroundJob::fileSave()
{
	while (running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		std::shared_ptr<IFileIO> pInfo{};
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_queue.empty())
				continue;

			pInfo = m_queue.front();
			m_queue.pop();
		}

		if (pInfo != nullptr)
			pInfo->DoFileIO();
	}
}

FileIOBackgroundJob::FileIOBackgroundJob()
	: running(false)
{
	running = true;
	const auto threadPool = Threadpool::GetInstance();
	threadPool->AddWork([this]{this->fileSave();});
}

HttpPostIO::HttpPostIO(const CString& strUrl)
	: m_strUrl(strUrl)
{

}

bool HttpPostIO::DoFileIO()
{
	const CString strServer = _T("localhost"); // or your server IP
	const INTERNET_PORT nPort = 7185;          // Your .NET Core port (usually 5000 or 7xxx)
	const auto pSession = theApp.GetSession();
	CHttpConnection* pConnection = NULL;
	CHttpFile* pFile = NULL;

	try
	{
		CString ip = GetIp();
		auto portNum = GetPort();
		INTERNET_PORT internetPort = portNum;
		pConnection = pSession->GetHttpConnection(ip.GetBuffer(), internetPort);

		// Update the connection flags to use SSL and ignore self-signed cert errors
		DWORD dwFlags = INTERNET_FLAG_RELOAD |
			INTERNET_FLAG_NO_CACHE_WRITE |
			INTERNET_FLAG_SECURE |                 // <--- Enables HTTPS
			INTERNET_FLAG_IGNORE_CERT_CN_INVALID | // <--- Ignores "localhost" name mismatch
			INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
		// 4. Open Request (POST)
		pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST,
			m_strUrl,
			NULL, 1, NULL, NULL,
			dwFlags);

		// 5. Add Headers
		// IMPORTANT: Tell server we are sending JSON
		CString strHeaders = _T("Content-Type: application/json; charset=utf-8");
		pFile->AddRequestHeaders(strHeaders);

		// 6. Convert CString (UTF-16) to UTF-8 for the web
		// This is crucial for .NET Core compatibility

		// 7. Send the Request
		PostData(pFile);

		// 8. Check Response Code
		DWORD dwStatusCode;
		pFile->QueryInfoStatusCode(dwStatusCode);

		if (dwStatusCode == 200)
		{
			// Success
			CString strLog;
			strLog.Format(_T("Data synced successfully!"));
			// ... log it ...
		}
		else
		{
			// --- ADD THIS BLOCK TO READ THE ERROR ---
			CString strResponse;
			CString strLine;
			// Read the "Explanation" sent by .NET Core
			while (pFile->ReadString(strLine))
			{
				strResponse += strLine;
			}

			// Show the error. 
			// Example output: {"type":"...","title":"One or more validation errors occurred.","errors":{"EquipmentId":["The value 'L01-3503-E' is not valid."]}}
			CString strErrorMsg;
			strErrorMsg.Format(_T("Server Rejected Data (400). Details:\n%s"), strResponse);
			AfxMessageBox(strErrorMsg);
		}
	}

	catch (CInternetException* pEx)
	{
		// Handle Network Errors
		TCHAR szErr[1024];
		pEx->GetErrorMessage(szErr, 1024);
		AfxMessageBox(szErr);
		pEx->Delete();
	}

	// 9. Cleanup
	if (pFile)
	{
		pFile->Close();
		delete pFile;
	}

	if (pConnection)
	{
		pConnection->Close();
		delete pConnection;
	}

	return true;
}

const double HttpPostIO::GenerateRandom(const double& min, const double& max) const
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(min, max);
	return dis(gen);
}

PostEquipmentIO::PostEquipmentIO(bool bEquipmentStatus)
	: HttpPostIO(_T("Equipments/Create"))
	, m_bEquipmentStatus(bEquipmentStatus)
{
	
}

bool PostEquipmentIO::PostData(CHttpFile* pFile)
{
	if (nullptr == pFile)
		return false;

	nlohmann::json j;
	j["EquipmentId"] = g_szEquipmentId;
	j["Name"] = g_szEquipmentName;
	j["Location"] = "Pajoo";
	j["Type"] = 1;
	j["IsActive"] = m_bEquipmentStatus;
	auto data = j.dump();
	if (pFile->SendRequest(NULL, 0, LPVOID(data.c_str()), static_cast<DWORD>(data.length())))
		return true;

	return false;
}

PostProductIO::PostProductIO()
	: HttpPostIO(_T("Products/Create"))
{
	const auto product = ProductManager::GetInstance()->GetLastProduct();
	this->product = product;
}

bool PostProductIO::PostData(CHttpFile* pFile)
{
	if (nullptr == pFile)
		return false;

	if (nullptr == product)
		return false;

	json j;
	// --- 3. Map Fields to C# Properties ---

	// C# ProductId (required) matches C++ Id
	j["ProductId"] = CT2CA(product->Id);

	// If your struct doesn't have it, send null or a hardcoded ID.
	j["EquipmentId"] = g_szEquipmentId;

	// C# ManufacturedAt (DateTime) matches C++ InputTime
	auto time = CT2CA(product->InputTime.Format(_T("%Y-%m-%dT%H:%M:%S")));
	j["ManufacturedAt"] = time;

	// C# Alignment Data (required doubles)
	j["dx"] = GenerateRandom(-3.0, 3.0);
	j["dy"] = GenerateRandom(-3.0, 3.0);
	j["theta"] = GenerateRandom(-1.2, 1.2);

	// --- 4. Handle Complex 'Curl' Data ---
	// C# expects an object { ValueA: ..., ValueB: ... }
	// C++ has an array dblCurlHeight[4]. 
	// We map the array indices to the JSON object properties.
	json curlObj;
	curlObj["Val1"] = GenerateRandom(7.0, 18.0);
	curlObj["Val2"] = GenerateRandom(7.0, 18.0); // Map Index 1
	curlObj["Val3"] = GenerateRandom(7.0, 18.0); // Map Index 1
	curlObj["Val4"] = GenerateRandom(7.0, 18.0); // Map Index 1

	// Note: If you need to send indices 2 and 3, you must 
	// update the C# CurlData class to have ValueC/ValueD

	j["Curl"] = curlObj;
	auto data = j.dump();
	if (pFile->SendRequest(NULL, 0, LPVOID(data.c_str()), static_cast<DWORD>(data.length())))
		return true;

	return false;
}

bool PostFlatnessIO::PostData(CHttpFile* pFile)
{
	if (nullptr == pFile)
		return false;

	if (nullptr == product)
		return false;

	json j;
	// --- 3. Map Fields to C# Properties ---

	// C# ProductId (required) matches C++ Id
	j["ProductId"] = CT2CA(product->Id);

	// If your struct doesn't have it, send null or a hardcoded ID.
	j["EquipmentId"] = g_szEquipmentId;

	// C# ManufacturedAt (DateTime) matches C++ InputTime
	auto time = CT2CA(product->InputTime.Format(_T("%Y-%m-%dT%H:%M:%S")));
	j["ManufacturedAt"] = time;

	// C# Alignment Data (required doubles)
	j["dx"] = GenerateRandom(-3.0, 3.0);
	j["dy"] = GenerateRandom(-3.0, 3.0);
	j["theta"] = GenerateRandom(-1.2, 1.2);

	// --- 4. Handle Complex 'Curl' Data ---
	// C# expects an object { ValueA: ..., ValueB: ... }
	// C++ has an array dblCurlHeight[4]. 
	// We map the array indices to the JSON object properties.
	json curlObj;
	curlObj["Val1"] = GenerateRandom(7.0, 18.0);
	curlObj["Val2"] = GenerateRandom(7.0, 18.0); // Map Index 1
	curlObj["Val3"] = GenerateRandom(7.0, 18.0); // Map Index 1
	curlObj["Val4"] = GenerateRandom(7.0, 18.0); // Map Index 1

	// Note: If you need to send indices 2 and 3, you must 
	// update the C# CurlData class to have ValueC/ValueD

	j["Curl"] = curlObj;
	auto data = j.dump();
	if (pFile->SendRequest(NULL, 0, LPVOID(data.c_str()), static_cast<DWORD>(data.length())))
		return true;

	return false;
}

bool PostBusbarIO::PostData(CHttpFile* pFile)
{
	return PostProductIO::PostData(pFile);
}

CString HttpIO::GetIp()
{
	// Use a buffer for the result
	TCHAR szResult[256]; 
	// 1. Prepare the path
	// Rename 'strModelPath' to 'strConfigPath' for clarity
	CString strConfigPath = _T("C:\\Ravid");

	// Safety: Check if the path already ends with '\' before adding another
	if (strConfigPath.Right(1) != _T("\\"))
	{
		strConfigPath += _T("\\");
	}
	strConfigPath += _T("ServerConfig.ini");

	// 2. Read the INI file
	// If the file or key is missing, it defaults to "localhost"
	GetPrivateProfileString(_T("Network"), 
					  _T("ServerIP"), 
					  _T("localhost"), 
					  szResult,           
					  256, 
					  strConfigPath);      
    
	// 3. Return
	// You can return szResult directly; CString constructs automatically
	return szResult;
}

UINT HttpIO::GetPort()
{
	CString strModelPath = 	_T("C:\\Ravid");
	strModelPath.AppendFormat(_T("\\ServerConfig.ini"));
	UINT nPort = GetPrivateProfileInt(_T("Network"), _T("Port"), 80, strModelPath.GetBuffer());
	
	return nPort;
}