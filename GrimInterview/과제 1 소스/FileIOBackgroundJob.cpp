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

double HttpPostIO::GenerateDouble(const double& min, const double& max) const
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(min, max);

	double val = dis(gen);

	// Round to 3 decimal places:
	// 1. Multiply by 1000 (1.23456 -> 1234.56)
	// 2. Round to nearest integer (1234.56 -> 1235)
	// 3. Divide by 1000 (1235 -> 1.235)
	return std::round(val * 1000.0) / 1000.0;
}

int HttpPostIO::GenerateInteger(const int& min, const int& max) const
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
    
	std::uniform_int_distribution<int> dis(min, max);
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
	j["EquipmentId"] = g_szEdgeCuttingEquipmentId;
	j["Name"] = g_szEquipmentName;
	j["Location"] = "Pajoo";
	j["Type"] = 1;
	j["IsActive"] = m_bEquipmentStatus;
	auto data = j.dump();
	if (pFile->SendRequest(NULL, 0, LPVOID(data.c_str()), static_cast<DWORD>(data.length())))
		return true;

	return false;
}

PostEdgeCuttingIO::PostEdgeCuttingIO()
	: HttpPostIO(_T("api/Products/Create"))
{
	const auto product = ProductManager::GetInstance()->GetLastProduct();
	this->product = product;
}

bool PostEdgeCuttingIO::PostData(CHttpFile* pFile)
{
	if (nullptr == pFile)
		return false;

	if (nullptr == product)
		return false;

	// 1. Create the Root Object (Matches ProductInputModel.cs)
	json root;
    
	// --- Metadata Fields ---
	root["ProductId"] = CT2CA(product->Id);
	root["EquipmentId"] = g_szEdgeCuttingEquipmentId; 
    
	// CRITICAL: Tell the server what kind of data this is.
	// This string must match what you check for in your C# UpdateChart logic.
	root["DataType"] = "EdgeCutting"; 

	// 2. Create the Payload Object (Matches GeometryPayload.cs)
	json payload;

	// --- Geometry Data ---
	// Note: I capitalized these keys (Dx, Dy) to match the C# properties.
	// However, if you set PropertyNameCaseInsensitive = true in C#, lowercase is fine too.
	payload["Dx"] = GenerateDouble(-3.0f, 3.0f);
	payload["Dy"] = GenerateDouble(-3.0f, 3.0f);
	payload["DTheta"] = GenerateDouble(-1.5f, 1.5f);
    
	// If you have 'Theta' in your C++ struct, add it here:
	// payload["Theta"] = product->dblTheta; 

	// --- Curl Data ---
	json curlObj;
	curlObj["Val1"] = GenerateDouble(7.0f, 20.0f);
	curlObj["Val2"] = GenerateDouble(7.0f, 20.0f);
	curlObj["Val3"] = GenerateDouble(7.0f, 20.0f);
	curlObj["Val4"] = GenerateDouble(7.0f, 20.0f);

	// Add Curl object into the Payload
	payload["Curl"] = curlObj;

	// 3. Attach the Payload to the Root
	// The C# API expects a property named "Payload" containing this object
	root["Payload"] = payload;

	// 4. Send the Request
	// We dump the 'root' object, which now contains the nested payload
	std::string data = root.dump();
	if (pFile->SendRequest(NULL, 0, (LPVOID)data.c_str(), (DWORD)data.length()))
		return true;

	return false;
}

bool PostFlatnessIO::PostData(CHttpFile* pFile)
{
	if (nullptr == pFile)
		return false;

	if (nullptr == product)
		return false;

	// Inside PostProductIO::PostData

	// 1. Root Object
	json root;
	root["ProductId"] = CT2CA(product->Id);
	root["EquipmentId"] = g_szFlatnessVisionEquipmentId;
	root["DataType"] = "Flatness"; // <--- Important: New DataType

	// 2. Prepare the Flatness Payload
	json payload;

	// Create C++ vectors or arrays to hold the JSON data
	std::vector<double> front;
	std::vector<double> rear;

	// Loop 24 times to fill the arrays
	for (int i = 0; i < 24; i++)
	{
		// Assuming your C++ product struct has arrays named dblFlatnessA and dblFlatnessB
		front.push_back(GenerateDouble(0.0, 1.2)); 
		rear.push_back(GenerateDouble(0.0, 1.1));
	}

	// Assign to payload
	payload["Front"] = front;
	payload["Rear"] = rear;

	// 3. Attach payload to root
	root["Payload"] = payload;

	std::string data = root.dump();

	// Note: If you need to send indices 2 and 3, you must 
	// update the C# CurlData class to have ValueC/ValueD
	if (pFile->SendRequest(NULL, 0, LPVOID(data.c_str()), static_cast<DWORD>(data.length())))
		return true;

	return false;
}

bool PostBusbarIO::PostData(CHttpFile* pFile)
{
	if (nullptr == pFile || nullptr == product)
       return false;

    // 1. Root Object
    json root;
    root["ProductId"] = CT2CA(product->Id);
    root["EquipmentId"] = g_szBusbarVisionEquipmentId;
    
    // CRITICAL: Set the correct DataType so the C# side knows to map it to BusbarPayload
    root["DataType"] = "Busbar"; 

    // 2. Prepare Payload Object
    json payload;

    // 3. Create Vectors to hold the array data
    // We use vectors because nlohmann::json handles them automatically
    std::vector<double> tfLen, trLen, bfLen, brLen;
    std::vector<int> tfCnt, trCnt, bfCnt, brCnt;

    // 4. Populate Vectors
    // Assuming you have a loop (e.g., 24 items, or however many leads you measure)
    const int ITEM_COUNT = 8; 
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        // Populate Lengths (Doubles)
        tfLen.push_back(GenerateDouble(5.0, 12.0));
        trLen.push_back(GenerateDouble(6.0, 11.5));
        bfLen.push_back(GenerateDouble(5.0, 12.0));
        brLen.push_back(GenerateDouble(6.0, 11.5));

        // Populate Counts (Integers)
        tfCnt.push_back(GenerateInteger(3, 7));
        trCnt.push_back(GenerateInteger(3, 7));
        bfCnt.push_back(GenerateInteger(3, 7));
        brCnt.push_back(GenerateInteger(3, 7));
    }

    // 5. Assign Vectors to JSON Properties
    // These keys MUST match the C# BusbarPayload property names exactly
    payload["TopFrontLeadLength"] = tfLen;
    payload["TopRearLeadLength"] = trLen;
    payload["BtmFrontLeadLength"] = bfLen;
    payload["BtmRearLeadLength"] = brLen;

    payload["TopFrontLeadCount"] = tfCnt;
    payload["TopRearLeadCount"] = trCnt;
    payload["BtmFrontLeadCount"] = bfCnt;
    payload["BtmRearLeadCount"] = brCnt;

    // 6. Attach Payload to Root
    root["Payload"] = payload;

    // 7. Send Request
    std::string data = root.dump();
    if (pFile->SendRequest(NULL, 0, (LPVOID)data.c_str(), (DWORD)data.length()))
       return true;

    return false;
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