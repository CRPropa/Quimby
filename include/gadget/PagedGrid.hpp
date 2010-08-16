/*
 * Grid.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef PAGED_GRID_HPP_
#define PAGED_GRID_HPP_

#include "MurmurHash2.hpp"

#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <fstream>
#ifdef DEBUG
#include <iostream>
#endif
#include <assert.h>
#include <inttypes.h>

struct index3_t {
	uint32_t x, y, z;

	index3_t() :
		x(0), y(0), z(0) {

	}

	index3_t(const index3_t &v) :
		x(v.x), y(v.y), z(v.z) {
	}

	explicit index3_t(uint32_t i) :
		x(i), y(i), z(i) {
	}

	explicit index3_t(const uint32_t &X, const uint32_t &Y, const uint32_t &Z) :
		x(X), y(Y), z(Z) {
	}

	bool operator <(const index3_t &v) const {
		if (x > v.x)
			return false;
		else if (x < v.x)
			return true;
		if (y > v.y)
			return false;
		else if (y < v.y)
			return true;
		if (z >= v.z)
			return false;
		else
			return true;
	}

	bool operator ==(const index3_t &v) const {
		if (x != v.x)
			return false;
		if (y != v.y)
			return false;
		if (z != v.z)
			return false;
		return true;
	}

	index3_t operator -(const index3_t &v) const {
		return index3_t(x - v.x, y - v.y, z - v.z);
	}

	index3_t operator +(const index3_t &v) const {
		return index3_t(x + v.x, y + v.y, z + v.z);
	}

	index3_t operator *(const index3_t &v) const {
		return index3_t(x * v.x, y * v.y, z * v.z);
	}

	index3_t operator *(const uint32_t &v) const {
		return index3_t(x * v, y * v, z * v);
	}

	index3_t operator /(const uint32_t &f) const {
		return index3_t(x / f, y / f, z / f);
	}

	index3_t &operator /=(const uint32_t &f) {
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	index3_t operator %(const uint32_t &f) const {
		return index3_t(x % f, y % f, z % f);
	}

	index3_t &operator %=(const uint32_t &f) {
		x %= f;
		y %= f;
		z %= f;
		return *this;
	}

	index3_t &operator *=(const uint32_t &f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	index3_t &operator +=(const index3_t &v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	index3_t &operator -=(const index3_t &v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	index3_t &operator =(const index3_t &v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	//	void incXYZ(const index3_t &lower, const index3_t &upper) {
	//		if (this->operator ==(upper))
	//			return;
	//
	//		z++;
	//		if (z == upper.z)
	//		{
	//			z = lower.z;
	//			y++;
	//			if (y == upper.y)
	//			{
	//				y = lower.y;
	//				x++;
	//			}
	//		}
	//	}

};

inline index3_t minByElement(const index3_t &a, const index3_t &b) {
	return index3_t(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

inline index3_t maxByElement(const index3_t &a, const index3_t &b) {
	return index3_t(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

inline std::ostream &operator <<(std::ostream &out, const index3_t &v) {
	out << v.x << " " << v.y << " " << v.z;
	return out;
}

/// Single Page containing the elements.
template<typename ELEMENT>
class Page {
public:
	typedef ELEMENT element_t;

	index3_t origin;
	std::vector<element_t> elements;
	bool dirty;
	time_t accessTime;
	uint32_t accessCount;
	uint32_t locks;

	Page();
	void reset();
	element_t &get(const index3_t &index, uint32_t size);
};

template<typename ELEMENT>
inline Page<ELEMENT>::Page() {
	reset();
}

template<typename ELEMENT>
inline void Page<ELEMENT>::reset() {
	dirty = false;
	accessTime = 0;
	accessCount = 0;
	locks = 0;
}

template<typename ELEMENT>
inline typename Page<ELEMENT>::element_t &Page<ELEMENT>::get(
		const index3_t &index, uint32_t size) {
	assert(index.x >= origin.x);
	assert(index.x < origin.x + size);
	assert(index.y >= origin.y);
	assert(index.y < origin.y + size);
	assert(index.z >= origin.z);
	assert(index.z < origin.z + size);

	return elements[(index.x - origin.x) * size * size + (index.y - origin.y)
			* size + (index.z - origin.z)];
}

template<typename ELEMENT>
class PageIO {
protected:
	uint32_t pageSize;

public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	void setPageSize(uint32_t pageSize) {
		this->pageSize = pageSize;
	}
	virtual void loadPage(page_t *page) = 0;
	virtual void savePage(page_t *page) = 0;
};

template<typename ELEMENT>
class SimpleTextPageIO: public PageIO<ELEMENT> {
public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	std::string prefix;
	element_t defaultValue;
	bool readOnly;

	void loadPage(page_t *page) {
		page->elements.resize(page->size * page->size * page->size);
		std::ifstream in(createFilename(page).c_str());
		if (in.good()) {
			for (size_t i = 0; i < page->elements.size() && in; i++) {
				in >> page->elements[i];
			}
		} else {
			for (size_t i = 0; i < page->elements.size(); i++) {
				page->elements[i] = defaultValue;
			}
		}
		page->dirty = false;

	}
	void savePage(page_t *page) {
		if (readOnly)
			return;
		std::ofstream out(createFilename(page).c_str(), std::ios::trunc);
		for (size_t i = 0; i < page->elements.size() && out; i++) {
			out << page->elements[i] << std::endl;
		}
		page->dirty = false;
	}
private:
	std::string createFilename(page_t *page) {
		std::stringstream sstr;
		sstr << prefix << "-" << page->size << "-" << page->origin.x << "_"
				<< page->origin.y << "_" << page->origin.z << ".txt";
		return sstr.str();
	}
};

template<typename ELEMENT>
class BinaryPageIO: public PageIO<ELEMENT> {
public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
private:
	std::string prefix;
	element_t defaultValue;
	bool readOnly;
	size_t loadedPages;
	size_t savedPages;

	/// elements per file per axis
	size_t elemetsPerFile;

	bool overwrite;

	bool forceDump;
public:
	BinaryPageIO();

	void loadPage(page_t *page);

	void savePage(page_t *page);

	void setPrefix(const std::string &prefix);

	/// set number of elements per file per axis
	void setElemetsPerFile(size_t elemetsPerFile);

	void setOverwrite(bool overwrite);
	void setForceDump(bool forceDump);
	void setDefaultValue(const element_t &defaultValue);

	size_t getLoadedPages();
	size_t getSavedPages();

private:
	size_t offset(page_t *page, const index3_t idx) {
		index3_t offset = idx + (page->origin % elemetsPerFile);
		return sizeof(element_t) * (offset.x + offset.y * elemetsPerFile
				+ offset.z * elemetsPerFile * elemetsPerFile);
	}

	std::string createFilename(page_t *page) {
		index3_t o = page->origin / elemetsPerFile;
		std::stringstream sstr;
		sstr << prefix << "-" << o.x << "-" << o.y << "-" << o.z << ".raw";
		return sstr.str();
	}

	bool checkFile(const std::string &filename) {
		std::ifstream in(filename.c_str(), std::ios::binary);
		if (in.good() == false) {
			//			std::cerr << "[BinaryPageIO] could not open " << filename << std::endl;
			return false;
		}

		in.seekg(0, std::ios::end);
		if (in.bad()) {
			//			std::cerr << "[BinaryPageIO] bad seek " << filename << std::endl;
			return false;
		}

		std::fstream::pos_type needed = elemetsPerFile * elemetsPerFile
				* elemetsPerFile * sizeof(element_t);
		if (in.tellg() != needed) {
			//			std::cerr << "baad size " << filename << std::endl;

			return false;
		}

		return true;

	}

	void reserveFile(const std::string &filename) {
		size_t needed = elemetsPerFile * elemetsPerFile * elemetsPerFile;
#ifdef DEBUG
		std::cerr << "[BinaryPageIO] reserve " << filename << std::endl;
#endif
		std::ofstream out(filename.c_str(), std::ios::trunc | std::ios::binary);
		for (size_t i = 0; i < needed; i++) {
			out.write((const char *) &defaultValue, sizeof(defaultValue));
		}
		out.close();

	}

};

template<typename ELEMENT>
BinaryPageIO<ELEMENT>::BinaryPageIO() :
	readOnly(false), loadedPages(0), savedPages(0), elemetsPerFile(1),
			overwrite(false), forceDump(false) {

}

template<typename ELEMENT>
void BinaryPageIO<ELEMENT>::loadPage(page_t *page) {
#if DEBUG
	std::cerr << "[BinaryPageIO] load page " << page->origin << ", size: "
	<< this->pageSize << std::endl;
#endif
	page->elements.resize(this->pageSize * this->pageSize * this->pageSize);
	std::string filename = createFilename(page);
	page->dirty = false;
	if (checkFile(filename) && overwrite == false) {
		std::ifstream in(filename.c_str(), std::ios::binary);
		index3_t index;
		for (index.z = 0; index.z < this->pageSize; index.z++) {
			for (index.y = 0; index.y < this->pageSize; index.y++) {
				for (index.x = 0; index.x < this->pageSize; index.x++) {
					in.seekg(offset(page, index), std::ios::beg);
					in.read((char *) &page->get(index + page->origin,
							this->pageSize), sizeof(element_t));
				}
			}
		}
	} else {
		for (size_t i = 0; i < page->elements.size(); i++) {
			page->elements[i] = defaultValue;
		}
		if (forceDump)
			page->dirty = true;
	}

	loadedPages += 1;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::savePage(page_t *page) {
	savedPages += 1;
	if (readOnly)
		return;

	if (page->dirty == false)
		return;

#ifdef DEBUG
	std::cout << "[BinaryPageIO] save page " << page->origin << std::endl;
#endif

	// page can span multiple files:
	index3_t fileStart = page->origin / elemetsPerFile;
	index3_t fileEnd = (page->origin + index3_t(this->pageSize - 1))
			/ elemetsPerFile;

	std::string filename = createFilename(page);

	// make sure the file is big enough
	if (checkFile(filename) == false)
		reserveFile(filename);

	std::fstream out(filename.c_str(), std::ios::in | std::ios::out
			| std::ios::binary);

	index3_t index;
	for (index.z = 0; index.z < this->pageSize; index.z++) {
		for (index.y = 0; index.y < this->pageSize; index.y++) {
			for (index.x = 0; index.x < this->pageSize; index.x++) {
				out.seekp(offset(page, index), std::ios::beg);
				out.write((const char *) &page->get(index + page->origin,
						this->pageSize), sizeof(element_t));
			}
		}
	}

	page->dirty = false;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setPrefix(const std::string &prefix) {
	this->prefix = prefix;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setElemetsPerFile(size_t elemetsPerFile) {
	this->elemetsPerFile = elemetsPerFile;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setForceDump(bool forceDump) {
	this->forceDump = forceDump;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setOverwrite(bool overwrite) {
	this->overwrite = overwrite;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setDefaultValue(
		const element_t &defaultValue) {
	this->defaultValue = defaultValue;
}

template<typename ELEMENT>
inline size_t BinaryPageIO<ELEMENT>::getLoadedPages() {
	return loadedPages;
}

template<typename ELEMENT>
inline size_t BinaryPageIO<ELEMENT>::getSavedPages() {
	return savedPages;
}

template<typename ELEMENT>
class PagingStrategy {
public:
	typedef Page<ELEMENT> page_t;
	virtual void loaded(page_t *page) = 0;
	virtual void cleared(page_t *page) = 0;
	virtual void accessed(page_t *page) = 0;
	virtual page_t *which() = 0;
};

template<typename ELEMENT>
class LastAccessPagingStrategy: public PagingStrategy<ELEMENT> {
public:
	typedef Page<ELEMENT> page_t;
	std::list<page_t *> pages;
	void loaded(page_t *page) {
		pages.push_back(page);
	}
	void cleared(page_t *page) {
		pages.remove(page);
	}
	void accessed(page_t *page) {
		pages.remove(page);
		pages.push_front(page);
	}
	page_t *which() {
		return pages.back();
	}
};

template<typename ELEMENT>
class LeastAccessPagingStrategy: public PagingStrategy<ELEMENT> {
public:
	typedef Page<ELEMENT> page_t;
	typedef std::map<page_t *, size_t> map_t;
	typedef typename map_t::iterator iterator_t;
	map_t pages;
	size_t maxCount;

	LeastAccessPagingStrategy() :
		maxCount(1) {
	}

	void loaded(page_t *page) {
		pages[page] = maxCount / 10;
	}

	void cleared(page_t *page) {
		pages[page] = 0;
	}

	void accessed(page_t *page) {
		size_t & v = pages[page];
		v += 1;
		if (v > maxCount)
			maxCount = v;
	}

	page_t *which() {
		size_t count = -1;
		page_t *page = 0;
		for (iterator_t i = pages.begin(); i != pages.end(); i++)
			if (i->second < count) {
				page = i->first;
				count = i->second;
			}
		return page;
	}
};

//static size_t hash3(const index3_t &v) {
//	return MurmurHash2(&v, sizeof(index3_t), 875685);
//}

template<typename ELEMENT>
class PagedGrid {
public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	typedef typename std::map<index3_t, page_t *> container_t;
	typedef typename container_t::iterator iterator_t;
	class Visitor {
	public:
		virtual void visit(PagedGrid<element_t> &grid, size_t x, size_t y,
				size_t z, element_t &value) = 0;
	};
protected:
	page_t *lastPage;
	size_t pageSize;
	PageIO<element_t> *io;
	size_t pageCount;
	/// size of the grid
	size_t size;

	PagingStrategy<element_t> *strategy;
	container_t pages;
	size_t pageMisses;

	void pageAccept(Page<element_t> *page, Visitor &v, const index3_t &l,
			const index3_t &u);
public:

	PagedGrid();
	PagedGrid(const size_t &grid_size, size_t page_size);
	~PagedGrid();

	/// set the number of elemets per axis
	void setSize(size_t size);

	void setStrategy(PagingStrategy<element_t> *strategy);
	void setIO(PageIO<element_t> *io);

	/// set the number of elemets per axis per page
	void setPageSize(uint32_t pageSize);
	/// set the number of pages held in memory
	void setPageCount(size_t count);

	element_t &getReadWrite(const index3_t &index);

	const element_t &getReadOnly(const index3_t &index);

	page_t *getPage(const index3_t &index);

	index3_t toOrigin(const index3_t &index);
	index3_t clamp(const index3_t &i);

	void clear();
	void flush();

	size_t getActivePageCount();
	size_t getPageMisses();
	void acceptXYZ(Visitor &v);

	void acceptZYX(Visitor &v);

	void acceptZYX(Visitor &v, const index3_t &l, const index3_t &u);
	void accept(Visitor &v, const index3_t &l, const index3_t &u);

};

template<typename ELEMENT>
inline PagedGrid<ELEMENT>::PagedGrid() :
	lastPage(0), pageSize(0), io(0), pageCount(1), size(0), strategy(0),
			pageMisses(0) {
}

template<typename ELEMENT>
inline PagedGrid<ELEMENT>::~PagedGrid() {
	clear();
}

/// set the number of elemets per axis
template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setSize(size_t size) {
	this->size = size;
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setStrategy(PagingStrategy<element_t> *strategy) {
	this->strategy = strategy;
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setIO(PageIO<element_t> *io) {
	this->io = io;
	if (io) {
		io->setPageSize(pageSize);
	}
}

/// set the number of elemets per axis per page
template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setPageSize(uint32_t pageSize) {
	assert(this->pageSize == 0);
	this->pageSize = pageSize;
	if (io) {
		io->setPageSize(pageSize);
	}
}

/// set the number of pages held in memory
template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setPageCount(size_t count) {
	if (count < pageCount) {
		size_t clearing = this->pageCount - count;
		for (size_t i = 0; i < clearing; i++) {
			page_t *page = strategy->which();
			if (page) {
				io->savePage(page);
				strategy->cleared(page);
				pages.erase(page->origin);
				delete page;
			}
		}
	}
	this->pageCount = count;
}

template<typename ELEMENT>
inline typename PagedGrid<ELEMENT>::element_t &PagedGrid<ELEMENT>::getReadWrite(
		const index3_t &index) {
	page_t *page = getPage(index);
	strategy->accessed(page);
	page->dirty = true;
	return page->get(index, pageSize);
}

template<typename ELEMENT>
inline const typename PagedGrid<ELEMENT>::element_t &PagedGrid<ELEMENT>::getReadOnly(
		const index3_t &index) {
	page_t *page = getPage(index);
	strategy->accessed(page);
	return page->get(index);
}

template<typename ELEMENT>
inline typename PagedGrid<ELEMENT>::page_t *PagedGrid<ELEMENT>::getPage(
		const index3_t &index) {
	index3_t orig = toOrigin(index);

	if (lastPage && (lastPage->origin == orig))
		return lastPage;

	iterator_t i = pages.find(orig);
	if (i != pages.end()) {
		assert(i->first == orig);
		assert(i->second->origin == orig);
		return i->second;
	}

	// check if page is loaded
	page_t *page = 0;
	if (pages.size() < pageCount) {
		page = new page_t;
	} else {
		// ask the paging strategy which page we should replace
		pageMisses++;
		page = strategy->which();

		// save and clear old page
		io->savePage(page);
		strategy->cleared(page);
		pages.erase(page->origin);
	}

	// load new page
	page->origin = orig;
	io->loadPage(page);
	strategy->loaded(page);
	lastPage = page;

	pages[orig] = page;

	return page;
}

template<typename ELEMENT>
inline index3_t PagedGrid<ELEMENT>::toOrigin(const index3_t &index) {
	index3_t origin;
	origin.x = size_t(index.x / pageSize) * pageSize;
	origin.y = size_t(index.y / pageSize) * pageSize;
	origin.z = size_t(index.z / pageSize) * pageSize;
	return origin;
}

template<typename ELEMENT>
inline index3_t PagedGrid<ELEMENT>::clamp(const index3_t &i) {
	index3_t index = i;
	if (index.x > size)
		index.x = size;
	if (index.y > size)
		index.y = size;
	if (index.z > size)
		index.z = size;

	return index;
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::clear() {
	iterator_t i = pages.begin();
	iterator_t end = pages.end();
	while (i != end) {
		delete (i->second);
		i++;
	}
	pages.clear();
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::flush() {
	iterator_t i = pages.begin();
	iterator_t end = pages.end();
	while (i != end) {
		io->savePage(i->second);
		i++;
	}
}

template<typename ELEMENT>
inline size_t PagedGrid<ELEMENT>::getActivePageCount() {
	return pages.size();
}

template<typename ELEMENT>
inline size_t PagedGrid<ELEMENT>::getPageMisses() {
	return pageMisses;
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::acceptXYZ(Visitor &v) {
	for (size_t iX = 0; iX < size; iX++) {
		for (size_t iY = 0; iY < size; iY++) {
			for (size_t iZ = 0; iZ < size; iZ++) {
				v.visit(*this, iX, iY, iZ, getReadWrite(iX, iY, iZ));
			}
		}
	}
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::acceptZYX(Visitor &v) {
	for (size_t iZ = 0; iZ < size; iZ++) {
		for (size_t iY = 0; iY < size; iY++) {
			for (size_t iX = 0; iX < size; iX++) {
				v.visit(*this, iX, iY, iZ, getReadWrite(iX, iY, iZ));
			}
		}
	}
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::acceptZYX(Visitor &v, const index3_t &l,
		const index3_t &u) {
	index3_t index;
	index3_t upper = clamp(u);
	for (size_t iZ = l.z; iZ < upper.z; iZ++) {
		index.z = iZ;
		for (size_t iY = l.y; iY < upper.y; iY++) {
			index.y = iY;
			for (size_t iX = l.x; iX < upper.x; iX++) {
				index.x = iX;
				v.visit(*this, iX, iY, iZ, getReadWrite(index));
			}
		}
	}
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::accept(Visitor &v, const index3_t &lower,
		const index3_t &u) {
	index3_t upper = minByElement(u, index3_t(size));

	index3_t lowerPage = lower / pageSize;
	index3_t upperPage = (upper - index3_t(1)) / pageSize;

	index3_t pageIndex;
	for (pageIndex.x = lowerPage.x; pageIndex.x <= upperPage.x; pageIndex.x++) {
		for (pageIndex.y = lowerPage.y; pageIndex.y <= upperPage.y; pageIndex.y++) {
			for (pageIndex.z = lowerPage.z; pageIndex.z <= upperPage.z; pageIndex.z++) {
				page_t *page = getPage(pageIndex * pageSize);
				assert(page != 0);
				strategy->accessed(page);
				page->dirty = true;
				pageAccept(page, v, lower, upper);
			}
		}
	}
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::pageAccept(Page<ELEMENT> *page, Visitor &v,
		const index3_t &l, const index3_t &u) {
	index3_t lower = maxByElement(l, page->origin);
	index3_t upper = minByElement(u, page->origin + index3_t(pageSize));
	uint32_t pageSize2 = pageSize * pageSize;
	index3_t offset = lower - page->origin;

	for (uint32_t x = lower.x; x < upper.x; x++) {
		offset.x = (x - page->origin.x) * pageSize2;
		for (uint32_t y = lower.y; y < upper.y; y++) {
			offset.y = (y - page->origin.y) * pageSize;
			for (uint32_t z = lower.x; z < upper.z; z++) {
				offset.z = z - page->origin.z;
				v.visit(*this, x, y, z, page->elements[offset.x + offset.y
						+ offset.z]);
			}
		}
	}

}
#endif /* PAGED_GRID_HPP_ */
