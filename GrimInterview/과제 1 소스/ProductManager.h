#pragma once
#include <memory>
#include <vector>
#include <mutex>

struct Product;
struct LaserCross
{
friend class ProductManager;
	
	CTime time;

	LaserCross()
	{
		time = CTime::GetCurrentTime();
	}
};

class ProductManager
{
public:
	static ProductManager* GetInstance();

	bool GetProduct(const CString& id, std::shared_ptr<Product>& data);

	const std::vector<std::shared_ptr<Product>>& GetProducts();

	void ClearBefore(const CTime& timeStamp);
	
	~ProductManager() = default;

	void AddProduct(const CString& id);

	std::shared_ptr<Product> GetLastProduct();

	void ResetLaserCross();

	void CheckCurlDataRange();

	static constexpr size_t PRODUCTS_CAPACITY = 400;

private:

	ProductManager();

	std::mutex m_mtx;
	
	std::vector<std::shared_ptr<Product>> m_vctProducts;

	static std::unique_ptr<ProductManager> s_pInstance;

	static std::mutex s_mtx;

};
