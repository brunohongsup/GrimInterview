#include "pch.h"
#include "ProductManager.h"

#include "Product.h"

std::unique_ptr<ProductManager> ProductManager::s_pInstance = nullptr;

std::mutex ProductManager::s_mtx{};

ProductManager* ProductManager::GetInstance()
{
	if (s_pInstance == nullptr)
	{
		std::lock_guard<std::mutex> lock(s_mtx);
		if (s_pInstance == nullptr)
		{
			s_pInstance = std::unique_ptr<ProductManager>(new ProductManager());
		}
	}

	return s_pInstance.get();
}

bool ProductManager::GetProduct(const CString& id, std::shared_ptr<Product>& data)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	auto findProduct = [&id](const std::shared_ptr<Product>& product)
	{
		if (id == product->Id)
			return true;

		else
			return false;
	};
	
	auto it = std::find_if(std::begin(m_vctProducts), std::end(m_vctProducts), findProduct);
	if (it == std::end(m_vctProducts))
		return false;

	data = *it;
	return true;
}

const std::vector<std::shared_ptr<Product>>& ProductManager::GetProducts()
{
	return m_vctProducts;
}

void ProductManager::ClearBefore(const CTime& timeStamp)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	for (auto it = std::begin(m_vctProducts); it != std::end(m_vctProducts);)
	{
		auto& product = *it;
		if (product->InputTime < timeStamp)
			it = m_vctProducts.erase(it);

		else
			++it;			
	}
}

void ProductManager::AddProduct(const CString& id)
{
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		auto lookUp = [&id](const std::shared_ptr<Product>& product)
		{
			if (product->Id == id)
				return true;

			return false;
		};

		if (m_vctProducts.size() >= PRODUCTS_CAPACITY)
			m_vctProducts.erase(std::begin(m_vctProducts), std::end(m_vctProducts) - 50);

		auto findProduct = std::find_if(std::begin(m_vctProducts), std::end(m_vctProducts), lookUp);
		if (findProduct != std::end(m_vctProducts))
			return;

		m_vctProducts.emplace_back(std::make_shared<Product>(id));
	}
}

std::shared_ptr<Product> ProductManager::GetLastProduct()
{
	std::lock_guard<std::mutex> lock(m_mtx);
	if (m_vctProducts.empty())
		return nullptr;

	return m_vctProducts.back();
}

ProductManager::ProductManager()
{
	m_vctProducts.reserve(PRODUCTS_CAPACITY + 5);
}
